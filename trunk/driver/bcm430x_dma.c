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

#include <linux/dmapool.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <asm/semaphore.h>


static struct bcm430x_ringallocator {
	struct dma_pool *pool;
	int refcnt;
} ringallocator;
static DECLARE_MUTEX(ringallocator_sem);


static inline int used_slots(struct bcm430x_dmaring *ring)
{
	if (ring->first_used < 0) {
		assert(ring->last_used == -1);
		return 0;
	}
	if (ring->last_used >= ring->first_used)
		return (ring->last_used - ring->first_used + 1);
	return ((ring->nr_slots - ring->first_used)
		+ (ring->last_used + 1));
}

static inline int free_slots(struct bcm430x_dmaring *ring)
{
	assert(ring->nr_slots - used_slots(ring) >= 0);
	return (ring->nr_slots - used_slots(ring));
}

static inline int first_used_slot(struct bcm430x_dmaring *ring)
{
	return ring->first_used;
}

static inline int last_used_slot(struct bcm430x_dmaring *ring)
{
	return ring->last_used;
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

static inline int first_free_slot(struct bcm430x_dmaring *ring)
{
	assert(free_slots(ring));
	return next_slot(ring, ring->last_used);
}

/* request a slot for usage. */
static inline void request_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(free_slots(ring));
	assert(slot == ring->last_used + 1);

	ring->last_used = next_slot(ring, ring->last_used);
	if (ring->first_used < 0)
		ring->first_used = slot;

	if (ring->last_used >= ring->first_used) {
		ring->vbase[ring->last_used].control |= BCM430x_DMADTOR_DTABLEEND;
		if (ring->last_used - 1 >= 0) {
			ring->vbase[ring->last_used - 1].control
				&= ~ BCM430x_DMADTOR_DTABLEEND;
		}
	}
}

/* return a slot to the free slots. */
static inline void return_slot(struct bcm430x_dmaring *ring, int slot)
{
	assert(slot == ring->last_used);

	ring->vbase[ring->last_used].control &= ~ BCM430x_DMADTOR_DTABLEEND;

	if (used_slots(ring) > 1) {
		ring->last_used = prev_slot(ring, ring->last_used);
		if (ring->last_used >= ring->first_used)
			ring->vbase[ring->last_used].control |= BCM430x_DMADTOR_DTABLEEND;
	} else {
		ring->first_used = -1;
		ring->last_used = -1;
	}
}

static inline void suspend_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->flags & BCM430x_RINGFLAG_TX);
	assert(!(ring->flags & BCM430x_RINGFLAG_SUSPENDED));
	netif_stop_queue(ring->bcm->net_dev);
	ring->flags |= BCM430x_RINGFLAG_SUSPENDED;
}

static inline void try_to_suspend_txqueue(struct bcm430x_dmaring *ring)
{
	if (free_slots(ring) < ring->suspend_mark)
		suspend_txqueue(ring);
}

static inline void resume_txqueue(struct bcm430x_dmaring *ring)
{
	assert(ring->flags & BCM430x_RINGFLAG_TX);
	assert(ring->flags & BCM430x_RINGFLAG_SUSPENDED);
	ring->flags &= ~ BCM430x_RINGFLAG_SUSPENDED;
	netif_wake_queue(ring->bcm->net_dev);
}

static inline void try_to_resume_txqueue(struct bcm430x_dmaring *ring)
{
	if (!(ring->flags & BCM430x_RINGFLAG_SUSPENDED))
		return;
	if (free_slots(ring) >= ring->resume_mark)
		resume_txqueue(ring);
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
	offset &= BCM430x_DMA_RXCTRL_FRAMEOFF_MASK;

	return offset;
}

static int alloc_ringmemory(struct bcm430x_dmaring *ring)
{
	const size_t ring_memsize = 4096;
	const size_t ring_memalign = 4096;

	int err = -ENOMEM;
	struct pci_dev *pci_dev = ring->bcm->pci_dev;
	struct device *dev = &pci_dev->dev;

	assert(ring->nr_slots * sizeof(struct bcm430x_dmadesc) < ring_memsize);

	down(&ringallocator_sem);

	if (ringallocator.refcnt == 0) {
		ringallocator.pool = dma_pool_create(DRV_NAME "_dmarings",
						     dev, ring_memsize,
						     ring_memalign, ring_memalign);
		if (!ringallocator.pool) {
			printk(KERN_ERR PFX "Could not create DMA-ring pool.\n");
			goto out_up;
		}
	}

	ring->vbase = dma_pool_alloc(ringallocator.pool, GFP_KERNEL, /*FIXME: | GFP_DMA? */
				     &ring->dmabase);
	if (!ring->vbase) {
		printk(KERN_ERR PFX "Could not allocate DMA ring.\n");
		err = -ENOMEM;
		goto err_destroy_pool;
	}
	memset(ring->vbase, 0, ring_memsize);

	ringallocator.refcnt++;
	err = 0;
out_up:
	up(&ringallocator_sem);

	return err;

err_destroy_pool:
	if (ringallocator.refcnt == 0) {
		dma_pool_destroy(ringallocator.pool);
		ringallocator.pool = 0;
	}
	goto out_up;
}

