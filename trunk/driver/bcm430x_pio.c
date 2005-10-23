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
void assert_hwswap(struct bcm430x_pioqueue *queue)
{
	assert(bcm430x_read32(queue->bcm,
			      BCM430x_MMIO_STATUS_BITFIELD)
	       & BCM430x_SBF_XFER_REG_BYTESWAP);
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
	if (list_empty(&queue->txfree))
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
int tx_devq_is_full(struct bcm430x_pioqueue *queue,
		    u16 tx_octets)
{
	assert(queue->tx_devq_packets <= BCM430x_PIO_MAXTXDEVQPACKETS);
	assert(queue->tx_devq_used <= queue->tx_devq_size);
	if (queue->tx_devq_packets == BCM430x_PIO_MAXTXDEVQPACKETS)
		return 1;
	if (queue->tx_devq_used + tx_octets > queue->tx_devq_size)
		return 1;
	return 0;
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
		assert_hwswap(queue);
		bcm430x_pio_write(queue, BCM430x_PIO_TXDATA, data);
		i += 2;
	}
	bcm430x_pio_write(queue, BCM430x_PIO_TXCTL,
			  BCM430x_PIO_TXCTL_WRITELO | BCM430x_PIO_TXCTL_WRITEHI);
	for ( ; i < octets; i += 2) {
		data = be16_to_cpu( *((u16 *)(packet + i)) );
		assert_hwswap(queue);
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
		assert_hwswap(queue);
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
			   struct bcm430x_pio_txpacket *packet)
{
	struct bcm430x_pio_txcontext *ctx = &packet->ctx;
	unsigned int octets;

	/*TODO: fragmented skb */

	if (likely(packet->no_txhdr == 0)) {
		if (unlikely(skb_headroom(skb) < sizeof(struct bcm430x_txhdr))) {
			printk(KERN_ERR PFX "PIO: Not enough skb headroom!\n");
			return;
		}
		__skb_push(skb, sizeof(struct bcm430x_txhdr));
		bcm430x_generate_txhdr(queue->bcm,
				       (struct bcm430x_txhdr *)skb->data,
				       skb->data + sizeof(struct bcm430x_txhdr),
				       skb->len - sizeof(struct bcm430x_txhdr),
				       (ctx->xmitted_frags == 0),
				       /*ctx->cookie*/0xCAFE);//FIXME
	}

	tx_start(queue);
	octets = skb->len;
	if (queue->bcm->current_core->rev < 3)
		octets -= 2;
	tx_data(queue, (u8 *)skb->data, octets);
	tx_complete(queue, skb);
}

static inline
int pio_tx_packet(struct bcm430x_pio_txpacket *packet)
{
	struct bcm430x_pioqueue *queue = packet->queue;
	struct ieee80211_txb *txb = packet->txb;
	struct sk_buff *skb;
	int i;

	for (i = packet->ctx.xmitted_frags; i < txb->nr_frags; i++) {
		skb = txb->fragments[i];
		if (tx_devq_is_full(queue, (u16)(skb->len + sizeof(struct bcm430x_txhdr)))) {
printk(KERN_INFO PFX "txQ full\n");
			return 1;
		}
		pio_tx_write_fragment(queue, skb, packet);

		packet->ctx.xmitted_frags++;
		queue->tx_devq_packets++;
		queue->tx_devq_used += skb->len;
	}

	return 0;
}

static void free_txpacket(struct bcm430x_pio_txpacket *packet)
{
	if (unlikely(packet->txb_is_dummy)) {
		u8 i;

		/* This is only a hack for the debugging function
		 * bcm430x_pio_tx_frame()
		 */
		for (i = 0; i < packet->txb->nr_frags; i++)
			dev_kfree_skb_any(packet->txb->fragments[i]);
		kfree(packet->txb);
		packet->txb_is_dummy = 0;
		packet->no_txhdr = 0;
		return;
	}
	ieee80211_txb_free(packet->txb);
}

static void cancel_txpacket(struct bcm430x_pio_txpacket *packet)
{
	list_del(&packet->list);
	free_txpacket(packet);
	INIT_LIST_HEAD(&packet->list);
	list_add(&packet->list, &packet->queue->txfree);

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

		if (packet->ctx.xmitted_frags == 0) {
			packet->timeout.function = tx_timeout;
			packet->timeout.data = (unsigned long)packet;
			packet->timeout.expires = jiffies + BCM430x_PIO_TXTIMEOUT;
			add_timer(&packet->timeout);
		}
		assert(packet->ctx.xmitted_frags <= packet->txb->nr_frags);
		if (packet->ctx.xmitted_frags == packet->txb->nr_frags)
			continue;

		/* Now try to transmit the packet.
		 * This may not completely succeed.
		 */
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
	spin_lock_init(&queue->txlock);
	INIT_WORK(&queue->txwork, txwork_handler, queue);

	value = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	value |= BCM430x_SBF_XFER_REG_BYTESWAP;
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, value);

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
	queue = NULL;
	goto out;
}

