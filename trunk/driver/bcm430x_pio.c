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
#include "bcm430x_main.h"


static inline
u16 bcm430x_pio_swapdata(struct bcm430x_pioqueue *queue,
			 u16 data)
{
	if (!queue->hwswap)
		return swab16(data);
	return data;
}

static inline
u16 bcm430x_pio_read(struct bcm430x_pioqueue *queue,
		     u16 offset)
{
	return bcm430x_read16(queue->bcm, queue->mmio_base + offset);
}

static inline
void bcm430x_pio_write(struct bcm430x_pioqueue *queue,
		       u16 offset, u16 value)
{
	bcm430x_write16(queue->bcm, queue->mmio_base + offset, value);
}

static inline
void suspend_txqueue(struct bcm430x_pioqueue *queue)
{
	assert(!queue->tx_suspended);
	netif_stop_queue(queue->bcm->net_dev);
	queue->tx_suspended = 1;
}

static inline
void try_to_suspend_txqueue(struct bcm430x_pioqueue *queue)
{
	if (queue->nr_txqueued < BCM430x_PIO_MAXTXPACKETS)
		return;
	suspend_txqueue(queue);
}

static inline
void resume_txqueue(struct bcm430x_pioqueue *queue)
{
	queue->tx_suspended = 0;
	netif_wake_queue(queue->bcm->net_dev);
}

static inline
void try_to_resume_txqueue(struct bcm430x_pioqueue *queue)
{
	if (!queue->tx_suspended)
		return;
	resume_txqueue(queue);
}

static inline
void tx_start(struct bcm430x_pioqueue *queue)
{
	bcm430x_pio_write(queue, BCM430x_PIO_TXCTL, BCM430x_PIO_TXCTL_INIT);
}

static inline
void tx_octet(struct bcm430x_pioqueue *queue,
	      u8 octet)
{
	//FIXME: data endianess?
	if (queue->bcm->current_core->rev < 3) {
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, octet);
		bcm430x_pio_write(queue, BCM430x_PIO_TXCTL, BCM430x_PIO_TXCTL_WRITEHI);
	} else {
		bcm430x_pio_write(queue, BCM430x_PIO_TXCTL, BCM430x_PIO_TXCTL_WRITEHI);
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, octet);
	}
}

static inline
void tx_data(struct bcm430x_pioqueue *queue,
	     u8 *packet,
	     unsigned int octets)
{
	u16 data;
	unsigned int i = 0;

	if (queue->bcm->current_core->rev < 3) {
		data = be16_to_cpu( *((u16 *)packet) );
		data = bcm430x_pio_swapdata(queue, data);
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, data);
		i += 2;
	}
	bcm430x_pio_write(queue, BCM430x_PIO_TXCTL,
			  BCM430x_PIO_TXCTL_WRITELO | BCM430x_PIO_TXCTL_WRITEHI);
	for ( ; i < octets; i += 2) {
		data = be16_to_cpu( *((u16 *)(packet + i)) );
		data = bcm430x_pio_swapdata(queue, data);
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, data);
	}
	if (octets % 2)
		tx_octet(queue, packet[octets - 1]);
}

static inline
void tx_complete(struct bcm430x_pioqueue *queue,
		 struct sk_buff *skb)
{
	u16 data;

	if (queue->bcm->current_core->rev < 3) {
		data = be16_to_cpu( *((u16 *)(skb->data + skb->len - 2)) );
		bcm430x_pio_swapdata(queue, data);
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, data);
		bcm430x_pio_write(queue, BCM430x_PIO_TXCTL,
				  BCM430x_PIO_TXCTL_WRITEHI | BCM430x_PIO_TXCTL_COMPLETE);
	} else {
		bcm430x_pio_write(queue, BCM430x_PIO_TXCTL, BCM430x_PIO_TXCTL_COMPLETE);
	}
}

static inline
void pio_tx_write_fragment(struct bcm430x_pioqueue *queue,
			   struct sk_buff *skb,
			   struct bcm430x_pio_txcontext *ctx)
{
	unsigned int octets;

	/*TODO: fragmented skb */

	if (unlikely(skb_headroom(skb) < sizeof(struct bcm430x_txhdr))) {
		printk(KERN_ERR PFX "PIO: Not enough skb headroom!\n");
		return;
	}
	__skb_push(skb, sizeof(struct bcm430x_txhdr));
	bcm430x_generate_txhdr(queue->bcm,
			       (struct bcm430x_txhdr *)skb->data,
			       skb->data + sizeof(struct bcm430x_txhdr),
			       skb->len - sizeof(struct bcm430x_txhdr),
			       (ctx->cur_frag == 0),
			       ctx->cookie);

	tx_start(queue);
	octets = skb->len;
	if (queue->bcm->current_core->rev < 3)
		octets -= 2;
	tx_data(queue, (u8 *)skb->data, octets);
	tx_complete(queue, skb);
}

static inline
void pio_tx_packet(struct bcm430x_pio_txpacket *packet)
{
	struct bcm430x_pioqueue *queue = packet->queue;
	struct ieee80211_txb *txb = packet->txb;
	struct sk_buff *skb;
	struct bcm430x_pio_txcontext ctx;
	int i;

	ctx.nr_frags = txb->nr_frags;
	ctx.cur_frag = 0;
	ctx.cookie = (u16)pio_txpacket_getindex(packet);
	for (i = 0; i < txb->nr_frags; i++) {
		skb = txb->fragments[i];
		pio_tx_write_fragment(queue, skb, &ctx);
		ctx.cur_frag++;
	}
}

