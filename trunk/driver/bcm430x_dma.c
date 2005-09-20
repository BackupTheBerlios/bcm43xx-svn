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

#include <linux/dmapool.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <asm/semaphore.h>


static struct bcm430x_ringallocator {
	struct dma_pool *pool;
	int refcnt;
} ringallocator;
static DECLARE_MUTEX(ringallocator_sem);


static inline int free_slots(struct bcm430x_dmaring *ring)
{
	int slots;

	slots = ring->nr_slots - ring->nr_used;
	assert(slots >= 0);

	return slots;
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
	memset(ring->vbase, 0, 4096);

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

static int dmacontroller_rx_reset(struct bcm430x_dmaring *ring)
{
	int i;
	u32 value;

	assert(!(ring->flags & BCM430x_RINGFLAG_TX));

	bcm430x_write32(ring->bcm,
			ring->mmio_base + BCM430x_DMA_RX_CONTROL,
			0x00000000);
	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(ring->bcm,
				       ring->mmio_base + BCM430x_DMA_RX_STATUS);
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

static int dmacontroller_tx_reset(struct bcm430x_dmaring *ring)
{
	int i;
	u32 value;

	assert(ring->flags & BCM430x_RINGFLAG_TX);

	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(ring->bcm,
				       ring->mmio_base + BCM430x_DMA_TX_STATUS);
		value &= BCM430x_DMA_TXSTAT_STAT_MASK;
		if (value == BCM430x_DMA_TXSTAT_STAT_DISABLED ||
		    value == BCM430x_DMA_TXSTAT_STAT_IDLEWAIT ||
		    value == BCM430x_DMA_TXSTAT_STAT_STOPPED)
			break;
		udelay(10);
	}
	bcm430x_write32(ring->bcm,
			ring->mmio_base + BCM430x_DMA_TX_CONTROL,
			0x00000000);
	for (i = 0; i < 1000; i++) {
		value = bcm430x_read32(ring->bcm,
				       ring->mmio_base + BCM430x_DMA_TX_STATUS);
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

static int dmacontroller_setup(struct bcm430x_dmaring *ring)
{
	int err;

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

static void free_descbuffer(struct bcm430x_dmaring *ring,
			    struct bcm430x_dmadesc *desc,
			    struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	if (meta->flags & BCM430x_DESCFLAG_MAPPED) {
		if (ring->flags & BCM430x_RINGFLAG_TX)
			dir = DMA_TO_DEVICE;
		else
			dir = DMA_FROM_DEVICE;
		dma_unmap_single(&ring->bcm->pci_dev->dev,
				 meta->dmaaddr, meta->size, dir);
	}
	kfree(meta->vaddr);
	memset(meta, 0, sizeof(*meta));
	memset(desc, 0, sizeof(*desc));
}

static void free_all_descbuffers(struct bcm430x_dmaring *ring)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	unsigned int i;

	for (i = 0; i < ring->nr_used; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;
		free_descbuffer(ring, desc, meta);
	}
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
	ring->last_used = -1;
//	ring->suspend_mark = nr_slots;
//	ring->resume_mark = nr_slots / 2;
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

#if 0

static int bcm430x_alloc_desc(struct bcm430x_dmaring *ring,
			      struct bcm430x_dmadesc *desc,
			      struct bcm430x_dmadesc_meta *meta)
{
	int err = -ENOMEM;
	void *buf;
	dma_addr_t dmaaddr;
	enum dma_data_direction dir;

	meta->size = 2048;
	buf = kmalloc(meta->size, GFP_KERNEL | GFP_DMA);
	if (!buf)
		goto out;
	if (ring->flags & BCM430x_RINGFLAG_TX)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	dmaaddr = dma_map_single(&ring->bcm->pci_dev->dev,
				 buf, meta->size, dir);
	if (!dmaaddr) {
		printk(KERN_ERR PFX "DMA remap failed.\n");
		goto err_kfree;
	}

	meta->vaddr = buf;
	meta->dmaaddr = dmaaddr;

	desc->control |= BCM430x_DMADTOR_BYTECNT_MASK & meta->size;
	desc->address = dmaaddr;

out:
	return err;

err_kfree:
	kfree(buf);
	goto out;
}

static int bcm430x_append_desc(struct bcm430x_dmaring *ring)
{
	int err;
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	unsigned int i;

	assert(bcm430x_free_slots(ring) >= 1);

	i = ring->last_used + 1;

	desc = ring->vbase + i;
	meta = ring->meta + i;
	err = bcm430x_alloc_desc(ring, desc, meta);
	if (err)
		return err;
	ring->last_used++;
	ring->nr_used++;

	return 0;
}

static int bcm430x_append_descs(struct bcm430x_dmaring *ring,
				unsigned int nr)
{
	struct bcm430x_dmadesc *desc;
	unsigned int i;

	for ( ; nr; nr--)
		bcm430x_append_desc(ring);
	i = ring->last_used + 1;
	desc = ring->vbase + i;
	desc->control = BCM430x_DMADTOR_DTABLEEND;

	return 0;
}
#endif

/* vim: set ts=8 sw=8 sts=8: */
