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

/* change to 1 to enable more debug printouts */
#if 0
#define dprintk printk
#else
#define dprintk(x...) do{} while(0)
#endif

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

static u16 bcm430x_read16be(struct bcm430x_private *bcm, u16 offset)
{
	u16 val;

	val = ioread16be(bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "read 16be  0x%04x  0x%04x\n", offset, val);
	return val;
}

static u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset)
{
	u16 val;

	val = ioread16(bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "read 16  0x%04x  0x%04x\n", offset, val);
	return val;
}

static void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	iowrite16(val, bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "write 16  0x%04x  0x%04x\n", offset, val);
}

static u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset)
{
	u32 val;

	val = ioread32(bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "read 32  0x%04x  0x%08x\n", offset, val);
	return val;
}

static u32 bcm430x_read32be(struct bcm430x_private *bcm, u16 offset)
{
	u32 val;

	val = ioread32be(bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "read 32be  0x%04x  0x%08x\n", offset, val);
	return val;
}

static void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	iowrite32(val, bcm->mmio_addr + offset);
	dprintk(KERN_INFO PFX "write 32  0x%04x  0x%08x\n", offset, val);
}

static u16 bcm430x_phy_read(struct bcm430x_private *bcm, u16 offset)
{
	bcm430x_write16(bcm, BCM430x_PHY_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_PHY_DATA);
}

static void bcm430x_phy_write(struct bcm430x_private *bcm, int offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_PHY_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_PHY_DATA, val);
}

static u32 bcm430x_shm_read32(struct bcm430x_private *bcm, u32 control)
{
	bcm430x_write32(bcm, BCM430x_SHM_CONTROL, control);
	return bcm430x_read32(bcm, BCM430x_SHM_DATA);
}

static void bcm430x_shm_write32(struct bcm430x_private *bcm, u32 control,
				u32 val)
{
	bcm430x_write32(bcm, BCM430x_SHM_CONTROL, control);
	bcm430x_write32(bcm, BCM430x_SHM_DATA, val);
}

static int bcm430x_pci_read_config_8(struct pci_dev *pdev, u16 offset, u8 * val)
{
	int err;

	err = pci_read_config_byte(pdev, offset, val);
	dprintk(KERN_INFO PFX "pci read 8  0x%04x  0x%02x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset,
				      u16 * val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
	dprintk(KERN_INFO PFX "pci read 16  0x%04x  0x%04x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset,
				      u32 * val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
	dprintk(KERN_INFO PFX "pci read 32  0x%04x  0x%08x\n", offset, *val);
	return err;
}

static int bcm430x_pci_write_config_8(struct pci_dev *pdev, int offset, u8 val)
{
	dprintk(KERN_INFO PFX "pci write 8  0x%04x  0x%02x\n", offset, val);
	return pci_write_config_byte(pdev, offset, val);
}

static int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset,
				       u16 val)
{
	dprintk(KERN_INFO PFX "pci write 16  0x%04x  0x%04x\n", offset, val);
	return pci_write_config_word(pdev, offset, val);
}

static int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset,
				       u32 val)
{
	dprintk(KERN_INFO PFX "pci write 32  0x%04x  0x%08x\n", offset, val);
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

	/* read MAC address into dev->dev_addr */
	*((u16 *)dev->dev_addr + 0) = bcm430x_read16be(bcm, BCM430x_SPROM_IL0MACADDR + 0);
	*((u16 *)dev->dev_addr + 1) = bcm430x_read16be(bcm, BCM430x_SPROM_IL0MACADDR + 2);
	*((u16 *)dev->dev_addr + 2) = bcm430x_read16be(bcm, BCM430x_SPROM_IL0MACADDR + 4);

}

static int bcm430x_clr_target_abort(struct bcm430x_private *bcm)
{
	u16 pci_status;

	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_STATUS, &pci_status);

	pci_status &= ~ PCI_STATUS_SIG_TARGET_ABORT;

	bcm430x_pci_write_config_16(bcm->pci_dev, PCI_STATUS, pci_status);

	return 0;
}

static int bcm430x_pctl_set_crystal(struct bcm430x_private *bcm, int on)
{
	u32 in, out, outenable;

	/* All the code in this function is derived from
	 * http://bcm-specs.sipsolutions.net/PowerControl */

	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_IN, &in);
	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &outenable);

	outenable |= (BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN);

	if (on) {
		out |= (BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN);

		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);
		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, outenable);
		udelay(1000);

		out &= ~BCM430x_PCTL_PLL_POWERDOWN;
		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);

		udelay(5000);
	} else {
		out &= ~BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN;
		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);
	}

	return 0;
}

/* Puts the index of the current core into user supplied core variable */
static int bcm430x_get_current_core(struct bcm430x_private *bcm, int *core)
{
	int err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_REG_ACTIVE_CORE, core);
	*core = (*core - 0x18000000) / 0x1000;
	return err;
}