static void free_txpacket(struct bcm430x_pio_txpacket *packet)
{
	ieee80211_txb_free(packet->txb);
}

static void cancel_txpacket(struct bcm430x_pio_txpacket *packet)
{
	list_del(&packet->list);
	free_txpacket(packet);
	INIT_LIST_HEAD(&packet->list);
	list_add(&packet->list, &packet->queue->txfree);
	packet->queue->nr_txfired--;
	packet->queue->nr_txqueued--;

	try_to_resume_txqueue(packet->queue);
}

static void tx_timeout(unsigned long d)
{
	struct bcm430x_pio_txpacket *packet = (struct bcm430x_pio_txpacket *)d;
	unsigned long flags;

	spin_lock_irqsave(&packet->queue->txlock, flags);

	dprintk(KERN_WARNING PFX "PIO packet %d timed out!\n",
		pio_txpacket_getindex(packet));
	cancel_txpacket(packet);
	spin_unlock_irqrestore(&packet->queue->txlock, flags);
}

static void txwork_handler(void *d)
{
	struct bcm430x_pioqueue *queue = d;
	unsigned long flags;
	struct bcm430x_pio_txpacket *packet, *tmp_packet;

	spin_lock_irqsave(&queue->txlock, flags);

	list_for_each_entry_safe(packet, tmp_packet, &queue->txqueue, list) {
printk(KERN_INFO PFX "sending packet %d to device\n",
       pio_txpacket_getindex(packet));

		//TODO: check if the devq is full. If so, retry later.

		list_del(&packet->list);
		INIT_LIST_HEAD(&packet->list);
		list_add(&packet->list, &queue->txfired);
		queue->nr_txfired++;

		packet->timeout.function = tx_timeout;
		packet->timeout.data = (unsigned long)packet;
		packet->timeout.expires = jiffies + BCM430x_PIO_TXTIMEOUT;
		add_timer(&packet->timeout);

		pio_tx_packet(packet);
	}

	spin_unlock_irqrestore(&queue->txlock, flags);
}

static void setup_txqueues(struct bcm430x_pioqueue *queue)
{
	struct bcm430x_pio_txpacket *packet;
	int i;

	for (i = 0; i < BCM430x_PIO_MAXTXPACKETS; i++) {
		packet = queue->__tx_packets_cache + i;

		packet->queue = queue;
		init_timer(&packet->timeout);
		INIT_LIST_HEAD(&packet->list);

		list_add(&packet->list, &queue->txfree);
	}
}

struct bcm430x_pioqueue * bcm430x_setup_pioqueue(struct bcm430x_private *bcm,
						 u16 pio_mmio_base)
{
	struct bcm430x_pioqueue *queue;
	u32 value;
	u16 qsize;

	queue = kmalloc(sizeof(*queue), GFP_KERNEL);
	if (!queue)
		goto out;
	memset(queue, 0, sizeof(*queue));

	queue->bcm = bcm;
	queue->mmio_base = pio_mmio_base;

	INIT_LIST_HEAD(&queue->txfree);
	INIT_LIST_HEAD(&queue->txqueue);
	INIT_LIST_HEAD(&queue->txfired);
	spin_lock_init(&queue->txlock);
	INIT_WORK(&queue->txwork, txwork_handler, queue);

	value = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	if (value & BCM430x_SBF_XFER_REG_BYTESWAP)
		queue->hwswap = 1;
printk(KERN_INFO PFX "PIO hwswap %u\n", queue->hwswap);

	qsize = bcm430x_read16(bcm, queue->mmio_base + BCM430x_PIO_TXQBUFSIZE);
	if (qsize <= BCM430x_PIO_TXQADJUST) {
		printk(KERN_ERR PFX "PIO tx queue too small (%u)\n", qsize);
		goto err_freequeue;
	}
	qsize -= BCM430x_PIO_TXQADJUST;
	queue->tx_devq_size = qsize;

	setup_txqueues(queue);

out:
	return queue;

err_freequeue:
	kfree(queue);
	queue = 0;
	goto out;
}

static void cancel_transfers(struct bcm430x_pioqueue *queue)
{
	struct bcm430x_pio_txpacket *packet, *tmp_packet;

	netif_stop_queue(queue->bcm->net_dev);
	flush_workqueue(queue->bcm->workqueue);

	list_for_each_entry_safe(packet, tmp_packet, &queue->txfired, list)
		del_timer_sync(&packet->timeout);
	list_for_each_entry_safe(packet, tmp_packet, &queue->txfired, list)
		cancel_txpacket(packet);
	list_for_each_entry_safe(packet, tmp_packet, &queue->txqueue, list)
		cancel_txpacket(packet);
}

void bcm430x_destroy_pioqueue(struct bcm430x_pioqueue *queue)
{
	if (!queue)
		return;

	cancel_transfers(queue);
	kfree(queue);
}

static inline
int pio_transfer_txb(struct bcm430x_pioqueue *queue,
		     struct ieee80211_txb *txb)
{
	struct bcm430x_pio_txpacket *packet;
	unsigned long flags;

	spin_lock_irqsave(&queue->txlock, flags);
	packet = queue->__tx_packets_cache + queue->nr_txqueued;

	packet->queue = queue;
	packet->txb = txb;
	list_del(&packet->list);
	INIT_LIST_HEAD(&packet->list);

	list_add(&packet->list, &queue->txqueue);
	queue->nr_txqueued++;
	try_to_suspend_txqueue(queue);
	spin_unlock_irqrestore(&queue->txlock, flags);

	queue_work(queue->bcm->workqueue, &queue->txwork);
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
