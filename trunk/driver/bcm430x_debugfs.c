/*

  Broadcom BCM430x wireless driver

  debugfs driver debugging code

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



#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <asm/io.h>

#include "bcm430x.h"
#include "bcm430x_main.h"
#include "bcm430x_debugfs.h"
#include "bcm430x_dma.h"
#include "bcm430x_pio.h"

#define REALLY_BIG_BUFFER_SIZE	(1024*256)

static struct bcm430x_debugfs fs;
static char really_big_buffer[REALLY_BIG_BUFFER_SIZE];
static DECLARE_MUTEX(big_buffer_sem);


static ssize_t write_file_dummy(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	return count;
}

static int open_file_generic(struct inode *inode, struct file *file)
{
	file->private_data = inode->u.generic_ip;
	return 0;
}

#define fappend(fmt, x...)	pos += snprintf(buf + pos, len - pos, fmt , ##x)

static ssize_t devinfo_read_file(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	struct net_device *net_dev;
	struct pci_dev *pci_dev;
	unsigned long flags;

	down(&big_buffer_sem);

	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		fappend("Board not initialized.\n");
		goto out;
	}
	net_dev = bcm->net_dev;
	pci_dev = bcm->pci_dev;

	/* This is where the information is written to the "devinfo" file */
	fappend("*** %s devinfo ***\n", net_dev->name);
	fappend("vendor:           0x%04x   device:           0x%04x\n",
		pci_dev->vendor, pci_dev->device);
	fappend("subsystem_vendor: 0x%04x   subsystem_device: 0x%04x\n",
		pci_dev->subsystem_vendor, pci_dev->subsystem_device);
	fappend("IRQ: %d\n", pci_dev->irq);
	fappend("mmio_addr: 0x%p   mmio_len: %u\n", bcm->mmio_addr, bcm->mmio_len);
	fappend("chip_id: 0x%04x   chip_rev: 0x%02x\n", bcm->chip_id, bcm->chip_rev);
	if ((bcm->core_80211[0].rev >= 3) && (bcm430x_read32(bcm, 0x0158) & (1 << 16)))
		fappend("Radio disabled by hardware!\n");
	if ((bcm->core_80211[0].rev < 3) && !(bcm430x_read16(bcm, 0x049A) & (1 << 4)))
		fappend("Radio disabled by hardware!\n");
	fappend("board_vendor: 0x%04x   board_type: 0x%04x\n", bcm->board_vendor,
	        bcm->board_type);

	fappend("\nCores:\n");
#define fappend_core(name, info) fappend("core \"" name "\" %s, %s, id: 0x%04x, "	\
					 "rev: 0x%02x, index: 0x%02x\n",		\
					 (info).flags & BCM430x_COREFLAG_AVAILABLE	\
						? "available" : "nonavailable",		\
					 (info).flags & BCM430x_COREFLAG_ENABLED	\
						? "enabled" : "disabled",		\
					 (info).id, (info).rev, (info).index)
	fappend_core("CHIPCOMMON", bcm->core_chipcommon);
	fappend_core("PCI", bcm->core_pci);
	fappend_core("V90", bcm->core_v90);
	fappend_core("PCMCIA", bcm->core_pcmcia);
	fappend_core("ETHERNET", bcm->core_ethernet);
	fappend_core("first 80211", bcm->core_80211[0]);
	fappend_core("second 80211", bcm->core_80211[1]);
#undef fappend_core

out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t drvinfo_read_file(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;

	down(&big_buffer_sem);

	/* This is where the information is written to the "driver" file */
	fappend(BCM430x_DRIVER_NAME "\n");
	fappend("Compiled at: %s %s\n", __DATE__, __TIME__);

	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t spromdump_read_file(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	unsigned long flags;

	down(&big_buffer_sem);
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		fappend("Board not initialized.\n");
		goto out;
	}

	/* This is where the information is written to the "sprom_dump" file */
	fappend("boardflags: 0x%04x\n", bcm->sprom.boardflags);

out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t tsf_read_file(struct file *file, char __user *userbuf,
			     size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	unsigned long flags;

	down(&big_buffer_sem);
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		fappend("Board not initialized.\n");
		goto out;
	}
	fappend("0x%08x%08x\n",
		ioread32(bcm->mmio_addr + BCM430x_MMIO_REV3PLUS_TSF_HIGH),
		ioread32(bcm->mmio_addr + BCM430x_MMIO_REV3PLUS_TSF_LOW));
	
out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t tsf_write_file(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	ssize_t buf_size;
	ssize_t res;
	unsigned long flags;
	u64 tsf;

	buf_size = min(count, sizeof (really_big_buffer) - 1);
	down(&big_buffer_sem);
	if (copy_from_user(buf, user_buf, buf_size)) {
	        res = -EFAULT;
		goto out_up;
	}
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		printk(KERN_INFO PFX "debugfs: Board not initialized.\n");
		res = -EFAULT;
		goto out_unlock;
	}
	if (sscanf(buf, "%lli", &tsf) == 1) {
		iowrite32(tsf & 0xFFFFFFFF, bcm->mmio_addr + BCM430x_MMIO_REV3PLUS_TSF_LOW);
		iowrite32((tsf >> 32), bcm->mmio_addr + BCM430x_MMIO_REV3PLUS_TSF_HIGH);
	} else {
		printk(KERN_INFO PFX "debugfs: invalid values for \"tsf\"\n");
		res = -EINVAL;
		goto out_unlock;
	}
	res = buf_size;
	
out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);
out_up:
	up(&big_buffer_sem);
	return res;
}