static void free_ringmemory(struct bcm430x_dmaring *ring)
{
	down(&ringallocator_sem);

	dma_pool_free(ringallocator.pool, ring->vbase, ring->dmabase);
	ringallocator.refcnt--;
	if (ringallocator.refcnt == 0) {
		dma_pool_destroy(ringallocator.pool);
		ringallocator.pool = 0;
	}

	up(&ringallocator_sem);
}

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
	}
	if (i != -1) {
		printk(KERN_ERR PFX "Error: Wait on DMA RX status timed out.\n");
		return -ENODEV;
	}

	return 0;
}

static inline int dmacontroller_rx_reset(struct bcm430x_dmaring *ring)
{
	assert(!(ring->flags & BCM430x_RINGFLAG_TX));

	return bcm430x_dmacontroller_rx_reset(ring->bcm, ring->mmio_base);
}

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
	assert(ring->flags & BCM430x_RINGFLAG_TX);

	return bcm430x_dmacontroller_tx_reset(ring->bcm, ring->mmio_base);
}

static int map_descbuffer(struct bcm430x_dmaring *ring,
			  struct bcm430x_dmadesc *desc,
			  struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	assert(!(meta->flags & BCM430x_DESCFLAG_MAPPED));

	if (ring->flags & BCM430x_RINGFLAG_TX)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	meta->dmaaddr = dma_map_single(&ring->bcm->pci_dev->dev,
				       meta->skb->data, meta->skb->len, dir);
	if (!meta->dmaaddr) /*FIXME: can this fail? */
		return -ENOMEM;
	meta->flags |= BCM430x_DESCFLAG_MAPPED;

	desc->address = meta->dmaaddr;
	desc->control |= BCM430x_DMADTOR_BYTECNT_MASK & meta->skb->len;

	return 0;
}

static void unmap_descbuffer(struct bcm430x_dmaring *ring,
			     struct bcm430x_dmadesc *desc,
			     struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	if (!(meta->flags & BCM430x_DESCFLAG_MAPPED))
		return;

	if (ring->flags & BCM430x_RINGFLAG_TX)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;

	desc->control = 0x00000000;
	desc->address = 0x00000000;

	dma_unmap_single(&ring->bcm->pci_dev->dev,
			 meta->dmaaddr, meta->size, dir);

	meta->flags &= ~ BCM430x_DESCFLAG_MAPPED;  
}

static inline int alloc_descbuffer(struct bcm430x_dmaring *ring,
				   struct bcm430x_dmadesc *desc,
				   struct bcm430x_dmadesc_meta *meta,
				   size_t size, unsigned int gfp_flags)
{
	meta->skb = __dev_alloc_skb(size, gfp_flags | GFP_DMA);
	if (unlikely(!meta->skb))
		return -ENOMEM;
	return 0;
}

static inline void free_descbuffer(struct bcm430x_dmaring *ring,
				   struct bcm430x_dmadesc *desc,
				   struct bcm430x_dmadesc_meta *meta,
				   int irq)
{
	if (!meta->skb)
		return;

	assert(!(meta->flags & BCM430x_DESCFLAG_MAPPED));
	if (irq)
		dev_kfree_skb_irq(meta->skb);
	else
		dev_kfree_skb(meta->skb);
	memset(meta, 0, sizeof(*meta));
}

static int alloc_initial_descbuffers(struct bcm430x_dmaring *ring)
{
	size_t buffersize = 0;
	int i, err = 0, num_buffers = 0;
	struct bcm430x_dmadesc *desc = 0;
	struct bcm430x_dmadesc_meta *meta;

	if (!(ring->flags & BCM430x_RINGFLAG_TX)) {
		num_buffers = BCM430x_NUM_RXBUFFERS;
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

		err = alloc_descbuffer(ring, desc, meta,
				       buffersize, GFP_KERNEL);
		if (err)
			goto err_unwind;
		err = map_descbuffer(ring, desc, meta);
		if (err)
			goto err_unwind;

		assert(used_slots(ring) <= ring->nr_slots);
		ring->last_used++;
	}
	desc->control |= BCM430x_DMADTOR_DTABLEEND;

out:
	return err;

err_unwind:
	for ( ; i >= 0; i--) {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		unmap_descbuffer(ring, desc, meta);
		free_descbuffer(ring, desc, meta, 0);
	}
	goto out;
}

