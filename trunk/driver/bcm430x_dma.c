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
#include <asm/semaphore.h>


struct bcm430x_ringallocator {
	struct dma_pool *pool;
	int refcnt;
};

static struct bcm430x_ringallocator ringallocator;
static DECLARE_MUTEX(ringallocator_sem);


static inline unsigned int bcm430x_free_slots(struct bcm430x_dmaring *ring)
{
	return (ring->nr_slots - ring->nr_used);
}

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

static void bcm430x_free_desc(struct bcm430x_dmaring *ring,
			      struct bcm430x_dmadesc *desc,
			      struct bcm430x_dmadesc_meta *meta)
{
	enum dma_data_direction dir;

	if (ring->flags & BCM430x_RINGFLAG_TX)
		dir = DMA_TO_DEVICE;
	else
		dir = DMA_FROM_DEVICE;
	dma_unmap_single(&ring->bcm->pci_dev->dev,
			 meta->dmaaddr, meta->size, dir);
	kfree(meta->vaddr);
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

static void bcm430x_free_descs(struct bcm430x_dmaring *ring)
{
	struct bcm430x_dmadesc *desc;
	struct bcm430x_dmadesc_meta *meta;
	unsigned int i;

	for (i = 0; i < ring->nr_used; i++) {
		desc = ring->vbase + i;
		meta = ring->meta + i;
		bcm430x_free_desc(ring, desc, meta);
	}
}

static int bcm430x_alloc_ringmemory(struct bcm430x_dmaring *ring)
{
	int err = 0;
	struct pci_dev *pci_dev = ring->bcm->pci_dev;
	struct device *dev = &pci_dev->dev;

	assert((ring->nr_slots + 1) * sizeof(struct bcm430x_dmadesc) < 4096);

	down(&ringallocator_sem);

	if (ringallocator.refcnt == 0) {
		ringallocator.pool = dma_pool_create(DRV_NAME "_dmarings",
						     dev, 4096, 4096, 4096);
		if (!ringallocator.pool) {
			printk(KERN_ERR PFX "Could not allocate DMA-ring pool.\n");
			err = -ENOMEM;
			goto out_up;
		}
	}

	ring->vbase = dma_pool_alloc(ringallocator.pool, GFP_KERNEL,
				     &ring->dmabase);
	if (!ring->vbase) {
		printk(KERN_ERR PFX "Could not allocate DMA ring.\n");
		err = -ENOMEM;
		goto err_free_ring;
	}
	memset(ring->vbase, 0, 4096);

	ringallocator.refcnt++;
out_up:
	up(&ringallocator_sem);

	return err;

err_free_ring:
	if (ringallocator.refcnt == 0) {
		dma_pool_destroy(ringallocator.pool);
		ringallocator.pool = 0;
	}
	goto out_up;
}

static void bcm430x_free_ringmemory(struct bcm430x_dmaring *ring)
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

struct bcm430x_dmaring * bcm430x_alloc_dmaring(struct bcm430x_private *bcm,
					       unsigned int nr_slots,
					       u16 mmio,
					       int tx)
{
	unsigned int initial_descs;
	struct bcm430x_dmaring *ring;

	ring = kmalloc(sizeof(*ring), GFP_KERNEL);
	if (!ring)
		goto out;
	memset(ring, 0, sizeof(*ring));

	ring->meta = kmalloc(sizeof(*ring->meta), GFP_KERNEL);
	if (!ring->meta)
		goto err_kfree_ring;

	spin_lock_init(&ring->lock);
	ring->bcm = bcm;
	ring->nr_slots = nr_slots;
	ring->suspend_mark = nr_slots;
	ring->resume_mark = nr_slots / 2;
	ring->mmio_base = mmio;
	if (tx)
		ring->flags |= BCM430x_RINGFLAG_TX;

	if (bcm430x_alloc_ringmemory(ring))
		goto err_kfree_meta;

	if (tx)
		initial_descs = 0;
	else
		initial_descs = 16;

	if (bcm430x_append_descs(ring, initial_descs))
		goto err_free_ring;

out:
	return ring;

err_free_ring:
	bcm430x_free_ringmemory(ring);
err_kfree_meta:
	kfree(ring->meta);
err_kfree_ring:
	kfree(ring);
	ring = 0;
	goto out;
}

void bcm430x_free_dmaring(struct bcm430x_dmaring *ring)
{
	bcm430x_free_descs(ring);
	bcm430x_free_ringmemory(ring);
	kfree(ring->meta);
	kfree(ring);
}

int bcm430x_post_dmaring(struct bcm430x_dmaring *ring)
{/*TODO*/
	if (ring->flags & BCM430x_RINGFLAG_TX) {
		/* Set Transmit Control register to "transmit enable" */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_CONTROL,
				BCM430x_DMA_TXCTRL_ENABLE);
		/* Set Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				ring->dmabase);
	} else {
		/* Set Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_RING,
				ring->dmabase);
	}
	return 0;
}

void bcm430x_unpost_dmaring(struct bcm430x_dmaring *ring)
{
	if (ring->flags & BCM430x_RINGFLAG_TX) {
		/* Zero out Transmit Control register. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_CONTROL,
				0x00000000);
		/* Zero out Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_TX_DESC_RING,
				0x00000000);
	} else {
		/* Zero out Transmit Control register. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_CONTROL,
				0x00000000);
		/* Zero out Transmit Descriptor ring address. */
		bcm430x_write32(ring->bcm,
				ring->mmio_base + BCM430x_DMA_RX_DESC_RING,
				0x00000000);
	}
}

/* vim: set ts=8 sw=8 sts=8: */