static ssize_t send_read_file(struct file *file, char __user *userbuf,
			      size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	unsigned long flags;

	down(&big_buffer_sem);
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		fappend("Board not initialized.\n");
		goto out;
	}
	fappend("TX:\n"
		"Data written to this file will be transmitted.\n"
		"A txhdr and PLCP will be prepended to the data.\n");

out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t send_write_file(struct file *file, const char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	ssize_t buf_size;
	ssize_t res;
	unsigned long flags;

	if (count <= 24) {
		printk(KERN_ERR PFX "Packet too small (no 80211 header?)\n");
		res = -EINVAL;
		goto out;
	}
	buf_size = min(count, sizeof (really_big_buffer) - 1);
	down(&big_buffer_sem);
	if (copy_from_user(buf, user_buf, buf_size)) {
	        res = -EFAULT;
		goto out_up;
	}
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		printk(KERN_INFO PFX "debugfs: Board not initialized.\n");
		res = -EFAULT;
		goto out_unlock;
	}

	bcm430x_printk_dump(buf, buf_size, "DebugFS: TX");

	if (bcm->pio_mode) {
		bcm430x_pio_tx_frame(bcm->current_core->pio->queue1,
				     buf, buf_size);
	} else {
		bcm430x_dma_tx_frame(bcm->current_core->dma->tx_ring1,
				     buf, buf_size);
	}

	res = buf_size;
out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);
out_up:
	up(&big_buffer_sem);
out:
	return res;
}

static ssize_t sendraw_read_file(struct file *file, char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	unsigned long flags;

	down(&big_buffer_sem);
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		fappend("Board not initialized.\n");
		goto out;
	}
	fappend("RAW TX:\n"
		"Data written to this file will be transmitted.\n"
		"The data must contain all headers (txhdr, PLCP)\n");

out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

static ssize_t sendraw_write_file(struct file *file, const char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	ssize_t buf_size;
	ssize_t res;
	unsigned long flags;

	if (count <= 24 + sizeof(struct bcm430x_txhdr)) {
		printk(KERN_ERR PFX "Packet too small (No 80211 header, "
				    "TX header or PLCP header?)\n");
		res = -EINVAL;
		goto out;
	}
	buf_size = min(count, sizeof (really_big_buffer) - 1);
	down(&big_buffer_sem);
	if (copy_from_user(buf, user_buf, buf_size)) {
	        res = -EFAULT;
		goto out_up;
	}
	spin_lock_irqsave(&bcm->lock, flags);
	if (!bcm->initialized) {
		printk(KERN_INFO PFX "debugfs: Board not initialized.\n");
		res = -EFAULT;
		goto out_unlock;
	}

	bcm430x_printk_dump(buf, buf_size, "DebugFS: RAW TX");

	/* Tempoarly disable txheader generation. */
	bcm->no_txhdr = 1;
	if (bcm->pio_mode) {
		bcm430x_pio_tx_frame(bcm->current_core->pio->queue1,
				     buf, buf_size);
	} else {
		bcm430x_dma_tx_frame(bcm->current_core->dma->tx_ring1,
				     buf, buf_size);
	}
	bcm->no_txhdr = 0;

	res = buf_size;
out_unlock:
	spin_unlock_irqrestore(&bcm->lock, flags);
out_up:
	up(&big_buffer_sem);
out:
	return res;
}

