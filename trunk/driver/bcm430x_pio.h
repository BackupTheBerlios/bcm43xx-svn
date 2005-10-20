#ifndef BCM430x_PIO_H_
#define BCM430x_PIO_H_

#include "bcm430x.h"

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>


#define BCM430x_PIO_TXCTL		0x00
#define BCM430x_PIO_TXDATA		0x02
#define BCM430x_PIO_TXQBUFSIZE		0x04
#define BCM430x_PIO_RXCTL		0x08
#define BCM430x_PIO_RXDATA		0x0A

#define BCM430x_PIO_TXCTL_WRITEHI	(1 << 0)
#define BCM430x_PIO_TXCTL_WRITELO	(1 << 1)
#define BCM430x_PIO_TXCTL_COMPLETE	(1 << 2)
#define BCM430x_PIO_TXCTL_INIT		(1 << 3)
#define BCM430x_PIO_TXCTL_SUSPEND	(1 << 7)

#define BCM430x_PIO_RXCTL_INIT		(1 << 0)
#define BCM430x_PIO_RXCTL_READY		(1 << 1)

/* PIO tuning knobs */
#define BCM430x_PIO_TXTIMEOUT		(HZ * 10) /* jiffies */
#define BCM430x_PIO_MAXTXPACKETS	20
#define BCM430x_PIO_MAXTXDEVQPACKETS	31
#define BCM430x_PIO_TXQADJUST		80


struct bcm430x_pioqueue;
struct bcm430x_xmitstatus;

struct bcm430x_pio_txcontext {
	u8 xmitted_frags;
};

struct bcm430x_pio_txpacket {
	struct bcm430x_pioqueue *queue;
	struct ieee80211_txb *txb;
	struct list_head list;
	struct timer_list timeout;
	struct bcm430x_pio_txcontext ctx;

#ifdef BCM430x_DEBUG
	/* Do not free the txb, but the skb contained in it.
	 * This is only used for debugging and the
	 * DebugFS "send" and "sendraw" files.
	 */
	int txb_is_dummy;
	/* For DebugFS "sendraw" */
	int no_txhdr;
#endif /* BCM430x_DEBUG */
};

#define pio_txpacket_getindex(packet) ((int)(packet - packet->queue->__tx_packets_cache)) 

struct bcm430x_pioqueue {
	struct bcm430x_private *bcm;
	u16 mmio_base;

	u8 tx_suspended:1;

	/* Adjusted size of the device internal TX buffer. */
	u16 tx_devq_size;
	/* Used octets of the device internal TX buffer. */
	u16 tx_devq_used;
	/* Used packet slots in the device internal TX buffer. */
	int tx_devq_packets;

	struct list_head txfree;
	struct list_head txqueue;
	spinlock_t txlock;
	struct work_struct txwork;
	struct bcm430x_pio_txpacket __tx_packets_cache[BCM430x_PIO_MAXTXPACKETS];
};

struct bcm430x_pioqueue * bcm430x_setup_pioqueue(struct bcm430x_private *bcm,
						 u16 pio_mmio_base);

void bcm430x_destroy_pioqueue(struct bcm430x_pioqueue *queue);

int bcm430x_pio_transfer_txb(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb);

void FASTCALL(bcm430x_pio_handle_xmitstatus(struct bcm430x_private *bcm,
					    struct bcm430x_xmitstatus *status));

void FASTCALL(bcm430x_pio_rx(struct bcm430x_pioqueue *queue));

#ifdef BCM430x_DEBUG
void bcm430x_pio_tx_frame(struct bcm430x_private *bcm,
			  const char *buf, size_t size);
#endif /* BCM430x_DEBUG */
#endif /* BCM430x_PIO_H_ */
