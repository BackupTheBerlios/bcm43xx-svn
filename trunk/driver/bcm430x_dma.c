/*

  Broadcom BCM430x wireless driver

  DMA ringbuffer and descriptor allocation/management

  Copyright (c) 2005 Michael Buesch <mbuesch@freenet.de>

  Some code in this file is derived from the b44.c driver
  Copyright (C) 2002 David S. Miller
  Copyright (C) Pekka Pietikainen

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#include "bcm430x.h"
#include "bcm430x_dma.h"
#include "bcm430x_main.h"
#include "bcm430x_debugfs.h"
#include "bcm430x_power.h"

#include <linux/dmapool.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <asm/semaphore.h>


static inline
void ring_sync_for_cpu(struct bcm430x_dmaring *ring)
{
	struct device *dev = &(ring->bcm->pci_dev->dev);

	/*XXX: I am not sure, if usage of coherent memory would
	 *     be better, performace wise. Syncing streaming mem
	 *     might mean copying the whole memory around (On which
	 *     platforms? Does the bcm430x support these platforms?)
	 *     On PPC and i386 this operation should be cheap, though.
	 */
	dma_sync_single_for_cpu(dev, ring->dmabase,
				BCM430x_DMA_RINGMEMSIZE,
				DMA_TO_DEVICE);
}

static inline
void ring_sync_for_device(struct bcm430x_dmaring *ring)
{
	struct device *dev = &(ring->bcm->pci_dev->dev);

	dma_sync_single_for_device(dev, ring->dmabase,
				   BCM430x_DMA_RINGMEMSIZE,
				   DMA_TO_DEVICE);
}

static inline int free_slots(struct bcm430x_dmaring *ring)
{
	return (ring->nr_slots - ring->used_slots);
}

static inline int next_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(slot >= -1 && slot <= ring->nr_slots - 1);
	if (slot == ring->nr_slots - 1)
		return 0;
	return slot + 1;
}

static inline int prev_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(slot >= 0 && slot <= ring->nr_slots - 1);
	if (slot == 0)
		return ring->nr_slots - 1;
	return slot - 1;
}

static inline void try_to_suspend_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);
	assert(!ring->suspended);

	if (free_slots(ring) < ring->suspend_mark) {
		netif_stop_queue(ring->bcm->net_dev);
		ring->suspended = 1;
	}
}

static inline void try_to_resume_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);

	if (!ring->suspended)
		return;
	if (free_slots(ring) >= ring->resume_mark) {
		ring->suspended = 0;
		netif_wake_queue(ring->bcm->net_dev);
	}
}

/* Request a slot for usage. */
static inline
int request_slot(struct bcm430x_dmaring *ring)
{
	int slot;

	if (unlikely(free_slots(ring) == 0))
		return -1;

	slot = next_slot(ring, ring->last_used);
	ring->last_used = slot;
	ring->meta[slot].used = 1;
	ring->used_slots++;

	if (ring->tx) {
		/* Check the number of available slots and suspend TX,
		 * if we are running low on free slots.
		 */
		try_to_suspend_txqueue(ring);
	}

	return slot;
}

/* Return a slot to the free slots. */
static inline
void return_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(ring->last_used >= 0);
	if (ring->used_slots == 1)
		ring->last_used = -1;
	ring->meta[slot].used = 0;
	ring->used_slots--;

	if (ring->tx) {
		/* Check if TX is suspended and check if we have
		 * enough free slots to resume it again.
		 */
		try_to_resume_txqueue(ring);
	}
}

static inline void dmacontroller_poke_tx(struct bcm430x_dmaring *ring,
					 int slot)
{
	/* Everything is ready to start. Buffers are DMA mapped and
	 * associated with slots.
	 * "slot" is the first slot of the new frame we want to transmit.
	 * Close your seat belts now, please.
	 */
	wmb();
	bcm430x_write32(ring->bcm,
			ring->mmio_base + BCM430x_DMA_TX_DESC_INDEX,
			(u32)(slot * sizeof(struct bcm430x_dmadesc)));
}

