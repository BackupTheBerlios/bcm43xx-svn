/*

  Broadcom BCM430x wireless driver

  Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
                     Stefano Brivio <st3@riseup.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
     
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
     
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/version.h>

MODULE_DESCRIPTION("Broadcom BCM430x wireless driver");
MODULE_AUTHOR("Martin Langer, Stefano Brivio");
MODULE_LICENSE("GPL");

#include "bcm430x.h"

static struct pci_device_id bcm430x_pci_tbl[] = {

	{ PCI_VENDOR_ID_BROADCOM, 0x4301, PCI_ANY_ID, PCI_ANY_ID, 0, 0, },
	/*      ID              Name
	 *      1028:0407               TrueMobile 1180 Onboard WLAN
	 *      1043:0120               WL-103b Wireless LAN PC Card
	 */

	{ PCI_VENDOR_ID_BROADCOM, 0x4318, PCI_ANY_ID, PCI_ANY_ID, 0, 0, },
	/*      ID              Name
	 *      1799:7000               Belkin F5D7000
	 */

	{ PCI_VENDOR_ID_BROADCOM, 0x4320, PCI_ANY_ID, PCI_ANY_ID, 0, 0, },
	/*      ID              Name
	 *      1028:0001               TrueMobile 1300 WLAN Mini-PCI Card
	 *      1028:0003               Wireless 1350 WLAN Mini-PCI Card
	 *      1043:100f               WL-100G
	 *      1057:7025               WN825G
	 *      106b:004e               AirPort Extreme
	 *      14e4:4320               Linksys WMP54G PCI
	 *      1737:4320               WPC54G
	 *      1799:7001               Belkin F5D7001 High-Speed Mode Wireless G Network Card
	 *      1799:7010               Belkin F5D7010 54g Wireless Network card
	 */

	{ 0, },
};

/*
static u16 bcm430x_read16(struct bcm430x *bcm, u16 offset);
static u32 bcm430x_read32(struct bcm430x *bcm, u16 offset);
static void bcm430x_write16(struct bcm430x *bcm, u16 offset, u16 val);
static void bcm430x_write32(struct bcm430x *bcm, u16 offset, u32 val);
*/

static u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset)
{
	u16 val;

	val = ioread16(bcm->mmio_addr + offset);
	printk(KERN_INFO PFX "read 16  0x%04x  0x%04x\n", offset, val);
	return val;
}

static void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	iowrite16(val, bcm->mmio_addr + offset);
	printk(KERN_INFO PFX "write 16  0x%04x  0x%04x\n", offset, val);
}

static u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset)
{
	u32 val;

	val = ioread32(bcm->mmio_addr + offset);
	printk(KERN_INFO PFX "read 32  0x%04x  0x%08x\n", offset, val);
	return val;
}

static void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	iowrite32(val, bcm->mmio_addr + offset);
	printk(KERN_INFO PFX "write 32  0x%04x  0x%08x\n", offset, val);
}

static u16 bcm430x_phy_read(struct bcm430x_private *bcm, u16 offset)
{
	bcm430x_write16(bcm, offset, BCM430x_PHY_CONTROL);
	return bcm430x_read16(bcm, BCM430x_PHY_DATA);
}

static void bcm430x_phy_write(struct bcm430x_private *bcm, int offset, u16 val)
{
	bcm430x_write16(bcm, offset, BCM430x_PHY_CONTROL);
	bcm430x_write16(bcm, val, BCM430x_PHY_DATA);
}

static u32 bcm430x_shm_read32(struct bcm430x_private *bcm, u32 control)
{
	bcm430x_write32(bcm, control, BCM430x_SHM_CONTROL);
	return bcm430x_read32(bcm, BCM430x_SHM_DATA);
}

static void bcm430x_shm_write32(struct bcm430x_private *bcm, u32 control,
				u32 val)
{
	bcm430x_write32(bcm, control, BCM430x_SHM_CONTROL);
	bcm430x_write32(bcm, val, BCM430x_SHM_DATA);
}

static int bcm430x_pci_read_config_8(struct pci_dev *pdev, u16 offset, u8 * val)
{
	int err;

	err = pci_read_config_byte(pdev, offset, val);
	printk(KERN_INFO PFX "pci read 8  0x%04x  0x%02x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset,
				      u16 * val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
	printk(KERN_INFO PFX "pci read 16  0x%04x  0x%04x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset,
				      u32 * val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
	printk(KERN_INFO PFX "pci read 32  0x%04x  0x%08x\n", offset, *val);
	return err;
}

static int bcm430x_pci_write_config_8(struct pci_dev *pdev, int offset, u8 val)
{
	printk(KERN_INFO PFX "pci write 8  0x%04x  0x%02x\n", offset, val);
	return pci_write_config_byte(pdev, offset, val);
}

static int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset,
				       u16 val)
{
	printk(KERN_INFO PFX "pci write 16  0x%04x  0x%04x\n", offset, val);
	return pci_write_config_word(pdev, offset, val);
}

static int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset,
				       u32 val)
{
	printk(KERN_INFO PFX "pci write 32  0x%04x  0x%08x\n", offset, val);
	return pci_write_config_dword(pdev, offset, val);
}

