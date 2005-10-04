/*

  Broadcom BCM430x wireless driver

  PIO Transmission

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
#include "bcm430x_pio.h"


static inline
void pio_tx_write_fragment(struct bcm430x_pioqueue *queue,
			   struct sk_buff *skb)
{
	struct bcm430x_private *bcm = queue->bcm;
	unsigned int i;
	u16 data;

	/*TODO: fragmented skb */

	if (bcm->current_core->rev < 3) {
		//TODO: see pio_tx_packet()
	}

	bcm430x_write16(bcm, queue->mmio_base + BCM430x_PIO_TXCTL,
			BCM430x_PIO_TXCTL_WRITEHI | BCM430x_PIO_TXCTL_WRITELO);

	for (i = 0; i < skb->len - 1; i += 2) {
		data = be16_to_cpu( *((u16 *)(skb->data + i)) );
//		if (!queue->hwswap)
//			data = swab16(data);
		bcm430x_write16(bcm, queue->mmio_base + BCM430x_PIO_TXDATA, data);
	}
	//TODO: byte left?
}

static inline
void pio_tx_packet(struct bcm430x_pioqueue *queue,
		   struct ieee80211_txb *txb)
{
	struct bcm430x_private *bcm = queue->bcm;
	struct sk_buff *skb;
	int i;

	bcm430x_write16(bcm, queue->mmio_base + BCM430x_PIO_TXCTL,
			BCM430x_PIO_TXCTL_INIT);
	if (bcm->current_core->rev < 3) {
		TODO(); /*TODO: save the last 16 bits for xmit later */
	}

	for (i = 0; i < txb->nr_frags; i++) {
		skb = txb->fragments[i];
		pio_tx_write_fragment(queue, skb);
	}

	if (bcm->current_core->rev < 3) {
		TODO(); /*TODO: see above */
	} else {
		bcm430x_write16(bcm, queue->mmio_base + BCM430x_PIO_TXCTL,
				BCM430x_PIO_TXCTL_COMPLETE);
	}
}

static void txwork_handler(void *d)
{
	struct bcm430x_pioqueue *queue = d;
	unsigned long flags;
	struct bcm430x_pio_txpacket *packet, *tmp_packet;

	spin_lock_irqsave(&queue->txlock, flags);

	list_for_each_entry_safe(packet, tmp_packet, &queue->txqueue, list) {
printk(KERN_INFO PFX "sending packet to device\n");
		pio_tx_packet(queue, packet->txb);

		list_del(&packet->list);
		kfree(packet);
	}

	spin_unlock_irqrestore(&queue->txlock, flags);
}

struct bcm430x_pioqueue * bcm430x_setup_pioqueue(struct bcm430x_private *bcm,
						 u16 pio_mmio_base)
{
	struct bcm430x_pioqueue *queue;
	u32 value;

	queue = kmalloc(sizeof(*queue), GFP_KERNEL);
	if (!queue)
		goto out;
	memset(queue, 0, sizeof(*queue));

	queue->bcm = bcm;
	queue->mmio_base = pio_mmio_base;

	INIT_LIST_HEAD(&queue->txqueue);
	spin_lock_init(&queue->txlock);
	INIT_WORK(&queue->txwork, txwork_handler, queue);

	value = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	if (value & BCM430x_SBF_XFER_REG_BYTESWAP)
		queue->hwswap = 1;

out:
	return queue;
}

void bcm430x_destroy_pioqueue(struct bcm430x_pioqueue *queue)
{
	if (!queue)
		return;

	//TODO: cancel all pending work, etc...
	kfree(queue);
}

static inline
int pio_transfer_txb(struct bcm430x_pioqueue *queue,
		     struct ieee80211_txb *txb)
{
	struct bcm430x_pio_txpacket *packet;
	unsigned long flags;
	int err;

	packet = kmalloc(sizeof(*packet), GFP_ATOMIC);
	if (!packet)
		return -ENOMEM;

	packet->txb = txb;
	INIT_LIST_HEAD(&packet->list);

	spin_lock_irqsave(&queue->txlock, flags);
	list_add(&packet->list, &queue->txqueue);
	spin_unlock_irqrestore(&queue->txlock, flags);

	err = queue_work(queue->bcm->workqueue, &queue->txwork);
printk("ERR %d\n", err);
	return 0;
}

int bcm430x_pio_transfer_txb(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb)
{
	int err;

	err = pio_transfer_txb(bcm->current_core->pio->queue1,
			       txb);

	return err;
}

/* vim: set ts=8 sw=8 sts=8: */