static ssize_t txstat_read_file(struct file *file, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	const size_t len = REALLY_BIG_BUFFER_SIZE;

	struct bcm430x_private *bcm = file->private_data;
	char *buf = really_big_buffer;
	size_t pos = 0;
	ssize_t res;
	unsigned long flags;
	struct bcm430x_dfsentry *e;
	struct bcm430x_xmitstatus *status;
	int i, cnt, j = 0;

	down(&big_buffer_sem);
	spin_lock_irqsave(&bcm->lock, flags);

	fappend("Last %d logged xmitstatus blobs (Latest first):\n\n",
		BCM430x_NR_LOGGED_XMITSTATUS);
	e = bcm->dfsentry;
	if (e->xmitstatus_printing == 0) {
		/* At the beginning, make a copy of all data to avoid
		 * concurrency, as this function is called multiple
		 * times for big logs. Without copying, the data might
		 * change between reads. This would result in total trash.
		 */
		e->xmitstatus_printing = 1;
		e->saved_xmitstatus_ptr = e->xmitstatus_ptr;
		e->saved_xmitstatus_cnt = e->xmitstatus_cnt;
		memcpy(e->xmitstatus_print_buffer, e->xmitstatus_buffer,
		       BCM430x_NR_LOGGED_XMITSTATUS * sizeof(*(e->xmitstatus_buffer)));
	}
	i = e->saved_xmitstatus_ptr - 1;
	if (i < 0)
		i = BCM430x_NR_LOGGED_XMITSTATUS - 1;
	cnt = e->saved_xmitstatus_cnt;
	while (cnt) {
		status = e->xmitstatus_print_buffer + i;
		fappend("0x%02x:   cookie: 0x%04x,  flags: 0x%02x,  "
			"cnt1: 0x%02x,  cnt2: 0x%02x,  seq: 0x%04x,  "
			"unk: 0x%04x\n", j,
			status->cookie, status->flags,
			status->cnt1, status->cnt2, status->seq,
			status->unknown);
		j++;
		cnt--;
		i--;
		if (i < 0)
			i = BCM430x_NR_LOGGED_XMITSTATUS - 1;
	}

	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	spin_lock_irqsave(&bcm->lock, flags);
	if (*ppos == pos) {
		/* Done. Drop the copied data. */
		e->xmitstatus_printing = 0;
	}
	spin_unlock_irqrestore(&bcm->lock, flags);
	up(&big_buffer_sem);
	return res;
}

#undef fappend


static struct file_operations devinfo_fops = {
	.read = devinfo_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};

static struct file_operations spromdump_fops = {
	.read = spromdump_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};

static struct file_operations drvinfo_fops = {
	.read = drvinfo_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};

static struct file_operations tsf_fops = {
	.read = tsf_read_file,
	.write = tsf_write_file,
	.open = open_file_generic,
};

static struct file_operations send_fops = {
	.read = send_read_file,
	.write = send_write_file,
	.open = open_file_generic,
};

static struct file_operations sendraw_fops = {
	.read = sendraw_read_file,
	.write = sendraw_write_file,
	.open = open_file_generic,
};

static struct file_operations txstat_fops = {
	.read = txstat_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};


void bcm430x_debugfs_add_device(struct bcm430x_private *bcm)
{
	struct bcm430x_dfsentry *e;
	char devdir[IFNAMSIZ];

	assert(bcm);
	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e) {
		printk(KERN_ERR PFX "out of memory\n");
		return;
	}
	e->bcm = bcm;
	e->xmitstatus_buffer = kzalloc(BCM430x_NR_LOGGED_XMITSTATUS
				       * sizeof(*(e->xmitstatus_buffer)),
				       GFP_KERNEL);
	if (!e->xmitstatus_buffer) {
		printk(KERN_ERR PFX "out of memory\n");
		kfree(e);
		return;
	}
	e->xmitstatus_print_buffer = kzalloc(BCM430x_NR_LOGGED_XMITSTATUS
					     * sizeof(*(e->xmitstatus_buffer)),
					     GFP_KERNEL);
	if (!e->xmitstatus_print_buffer) {
		printk(KERN_ERR PFX "out of memory\n");
		kfree(e);
		return;
	}


	bcm->dfsentry = e;

	strncpy(devdir, bcm->net_dev->name, ARRAY_SIZE(devdir));
	e->subdir = debugfs_create_dir(devdir, fs.root);
	e->dentry_devinfo = debugfs_create_file("devinfo", 0444, e->subdir,
						bcm, &devinfo_fops);
	if (!e->dentry_devinfo)
		printk(KERN_ERR PFX "debugfs: creating \"devinfo\" for \"%s\" failed!\n", devdir);
	e->dentry_spromdump = debugfs_create_file("sprom_dump", 0444, e->subdir,
						  bcm, &spromdump_fops);
	if (!e->dentry_spromdump)
		printk(KERN_ERR PFX "debugfs: creating \"sprom_dump\" for \"%s\" failed!\n", devdir);
	e->dentry_tsf = debugfs_create_file("tsf", 0666, e->subdir,
	                                    bcm, &tsf_fops);
	if (!e->dentry_tsf)
		printk(KERN_ERR PFX "debugfs: creating \"tsf\" for \"%s\" failed!\n", devdir);
	e->dentry_send = debugfs_create_file("send", 0666, e->subdir,
					     bcm, &send_fops);
	if (!e->dentry_send)
		printk(KERN_ERR PFX "debugfs: creating \"send\" for \"%s\" failed!\n", devdir);
	e->dentry_sendraw = debugfs_create_file("sendraw", 0666, e->subdir,
						bcm, &sendraw_fops);
	if (!e->dentry_sendraw)
		printk(KERN_ERR PFX "debugfs: creating \"sendraw\" for \"%s\" failed!\n", devdir);
	e->dentry_txstat = debugfs_create_file("tx_status", 0444, e->subdir,
						bcm, &txstat_fops);
	if (!e->dentry_txstat)
		printk(KERN_ERR PFX "debugfs: creating \"tx_status\" for \"%s\" failed!\n", devdir);
}