#ifdef BCM430x_DEBUG
/* Debugging helper to dump the contents of the ringmemory (slots) */
static __attribute_used__
void dump_ringmemory(struct bcm430x_dmaring *ring)
{
	int i;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;

	printk(KERN_INFO PFX "*** DMA Ringmemory dump (%s) ***\n",
	       (ring->tx) ? "tx" : "rx");
	printk(KERN_INFO PFX "used_slots: 0x%04x\n", ring->used_slots);
	if (ring->last_used >= 0)
		printk(KERN_INFO PFX "last_used:  0x%04x\n", ring->last_used);
	else
		printk(KERN_INFO PFX "last_used:  NONE\n");

	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		printk(KERN_INFO PFX "0x%04x:  ctl: 0x%08x, adr: 0x%08x, "
				     "txb: 0x%p, skb: 0x%p(%s), bus: 0x%08x\n",
		       i, get_desc_ctl(desc), get_desc_addr(desc),
		       meta->txb, meta->skb, (meta->free_skb) ? "f" : " ",
		       meta->dmaaddr);
	}
}
#endif /* BCM430x_DEBUG */

static inline
dma_addr_t map_descbuffer(struct bcm430x_dmaring *ring,
			  unsigned char *buf,
			  size_t len)
{
	dma_addr_t dmaaddr;
	enum dma_data_direction dir;

	if (ring->tx)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;

	dmaaddr = dma_map_single(&ring->bcm->pci_dev->dev,
				 buf, len, dir);

	return dmaaddr;
}

static inline
void unmap_descbuffer(struct bcm430x_dmaring *ring,
		      dma_addr_t addr,
		      size_t len)
{
	enum dma_data_direction dir;

	if (ring->tx)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;

	dma_unmap_single(&ring->bcm->pci_dev->dev,
			 addr, len, dir);
}

/* Unmap and free a descriptor buffer. */
static void free_descriptor_buffer(struct bcm430x_dmaring *ring,
				   struct bcm430x_dmadesc *desc,
				   struct bcm430x_dmadesc_meta *meta,
				   int irq_context)
{
	unmap_descbuffer(ring, meta->dmaaddr, meta->skb->len);
	if (meta->free_skb) {
		if (irq_context)
			dev_kfree_skb_irq(meta->skb);
		else
			dev_kfree_skb(meta->skb);
	}
	meta->skb = NULL;
	if (meta->txb) {
		ieee80211_txb_free(meta->txb);
		meta->txb = NULL;
	}
}

/* Free all stuff belonging to a complete TX frame.
 * Begin at slot.
 * This is to be called on completion IRQ.
 */
static void free_descriptor_frame(struct bcm430x_dmaring *ring,
				  int slot)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	int is_last_fragment;

	ring_sync_for_cpu(ring);
	assert(get_desc_ctl(ring->vbase + slot) & BCM430x_DMADTOR_FRAMESTART);

	while (1) {
		assert(slot < ring->nr_slots);
		desc = ring->vbase + slot;
		meta = ring->meta + slot;

		is_last_fragment = !!(get_desc_ctl(desc) & BCM430x_DMADTOR_FRAMEEND);
		free_descriptor_buffer(ring, desc, meta, 1);
		/* Everything belonging to the slot is unmapped
		 * and freed, so we can return it.
		 */
		return_slot(ring, slot);

		if (is_last_fragment)
			break;
		slot++;
	}
	ring_sync_for_device(ring);
}

