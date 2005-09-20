#ifndef BCM430x_DMA_H_
#define BCM430x_DMA_H_

#include <linux/spinlock.h>


#define BCM430x_TXRING_SLOTS	256
#define BCM430x_RXRING_SLOTS	256


struct bcm430x_private;

struct bcm430x_dmadesc {
	u32 control;
	u32 address;
} __attribute__((__packed__));

#define BCM430x_DESCFLAG_MAPPED	(1 << 0)

struct bcm430x_dmadesc_meta {
	void *vaddr;
	dma_addr_t dmaaddr;
	size_t size;
	u32 flags;
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
	int nr_slots;
	/* last used slot in the ring. */
	int last_used;
	/* number of used slots in the ring. */
	int nr_used;
	/* Marks to suspend/resume the queue. */
	int suspend_mark;
	int resume_mark;
	/* The MMIO base register of the DMA controller, this
	 * ring is posted to.
	 */
	u16 mmio_base;
	/* ring flags. */
	u32 flags;
};

struct bcm430x_dmaring * bcm430x_setup_dmaring(struct bcm430x_private *bcm,
					       u16 dma_controller_base,
					       int nr_descriptor_slots,
					       int tx);

void bcm430x_destroy_dmaring(struct bcm430x_dmaring *ring);

#endif /* BCM430x_DMA_H_ */
