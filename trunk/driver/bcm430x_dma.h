#ifndef BCM430x_DMA_H_
#define BCM430x_DMA_H_

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <asm/atomic.h>


#define BCM430x_TXRING_SLOTS		5
#define BCM430x_RXRING_SLOTS		256
#define BCM430x_DMA1_RX_FRAMEOFFSET	30
#define BCM430x_NUM_RXBUFFERS		16
#define BCM430x_DMA1_RXBUFFERSIZE	2048
#define BCM430x_DMA4_RXBUFFERSIZE	16
#define BCM430x_DMA_TXTIMEOUT		(HZ) /* jiffies */
#define BCM430x_DMA_TXWORK_RETRY_DELAY	(HZ / 100) /* jiffies */

/* suspend the tx queue, if less than this percent slots are free. */
#define BCM430x_TXSUSPEND_PERCENT	20
/* resume the tx queue, if more than this percent slots are free. */
#define BCM430x_TXRESUME_PERCENT	50


struct sk_buff;
struct bcm430x_private;


struct bcm430x_dmadesc {
	u32 control;
	u32 address;
} __attribute__((__packed__));

struct bcm430x_dmadesc_meta {
	/* The kernel DMA-able buffer. */
	struct sk_buff *skb;
	/* DMA base bus-address of the descriptor buffer. */
	dma_addr_t dmaaddr;
	/* Various flags. */
	u8 mapped:1,
	   nofree_skb:1;
	/* Pointer to our txb (if any). This should be freed in irq. */
	struct ieee80211_txb *txb;
};

struct bcm430x_dmaring;

struct bcm430x_dma_txitem {
	struct bcm430x_dmaring *ring;
	int slot;
	struct timer_list timeout;
	struct list_head list;
	u8 cancelled:1;
};

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
	/* first used slot in the ring. */
	int first_used;
	/* last used slot in the ring. */
	int last_used;
	/* Marks to suspend/resume the queue. */
	int suspend_mark;
	int resume_mark;
	/* The MMIO base register of the DMA controller, this
	 * ring is posted to.
	 */
	u16 mmio_base;
	/* ring flags. */
	u8 tx:1,
	   suspended:1;
	/* TX stuff. */
	struct bcm430x_dma_txitem __tx_items_cache[BCM430x_TXRING_SLOTS];
	/* pending transfers */
	struct list_head xfers;
};

struct bcm430x_dma_txcontext {
	u8 nr_frags;
	u8 cur_frag;
	int first_slot; /* to poke the device. */
};


struct bcm430x_dmaring * bcm430x_setup_dmaring(struct bcm430x_private *bcm,
					       u16 dma_controller_base,
					       int nr_descriptor_slots,
					       int tx);

void bcm430x_destroy_dmaring(struct bcm430x_dmaring *ring);

int bcm430x_dmacontroller_rx_reset(struct bcm430x_private *bcm,
				   u16 dmacontroller_mmio_base);

int bcm430x_dmacontroller_tx_reset(struct bcm430x_private *bcm,
				   u16 dmacontroller_mmio_base);

int bcm430x_dma_transfer_txb(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb);

void bcm430x_dma_completion_irq(struct bcm430x_private *bcm,
				int cookie/*TODO: cookie as parameter */);

#endif /* BCM430x_DMA_H_ */
