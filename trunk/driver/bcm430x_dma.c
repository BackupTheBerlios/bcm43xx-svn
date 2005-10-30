/*

  Broadcom BCM430x wireless driver

  DMA ringbuffer and descriptor allocation/management

  Copyright (c) 2005 Michael Buesch <mbuesch@freenet.de>

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


static void unmap_descbuffer(struct bcm430x_dmaring *ring,
			     struct bcm430x_dmadesc *desc,
			     struct bcm430x_dmadesc_meta *meta);

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

static inline int used_slots(struct bcm430x_dmaring *ring)
{
	assert(ring->used_slots >= 0 && ring->used_slots <= ring->nr_slots);
	return ring->used_slots;
}

static inline int free_slots(struct bcm430x_dmaring *ring)
{
	return (ring->nr_slots - used_slots(ring));
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

static inline void suspend_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);
	assert(!ring->suspended);
	netif_stop_queue(ring->bcm->net_dev);
	ring->suspended = 1;
}

static inline void try_to_suspend_txqueue(struct bcm430x_dmaring *ring)
{
	if (free_slots(ring) < ring->suspend_mark)
		suspend_txqueue(ring);
}

static inline void resume_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);
	assert(ring->suspended);
	ring->suspended = 0;
	netif_wake_queue(ring->bcm->net_dev);
}

static inline void try_to_resume_txqueue(struct bcm430x_dmaring *ring)
{
	if (!ring->suspended)
		return;
	if (free_slots(ring) >= ring->resume_mark)
		resume_txqueue(ring);
}

/* Request a slot for usage.
 * Make sure to have the ring synced for CPU, before calling this.
 * Check if there are free slots, _before_ calling this function.
 */
static int request_slot(struct bcm430x_dmaring *ring)
{
	int slot;
	int prev;

	assert(free_slots(ring));

	prev = ring->last_used;
	slot = next_slot(ring, prev);
	ring->last_used = slot;
	if (ring->first_used < 0) {
		assert(ring->first_used == -1);
		ring->first_used = slot;
	}
	if (slot == ring->nr_slots - 1) {
		set_desc_ctl(ring->vbase + slot,
			     get_desc_ctl(ring->vbase + slot)
			     | BCM430x_DMADTOR_DTABLEEND);
	}
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

/* Return a slot to the free slots.
 * Make sure to have the ring synced for CPU, before calling this.
 */
static void return_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(ring->first_used != -1 && ring->last_used != -1);
	if (used_slots(ring) > 1) {
		assert(ring->first_used != ring->last_used);
		if (ring->first_used == slot) {
			ring->first_used = next_slot(ring, slot);
			assert(ring->meta[ring->first_used].used == 1);
		} else if (ring->last_used == slot) {
			ring->last_used = prev_slot(ring, slot);
			assert(ring->meta[ring->last_used].used == 1);
		}
	} else {
		/* slot is the last used.
		 * Mark the ring as "no used slots"
		 */
		assert(ring->first_used == ring->last_used);
		ring->first_used = -1;
		ring->last_used = -1;
	}
	ring->meta[slot].used = 0;
	ring->used_slots--;

	if (ring->tx) {
		/* Check if TX is suspended and check if we have
		 * enough free slots to resume it again.
		 */
		try_to_resume_txqueue(ring);
	}
}

static inline u32 calc_rx_frameoffset(u16 dmacontroller_mmio_base)
{
	u32 offset = 0x00000000;

	switch (dmacontroller_mmio_base) {
	case BCM430x_MMIO_DMA1_BASE:
		offset = BCM430x_DMA1_RX_FRAMEOFFSET;
		break;
	}

	offset = (offset << BCM430x_DMA_RXCTRL_FRAMEOFF_SHIFT);
	assert(!(offset & ~BCM430x_DMA_RXCTRL_FRAMEOFF_MASK));

	return offset;
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
	if (ring->first_used >= 0)
		printk(KERN_INFO PFX "first_used: 0x%04x\n", ring->first_used);
	else
		printk(KERN_INFO PFX "first_used: NONE\n");
	if (ring->last_used >= 0)
		printk(KERN_INFO PFX "last_used:  0x%04x\n", ring->last_used);
	else
		printk(KERN_INFO PFX "last_used:  NONE\n");

	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		printk(KERN_INFO PFX "0x%04x:  ctl: 0x%08x, adr: 0x%08x, "
				     "txb: 0x%p, skb: 0x%p(%s), bus: 0x%08x(%s)\n",
		       i, get_desc_ctl(desc), get_desc_addr(desc),
		       meta->txb, meta->skb, (meta->nofree_skb) ? " " : "f",
		       meta->dmaaddr, (meta->mapped) ? "m" : " ");
	}
}
#endif /* BCM430x_DEBUG */