static int alloc_ringmemory(struct bcm430x_dmaring *ring)
{
	int err = -ENOMEM;
	struct device *dev = &(ring->bcm->pci_dev->dev);

	ring->vbase = (struct bcm430x_dmadesc *)__get_free_page(GFP_KERNEL);
	if (!ring->vbase) {
		printk(KERN_ERR PFX "DMA ringmemory allocation failed\n");
		goto out;
	}
	memset(ring->vbase, 0, BCM430x_DMA_RINGMEMSIZE);
	ring->dmabase = dma_map_single(dev, ring->vbase,
				       BCM430x_DMA_RINGMEMSIZE,
				       DMA_TO_DEVICE);
	/* sanity checks... */
	if (ring->dmabase & 0x000003FF) {
		printk(KERN_ERR PFX "Error: DMA ringmemory not 1024 byte aligned!\n");
		goto err_unmap;
	}
	if (ring->dmabase & ~BCM430x_DMA_DMABUSADDRMASK) {
		printk(KERN_ERR PFX "Error: DMA ringmemory above 1G mark!\n");
		goto err_unmap;
	}
	err = 0;
out:
	return err;

err_unmap:
	dma_unmap_single(dev, ring->dmabase,
			 BCM430x_DMA_RINGMEMSIZE,
			 DMA_TO_DEVICE);
	free_page((unsigned long)(ring->vbase));
	goto out;
}

static void free_ringmemory(struct bcm430x_dmaring *ring)
{
	dma_unmap_single(&(ring->bcm->pci_dev->dev),
			 ring->dmabase,
			 BCM430x_DMA_RINGMEMSIZE,
			 DMA_TO_DEVICE);
	free_page((unsigned long)(ring->vbase));
}

/* Reset the RX DMA channel */
int bcm430x_dmacontroller_rx_reset(struct bcm430x_private *bcm,
				   u16 mmio_base)
{
	int i;
	u32 value;

	bcm430x_write32(bcm,
			mmio_base + BCM430x_DMA_RX_CONTROL,
			0x00000000);
	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(bcm,
				       mmio_base + BCM430x_DMA_RX_STATUS);
		value &= BCM430x_DMA_RXSTAT_STAT_MASK;
		if (value == BCM430x_DMA_RXSTAT_STAT_DISABLED) {
			i = -1;
			break;
		}
		udelay(10);
	}
	if (i != -1) {
		printk(KERN_ERR PFX "Error: Wait on DMA RX status timed out.\n");
		return -ENODEV;
	}

	return 0;
}

static inline int dmacontroller_rx_reset(struct bcm430x_dmaring *ring)
{
	assert(!ring->tx);

	return bcm430x_dmacontroller_rx_reset(ring->bcm, ring->mmio_base);
}

/* Reset the RX DMA channel */
int bcm430x_dmacontroller_tx_reset(struct bcm430x_private *bcm,
				   u16 mmio_base)
{
	int i;
	u32 value;

	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(bcm,
				       mmio_base + BCM430x_DMA_TX_STATUS);
		value &= BCM430x_DMA_TXSTAT_STAT_MASK;
		if (value == BCM430x_DMA_TXSTAT_STAT_DISABLED ||
		    value == BCM430x_DMA_TXSTAT_STAT_IDLEWAIT ||
		    value == BCM430x_DMA_TXSTAT_STAT_STOPPED)
			break;
		udelay(10);
	}
	bcm430x_write32(bcm,
			mmio_base + BCM430x_DMA_TX_CONTROL,
			0x00000000);
	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(bcm,
				       mmio_base + BCM430x_DMA_TX_STATUS);
		value &= BCM430x_DMA_TXSTAT_STAT_MASK;
		if (value == BCM430x_DMA_TXSTAT_STAT_DISABLED) {
			i = -1;
			break;
		}
		udelay(10);
	}
	if (i != -1) {
		printk(KERN_ERR PFX "Error: Wait on DMA TX status timed out.\n");
		return -ENODEV;
	}
	/* ensure the reset is completed. */
	udelay(300);

	return 0;
}

static inline int dmacontroller_tx_reset(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);

	return bcm430x_dmacontroller_tx_reset(ring->bcm, ring->mmio_base);
}