static int bcm430x_switch_core(struct bcm430x_private *bcm, int core)
{
	int attempts = 0;
	int current_core = -1;

	/* refuse to map a negative core number */
	if (core < 0) return -1;
	
	bcm430x_get_current_core(bcm, &current_core);

	/* Write the computed value to the register. This doesn't always
	   succeed so we retry BCM430x_SWITCH_CORE_MAX_RETRIES times */
	while (current_core != core) {
		if (attempts++ > BCM430x_SWITCH_CORE_MAX_RETRIES) goto err_out;
		bcm430x_pci_write_config_32(bcm->pci_dev,
					    BCM430x_REG_ACTIVE_CORE,
					    (core * 0x1000) + 0x18000000);
		bcm430x_get_current_core(bcm, &current_core);
	}

	/* sucess */
	return 0;

err_out:
	printk(KERN_ERR PFX
	       "unable to switch to core %u, retried %i times",
	       core, attempts);
	return -1;

}

/* returns non-zero if the current core is enabled, zero otherwise */
static int bcm430x_core_enabled(struct bcm430x_private *bcm)
{
	/* fetch sbtmstatelow from core information registers */
	bcm->sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);

	bcm->sbtmstatelow = bcm->sbtmstatelow & (BCM430x_SBTMSTATELOW_CLOCK |
			BCM430x_SBTMSTATELOW_RESET | BCM430x_SBTMSTATELOW_REJECT);

	return ~ (bcm->sbtmstatelow ^ BCM430x_SBTMSTATELOW_CLOCK);
}

/* disable current core */
static int bcm430x_core_disable(struct bcm430x_private *bcm, int core_flags)
{
	int i;

	/* fetch sbtmstatelow from core information registers */
	bcm->sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);

	/* core is already in reset */
	if (bcm->sbtmstatelow | BCM430x_SBTMSTATELOW_RESET) return 0;

	if (! (bcm->sbtmstatelow | BCM430x_SBTMSTATELOW_CLOCK)) {
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
				BCM430x_SBTMSTATELOW_CLOCK |
				BCM430x_SBTMSTATELOW_REJECT);
		
		i = 0;
		while(1) {
			if (bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW) | 
				BCM430x_SBTMSTATELOW_REJECT) break;
			if (i++ > 5000) break;
		}

		printk (KERN_INFO PFX "Disabling core looped %d times.\n", i);
		i = 0;
		while(1) {
			if (bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH) | 
				BCM430x_SBTMSTATEHIGH_BUSY) break;
			if (i++ > 5000) break;
		}
		printk (KERN_INFO PFX "Disabling core looped %d times.\n", i);

		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_RESET |
			BCM430x_SBTMSTATELOW_REJECT |
			BCM430x_SBTMSTATELOW_CLOCK |
			BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
			core_flags);

		udelay(10);

	}

	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_RESET |
			BCM430x_SBTMSTATELOW_REJECT |
			core_flags);
}

/* enable current core */
static int bcm430x_core_enable(struct bcm430x_private *bcm, u32 core_flags)
{
	bcm430x_core_disable(bcm, core_flags);

	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_CLOCK |
			BCM430x_SBTMSTATELOW_RESET |
			BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
			core_flags);

	udelay(1);

	bcm->sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
	if (bcm->sbtmstatehigh | BCM430x_SBTMSTATEHIGH_SERROR) {
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATEHIGH, 0);
	}

	bcm->sbimstate = bcm430x_read32(bcm, BCM430x_CIR_SBIMSTATE);
	if (bcm->sbimstate | BCM430x_SBIMSTATE_IB_ERROR | BCM430x_SBIMSTATE_TIMEOUT) {
		bcm->sbimstate &= ~(BCM430x_SBIMSTATE_IB_ERROR | BCM430x_SBIMSTATE_TIMEOUT);
		bcm430x_write32(bcm, BCM430x_CIR_SBIMSTATE, bcm->sbimstate);
	}

	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_CLOCK |
			BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
			core_flags);

	udelay(1);

	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_CLOCK |
			core_flags);

	return 0;
}

/* Validate chip access
 * http://bcm-specs.sipsolutions.net/ValidateChipAccess */
static int bcm430x_validate_chip(struct bcm430x_private *bcm)
{
	u32 status;
	u32 shm_backup;
	u16 phy_version;

	/* some magic from http://bcm-specs.sipsolutions.net/DeviceInitialization */

	/* select and enable 80211 core */
	bcm430x_switch_core(bcm, bcm->core_index);
	bcm430x_core_enable(bcm, 0x20040000);

	/* set bit in status register */
	status = bcm430x_read32(bcm, 0x120);
	status |= 0x400;
	bcm430x_write32(bcm, 0x120, status);

	/* get phy version */
	phy_version = bcm430x_read16(bcm, 0x3E0);

	bcm->phy_version = (phy_version & 0xF000) >> 12;
	bcm->phy_type = (phy_version & 0x0F00) >> 8;
	bcm->phy_rev = (phy_version & 0xF);

	printk(KERN_INFO PFX "phy UnkVer: %x, Type %x, Revision %x\n",
			bcm->phy_version, bcm->phy_type, bcm->phy_rev);

	shm_backup = bcm430x_shm_read32(bcm, 0x00010000);
	bcm430x_shm_write32(bcm, 0x00010000, 0xAA5555AA);
	if (bcm430x_shm_read32(bcm, 0x00010000) != 0xAA5555AA) {
		printk(KERN_ERR PFX "SHM mismatch (1) validating chip.\n");
	}

	bcm430x_shm_write32(bcm, 0x00010000, 0x55AAAA55);
	if (bcm430x_shm_read32(bcm, 0x00010000) != 0x55AAAA55) {
		printk(KERN_ERR PFX "SHM mismatch (2) validating chip.\n");
	}

	bcm430x_shm_write32(bcm, 0x00010000, shm_backup);

	if (bcm430x_read32(bcm, 0x128) != 0) {
		printk(KERN_ERR PFX "Bad interrupt reason code (?) validating chip.\n");
	}
	
	if (bcm->phy_type > 2) {
		printk(KERN_ERR PFX "Unknown PHY Type: %x\n", bcm->phy_type);
	}

	return 0;
}

