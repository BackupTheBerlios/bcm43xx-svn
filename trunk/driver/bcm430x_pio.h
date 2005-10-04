#ifndef BCM430x_PIO_H_
#define BCM430x_PIO_H_

#include "bcm430x.h"

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/skbuff.h>


#define BCM430x_PIO_TXCTL		0x00
#define BCM430x_PIO_TXDATA		0x02
/*#define BCM430x_PIO_???		0x04 FIXME */
#define BCM430x_PIO_RXCTL		0x08
#define BCM430x_PIO_RXDATA		0x0A
/*#define BCM430x_PIO_???		0x0C FIXME */

#define BCM430x_PIO_TXCTL_WRITEHI	(1 << 0)
#define BCM430x_PIO_TXCTL_WRITELO	(1 << 1)
#define BCM430x_PIO_TXCTL_COMPLETE	(1 << 2)
#define BCM430x_PIO_TXCTL_INIT		(1 << 3)

#define BCM430x_PIO_RXCTL_INIT		(1 << 0)
#define BCM430x_PIO_RXCTL_READY		(1 << 1)
/*#define BCM430x_PIO_RXCTL_???		(1 << 2) FIXME */
/*#define BCM430x_PIO_RXCTL_???		(1 << 3) FIXME */

struct bcm430x_pio_txpacket {
	struct ieee80211_txb *txb;
	struct list_head list;
};

struct bcm430x_pioqueue {
	struct bcm430x_private *bcm;
	u16 mmio_base;

	u32 hwswap:1;

	struct list_head txqueue;
	spinlock_t txlock;//FIXME: maybe we can do this all lockless, but let's see...
	int nr_txpackets;
	struct work_struct txwork;
};

struct bcm430x_pioqueue * bcm430x_setup_pioqueue(struct bcm430x_private *bcm,
						 u16 pio_mmio_base);

void bcm430x_destroy_pioqueue(struct bcm430x_pioqueue *queue);

int bcm430x_pio_transfer_txb(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb);

#endif /* BCM430x_PIO_H_ */