static int setup_rx_descbuffer(struct bcm430x_dmaring *ring,
			       struct bcm430x_dmadesc *desc,
			       struct bcm430x_dmadesc_meta *meta,
			       gfp_t gfp_flags)
{
	struct bcm430x_rxhdr *rxhdr;
	dma_addr_t dmaaddr;
	u32 desc_addr;
	u32 desc_ctl;
	const int slot = (int)(desc - ring->vbase);

	assert(slot >= 0 && slot < ring->nr_slots);
	assert(!ring->tx);

	meta->skb = __dev_alloc_skb(ring->rx_buffersize, gfp_flags);
	if (!meta->skb)
		return -ENOMEM;
	meta->skb->dev = ring->bcm->net_dev;
	meta->free_skb = 1;

	dmaaddr = map_descbuffer(ring, meta->skb->data, ring->rx_buffersize);
	desc_addr = (u32)(dmaaddr + BCM430x_DMA_DMABUSADDROFFSET);
	desc_ctl = (BCM430x_DMADTOR_BYTECNT_MASK &
		    (u32)(ring->rx_buffersize - ring->frameoffset));
	if (slot == ring->nr_slots - 1)
		desc_ctl |= BCM430x_DMADTOR_DTABLEEND;
	set_desc_addr(desc, desc_addr);
	set_desc_ctl(desc, desc_ctl);

	rxhdr = (struct bcm430x_rxhdr *)(meta->skb->data);
	rxhdr->frame_length = cpu_to_le16(0x0000);
	rxhdr->flags1 = cpu_to_le16(0x0000);

	return 0;
}

/* Allocate the initial descbuffers.
 * This is used for an RX ring only.
 */
static int alloc_initial_descbuffers(struct bcm430x_dmaring *ring)
{
	int i, err = 0;
	struct bcm430x_dmadesc *desc = NULL;
	struct bcm430x_dmadesc_meta *meta;

	ring_sync_for_cpu(ring);
	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		err = setup_rx_descbuffer(ring, desc, meta, GFP_KERNEL);
		if (err)
			goto err_unwind;

		assert(ring->used_slots <= ring->nr_slots);
		ring->last_used++;
		ring->used_slots++;
	}
	ring_sync_for_device(ring);

out:
	return err;

err_unwind:
	for ( ; i >= 0; i--) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		unmap_descbuffer(ring, meta->dmaaddr, meta->skb->len);
		dev_kfree_skb(meta->skb);
	}
	goto out;
}

/* Do initial setup of the DMA controller.
 * Reset the controller, write the ring busaddress
 * and switch the "enable" bit on.
 */
static int dmacontroller_setup(struct bcm430x_dmaring *ring)
{
	int err = 0;
	u32 value;

	if (ring->tx) {
		/* Set Transmit Control register to "transmit enable" */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_CONTROL,
				BCM430x_DMA_TXCTRL_ENABLE);
		/* Set Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				ring->dmabase + BCM430x_DMA_DMABUSADDROFFSET);
	} else {
		err = alloc_initial_descbuffers(ring);
		if (err)
			goto out;
		/* Set Receive Control "receive enable" and frame offset */
		value = (ring->frameoffset << BCM430x_DMA_RXCTRL_FRAMEOFF_SHIFT);
		value |= BCM430x_DMA_RXCTRL_ENABLE;
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_CONTROL,
				value);
		/* Set Receive Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_RING,
				ring->dmabase + BCM430x_DMA_DMABUSADDROFFSET);
		/* Init the descriptor pointer. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_INDEX,
				200);
	}

out:
	return err;
}

/* Shutdown the DMA controller. */
static void dmacontroller_cleanup(struct bcm430x_dmaring *ring)
{
	if (ring->tx) {
		dmacontroller_tx_reset(ring);
		/* Zero out Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				0x00000000);
	} else {
		dmacontroller_rx_reset(ring);
		/* Zero out Receive Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_RING,
				0x00000000);
	}
}

/* Loop through all used descriptors and free the buffers.
 * This is used on controller shutdown, so no need to return
 * the slots.
 */
static void free_all_descbuffers(struct bcm430x_dmaring *ring)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	int i;

	if (!ring->used_slots)
		return;
	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		if (!meta->used)
			continue;

		unmap_descbuffer(ring, meta->dmaaddr, meta->skb->len);
		free_descriptor_buffer(ring, desc, meta, 0);
	}
}