static int bcm430x_open(struct net_device *dev)
{
	return 0;
}

static int bcm430x_stop(struct net_device *dev)
{
	return 0;
}

static struct net_device_stats *bcm430x_get_stats(struct net_device *dev)
{
	struct bcm430x_private *bcm = netdev_priv(dev);

	return &bcm->stats; 
}

static void bcm430x_tx_timeout(struct net_device *dev)
{

}

/* Read SPROM and fill the useful values in the net_device struct */
static void bcm430x_read_sprom(struct net_device *dev)
{
	struct bcm430x_private *bcm = netdev_priv(dev);
	u16 mac[3];

	/* Deal with short offsets */
#define READ_SPROM(addr) bcm430x_read16(bcm, 0x1000 + 2*(addr))

	mac[0] = READ_SPROM(BCM430x_SPROM_IL0MACADDR + 0x00);
	mac[1] = READ_SPROM(BCM430x_SPROM_IL0MACADDR + 0x01);
	mac[2] = READ_SPROM(BCM430x_SPROM_IL0MACADDR + 0x02);

#undef READ_SPROM

	dev->dev_addr[0] = (mac[0] >> 8) & 0xFF;
	dev->dev_addr[1] = (mac[0] >> 0) & 0xFF;
	dev->dev_addr[2] = (mac[1] >> 8) & 0xFF;
	dev->dev_addr[3] = (mac[1] >> 0) & 0xFF;
	dev->dev_addr[4] = (mac[2] >> 8) & 0xFF;
	dev->dev_addr[5] = (mac[2] >> 0) & 0xFF;

}

/*
 * do we need this?
 * static u32 bcm430x_read_core(struct bcm430x *bcm)
 * {
 *	u32 val;
 *	val = (bcm430x_pci_read_config_32(bcm->pci, BCM430x_REG_ACTIVE_CORE) - 0x18000000) / 0x1000;
 *	return val;
 * }
 */

static void bcm430x_switch_core(struct bcm430x_private *bcm, u32 core)
{

	int i = 0;
	u32 real, wanted;

	wanted = 0x18000000 + core * 0x1000;
	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_REG_ACTIVE_CORE,
				   &real);

	/* Write the computed value to the register. This doesn't always
	   succeed so we retry BCM430x_SWITCH_CORE_MAX_RETRIES times */
	while (real != wanted && i < BCM430x_SWITCH_CORE_MAX_RETRIES) {
		bcm430x_pci_write_config_32(bcm->pci_dev,
					    BCM430x_REG_ACTIVE_CORE, wanted);
		bcm430x_pci_read_config_32(bcm->pci_dev,
					   BCM430x_REG_ACTIVE_CORE, &real);
		i++;
	}

	if (real != wanted) {
		printk(KERN_ERR PFX
		       "unable to switch to core %u, retried %i times",
		       core, i);
	}

	/* Set core_vendor, core_id and core_rev according to the new selected
	   core. This code is commented out because these values aren't currently
	   needed.
	   reg = bcm430x_readw(bcm->virt_mem + BCM430x_SB_ID_HI);
	   bcm->core_vendor = (reg & 0xffff0000) >> 16;
	   bcm->core_id = (reg & 0x0000fff0) >> 4;
	   bcm->core_rev = reg & 0x0000000f;
	 */

}

static void __bcm430x_cleanup_dev(struct net_device *dev)
{
	struct bcm430x_private *bcm = netdev_priv(dev);
	struct pci_dev *pdev;

	pdev = bcm->pci_dev;

	if (bcm->mmio_addr)
		iounmap(bcm->mmio_addr);

	/* it's ok to call this even if we have no regions to free */
	pci_release_regions(pdev);

	free_netdev(dev);
	pci_set_drvdata(pdev, NULL);
}