static void cancel_transfers(struct bcm430x_pioqueue *queue)
{
	struct bcm430x_pio_txpacket *packet, *tmp_packet;

	netif_stop_queue(queue->bcm->net_dev);
	flush_workqueue(queue->bcm->workqueue);

	list_for_each_entry_safe(packet, tmp_packet, &queue->txqueue, list)
		del_timer_sync(&packet->timeout);
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
		     struct ieee80211_txb *txb,
		     const int txb_is_dummy)
{
	struct bcm430x_pio_txpacket *packet;
	unsigned long flags;

	spin_lock_irqsave(&queue->txlock, flags);
	assert(!list_empty(&queue->txfree));
	packet = list_entry(queue->txfree.next, struct bcm430x_pio_txpacket, list);

	packet->queue = queue;
	packet->txb = txb;
	list_del(&packet->list);
	INIT_LIST_HEAD(&packet->list);
	if (unlikely(txb_is_dummy))
		packet->txb_is_dummy = 1;
	if (unlikely(queue->bcm->no_txhdr))
		packet->no_txhdr = 1;

	memset(&packet->ctx, 0, sizeof(packet->ctx));

	list_add(&packet->list, &queue->txqueue);
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
			       txb, 0);

	return err;
}

void bcm430x_pio_tx_frame(struct bcm430x_pioqueue *queue,
			  const char *buf, size_t size)
{
	struct sk_buff *skb;
	size_t skb_size;
	struct ieee80211_txb *dummy_txb;

	skb_size = size;
	if (!queue->bcm->no_txhdr)
		skb_size += sizeof(struct bcm430x_txhdr);
	skb = dev_alloc_skb(skb_size);
	if (!skb) {
		printk(KERN_ERR PFX "Out of memory!\n");
		return;
	}
	if (!queue->bcm->no_txhdr)
		skb_reserve(skb, sizeof(struct bcm430x_txhdr));
	memcpy(skb->data, buf, size);

	/* Setup a dummy txb. Be careful to not free this
	 * with ieee80211_txb_free()
	 */
	dummy_txb = kzalloc(sizeof(*dummy_txb) + sizeof(u8 *),
			    GFP_ATOMIC);
	if (!dummy_txb) {
		dev_kfree_skb_any(skb);
		printk(KERN_ERR PFX "Out of memory!\n");
		return;
	}
	dummy_txb->nr_frags = 1;
	dummy_txb->fragments[0] = skb;

	pio_transfer_txb(queue, dummy_txb, 1);
}

void fastcall
bcm430x_pio_handle_xmitstatus(struct bcm430x_private *bcm,
			      struct bcm430x_xmitstatus *status)
{
	/*TODO*/
}

void fastcall
bcm430x_pio_rx(struct bcm430x_pioqueue *queue)
{
	/*TODO*/
}

/* vim: set ts=8 sw=8 sts=8: */