/* Main initialization function. */
struct bcm430x_dmaring * bcm430x_setup_dmaring(struct bcm430x_private *bcm,
					       u16 dma_controller_base,
					       int nr_descriptor_slots,
					       int tx)
{
	struct bcm430x_dmaring *ring;
	int err;

	ring = kzalloc(sizeof(*ring), GFP_KERNEL);
	if (!ring)
		goto out;

	ring->meta = kzalloc(sizeof(*ring->meta) * nr_descriptor_slots,
			     GFP_KERNEL);
	if (!ring->meta)
		goto err_kfree_ring;

	spin_lock_init(&ring->lock);
	ring->bcm = bcm;
	ring->nr_slots = nr_descriptor_slots;
	ring->last_used = -1;
	ring->suspend_mark = ring->nr_slots * BCM430x_TXSUSPEND_PERCENT / 100;
	ring->resume_mark = ring->nr_slots * BCM430x_TXRESUME_PERCENT / 100;
	assert(ring->suspend_mark < ring->resume_mark);
	ring->mmio_base = dma_controller_base;
	if (tx) {
		ring->tx = 1;
	} else {
		switch (dma_controller_base) {
		case BCM430x_MMIO_DMA1_BASE:
			ring->rx_buffersize = BCM430x_DMA1_RXBUFFERSIZE;
			ring->frameoffset = BCM430x_DMA1_RX_FRAMEOFFSET;
			break;
		case BCM430x_MMIO_DMA4_BASE:
			ring->rx_buffersize = BCM430x_DMA4_RXBUFFERSIZE;
			ring->frameoffset = BCM430x_DMA4_RX_FRAMEOFFSET;
			break;
		default:
			assert(0);
		}
	}

	err = alloc_ringmemory(ring);
	if (err)
		goto err_kfree_meta;
	err = dmacontroller_setup(ring);
	if (err)
		goto err_free_ringmemory;

out:
	return ring;

err_free_ringmemory:
	free_ringmemory(ring);
err_kfree_meta:
	kfree(ring->meta);
err_kfree_ring:
	kfree(ring);
	ring = NULL;
	goto out;
}

/* Cancel all pending transfers.
 * Called on device shutdown, so make sure no transfers
 * are still running on return from this function.
 */
static void cancel_transfers(struct bcm430x_dmaring *ring)
{
	int slot;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;

	ring_sync_for_cpu(ring);
	for (slot = 0; slot < ring->nr_slots; slot++) {
		desc = ring->vbase + slot;
		meta = ring->meta + slot;

		if (!meta->used)
			continue;
		free_descriptor_frame(ring, slot);
	}
	ring_sync_for_device(ring);
}

/* Main cleanup function. */
void bcm430x_destroy_dmaring(struct bcm430x_dmaring *ring)
{
	if (!ring)
		return;

	/* Device IRQs are disabled prior entering this function,
	 * so no need to take care of concurrency with rx handler stuff.
	 */
	cancel_transfers(ring);
	dmacontroller_cleanup(ring);
	/* Free all remaining descriptor buffers
	 * (For example when this is an RX ring)
	 */
	free_all_descbuffers(ring);
	free_ringmemory(ring);

	kfree(ring->meta);
	kfree(ring);
}

/* Generate a cookie for the TX header. */
static inline
u16 generate_cookie(struct bcm430x_dmaring *ring,
		    int slot)
{
	u16 cookie = 0x0000;

	/* Use the upper 4 bits of the cookie as
	 * DMA controller ID and store the slot number
	 * in the lower 12 bits
	 */
	switch (ring->mmio_base) {
	default:
		assert(0);
	case BCM430x_MMIO_DMA1_BASE:
		break;
	case BCM430x_MMIO_DMA2_BASE:
		cookie = 0x1000;
		break;
	case BCM430x_MMIO_DMA3_BASE:
		cookie = 0x2000;
		break;
	case BCM430x_MMIO_DMA4_BASE:
		cookie = 0x3000;
		break;
	}
	assert(((u16)slot & 0xF000) == 0x0000);
	cookie |= (u16)slot;

	return cookie;
}

