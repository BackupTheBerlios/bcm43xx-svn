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


static void unmap_descbuffer(struct bcm430x_dmaring *ring,
			     struct bcm430x_dmadesc *desc,
			     struct bcm430x_dmadesc_meta *meta);


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

/* request a slot for usage. */
static inline int request_slot(struct bcm430x_dmaring *ring)
{
	int slot;
	int prev;
	struct bcm430x_dmadesc *desc;

	assert(free_slots(ring));

	prev = ring->last_used;
	slot = next_slot(ring, ring->last_used);
	ring->last_used = slot;
	if (ring->first_used < 0)
		ring->first_used = slot;

	if (prev < slot) {
		desc = ring->vbase + prev;
		set_desc_ctl(desc, get_desc_ctl(desc) & ~ BCM430x_DMADTOR_DTABLEEND);
	}

	if (ring->tx)
		try_to_suspend_txqueue(ring);

	return slot;
}

/* return a slot to the free slots. */
static inline void return_slot(struct bcm430x_dmaring *ring, int slot)
{
	struct bcm430x_dmadesc *desc;

	if (used_slots(ring) > 1) {
		if (ring->first_used == slot)
			ring->first_used = next_slot(ring, slot);
		assert(ring->last_used != slot);
	} else {
		ring->first_used = -1;
		ring->last_used = -1;
	}
	desc = ring->vbase + slot;
	set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_DTABLEEND);

	if (ring->tx)
		try_to_resume_txqueue(ring);
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

static inline void dmacontroller_poke_tx(struct bcm430x_dmaring *ring,
					 int slot)
{
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

/* Free all stuff belonging to a complete TX frame.
 * Begin at slot.
 * This is to be called on tx_timeout and completion IRQ.
 */
static void free_descriptor_frame(struct bcm430x_dmaring *ring,
				  int slot)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	int last_fragment;

	assert(get_desc_ctl(ring->vbase + slot) & BCM430x_DMADTOR_FRAMESTART);

	while (1) {
		assert(slot < ring->nr_slots);
		desc = ring->vbase + slot;
		meta = ring->meta + slot;

		last_fragment = !!(get_desc_ctl(desc) & BCM430x_DMADTOR_FRAMEEND);
		unmap_descbuffer(ring, desc, meta);
		if (!meta->nofree_skb)
			dev_kfree_skb_any(meta->skb);
		meta->skb = 0;
		if (meta->txb) {
			ieee80211_txb_free(meta->txb);
			meta->txb = 0;
		}
		/*TODO?*/

		if (last_fragment)
			break;
		slot++;
	}
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

static void setup_ringmemory(struct bcm430x_dmaring *ring)
{
	int i;
	struct bcm430x_dmadesc *desc;

	for (i = 0; i < ring->nr_slots; i++) {
		desc = ring->vbase + i;

		set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_DTABLEEND);
	}
}

static void setup_txitems_cache(struct bcm430x_dmaring *ring)
{
	int i;

	for (i = 0; i < BCM430x_TXRING_SLOTS; i++) {
		ring->__tx_items_cache[i].ring = ring;
		init_timer(&ring->__tx_items_cache[i].timeout);
	}
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

static int map_descbuffer(struct bcm430x_dmaring *ring,
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
	if (!meta->dmaaddr) /*FIXME: can this fail? */
		return -ENOMEM;
	meta->mapped = 1;

	set_desc_addr(desc, meta->dmaaddr);
	set_desc_ctl(desc, get_desc_ctl(desc) | (BCM430x_DMADTOR_BYTECNT_MASK & meta->skb->len));

	return 0;
}

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

	set_desc_ctl(desc, 0x00000000);
	set_desc_addr(desc, 0x00000000);

	dma_unmap_single(&ring->bcm->pci_dev->dev,
			 meta->dmaaddr, meta->skb->len, dir);

	meta->mapped = 0;  
}

