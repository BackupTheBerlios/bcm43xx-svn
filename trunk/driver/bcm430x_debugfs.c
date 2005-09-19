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

#include "bcm430x_debugfs.h"
#include "bcm430x.h"

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <asm/io.h>

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
#define fappend_ioblock32(length, mmio_offset) \
	do {									\
		u32 __d;							\
		unsigned int __i;						\
		for (__i = 0; __i < (length); __i++) {				\
			__d = ioread32(bcm->mmio_addr + (mmio_offset));		\
			if (__i % 6 == 0)					\
				fappend("\n0x%04x:  0x%08x, ", __i, __d);	\
			else							\
				fappend("0x%08x, ", __d);			\
		}								\
	} while (0)


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
	if (!(bcm->status & BCM430x_STAT_BOARDINITDONE)) {
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
	fappend("sbimstate: 0x%08x   sbtmstatelow: 0x%08x   sbtmstatehigh: 0x%08x\n",
		bcm->sbimstate, bcm->sbtmstatelow, bcm->sbtmstatehigh);
	fappend("phy_version: 0x%02x   phy_type: 0x%02x   phy_rev: 0x%02x\n",
		bcm->phy_version, bcm->phy_type, bcm->phy_rev);
	fappend("radio_id: 0x%08x\n", bcm->radio_id);
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
	if (!(bcm->status & BCM430x_STAT_BOARDINITDONE)) {
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

static ssize_t shmdump_read_file(struct file *file, char __user *userbuf,
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
	if (!(bcm->status & BCM430x_STAT_BOARDINITDONE)) {
		fappend("Board not initialized.\n");
		goto out;
	}

	/* This is where the information is written to the "shm_dump" file */
	/*TODO: dump shared_memory, hw_mac, init_ucode, 0x0002 and maybe others */
	fappend("PCM data (size: %u bytes):", bcm->pcm_size);
	fappend("\nFIXME: This is all 0x00000000 on my machine. Maybe we cannot read "
		"PCM data back... (Michael)");
	iowrite32(BCM430x_SHM_PCM + 0x01eb, bcm->mmio_addr + BCM430x_MMIO_SHM_CONTROL);
	fappend_ioblock32(bcm->pcm_size / sizeof(u32), BCM430x_MMIO_SHM_DATA);
	fappend("\nPCM data end\n\n");

	fappend("Microcode (size: %u bytes):", bcm->ucode_size);
	iowrite32(BCM430x_SHM_UCODE + 0x0000, bcm->mmio_addr + BCM430x_MMIO_SHM_CONTROL);
	fappend_ioblock32(bcm->ucode_size / sizeof(u32), BCM430x_MMIO_SHM_DATA);
	fappend("\nMicrocode end\n\n");

out:
	spin_unlock_irqrestore(&bcm->lock, flags);
	res = simple_read_from_buffer(userbuf, count, ppos, buf, pos);
	up(&big_buffer_sem);
	return res;
}

#undef fappend
#undef fappend_ioblock32


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

static struct file_operations shmdump_fops = {
	.read = shmdump_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};

static struct file_operations drvinfo_fops = {
	.read = drvinfo_read_file,
	.write = write_file_dummy,
	.open = open_file_generic,
};


static struct bcm430x_dfsentry * find_dfsentry(struct bcm430x_private *bcm)
{
	struct bcm430x_dfsentry *e;

	list_for_each_entry(e, &fs.entries, list) {
		if (e->bcm == bcm)
			return e;
	}

	return 0;
}

void bcm430x_debugfs_add_device(struct bcm430x_private *bcm)
{
	struct bcm430x_dfsentry *e;
	char devdir[10];

	if (!bcm)
		return;

	e = kmalloc(sizeof(*e), GFP_KERNEL);
	if (!e) {
		printk(KERN_ERR PFX "out of memory\n");
		return;
	}
	memset(e, 0, sizeof(*e));
	INIT_LIST_HEAD(&e->list);
	e->bcm = bcm;

	down(&fs.sem);
	snprintf(devdir, ARRAY_SIZE(devdir), "dev%d", fs.nr_entries);
	e->subdir = debugfs_create_dir(devdir, fs.root);
	e->dentry_devinfo = debugfs_create_file("devinfo", 0444, e->subdir,
						bcm, &devinfo_fops);
	if (!e->dentry_devinfo)
		printk(KERN_ERR PFX "debugfs: creating \"devinfo\" for \"%s\" failed!\n", devdir);
	e->dentry_spromdump = debugfs_create_file("sprom_dump", 0444, e->subdir,
						  bcm, &spromdump_fops);
	if (!e->dentry_spromdump)
		printk(KERN_ERR PFX "debugfs: creating \"sprom_dump\" for \"%s\" failed!\n", devdir);
	e->dentry_shmdump = debugfs_create_file("shm_dump", 0444, e->subdir,
						bcm, &shmdump_fops);
	if (!e->dentry_shmdump)
		printk(KERN_ERR PFX "debugfs: creating \"shm_dump\" for \"%s\" failed!\n", devdir);
	/* Add new files, here. */
	list_add(&e->list, &fs.entries);
	fs.nr_entries++;
	up(&fs.sem);
}

void bcm430x_debugfs_remove_device(struct bcm430x_private *bcm)
{
	struct bcm430x_dfsentry *e;

	if (!bcm)
		return;

	down(&fs.sem);
	e = find_dfsentry(bcm);
	if (!e)
		goto out_up;
	/* Don't forget to remove any file, here. */
	debugfs_remove(e->dentry_shmdump);
	debugfs_remove(e->dentry_spromdump);
	debugfs_remove(e->dentry_devinfo);
	debugfs_remove(e->subdir);
	list_del(&e->list);
	fs.nr_entries--;
	if (fs.nr_entries < 0)
		printk(KERN_ERR PFX "debugfs: fs.nr_entries went negative!\n");
	kfree(e);
out_up:
	up(&fs.sem);
}

void bcm430x_debugfs_init(void)
{
	memset(&fs, 0, sizeof(fs));
	init_MUTEX(&fs.sem);
	INIT_LIST_HEAD(&fs.entries);
	fs.nr_entries = 0;
	fs.root = debugfs_create_dir(DRV_NAME, 0);
	if (!fs.root)
		printk(KERN_ERR PFX "debugfs: creating \"" DRV_NAME "\" subdir failed!\n");
	fs.dentry_driverinfo = debugfs_create_file("driver", 0444, fs.root, 0, &drvinfo_fops);
	if (!fs.dentry_driverinfo)
		printk(KERN_ERR PFX "debugfs: creating \"" DRV_NAME "/driver\" failed!\n");
}

void bcm430x_debugfs_exit(void)
{
	down(&fs.sem);
	if (!list_empty(&fs.entries)) {
		printk(KERN_ERR PFX "Woops, still %d devices registered on module exit!\n",
		       fs.nr_entries);
	}
	debugfs_remove(fs.dentry_driverinfo);
	debugfs_remove(fs.root);
	up(&fs.sem);
}

/* vim: set ts=8 sw=8 sts=8: */