/* Unmap and free a descriptor buffer. */
static void free_descriptor_buffer(struct bcm430x_dmaring *ring,
				   struct bcm430x_dmadesc *desc,
				   struct bcm430x_dmadesc_meta *meta,
				   int irq_context)
{
	unmap_descbuffer(ring, desc, meta);
	if (!meta->nofree_skb) {
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
 * This is to be called on tx_timeout and completion IRQ.
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

/* DMA map a descriptor buffer.
 * The descriptor buffer is the skb contained as reference
 * in the descriptor meta data structure.
 * Make sure to have the ring synced for CPU before calling this.
 */
static void map_descbuffer(struct bcm430x_dmaring *ring,
			   struct bcm430x_dmadesc *desc,
			   struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	assert(!meta->mapped);

	if (ring->tx)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	meta->dmaaddr = dma_map_single(&ring->bcm->pci_dev->dev,
				       meta->skb->data, meta->skb->len, dir);
	meta->mapped = 1;

	/* Tell the device about the buffer */
	set_desc_addr(desc, meta->dmaaddr + BCM430x_DMA_DMABUSADDROFFSET);
	set_desc_ctl(desc, get_desc_ctl(desc) | (BCM430x_DMADTOR_BYTECNT_MASK & meta->skb->len));
}

/* DMA unmap a descriptor buffer.
 * The descriptor buffer is the skb contained as reference
 * in the descriptor meta data structure.
 * Make sure to have the ring synced for CPU before calling this.
 */
static void unmap_descbuffer(struct bcm430x_dmaring *ring,
			     struct bcm430x_dmadesc *desc,
			     struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	if (!meta->mapped)
		return;

	if (ring->tx)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;

	/* First tell the device it is going away. */
	set_desc_ctl(desc, 0x00000000);
	set_desc_addr(desc, 0x00000000);
	mb();//FIXME: need a memory barrier here?

	dma_unmap_single(&ring->bcm->pci_dev->dev,
			 meta->dmaaddr, meta->skb->len, dir);
	meta->dmaaddr = 0x00000000;

	meta->mapped = 0;  
}

/* Allocate the initial descbuffers.
 * This is used for an RX ring only.
 *FIXME: Not sure if we allocate enough buffers with sufficient size, though...
 */
static int alloc_initial_descbuffers(struct bcm430x_dmaring *ring)
{
	size_t buffersize = 0;
	int i, err = 0, num_buffers = 0;
	struct bcm430x_dmadesc *desc = NULL;
	struct bcm430x_dmadesc_meta *meta;

	if (!ring->tx) {
		num_buffers = BCM430x_DMA_NUM_RXBUFFERS;
		if (ring->mmio_base == BCM430x_MMIO_DMA1_BASE)
			buffersize = BCM430x_DMA1_RXBUFFERSIZE;
		else if (ring->mmio_base == BCM430x_MMIO_DMA4_BASE)
			buffersize = BCM430x_DMA4_RXBUFFERSIZE;
		else
			assert(0);
	} else
		assert(0);

	ring->first_used = 0;
	ring->last_used = 0;
	for (i = 0; i < num_buffers; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		meta->skb = __dev_alloc_skb(buffersize, GFP_KERNEL);
		if (!meta->skb)
			goto err_unwind;
		map_descbuffer(ring, desc, meta);

		assert(used_slots(ring) <= ring->nr_slots);
		ring->last_used++;
	}
	ring_sync_for_cpu(ring);
	set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_DTABLEEND);
	ring_sync_for_device(ring);

out:
	return err;

err_unwind:
	for ( ; i >= 0; i--) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		unmap_descbuffer(ring, desc, meta);
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
	int err;
	u32 value;

//FIXME: Do we have to reset the controllers here?
	if (ring->tx) {
#if 0
		err = dmacontroller_tx_reset(ring);
		if (err)
			goto out;
#endif
err = 0;
		/* Set Transmit Control register to "transmit enable" */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_CONTROL,
				BCM430x_DMA_TXCTRL_ENABLE);
		/* Set Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				ring->dmabase + BCM430x_DMA_DMABUSADDROFFSET);
	} else {
#if 0
		err = dmacontroller_rx_reset(ring);
		if (err)
			goto out;
#endif
		err = alloc_initial_descbuffers(ring);
		if (err)
			goto out;
		/* Set Receive Control "receive enable" and frame offset */
		value = calc_rx_frameoffset(ring->mmio_base);
		value |= BCM430x_DMA_RXCTRL_ENABLE;
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_CONTROL,
				value);
		/* Set Receive Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_RING,
				ring->dmabase + BCM430x_DMA_DMABUSADDROFFSET);
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

	if (!used_slots(ring))
		return;
	i = ring->first_used;
	do {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		unmap_descbuffer(ring, desc, meta);
		free_descriptor_buffer(ring, desc, meta, 0);

		i = next_slot(ring, i);
	} while (i != ring->last_used);
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
	ring->first_used = -1;
	ring->last_used = -1;
	ring->suspend_mark = ring->nr_slots * BCM430x_TXSUSPEND_PERCENT / 100;
	ring->resume_mark = ring->nr_slots * BCM430x_TXRESUME_PERCENT / 100;
	assert(ring->suspend_mark < ring->resume_mark);
	ring->mmio_base = dma_controller_base;
	if (tx)
		ring->tx = 1;

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

static void tx_timeout(struct bcm430x_dmaring *ring)
{
	assert(ring->tx);

	dprintk("DMA TX timeout on controller 0x%04x\n", ring->mmio_base);
	dmacontroller_tx_reset(ring);//FIXME: The controller does not wake up from the reset.
	cancel_transfers(ring);
}

void bcm430x_dma_tx_timeout(struct bcm430x_private *bcm)
{
	struct bcm430x_dmaring *ring;
	unsigned long flags;

	ring = bcm->current_core->dma->tx_ring0;
	spin_lock_irqsave(&ring->lock, flags);
	tx_timeout(ring);
	spin_unlock_irqrestore(&ring->lock, flags);

	ring = bcm->current_core->dma->tx_ring1;
	spin_lock_irqsave(&ring->lock, flags);
	tx_timeout(ring);
	spin_unlock_irqrestore(&ring->lock, flags);

	ring = bcm->current_core->dma->tx_ring2;
	spin_lock_irqsave(&ring->lock, flags);
	tx_timeout(ring);
	spin_unlock_irqrestore(&ring->lock, flags);

	ring = bcm->current_core->dma->tx_ring3;
	spin_lock_irqsave(&ring->lock, flags);
	tx_timeout(ring);
	spin_unlock_irqrestore(&ring->lock, flags);
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

	/* Make sure we have enough free slots.
	 * We check for frags+2, because we might need an additional
	 * one, if we do not have enough skb_headroon. (see below)
	 */
	if (unlikely(free_slots(ring) < skb_shinfo(skb)->nr_frags + 2)) {
		/* The queue should be stopped,
		 * if we are low on free slots.
		 * If this ever triggers, we have to lower the suspend_mark.
		 */
		printk(KERN_ERR PFX "Out of DMA descriptor slots!\n");
		return -ENOMEM;
	}

	ring_sync_for_cpu(ring);

	slot = request_slot(ring);
	desc = ring->vbase + slot;
	meta = ring->meta + slot;

	if (ctx->cur_frag == 0) {
		/* This is the first fragment. */
		set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_FRAMESTART);
		/* Save the whole txb for freeing later in 
		 * completion irq (or timeout work handler)
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
		meta->nofree_skb = 0;
	} else {
		/* We do not free the skb, as it is freed as
		 * part of the txb freeing.
		 */
		meta->nofree_skb = 1;
	}
	map_descbuffer(ring, desc, meta);

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
		set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_FRAMEEND);
		set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_COMPIRQ);
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
	skb_put(skb, skb_size);
	if (!ring->bcm->no_txhdr)
		skb_reserve(skb, sizeof(struct bcm430x_txhdr));
	memcpy(skb->data, buf, size);

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
	struct bcm430x_rxhdr rxhdr;
	unsigned char *data;

printk(KERN_INFO PFX "Data received on DMA controller 0x%04x slot %d\n",
       ring->mmio_base, slot);

	desc = ring->vbase + slot;
	meta = ring->meta + slot;

	unmap_descbuffer(ring, desc, meta);
	data = meta->skb->data;
	bcm430x_rxhdr_to_cpuorder(&rxhdr, (struct bcm430x_hwrxhdr *)data);

	//TODO: Interpret the rxhdr.
	//TODO: map the descbuffer again and make the device ready for new rx.
}

void fastcall
bcm430x_dma_rx(struct bcm430x_dmaring *ring)
{
	u32 status;
	u16 descptr;
	int slot;

	assert(!ring->tx);
	status = bcm430x_read32(ring->bcm, ring->mmio_base + BCM430x_DMA_RX_STATUS);
	descptr = (status & BCM430x_DMA_RXSTAT_DPTR_MASK);
	slot = descptr / sizeof(struct bcm430x_dmadesc);
	assert(slot >= 0 && slot < ring->nr_slots);
	dma_rx(ring, slot);
}

/* vim: set ts=8 sw=8 sts=8: */