static inline int alloc_descbuffer(struct bcm430x_dmaring *ring,
				   struct bcm430x_dmadesc *desc,
				   struct bcm430x_dmadesc_meta *meta,
				   size_t size, unsigned int gfp_flags)
{
	assert(!meta->mapped);
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

	assert(!meta->mapped);
	if (meta->nofree_skb) {
		assert(meta->txb);
		ieee80211_txb_free(meta->txb);
	} else {
		if (irq)
			dev_kfree_skb_irq(meta->skb);
		else
			dev_kfree_skb(meta->skb);
	}
	memset(meta, 0, sizeof(*meta));
}

static int alloc_initial_descbuffers(struct bcm430x_dmaring *ring)
{
	size_t buffersize = 0;
	int i, err = 0, num_buffers = 0;
	struct bcm430x_dmadesc *desc = 0;
	struct bcm430x_dmadesc_meta *meta;

	if (!ring->tx) {
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
	set_desc_ctl(desc, get_desc_ctl(desc) | BCM430x_DMADTOR_DTABLEEND);

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
	int err = 0;
	u32 value;

	if (ring->tx) {
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
		ring->tx = 1;
	INIT_LIST_HEAD(&ring->xfers);

	err = alloc_ringmemory(ring);
	if (err)
		goto err_kfree_meta;
	setup_ringmemory(ring);
	setup_txitems_cache(ring);
	err = dmacontroller_setup(ring);
	if (err)
		goto err_free_ringmemory;

//dump_ringmemory(ring);
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

static void cancel_txitem(struct bcm430x_dma_txitem *item)
{
	int slot;

	slot = dma_txitem_getslot(item);
	free_descriptor_frame(item->ring, slot);
	list_del(&item->list);
	return_slot(item->ring, slot);
}

static void cancel_transfers(struct bcm430x_dmaring *ring)
{
	struct bcm430x_dma_txitem *item, *tmp_item;

	list_for_each_entry_safe(item, tmp_item, &ring->xfers, list)
		del_timer_sync(&item->timeout);

	list_for_each_entry_safe(item, tmp_item, &ring->xfers, list)
		cancel_txitem(item);

	assert(list_empty(&ring->xfers));
}

void bcm430x_destroy_dmaring(struct bcm430x_dmaring *ring)
{
	if (!ring)
		return;

	/* Device IRQs are disabled prior entering this function,
	 * so no need to take care of concurrency with rx handler stuff.
	 */
	cancel_transfers(ring);
	dmacontroller_cleanup(ring);
	free_all_descbuffers(ring);
	free_ringmemory(ring);

	kfree(ring->meta);
	kfree(ring);
}

static void tx_timeout(unsigned long d)
{
	struct bcm430x_dma_txitem *item = (struct bcm430x_dma_txitem *)d;
	unsigned long flags;

	spin_lock_irqsave(&item->ring->lock, flags);

	/* This txqueue_item timed out.
	 * Drop it and unmap/free all buffers
	 */
	dprintk(KERN_WARNING PFX "DMA TX slot %d timed out!\n",
		dma_txitem_getslot(item));
	cancel_txitem(item);
	spin_unlock_irqrestore(&item->ring->lock, flags);
}

static inline void tx_xfer(struct bcm430x_dmaring *ring,
			   int slot)
{
	struct bcm430x_dma_txitem *item;

	/* "allocate" a txitem from the cache */
	assert(slot < ARRAY_SIZE(ring->__tx_items_cache));
	item = ring->__tx_items_cache + slot;
	assert(item->ring == ring);

	INIT_LIST_HEAD(&item->list);
	list_add_tail(&item->list, &ring->xfers);

	item->timeout.function = tx_timeout;
	item->timeout.data = (unsigned long)item;
	item->timeout.expires = jiffies + BCM430x_DMA_TXTIMEOUT;
	add_timer(&item->timeout);

	dmacontroller_poke_tx(ring, slot);
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
				  struct ieee80211_txb *txb,
				  struct bcm430x_dma_txcontext *ctx)
{
	int err = -ENODEV;
	int slot;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	struct sk_buff *header_skb;

	if (unlikely(free_slots(ring) < skb_shinfo(skb)->nr_frags + 2)) {
		/* The queue should be stopped,
		 * if we are low on free slots.
		 */
		printk(KERN_ERR PFX "Out of DMA descriptor slots!\n");
		return -ENOMEM;
	}

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
		/* Save the first slot number for later. */
		ctx->first_slot = slot;
	}

	if (unlikely(skb_headroom(skb) < sizeof(struct bcm430x_txhdr))) {
		/* SKB has not enough headroom. Blame the ieee80211 subsys for this.
		 * On latest 80211 subsys this should not trigger.
		 * Request another descriptor, which will hold
		 * the device TX header (and PLCP header).
		 */
		dprintk(KERN_WARNING PFX "Not enough skb headroom. "
					 "Using additional descriptor for header.\n");
		header_skb = dev_alloc_skb(sizeof(struct bcm430x_txhdr));
		if (unlikely(!header_skb))
			return -ENOMEM;
		meta->skb = header_skb;
		meta->nofree_skb = 0;
		/* Now calculate and add the tx header.
		 * The tx header includes the PLCP header.
		 */
		bcm430x_generate_txhdr(ring->bcm,
				       (struct bcm430x_txhdr *)header_skb->data,
				       skb->data, skb->len,
				       (ctx->cur_frag == 0),
				       (u16)slot);
		err = map_descbuffer(ring, desc, meta);
		if (unlikely(err)) {
			printk(KERN_ERR PFX "Could not DMA map a sk_buff!\n");
			return_slot(ring, slot);
			goto out;
		}
		/* Request a new slot for the real data. */
		slot = request_slot(ring);
		desc = ring->vbase + slot;
		meta = ring->meta + slot;
	} else {
		/* Reserve enough headroom for tzhe device tx header. */
		__skb_push(skb, sizeof(struct bcm430x_txhdr));
		/* Now calculate and add the tx header.
		 * The tx header includes the PLCP header.
		 */
		bcm430x_generate_txhdr(ring->bcm,
				       (struct bcm430x_txhdr *)skb->data,
				       skb->data + sizeof(struct bcm430x_txhdr),
				       skb->len - sizeof(struct bcm430x_txhdr),
				       (ctx->cur_frag == 0),
				       (u16)slot);
	}
//bcm430x_printk_dump(skb->data, skb->len, "SKB");

	/* write the buffer to the descriptor and map it. */
	meta->skb = skb;
	meta->nofree_skb = 1;
printk(KERN_INFO PFX "mapping %d\n", slot);
	err = map_descbuffer(ring, desc, meta);
	if (unlikely(err)) {
		printk(KERN_ERR PFX "Could not DMA map a sk_buff!\n");
		return_slot(ring, slot);
		goto out;
	}

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
printk(KERN_INFO PFX "transmitting slot %d\n", ctx->first_slot);
		/* Now transfer the whole frame. */
		tx_xfer(ring, ctx->first_slot);
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
		err = dma_tx_fragment(ring, skb, txb, &ctx);
		if (err)
			break;//TODO: correct error handling.
		ctx.cur_frag++;
	}
	spin_unlock_irqrestore(&ring->lock, flags);

//dump_ringmemory(ring);
	return err;
}

int fastcall
bcm430x_dma_transfer_txb(struct bcm430x_private *bcm,
			 struct ieee80211_txb *txb)
{
	return dma_transfer_txb(bcm->current_core->dma->tx_ring1,
				txb);
}

static inline void dma_completion_irq(struct bcm430x_dmaring *ring,
				      u16 cookie)
{
}

void fastcall
bcm430x_dma_completion_irq(struct bcm430x_private *bcm,
			   u16 cookie)
{
	dma_completion_irq(bcm->current_core->dma->tx_ring1,
			   cookie);
}

/* vim: set ts=8 sw=8 sts=8: */