static int bcm430x_probe_cores(struct bcm430x_private *bcm)
{
	int original_core, current_core, core_count;
	int core_vendor, core_id, core_rev, core_enabled;
	u32 sb_id_hi, chip_id_32 = 0;
	u16 pci_device, chip_id_16;

	/* save current core */
	bcm430x_get_current_core(bcm, &original_core);

	/* map core 0 */
	bcm430x_switch_core(bcm, 0);

	/* fetch sb_id_hi from core information registers */
	sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

	core_id = (sb_id_hi & 0xFFF0) >> 4;
	core_rev = (sb_id_hi & 0xF);
	core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;

	printk(KERN_INFO PFX "Core 0: ID 0x%x, rev 0x%x, vendor 0x%x\n", core_id,
			core_rev, core_vendor);

	/* if present, chipcommon is always core 0; read the chipid from it */
	if (core_id == BCM430x_COREID_CHIPCOMMON) {
		chip_id_32 = bcm430x_read32(bcm, 0);
		chip_id_16 = chip_id_32 & 0xFFFF;
	} else {
		/* without a chipCommon, use a hard coded table. */
		pci_device = bcm->pci_dev->device;
		if (pci_device == 0x4301) {
			chip_id_16 = 0x4301;
		} else if ((pci_device >= 0x4305) && (pci_device <= 0x4307)) {
			chip_id_16 = 0x4307;
		} else if ((pci_device >= 0x4402) && (pci_device <= 0x4403)) {
			chip_id_16 = 0x4402;
		} else if ((pci_device >= 0x4610) && (pci_device <= 0x4615)) {
			chip_id_16 = 0x4610;
		} else if ((pci_device >= 0x4710) && (pci_device <= 0x4715)) {
			chip_id_16 = 0x4710;
		} else {
			/* Presumably devices not listed above are not
			 * put into the pci device table for this driver,
			 * so we should never get here */
			chip_id_16 = 0x2BAD;
		}
	}

	/* ChipCommon with Core Rev >=4 encodes number of cores,
	 * otherwise consult hardcoded table */
	if (core_id == 0x800 && core_rev >= 4) {
		core_count = (chip_id_32 & 0x0F000000) >> 24;
	} else {
		switch (chip_id_16) {
			case 0x4610:
			case 0x4704:
			case 0x4710:
				core_count = 9;
				break;
			case 0x4310:
				core_count = 8;
				break;
			case 0x5365:
				core_count = 7;
				break;
			case 0x4306:
				core_count = 6;
				break;
			case 0x4301:
			case 0x4307:
				core_count = 5;
				break;
			case 0x4402:
				core_count = 3;
				break;
			default:
				/* SOL if we get here */
				core_count = 1;
		}
	}

	for (current_core = 1; current_core < core_count; current_core++) {
		bcm430x_switch_core(bcm, current_core);

		/* fetch sb_id_hi from core information registers */
		sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

		/* extract core_id, core_rev, core_vendor */
		core_id = (sb_id_hi & 0xFFF0) >> 4;
		core_rev = (sb_id_hi & 0xF);
		core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;
		
		core_enabled = bcm430x_core_enabled(bcm);

		/* if we find an 802.11 core, reset/enable it
		 * and store its info in the device struct */
		if (core_id == BCM430x_COREID_80211) {
			bcm->core_id = core_id;
			bcm->core_index = current_core;
			bcm->core_rev = core_rev;
		}

		printk(KERN_INFO PFX "Core %d: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
				current_core, core_id, core_rev, core_vendor,
				core_enabled ? "enabled" : "disabled" );
	}

	/* restore original core mapping */
	bcm430x_switch_core(bcm, original_core);

	return 0;
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

	bcm430x_probe_cores(bcm);
	bcm430x_pctl_set_crystal(bcm, 1);
	bcm430x_clr_target_abort(bcm);
	bcm430x_validate_chip(bcm);

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
	struct bcm430x_private *bcm = netdev_priv(dev);

	if (dev) {
		unregister_netdev(dev);
		bcm430x_pctl_set_crystal(bcm, 0);
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