/* Inspect a cookie and find out to which controller/slot it belongs. */
static inline
struct bcm430x_dmaring * parse_cookie(struct bcm430x_private *bcm,
				      u16 cookie, int *slot)
{
	struct bcm430x_dmaring *ring = NULL;

	switch (cookie & 0xF000) {
	case 0x0000:
		ring = bcm->current_core->dma->tx_ring0;
		break;
	case 0x1000:
		ring = bcm->current_core->dma->tx_ring1;
		break;
	case 0x2000:
		ring = bcm->current_core->dma->tx_ring2;
		break;
	case 0x3000:
		ring = bcm->current_core->dma->tx_ring3;
		break;
	default:
		assert(0);
	}
	*slot = (cookie & 0x0FFF);
	assert(*slot >= 0 && *slot < ring->nr_slots);

	return ring;
}

static inline int dma_tx_fragment_sg(struct bcm430x_dmaring *ring,
				     struct sk_buff *skb,
				     struct bcm430x_dma_txcontext *ctx)
{
	/*TODO: Scatter/Gather IO */
	return -EINVAL;
}

static fastcall
int dma_tx_fragment(struct bcm430x_dmaring *ring,
		    struct sk_buff *skb,
		    struct ieee80211_txb *txb,
		    struct bcm430x_dma_txcontext *ctx,
		    const int forcefree_skb)
{
	int err = 0;
	int slot;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	dma_addr_t dmaaddr;
	u32 desc_ctl = 0;
	u32 desc_addr;

	slot = request_slot(ring);
	if (unlikely(slot < 0)) {
		/* The queue should be stopped,
		 * if we are low on free slots.
		 * If this ever triggers, we have to lower the suspend_mark.
		 */
		printk(KERN_ERR PFX "Out of DMA descriptor slots!\n");
		return -ENOMEM;
	}
	desc = ring->vbase + slot;
	meta = ring->meta + slot;

	ring_sync_for_cpu(ring);
	if (ctx->cur_frag == 0) {
		/* This is the first fragment. */
		desc_ctl |= BCM430x_DMADTOR_FRAMESTART;
		/* Save the whole txb for freeing later in 
		 * completion irq
		 */
		meta->txb = txb;
		/* Save the first slot number for later in tx_xfer() */
		ctx->first_slot = slot;
	}

	if (likely(ring->bcm->no_txhdr == 0)) {
		/* Add a device specific TX header. */
		if (unlikely(skb_headroom(skb) < sizeof(struct bcm430x_txhdr))) {
			/* SKB has not enough headroom. Blame the ieee80211 subsys for this.
			 * On latest 80211 subsys this should not trigger.
			 */
			err = skb_cow(skb, sizeof(struct bcm430x_txhdr));
			if (unlikely(err)) {
				return_slot(ring, slot);
				ring_sync_for_device(ring);
				printk(KERN_ERR PFX "DMA: Not enough skb headroom!\n");
				return err;
			}
		}
		assert(skb_headroom(skb) >= sizeof(struct bcm430x_txhdr));
		/* Reserve enough headroom for the device tx header. */
		__skb_push(skb, sizeof(struct bcm430x_txhdr));
		/* Now calculate and add the tx header.
		 * The tx header includes the PLCP header.
		 */
		bcm430x_generate_txhdr(ring->bcm,
				       (struct bcm430x_txhdr *)skb->data,
				       skb->data + sizeof(struct bcm430x_txhdr),
				       skb->len - sizeof(struct bcm430x_txhdr),
				       (ctx->cur_frag == 0),
				       generate_cookie(ring, slot));
	} else {
		struct bcm430x_txhdr *txhdr;

		/* We have to modify the cookie to free buffers later. */
		dprintk(KERN_INFO PFX "Modifying cookie in given TX header.\n");
		txhdr = (struct bcm430x_txhdr *)skb->data;
		txhdr->cookie = cpu_to_le16(generate_cookie(ring, slot));
	}
//bcm430x_printk_dump(skb->data, skb->len, "SKB");

	/* Write the buffer to the descriptor and map it. */
	meta->skb = skb;
	if (unlikely(forcefree_skb)) {
		meta->free_skb = 1;
	} else {
		/* We do not free the skb, as it is freed as
		 * part of the txb freeing.
		 */
		meta->free_skb = 0;
	}
	dmaaddr = map_descbuffer(ring, skb->data, skb->len);

	desc_addr = (u32)(dmaaddr + BCM430x_DMA_DMABUSADDROFFSET);
	desc_ctl |= (BCM430x_DMADTOR_BYTECNT_MASK &
		     (meta->skb->len - ring->frameoffset));
	if (slot == ring->nr_slots - 1)
		desc_ctl |= BCM430x_DMADTOR_DTABLEEND;

	if (skb_shinfo(skb)->nr_frags != 0) {
		/* Map the remaining Scatter/Gather fragments */
		err = dma_tx_fragment_sg(ring, skb, ctx);
		if (err) {
			/*TODO: error handling: unmap, other stuff? */
			return_slot(ring, slot);
			goto out;
		}
		/*TODO: modify desc and meta to point to the last SC buffer */
	}

	if (ctx->cur_frag == ctx->nr_frags - 1) {
		/* This is the last fragment */
		desc_ctl |= BCM430x_DMADTOR_FRAMEEND | BCM430x_DMADTOR_COMPIRQ;
		set_desc_addr(desc, desc_addr);
		set_desc_ctl(desc, desc_ctl);
		assert(ctx->first_slot != -1);
		ring_sync_for_device(ring);
		/* Now transfer the whole frame. */
		dmacontroller_poke_tx(ring, ctx->first_slot);
	} else {
		ring_sync_for_device(ring);
	}
out:
	return err;
}

