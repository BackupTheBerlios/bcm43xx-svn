#ifndef BCM430x_DMA_H_
#define BCM430x_DMA_H_

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/linkage.h>
#include <asm/atomic.h>


/* DMA-Interrupt reasons. */
/*TODO: add the missing ones. */
#define BCM430x_DMAIRQ_ERR0		(1 << 10)
#define BCM430x_DMAIRQ_ERR1		(1 << 11)
#define BCM430x_DMAIRQ_ERR2		(1 << 12)
#define BCM430x_DMAIRQ_ERR3		(1 << 13)
#define BCM430x_DMAIRQ_ERR4		(1 << 14)
#define BCM430x_DMAIRQ_ERR5		(1 << 15)
#define BCM430x_DMAIRQ_RX_DONE		(1 << 16)
/* helpers */
#define BCM430x_DMAIRQ_ANYERR		(BCM430x_DMAIRQ_ERR0 | \
					 BCM430x_DMAIRQ_ERR1 | \
					 BCM430x_DMAIRQ_ERR2 | \
					 BCM430x_DMAIRQ_ERR3 | \
					 BCM430x_DMAIRQ_ERR4 | \
					 BCM430x_DMAIRQ_ERR5)
#define BCM430x_DMAIRQ_FATALERR		(BCM430x_DMAIRQ_ERR0 | \
					 BCM430x_DMAIRQ_ERR1 | \
					 BCM430x_DMAIRQ_ERR2 | \
					 BCM430x_DMAIRQ_ERR4 | \
					 BCM430x_DMAIRQ_ERR5)
#define BCM430x_DMAIRQ_NONFATALERR	BCM430x_DMAIRQ_ERR3

/* DMA controller register offsets. (relative to BCM430x_DMA#_BASE) */
#define BCM430x_DMA_TX_CONTROL		0x00
#define BCM430x_DMA_TX_DESC_RING	0x04
#define BCM430x_DMA_TX_DESC_INDEX	0x08
#define BCM430x_DMA_TX_STATUS		0x0c
#define BCM430x_DMA_RX_CONTROL		0x10
#define BCM430x_DMA_RX_DESC_RING	0x14
#define BCM430x_DMA_RX_DESC_INDEX	0x18
#define BCM430x_DMA_RX_STATUS		0x1c

/* DMA controller channel control word values. */
#define BCM430x_DMA_TXCTRL_ENABLE		(1 << 0)
#define BCM430x_DMA_TXCTRL_SUSPEND		(1 << 1)
#define BCM430x_DMA_TXCTRL_LOOPBACK		(1 << 2)
#define BCM430x_DMA_TXCTRL_FLUSH		(1 << 4)
#define BCM430x_DMA_RXCTRL_ENABLE		(1 << 0)
#define BCM430x_DMA_RXCTRL_FRAMEOFF_MASK	0x000000fe
#define BCM430x_DMA_RXCTRL_FRAMEOFF_SHIFT	1
#define BCM430x_DMA_RXCTRL_PIO			(1 << 8)
/* DMA controller channel status word values. */
#define BCM430x_DMA_TXSTAT_DPTR_MASK		0x00000fff
#define BCM430x_DMA_TXSTAT_STAT_MASK		0x0000f000
#define BCM430x_DMA_TXSTAT_STAT_DISABLED	0x00000000
#define BCM430x_DMA_TXSTAT_STAT_ACTIVE		0x00001000
#define BCM430x_DMA_TXSTAT_STAT_IDLEWAIT	0x00002000
#define BCM430x_DMA_TXSTAT_STAT_STOPPED		0x00003000
#define BCM430x_DMA_TXSTAT_STAT_SUSP		0x00004000
#define BCM430x_DMA_TXSTAT_ERROR_MASK		0x000f0000
#define BCM430x_DMA_TXSTAT_FLUSHED		(1 << 20)
#define BCM430x_DMA_RXSTAT_DPTR_MASK		0x00000fff
#define BCM430x_DMA_RXSTAT_STAT_MASK		0x0000f000
#define BCM430x_DMA_RXSTAT_STAT_DISABLED	0x00000000
#define BCM430x_DMA_RXSTAT_STAT_ACTIVE		0x00001000
#define BCM430x_DMA_RXSTAT_STAT_IDLEWAIT	0x00002000
#define BCM430x_DMA_RXSTAT_STAT_RESERVED	0x00003000
#define BCM430x_DMA_RXSTAT_STAT_ERRORS		0x00004000
#define BCM430x_DMA_RXSTAT_ERROR_MASK		0x000f0000

/* DMA descriptor control field values. */
#define BCM430x_DMADTOR_BYTECNT_MASK		0x00001fff
#define BCM430x_DMADTOR_DTABLEEND		(1 << 28) /* end of descriptor table */
#define BCM430x_DMADTOR_COMPIRQ			(1 << 29) /* irq on comppletion request */
#define BCM430x_DMADTOR_FRAMEEND		(1 << 30)
#define BCM430x_DMADTOR_FRAMESTART		(1 << 31)


/* DMA engine tuning knobs */
#define BCM430x_DMA_RINGMEMSIZE		PAGE_SIZE
#define BCM430x_DMA_DMABUSADDRMASK	0x3FFFFFFF
#define BCM430x_DMA_DMABUSADDROFFSET	(1 << 30)
#define BCM430x_TXRING_SLOTS		20 /*FIXME: set to 256 again */
#define BCM430x_RXRING_SLOTS		256
#define BCM430x_DMA1_RX_FRAMEOFFSET	30
#define BCM430x_NUM_RXBUFFERS		16
#define BCM430x_DMA1_RXBUFFERSIZE	2048
#define BCM430x_DMA4_RXBUFFERSIZE	16
#define BCM430x_DMA_TXTIMEOUT		(HZ * 10) /* jiffies */

/* suspend the tx queue, if less than this percent slots are free. */
#define BCM430x_TXSUSPEND_PERCENT	20
/* resume the tx queue, if more than this percent slots are free. */
#define BCM430x_TXRESUME_PERCENT	50


struct sk_buff;
struct bcm430x_private;
struct bcm430x_xmitstatus;


struct bcm430x_dmadesc {
	u32 _control;
	u32 _address;
} __attribute__((__packed__));

/* macros to access the bcm430x_dmadesc struct */
#define get_desc_ctl(desc)		le32_to_cpu((desc)->_control)
#define set_desc_ctl(desc, ctl)		do { (desc)->_control = cpu_to_le32(ctl); } while (0)
#define get_desc_addr(desc)		le32_to_cpu((desc)->_address)
#define set_desc_addr(desc, addr)	do { (desc)->_address = cpu_to_le32(addr); } while (0)

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
	struct timer_list timeout;
	struct list_head list;
};

/* Get the slot for a txitem */
#define dma_txitem_getslot(item)  ((int)((item) - (item)->ring->__tx_items_cache))

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
	/* packet size including all wireless headers. */
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

int FASTCALL(bcm430x_dma_transfer_txb(struct bcm430x_private *bcm,
				      struct ieee80211_txb *txb));

void FASTCALL(bcm430x_dma_handle_xmitstatus(struct bcm430x_private *bcm,
					    struct bcm430x_xmitstatus *status));

#ifdef BCM430x_DEBUG
void bcm430x_dma_tx_frame(struct bcm430x_private *bcm,
			  const char *buf, size_t size);
#endif /* BCM430x_DEBUG */
#endif /* BCM430x_DMA_H_ */
