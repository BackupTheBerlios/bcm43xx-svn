#ifndef BCM430x_DMA_H_
#define BCM430x_DMA_H_

#include <linux/spinlock.h>


struct bcm430x_private;

struct bcm430x_dmadesc {
	u32 control;
	u32 address;
} __attribute__((__packed__));

struct bcm430x_dmadesc_meta {
	void *vaddr;
	dma_addr_t dmaaddr;
	size_t size;
};

#define BCM430x_RINGFLAG_TX	(1 << 0)

struct bcm430x_dmaring {
	spinlock_t lock;
	struct bcm430x_private *bcm;
	/* Kernel virtual base address of the desc ring. */
	struct bcm430x_dmadesc *vbase;
	/* DMA base bus-address of the desc ring. */
	dma_addr_t dmabase;
	/* Meta data about the allocated descriptors. */
	struct bcm430x_dmadesc_meta *meta;
	/* Number of descriptor slots in the ring. */
	unsigned int nr_slots;
	/* last used slot in the ring. */
	unsigned int last_used;
	/* number of used slots in the ring. */
	unsigned int nr_used;
	/* Marks to suspend/resume the queue. */
	unsigned int suspend_mark;
	unsigned int resume_mark;
	/* The MMIO base register of the DMA controller, this
	 * ring is posted to.
	 */
	u16 mmio_base;
	/* ring flags. */
	u32 flags;
};


struct bcm430x_dmaring * bcm430x_alloc_dmaring(struct bcm430x_private *bcm,
					       unsigned int nr_slots,
					       u16 mmio,
					       int tx);
void bcm430x_free_dmaring(struct bcm430x_dmaring *ring);

int bcm430x_post_dmaring(struct bcm430x_dmaring *ring);
void bcm430x_unpost_dmaring(struct bcm430x_dmaring *ring);

#endif /* BCM430x_DMA_H_ */