static inline int dma_transfer_txb(struct bcm430x_dmaring *ring,
				   struct ieee80211_txb *txb)
{
	/* We just received a packet from the kernel network subsystem.
	 * Reformat and write it to some DMA buffers. Poke
	 * the device to send the stuff.
	 * Note that this is called from atomic context.
	 */

	int i, err = -EINVAL;
	struct sk_buff *skb;
	struct bcm430x_dma_txcontext ctx;
	unsigned long flags;

	assert(ring->tx);

	ctx.nr_frags = txb->nr_frags;
	ctx.cur_frag = 0;

	spin_lock_irqsave(&ring->lock, flags);
	for (i = 0; i < txb->nr_frags; i++) {
		skb = txb->fragments[i];
		ctx.first_slot = -1;
		err = dma_tx_fragment(ring, skb, txb, &ctx, 0);
		if (err)
			break;//TODO: correct error handling.
		ctx.cur_frag++;
	}
	spin_unlock_irqrestore(&ring->lock, flags);

//dump_ringmemory(ring);
	return err;
}

/* Write a frame directly to the ring.
 * This is only to be used for debugging and testing (loopback)
 */
void bcm430x_dma_tx_frame(struct bcm430x_dmaring *ring,
			  const char *buf, size_t size)
{
	struct sk_buff *skb;
	struct bcm430x_dma_txcontext ctx;
	int err;
	size_t skb_size = size;
	unsigned long flags;

	assert(ring->tx);

	if (!ring->bcm->no_txhdr)
		skb_size += sizeof(struct bcm430x_txhdr);
	skb = dev_alloc_skb(skb_size);
	if (!skb) {
		printk(KERN_ERR PFX "Out of memory!\n");
		return;
	}
	skb->dev = ring->bcm->net_dev;
	if (!ring->bcm->no_txhdr)
		skb_reserve(skb, sizeof(struct bcm430x_txhdr));
	memcpy(skb_put(skb, size), buf, size);

	ctx.nr_frags = 1;
	ctx.cur_frag = 0;
	ctx.first_slot = -1;
	spin_lock_irqsave(&ring->lock, flags);
	err = dma_tx_fragment(ring, skb, NULL, &ctx, 1);
	spin_unlock_irqrestore(&ring->lock, flags);
	if (err)
		printk(KERN_ERR PFX "TX FRAME failed!\n");
}