void bcm430x_debugfs_remove_device(struct bcm430x_private *bcm)
{
	struct bcm430x_dfsentry *e;

	if (!bcm)
		return;

	e = bcm->dfsentry;
	assert(e);
	debugfs_remove(e->dentry_spromdump);
	debugfs_remove(e->dentry_devinfo);
	debugfs_remove(e->dentry_tsf);
	debugfs_remove(e->dentry_send);
	debugfs_remove(e->dentry_sendraw);
	debugfs_remove(e->dentry_txstat);
	debugfs_remove(e->subdir);
	kfree(e->xmitstatus_buffer);
	kfree(e->xmitstatus_print_buffer);
	kfree(e);
}

void bcm430x_debugfs_log_txstat(struct bcm430x_private *bcm,
				struct bcm430x_xmitstatus *status)
{
	struct bcm430x_dfsentry *e;
	struct bcm430x_xmitstatus *savedstatus;

	/* This is protected by bcm->lock */
	e = bcm->dfsentry;
	assert(e);
	savedstatus = e->xmitstatus_buffer + e->xmitstatus_ptr;
	memcpy(savedstatus, status, sizeof(*status));
	e->xmitstatus_ptr++;
	if (e->xmitstatus_ptr >= BCM430x_NR_LOGGED_XMITSTATUS)
		e->xmitstatus_ptr = 0;
	if (e->xmitstatus_cnt < BCM430x_NR_LOGGED_XMITSTATUS)
		e->xmitstatus_cnt++;
}

void bcm430x_debugfs_init(void)
{
	memset(&fs, 0, sizeof(fs));
	fs.root = debugfs_create_dir(DRV_NAME, NULL);
	if (!fs.root)
		printk(KERN_ERR PFX "debugfs: creating \"" DRV_NAME "\" subdir failed!\n");
	fs.dentry_driverinfo = debugfs_create_file("driver", 0444, fs.root, NULL, &drvinfo_fops);
	if (!fs.dentry_driverinfo)
		printk(KERN_ERR PFX "debugfs: creating \"" DRV_NAME "/driver\" failed!\n");
}

void bcm430x_debugfs_exit(void)
{
	debugfs_remove(fs.dentry_driverinfo);
	debugfs_remove(fs.root);
}

void bcm430x_printk_dump(const char *data,
			 size_t size,
			 const char *description)
{
	size_t i;
	char c;

	printk(KERN_INFO PFX "Data dump (%s, %u bytes):",
	       description, size);
	for (i = 0; i < size; i++) {
		c = data[i];
		if (i % 8 == 0)
			printk("\n" KERN_INFO PFX "0x%08x:  0x%02x, ", i, c & 0xff);
		else
			printk("0x%02x, ", c & 0xff);
	}
	printk("\n");
}

void bcm430x_printk_bitdump(const unsigned char *data,
			    size_t bytes, int msb_to_lsb,
			    const char *description)
{
	size_t i;
	int j;
	const unsigned char *d;

	printk(KERN_INFO PFX "*** Bitdump (%s, %u bytes, %s) ***",
	       description, bytes, msb_to_lsb ? "MSB to LSB" : "LSB to MSB");
	for (i = 0; i < bytes; i++) {
		d = data + i;
		if (i % 8 == 0)
			printk("\n" KERN_INFO PFX "0x%08x:  ", i);
		if (msb_to_lsb) {
			for (j = 7; j >= 0; j--) {
				if (*d & (1 << j))
					printk("1");
				else
					printk("0");
			}
		} else {
			for (j = 0; j < 8; j++) {
				if (*d & (1 << j))
					printk("1");
				else
					printk("0");
			}
		}
		printk(" ");
	}
	printk("\n");
}

/* vim: set ts=8 sw=8 sts=8: */