static int bcm430x_init_board(struct pci_dev *pdev, struct net_device **dev_out)
{
	void *ioaddr;
	struct net_device *dev;
	struct bcm430x_private *bcm;
	unsigned long mmio_start, mmio_end, mmio_flags, mmio_len;
	int err;

	*dev_out = NULL;

	/* dev and priv zeroed in alloc_etherdev */
	dev = alloc_etherdev(sizeof(*bcm));
	if (dev == NULL) {
		printk(KERN_ERR PFX
		       "could not allocate memory for new device %s\n",
		       pci_name(pdev));
		return -ENOMEM;
	}

	SET_MODULE_OWNER(dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	dev->open = bcm430x_open;
	dev->stop = bcm430x_stop;
	dev->get_stats = bcm430x_get_stats;
	dev->tx_timeout = bcm430x_tx_timeout;

	bcm = netdev_priv(dev);
	bcm->pci_dev = pdev;

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR PFX "unable to wake up pci device (%i)\n", err);
		goto err_out;
	}

	mmio_start = pci_resource_start(pdev, 0);
	mmio_end = pci_resource_end(pdev, 0);
	mmio_flags = pci_resource_flags(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);

	/* make sure PCI base addr is MMIO */
	if (!(mmio_flags & IORESOURCE_MEM)) {
		printk(KERN_ERR PFX
		       "%s, region #0 not an MMIO resource, aborting\n",
		       pci_name(pdev));
		err = -ENODEV;
		goto err_out;
	}
	if (mmio_len != BCM430x_IO_SIZE) {
		printk(KERN_ERR PFX
		       "%s: invalid PCI mem region size(s), aborting\n",
		       pci_name(pdev));
		err = -ENODEV;
		goto err_out;
	}

	err = pci_request_regions(pdev, "bcm430x");
	if (err) {
		printk(KERN_ERR PFX
		       "could not access PCI resources (%i)\n", err);
		goto err_out;
	}

	/* enable PCI bus-mastering */
	pci_set_master(pdev);

	/* ioremap MMIO region */
	ioaddr = ioremap(mmio_start, mmio_len);
	if (ioaddr == NULL) {
		printk(KERN_ERR PFX "%s: cannot remap MMIO, aborting\n",
		       pci_name(pdev));
		err = -EIO;
		goto err_out;
	}

	dev->base_addr = (long)ioaddr;
	bcm->mmio_addr = ioaddr;
	bcm->regs_len = mmio_len;

//      bcm430x_chip_init (ioaddr);

	*dev_out = dev;
	return 0;

err_out:
	__bcm430x_cleanup_dev(dev);
	pci_disable_device(pdev);
	return err;
}

static int __devinit bcm430x_init_one(struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct net_device *dev = NULL;
	struct bcm430x_private *bcm;
	int err = 0;
	void *ioaddr;
	u8 pci_rev;

	assert(pdev != NULL);
	assert(ent != NULL);

	/* when we're built into the kernel, the driver version message
	 * is only printed if at least one bcm430x board has been found
	 */
#ifndef MODULE
	{
		static int printed_version;
		if (!printed_version++)
			printk(KERN_INFO BCM430x_DRIVER_NAME "\n");
	}
#endif

	bcm430x_pci_read_config_8(pdev, PCI_REVISION_ID, &pci_rev);

	err = bcm430x_init_board(pdev, &dev);
	if (err) {
		printk(KERN_ERR PFX "bcm430x_init_board failed, "
			"aborting.\n");
		goto err_out;
	}

	bcm = netdev_priv(dev);
	ioaddr = bcm->mmio_addr;

	bcm430x_read_sprom(dev);
	err = register_netdev(dev);
	if (err) {
		printk(KERN_ERR PFX "Cannot register net device, "
			"aborting.\n");
		goto err_out;
	}

	pci_set_drvdata(pdev, dev);

	return 0;

err_out:
	__bcm430x_cleanup_dev(dev);
	return err;
}

static void __devexit bcm430x_remove_one(struct pci_dev *pdev)
{
	struct net_device *dev = pci_get_drvdata(pdev);

	if (dev) {
		unregister_netdev(dev);
		__bcm430x_cleanup_dev(dev);
	}
	pci_disable_device(pdev);
}

#ifdef CONFIG_PM

static int bcm430x_suspend(struct pci_dev *pdev, pm_message_t state)
{
	return 0;
}

static int bcm430x_resume(struct pci_dev *pdev)
{
	return 0;
}

#endif				/* CONFIG_PM */

static struct pci_driver bcm430x_pci_driver = {
	.name = BCM430x_DRIVER_NAME,
	.id_table = bcm430x_pci_tbl,
	.probe = bcm430x_init_one,
	.remove = __devexit_p(bcm430x_remove_one),
#ifdef CONFIG_PM
	.suspend = bcm430x_suspend,
	.resume = bcm430x_resume,
#endif				/* CONFIG_PM */
};

static int __init bcm430x_init(void)
{
#ifdef MODULE
	printk(KERN_INFO BCM430x_DRIVER_NAME "\n");
#endif

	return pci_module_init(&bcm430x_pci_driver);
}

static void __exit bcm430x_exit(void)
{
	pci_unregister_driver(&bcm430x_pci_driver);
}

module_init(bcm430x_init)
module_exit(bcm430x_exit)

/* vim: set ts=8 sw=8 sts=8: */