int fastcall
bcm430x_dma_transfer_txb(struct bcm430x_private *bcm,
			 struct ieee80211_txb *txb)
{
	return dma_transfer_txb(bcm->current_core->dma->tx_ring1,
				txb);
}

void fastcall
bcm430x_dma_handle_xmitstatus(struct bcm430x_private *bcm,
			      struct bcm430x_xmitstatus *status)
{
	struct bcm430x_dmaring *ring;
	int slot;
	unsigned long flags;

	//FIXME: Can an xmitstatus indicate a failed tx?

	ring = parse_cookie(bcm, status->cookie, &slot);
	assert(ring);
	spin_lock_irqsave(&ring->lock, flags);
	free_descriptor_frame(ring, slot);
	spin_unlock_irqrestore(&ring->lock, flags);
}

static inline
void dma_rx(struct bcm430x_dmaring *ring,
	    int slot)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	struct bcm430x_rxhdr *rxhdr;
	struct sk_buff *skb;
	struct ieee80211_rx_stats rx_stats;
	u16 len;
	int err;

	//TODO: This can be optimized a lot.
#if 1
printk(KERN_INFO PFX "Data received on DMA controller 0x%04x slot %d\n",
       ring->mmio_base, slot);
#endif

	memset(&rx_stats, 0, sizeof(rx_stats));

	desc = ring->vbase + slot;
	meta = ring->meta + slot;

	ring_sync_for_cpu(ring);
	unmap_descbuffer(ring, meta->dmaaddr, meta->skb->len);
	skb = meta->skb;
	rxhdr = (struct bcm430x_rxhdr *)skb->data;
	len = cpu_to_le16(rxhdr->frame_length);
	if (len == 0) {
		int i = 0;

		do {
			udelay(2);
			barrier();
			len = cpu_to_le16(rxhdr->frame_length);
		} while (len == 0 && i++ < 5);
		if (unlikely(len == 0)) {
			dprintkl(KERN_ERR PFX "DMA RX: len zero, dropping...\n");
			goto drop;
		}
	}
	if (unlikely(len > ring->rx_buffersize)) {
		//FIXME: Can this trigger, if we span multiple descbuffers?
		dprintkl(KERN_ERR PFX "DMA RX: invalid length\n");
		goto drop;
	}
	skb_put(skb, len);
	skb_pull(skb, ring->frameoffset);

	//TODO: interpret more rxhdr stuff.

	err = ieee80211_rx(ring->bcm->ieee, skb, &rx_stats);
	if (unlikely(err == 0)) {
		dprintkl(KERN_ERR PFX "ieee80211_rx() failed with %d\n", err);
		goto drop;
	}

setup_new:
	err = setup_rx_descbuffer(ring, desc, meta, GFP_ATOMIC);
	if (unlikely(err)) {
		//TODO: What to do here?
	}
	ring_sync_for_device(ring);

	return;

drop:
	dev_kfree_skb_irq(skb);
	goto setup_new;
}

void fastcall
bcm430x_dma_rx(struct bcm430x_dmaring *ring)
{
	u32 status;
	u16 descptr;
	int slot, current_slot;

	assert(!ring->tx);
	status = bcm430x_read32(ring->bcm, ring->mmio_base + BCM430x_DMA_RX_STATUS);
	descptr = (status & BCM430x_DMA_RXSTAT_DPTR_MASK);
	current_slot = descptr / sizeof(struct bcm430x_dmadesc);
	assert(current_slot >= 0 && current_slot < ring->nr_slots);
	slot = ring->cur_rx_slot;
	for ( ; slot != current_slot; slot = next_slot(ring, slot))
		dma_rx(ring, slot);
	bcm430x_write32(ring->bcm,
			ring->mmio_base + BCM430x_DMA_RX_DESC_INDEX,
			(u32)(slot * sizeof(struct bcm430x_dmadesc)));
	ring->cur_rx_slot = slot;
}

/* vim: set ts=8 sw=8 sts=8: */