static int dmacontroller_setup(struct bcm430x_dmaring *ring)
{
	int err;
	u32 value;

	if (ring->flags & BCM430x_RINGFLAG_TX) {
		err = dmacontroller_tx_reset(ring);
		if (err)
			goto out;
		/* Set Transmit Control register to "transmit enable" */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_CONTROL,
				BCM430x_DMA_TXCTRL_ENABLE);
		/* Set Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				ring->dmabase);
	} else {
		err = dmacontroller_rx_reset(ring);
		if (err)
			goto out;
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
				ring->dmabase);
	}

out:
	return err;
}

static void dmacontroller_cleanup(struct bcm430x_dmaring *ring)
{
	if (ring->flags & BCM430x_RINGFLAG_TX) {
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

static void free_all_descbuffers(struct bcm430x_dmaring *ring)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	int i;

	if (!used_slots(ring))
		return;

	i = first_used_slot(ring);
	do {
		desc = ring->vbase + i;
		meta = ring->meta + i;

		unmap_descbuffer(ring, desc, meta);
		free_descbuffer(ring, desc, meta, 0);

		i = next_slot(ring, i);
	} while (i != last_used_slot(ring));
	//TODO?
}

struct bcm430x_dmaring * bcm430x_setup_dmaring(struct bcm430x_private *bcm,
					       u16 dma_controller_base,
					       int nr_descriptor_slots,
					       int tx)
{
	struct bcm430x_dmaring *ring;
	int err;

	ring = kmalloc(sizeof(*ring), GFP_KERNEL);
	if (!ring)
		goto out;
	memset(ring, 0, sizeof(*ring));

	ring->meta = kmalloc(sizeof(*ring->meta) * nr_descriptor_slots,
			     GFP_KERNEL);
	if (!ring->meta)
		goto err_kfree_ring;
	memset(ring->meta, 0, sizeof(*ring->meta) * nr_descriptor_slots);

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
		ring->flags |= BCM430x_RINGFLAG_TX;

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
	ring = 0;
	goto out;
}

void bcm430x_destroy_dmaring(struct bcm430x_dmaring *ring)
{
	if (!ring)
		return;

	dmacontroller_cleanup(ring);
	free_all_descbuffers(ring);
	free_ringmemory(ring);

	kfree(ring->meta);
	kfree(ring);
}

static inline int dma_tx_fragment_sg(struct bcm430x_dmaring *ring,
				     struct sk_buff *skb,
				     struct bcm430x_dma_txcontext *ctx)
{
	/*TODO: Scatter/Gather IO */
	return -EINVAL;
}

static inline int dma_tx_fragment(struct bcm430x_dmaring *ring,
				  struct sk_buff *skb,
				  struct bcm430x_dma_txcontext *ctx)
{
	int err = -ENODEV;
	int slot;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;

printk("free slots %d\n", free_slots(ring));
//TODO: end of dtor table. bytecounts. slot counts.
	if (unlikely(free_slots(ring) < skb_shinfo(skb)->nr_frags + 1)) {
		/* The queue should be stopped,
		 * if we are low on free slots.
		 */
		printk(KERN_ERR PFX "Out of DMA descriptor slots!\n");
		return -ENOMEM;
	}

	slot = first_free_slot(ring);
	request_slot(ring, slot);
	desc = ring->vbase + slot;
	assert(desc->control == 0x00000000);
	meta = ring->meta + slot;

	if (ctx->cur_frag == 0) {
		/* first fragment.
		 * TODO: insert the txheader
		 * TODO: append the PLCP header
		 */
		desc->control |= BCM430x_DMADTOR_FRAMESTART;
		ctx->desc_index = (u32)slot * sizeof(struct bcm430x_dmadesc);
	}

bcm430x_printk_dump(skb->data, skb->len, "SKB");

	/*TODO: map the buffer. */
//	meta->skb = skb;
	/*TODO: How about freeing the skb? The skb is freed by the ieee80211 subsys on txb free... */
#if 0
	err = map_descbuffer(ring, desc, meta);
	if (unlikely(err)) {
		printk(KERN_ERR PFX "Could not DMA map a sk_buff!\n");
		return_slot(ring, slot);
		goto out;
	}
#endif

	if (skb_shinfo(skb)->nr_frags != 0) {
		/* Map the remaining Scatter/Gather fragments */
		err = dma_tx_fragment_sg(ring, skb, ctx);
		if (err) {
			/*TODO: error handling: unmap, other stuff? */
			return_slot(ring, slot);
			goto out;
		}
		/*TODO: modify desc and meta to point to the last SC buffers */
	}

	if (ctx->cur_frag == ctx->nr_frags - 1) {
		desc->control |= BCM430x_DMADTOR_FRAMEEND;
		desc->control |= BCM430x_DMADTOR_COMPIRQ;
		assert(ctx->desc_index != 0xffffffff);
		/* poke the device. */
printk("Posted desc_index %u on chip\n", ctx->desc_index);
//TODO: do not poke the device directly, but put the request to a queue and hand off to a workqueue or something.
#if 0
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_INDEX,
				ctx->desc_index);
#endif
	}
	/*TODO: return the slots in the interrupt handler. Free skbs (in irq handler)! */

	try_to_suspend_txqueue(ring);

out:
	return err;
}

int bcm430x_dma_transfer_txb(struct bcm430x_dmaring *ring,
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

	assert(ring->flags & BCM430x_RINGFLAG_TX);

	ctx.nr_frags = txb->nr_frags;
	ctx.cur_frag = 0;

	for (i = 0; i < txb->nr_frags; i++) {
		skb = txb->fragments[i];
		ctx.desc_index = 0xffffffff;
		err = dma_tx_fragment(ring, skb, &ctx);
		if (err)
			break;
		ctx.cur_frag++;
	}

	return err;
}

/* vim: set ts=8 sw=8 sts=8: */
