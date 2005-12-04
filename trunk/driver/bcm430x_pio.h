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

#define BCM430x_PIO_RXCTL_DATAAVAILABLE	(1 << 0)
#define BCM430x_PIO_RXCTL_READY		(1 << 1)

/* PIO constants */
#define BCM430x_PIO_MAXTXDEVQPACKETS	31
#define BCM430x_PIO_TXQADJUST		80

/* PIO tuning knobs */
#define BCM430x_PIO_MAXTXPACKETS	256


struct bcm430x_pioqueue;
struct bcm430x_xmitstatus;

struct bcm430x_pio_txpacket {
	struct bcm430x_pioqueue *queue;
	struct ieee80211_txb *txb;
	struct list_head list;

	u8 xmitted_frags;
	u16 xmitted_octets;

	/* Do not free the txb, but the skb contained in it.
	 * This is only used for debugging and the
	 * DebugFS "send" and "sendraw" files.
	 */
	u8 txb_is_dummy:1;
	/* For DebugFS "sendraw" */
	u8 no_txhdr:1;
};

#define pio_txpacket_getindex(packet) ((int)((packet) - (packet)->queue->__tx_packets_cache)) 

struct bcm430x_pioqueue {
	struct bcm430x_private *bcm;
	u16 mmio_base;

	u8 tx_suspended:1;

	/* Adjusted size of the device internal TX buffer. */
	u16 tx_devq_size;
	/* Used octets of the device internal TX buffer. */
	u16 tx_devq_used;
	/* Used packet slots in the device internal TX buffer. */
	u8 tx_devq_packets;
	/* Packets from the txfree list can
	 * be taken on incoming TX requests.
	 */
	struct list_head txfree;
	/* Packets on the txqueue are queued,
	 * but not completely written to the chip, yet.
	 */
	struct list_head txqueue;
	/* Packets on the txrunning queue are completely
	 * posted to the device. We are waiting for the txstatus.
	 */
	struct list_head txrunning;
	/* Locking of the TX queues and the accounting. */
	spinlock_t txlock;
	struct work_struct txwork;
	struct bcm430x_pio_txpacket __tx_packets_cache[BCM430x_PIO_MAXTXPACKETS];
};

int bcm430x_pio_init(struct bcm430x_private *bcm);
void bcm430x_pio_free(struct bcm430x_private *bcm);

int FASTCALL(bcm430x_pio_transfer_txb(struct bcm430x_private *bcm,
				      struct ieee80211_txb *txb));
void FASTCALL(bcm430x_pio_handle_xmitstatus(struct bcm430x_private *bcm,
					    struct bcm430x_xmitstatus *status));
int FASTCALL(bcm430x_pio_tx_frame(struct bcm430x_pioqueue *queue,
				  const char *buf, size_t size));

void FASTCALL(bcm430x_pio_rx(struct bcm430x_pioqueue *queue));
#endif /* BCM430x_PIO_H_ */
