/*

  Broadcom BCM430x wireless driver

  Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
                     Stefano Brivio <st3@riseup.net>
                     Michael Buesch <mbuesch@freenet.de>
                     Danny van Dyk <kugelfang@gentoo.org>
                     Andreas Jaggi <andreas.jaggi@waterwave.ch>

  Some parts of the code in this file are derived from the ipw2200
  driver  Copyright(c) 2003 - 2004 Intel Corporation.

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

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/wireless.h>
#include <linux/workqueue.h>
#include <net/iw_handler.h>

#include "bcm430x.h"
#include "bcm430x_main.h"
#include "bcm430x_debugfs.h"
#include "bcm430x_radio.h"
#include "bcm430x_phy.h"
#include "bcm430x_dma.h"
#include "bcm430x_power.h"
#include "bcm430x_wx.h"

#ifdef dprintk
# undef dprintk
#endif
#ifdef BCM430x_DEBUG
# define dprintk printk
#else
# define dprintk(x...) do {} while (0)
#endif

MODULE_DESCRIPTION("Broadcom BCM430x wireless driver");
MODULE_AUTHOR("Martin Langer");
MODULE_AUTHOR("Stefano Brivio");
MODULE_AUTHOR("Michael Buesch");
MODULE_LICENSE("GPL");

/* module parameters */
static int mode = 0;
static int pio = 0;

/* If you want to debug with just a single device, enable this,
 * where the string is the pci device ID (as given by the kernel's
 * pci_name function) of the device to be used.
 */
//#define DEBUG_SINGLE_DEVICE_ONLY	"0001:11:00.0"


static struct pci_device_id bcm430x_pci_tbl[] = {

	/* Broadcom 4303 802.11b */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1028, 0x0407, 0, 0, 0 }, /* Dell TrueMobile 1180 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4301, 0x1043, 0x0120, 0, 0, 0 }, /* Asus WL-103b PC Card */

	/* Broadcom 4307 802.11b */
//	{ PCI_VENDOR_ID_BROADCOM, 0x4307, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },

	/* Broadcom 4318 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x103c, 0x1355, 0, 0, 0 }, /* Compag v2000z Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x106b, 0x4318, 0, 0, 0 }, /* Apple AirPort Extreme 2 Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1799, 0x7000, 0, 0, 0 }, /* Belkin F5D7000 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x17f9, 0x0006, 0, 0, 0 }, /* Amilo A1650 Laptop */

	/* Broadcom 4306 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x02fa, 0x3010, 0, 0, 0 }, /* Siemens Gigaset PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1300 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0002, 0, 0, 0 }, /* Dell TrueMobile 1300 PCMCIA Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1350 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x103c, 0x12fa, 0, 0, 0 }, /* Compaq Presario R3xxx PCI on board */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1043, 0x100f, 0, 0, 0 }, /* Asus WL-100G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1057, 0x7025, 0, 0, 0 }, /* Motorola WN825G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x106b, 0x004e, 0, 0, 0 }, /* Apple AirPort Extreme Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x14e4, 0x0013, 0, 0, 0 }, /* Linksys WMP54G PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1737, 0x0015, 0, 0, 0 }, /* Linksys WMP54GS PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7001, 0, 0, 0 }, /* Belkin F5D7001 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7010, 0, 0, 0 }, /* Belkin F5D7010 PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x185f, 0x1220, 0, 0, 0 }, /* Linksys WMP54G PCI Card */

	/* Broadcom 4306 802.11a */
//	{ PCI_VENDOR_ID_BROADCOM, 0x4321, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },

	/* Broadcom 4309 802.11a/b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4324, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1400 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4324, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1450 Mini-PCI Card */

	/* Broadcom 43XG 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4325, 0x1414, 0x0003, 0, 0, 0 }, /* Microsoft MN-720 PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4325, 0x1414, 0x0004, 0, 0, 0 }, /* Microsoft MN-730 PCI Card */

	/* required last entry */
	{ 0, },
};


u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset)
{
	u16 val;

	val = ioread16(bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "read 16  0x%04x  0x%04x\n", offset, val);
	return val;
}

void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	iowrite16(val, bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "write 16  0x%04x  0x%04x\n", offset, val);
}

u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset)
{
	u32 val;

	val = ioread32(bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "read 32  0x%04x  0x%08x\n", offset, val);
	return val;
}

void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	iowrite32(val, bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "write 32  0x%04x  0x%08x\n", offset, val);
}


static void bcm430x_ram_write(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_RAM_CONTROL, offset);
	bcm430x_write32(bcm, BCM430x_MMIO_RAM_DATA, val);
}

void bcm430x_shm_control(struct bcm430x_private *bcm, u32 control)
{
	bcm->shm_addr = (control & 0x0000FFFF);
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_CONTROL, control);
}

u16 bcm430x_shm_read16(struct bcm430x_private *bcm)
{
	return bcm430x_read16(bcm, BCM430x_MMIO_SHM_DATA + (bcm->shm_addr % 4));
}

u32 bcm430x_shm_read32(struct bcm430x_private *bcm)
{
	return bcm430x_read32(bcm, BCM430x_MMIO_SHM_DATA);
}

void bcm430x_shm_write16(struct bcm430x_private *bcm, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_SHM_DATA + (bcm->shm_addr % 4), val);
}

void bcm430x_shm_write32(struct bcm430x_private *bcm, u32 val)
{
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA, val);
}

int bcm430x_pci_read_config_8(struct pci_dev *pdev, u16 offset, u8 * val)
{
	int err;

	err = pci_read_config_byte(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 8  0x%04x  0x%02x\n", offset, *val);
	return err;
}

int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset,
				      u16 * val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 16  0x%04x  0x%04x\n", offset, *val);
	return err;
}

int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset,
				      u32 * val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 32  0x%04x  0x%08x\n", offset, *val);
	return err;
}

int bcm430x_pci_write_config_8(struct pci_dev *pdev, int offset, u8 val)
{
//	dprintk(KERN_INFO PFX "pci write 8  0x%04x  0x%02x\n", offset, val);
	return pci_write_config_byte(pdev, offset, val);
}

int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset,
				       u16 val)
{
//	dprintk(KERN_INFO PFX "pci write 16  0x%04x  0x%04x\n", offset, val);
	return pci_write_config_word(pdev, offset, val);
}

int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset,
				       u32 val)
{
//	dprintk(KERN_INFO PFX "pci write 32  0x%04x  0x%08x\n", offset, val);
	return pci_write_config_dword(pdev, offset, val);
}

static void bcm430x_read_radio_id(struct bcm430x_private *bcm)
{
	u32 val;
	
	if (bcm->chip_id == 0x4317) {
		if (bcm->chip_rev == 0x00)
			val = 0x3205017F;
		else if (bcm->chip_rev == 0x01)
			val = 0x4205017F;
		else
			val = 0x5205017F; /* Yes, this is correct. */
	} else {
		bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, BCM430x_RADIO_ID);
		val = (u32)bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA) << 16;
		val |= bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA_LOW);
	}
	
	bcm->radio_id = val;
	printk(KERN_INFO PFX "Radio ID: %x (Manuf: %x Ver: %x Rev: %x)\n",
		bcm->radio_id,
		(bcm->radio_id & 0x00000FFF),
		(bcm->radio_id & 0x0FFFF000) >> 12,
		(bcm->radio_id & 0xF0000000) >> 28);
}	

/* Read SPROM and fill the useful values in the net_device struct */
static void bcm430x_read_sprom(struct bcm430x_private *bcm)
{
	int i;
	u16 value;
	struct net_device *net_dev = bcm->net_dev;

	/* read MAC address into dev->dev_addr */
	value = bcm430x_read16(bcm, BCM430x_SPROM_IL0MACADDR + 0);
	*((u16 *)net_dev->dev_addr + 0) = be16_to_cpu(value);
	value = bcm430x_read16(bcm, BCM430x_SPROM_IL0MACADDR + 2);
	*((u16 *)net_dev->dev_addr + 1) = be16_to_cpu(value);
	value = bcm430x_read16(bcm, BCM430x_SPROM_IL0MACADDR + 4);
	*((u16 *)net_dev->dev_addr + 2) = be16_to_cpu(value);

	value = bcm430x_read16(bcm, BCM430x_SPROM_BOARDFLAGS);
	if (value == 0xffff)
		value = 0x0000;
	bcm->sprom.boardflags = value;

	/* read LED infos */
	value = be16_to_cpu(bcm430x_read16(bcm, BCM430x_SPROM_WL0GPIO0));
	bcm->leds[0] = value & 0x00FF;
	bcm->leds[1] = (value & 0xFF00) >> 8;
	value = be16_to_cpu(bcm430x_read16(bcm, BCM430x_SPROM_WL0GPIO2));
	bcm->leds[2] = value & 0x00FF;
	bcm->leds[3] = (value & 0xFF00) >> 8;

	for (i = 0; i < BCM430x_LED_COUNT; i++) {
		if ((bcm->leds[i] & ~BCM430x_LED_ACTIVELOW) == BCM430x_LED_INACTIVE) {
			bcm->leds[i] = 0xFF;
			continue;
		};
		if (bcm->leds[i] == 0xFF) {
			switch (i) {
			case 0:
				bcm->leds[0] = ((bcm->board_vendor == PCI_VENDOR_ID_COMPAQ)
				                ? BCM430x_LED_RADIO_ALL
				                : BCM430x_LED_ACTIVITY);
				break;
			case 1:
				bcm->leds[1] = BCM430x_LED_RADIO_B;
				break;
			case 2:
				bcm->leds[2] = BCM430x_LED_RADIO_A;
				break;
			case 3:
				bcm->leds[3] = BCM430x_LED_OFF;
				break;
			}
		}
	}
}


/* DummyTransmission function, as documented on 
 * http://bcm-specs.sipsolutions.net/DummyTransmission
 */
int bcm430x_dummy_transmission(struct bcm430x_private *bcm)
{
	unsigned int i, max_loop;
	u16 value = 0;
	u32 buffer[5] = {
		0x00000000,
		cpu_to_be32(0x0000D400),
		0x00000000,
		cpu_to_be32(0x00000001),
		0x00000000,
	};
	u8 *tmp;

	switch (bcm->phy_type) {
	case BCM430x_PHYTYPE_A:
		max_loop = 0x1E;
		buffer[0] = cpu_to_be32(0xCC010200);
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		max_loop = 0xFA;
		buffer[0] = cpu_to_be32(0x6E840B00); 
		break;
	default:
		printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return -1;
	}

#if BCM430x_DEBUG
	printk(KERN_WARNING PFX "DummyTransmission():\n");
	printk(KERN_WARNING PFX "Packet:\n");
	for (i = 0; i < 5; i++) {
		tmp = (u8 *)&buffer[i];
		printk(KERN_WARNING PFX "%02x %02x %02x %02x\n", tmp[0], tmp[1], tmp[2], tmp[3]);
	}
#endif
	for (i = 0; i < 5; i++) {
		bcm430x_ram_write(bcm, i * 4, buffer[i]);
	}
#if BCM430x_DEBUG
	printk(KERN_WARNING PFX "Packet written\n");
#endif

	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);

	bcm430x_write16(bcm, 0x0568, 0x0000);
	bcm430x_write16(bcm, 0x07C0, 0x0000);
	bcm430x_write16(bcm, 0x050C, ((bcm->phy_type == BCM430x_PHYTYPE_A) ? 1 : 0));
	bcm430x_write16(bcm, 0x0508, 0x0000);
	bcm430x_write16(bcm, 0x050A, 0x0000);
	bcm430x_write16(bcm, 0x054C, 0x0000);
	bcm430x_write16(bcm, 0x056A, 0x0014);
	bcm430x_write16(bcm, 0x0568, 0x0826);
	bcm430x_write16(bcm, 0x0500, 0x0000);
	bcm430x_write16(bcm, 0x0502, 0x0030);

	
	for (i = 0x00; i < max_loop; i++) {
		value = bcm430x_read16(bcm, 0x050E);
#ifdef BCM430x_DEBUG
		printk(KERN_INFO PFX "dummy_tx(): loop1, iteration %d, value = %04x\n", i, value);
#endif
		if ((value & 0x0080) != 0)
			break;
		udelay(10);
	}
	for (i = 0x00; i < 0x0A; i++) {
		value = bcm430x_read16(bcm, 0x050E);
#ifdef BCM430x_DEBUG
		printk(KERN_INFO PFX "dummy_tx(): loop2, iteration %d, value = %04x\n", i, value);
#endif
		if ((value & 0x0400) != 0)
			break;
		udelay(10);
	}
	for (i = 0x00; i < 0x0A; i++) {
		value = bcm430x_read16(bcm, 0x0690);
#ifdef BCM430x_DEBUG
		printk(KERN_INFO PFX "dummy_tx(): loop3, iteration %d, value = %04x\n", i, value);
#endif
		if ((value & 0x0100) == 0)
			break;
		udelay(10);
	}

	return 0;
}

/* Puts the index of the current core into user supplied core variable.
 * This function reads the value from the device.
 * Almost always you don't want to call this, but use bcm->current_core
 */
static int _get_current_core(struct bcm430x_private *bcm, int *core)
{
	int err;

	err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_REG_ACTIVE_CORE, core);
	if (err) {
		printk(KERN_ERR PFX "Error: cannot read ACTIVE_CORE register!\n");
		return -ENODEV;
	}
	*core = (*core - 0x18000000) / 0x1000;

	return 0;
}

/* Lowlevel core-switch function. This is only to be used in
 * bcm430x_switch_core() and bcm430x_probe_cores()
 */
static int _switch_core(struct bcm430x_private *bcm, int core)
{
	int err;
	int attempts = 0;
	int current_core = -1;

	assert(core >= 0);

	err = _get_current_core(bcm, &current_core);
	if (err)
		goto out;

	/* Write the computed value to the register. This doesn't always
	   succeed so we retry BCM430x_SWITCH_CORE_MAX_RETRIES times */
	while (current_core != core) {
		if (attempts++ > BCM430x_SWITCH_CORE_MAX_RETRIES) {
			err = -ENODEV;
			printk(KERN_ERR PFX
			       "unable to switch to core %u, retried %i times",
			       core, attempts);
			goto out;
		}
		bcm430x_pci_write_config_32(bcm->pci_dev,
					    BCM430x_REG_ACTIVE_CORE,
					    (core * 0x1000) + 0x18000000);
		_get_current_core(bcm, &current_core);
	}

	assert(err == 0);
out:
	return err;
}

int bcm430x_switch_core(struct bcm430x_private *bcm, struct bcm430x_coreinfo *new_core)
{
	int err;

	if (!new_core)
		return 0;

	if (!(new_core->flags & BCM430x_COREFLAG_AVAILABLE))
		return -ENODEV;
	if (bcm->current_core == new_core)
		return 0;
	err = _switch_core(bcm, new_core->index);
	if (!err)
		bcm->current_core = new_core;

	return err;
}

/* returns non-zero if the current core is enabled, zero otherwise */
static inline int bcm430x_core_enabled(struct bcm430x_private *bcm)
{
	u32 value;

	value = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
	value &= BCM430x_SBTMSTATELOW_CLOCK | BCM430x_SBTMSTATELOW_RESET
		 | BCM430x_SBTMSTATELOW_REJECT;

	return (value == BCM430x_SBTMSTATELOW_CLOCK);
}

/* disable current core */
static int bcm430x_core_disable(struct bcm430x_private *bcm, int core_flags)
{
	int i;

	/* fetch sbtmstatelow from core information registers */
	bcm->sbtmstatelow = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);

	/* core is already in reset */
	if (bcm->sbtmstatelow | BCM430x_SBTMSTATELOW_RESET)
		goto out;

	if (! (bcm->sbtmstatelow | BCM430x_SBTMSTATELOW_CLOCK)) {
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
				BCM430x_SBTMSTATELOW_CLOCK |
				BCM430x_SBTMSTATELOW_REJECT);
		
		i = 0;
		while (1) {
			if (bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW) | 
			    BCM430x_SBTMSTATELOW_REJECT)
				break;
			if (i++ > 5000)
				break;
		}

		printk (KERN_INFO PFX "Disabling core looped %d times.\n", i);
		i = 0;
		while (1) {
			if (bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH) | 
			    BCM430x_SBTMSTATEHIGH_BUSY)
				break;
			if (i++ > 5000)
				break;
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

out:
	bcm->current_core->flags &= ~ BCM430x_COREFLAG_ENABLED;
	return 0;
}

/* enable current core */
static int bcm430x_core_enable(struct bcm430x_private *bcm, u32 core_flags)
{
	int err;

	err = bcm430x_core_disable(bcm, core_flags);
	if (err)
		goto out;

	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
			BCM430x_SBTMSTATELOW_CLOCK |
			BCM430x_SBTMSTATELOW_RESET |
			BCM430x_SBTMSTATELOW_FORCE_GATE_CLOCK |
			core_flags);

	udelay(1);

	bcm->sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
	if (bcm->sbtmstatehigh | BCM430x_SBTMSTATEHIGH_SERROR)
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATEHIGH, 0);

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

	bcm->current_core->flags |= BCM430x_COREFLAG_ENABLED;
	assert(err == 0);
out:
	return err;
}

/* http://bcm-specs.sipsolutions.net/80211CoreReset */
void bcm430x_wireless_core_reset(struct bcm430x_private *bcm, int connect_phy)
{
	u32 flags = 0x00040000;

	if ((bcm430x_core_enabled(bcm)) && (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA)) {
		/* reset all used DMA controllers. */
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA1_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA2_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA3_BASE);
		bcm430x_dmacontroller_tx_reset(bcm, BCM430x_MMIO_DMA4_BASE);
		bcm430x_dmacontroller_rx_reset(bcm, BCM430x_MMIO_DMA1_BASE);
		if (bcm->current_core->rev < 5)
			bcm430x_dmacontroller_rx_reset(bcm, BCM430x_MMIO_DMA4_BASE);
	}
	if (bcm->status & BCM430x_STAT_DEVSHUTDOWN)
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
		                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
				& ~(BCM430x_SBF_MAC_ENABLED | 0x00000002));
	else {
		if (connect_phy)
			flags |= 0x20000000;
		bcm430x_phy_connect(bcm, connect_phy);
		bcm430x_core_enable(bcm, flags);
		bcm430x_write16(bcm, 0x03E6, 0x0000);
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00000400);
		//XXX: bcm->dma_savestatus[0--3] = { 0, 0, 0, 0 };
	}
}

static void bcm430x_wireless_core_disable(struct bcm430x_private *bcm)
{
	if (bcm->status & BCM430x_STAT_DEVSHUTDOWN) {
		bcm430x_radio_turn_off(bcm);
		bcm430x_write16(bcm, 0x03E6, 0x00F4);
		bcm430x_core_disable(bcm, bcm->current_core->flags);
	} else {
		if (bcm->status & BCM430x_STAT_RADIOENABLED) {
			bcm430x_radio_turn_off(bcm);
		} else {
			if ((bcm->current_core->rev >= 3) && (bcm430x_read32(bcm, 0x0158) & (1 << 16)))
				bcm430x_radio_turn_off(bcm);
			if ((bcm->current_core->rev < 3) && !(bcm430x_read16(bcm, 0x049A) & (1 << 4)))
				bcm430x_radio_turn_off(bcm);
		}
	}
}

/* Enable a Generic IRQ. "mask" is the mask of which IRQs to enable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm430x_interrupt_enable(struct bcm430x_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_MASK, old_mask | mask);

	return old_mask;
}

/* Disable a Generic IRQ. "mask" is the mask of which IRQs to disable.
 * Returns the _previously_ enabled IRQ mask.
 */
static inline u32 bcm430x_interrupt_disable(struct bcm430x_private *bcm, u32 mask)
{
	u32 old_mask;

	old_mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_MASK, old_mask & ~mask);

	return old_mask;
}

/* Interrupt handler bottom-half */
static void bcm430x_interrupt_tasklet(struct bcm430x_private *bcm)
{
	u32 reason;
	unsigned long flags;

#ifdef BCM430x_DEBUG
	int _handled = 0;
# define bcmirq_handled()	do { _handled = 1; } while (0)
#else
# define bcmirq_handled()	do { /* nothing */ } while (0)
#endif /* BCM430x_DEBUG */

	spin_lock_irqsave(&bcm->lock, flags);
	reason = bcm->irq_reason;

printkl(KERN_INFO PFX "We got an interrupt! 0x%08x, DMA: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
	reason, bcm->dma_reason[0], bcm->dma_reason[1], bcm->dma_reason[2], bcm->dma_reason[3]);

	if (reason & BCM430x_IRQ_BEACON) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_TBTT) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_REG124) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_PMQ) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_TXFIFO_ERROR) {
		printkl(KERN_ERR PFX "TX FIFO error. DMA: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			bcm->dma_reason[0], bcm->dma_reason[1],
			bcm->dma_reason[2], bcm->dma_reason[3]);
		bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_SCAN) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_BGNOISE) {
		/*TODO*/
		//bcmirq_handled();
	}

	if (reason & BCM430x_IRQ_XMIT_STATUS) {
		/*TODO*/
		//bcmirq_handled();
	}

#ifdef BCM430x_DEBUG
	if (!_handled)
		printkl(KERN_WARNING PFX "Unhandled IRQ! Reason: 0x%08x\n", reason);
#endif
#undef bcmirq_handled

	bcm430x_interrupt_enable(bcm, bcm->irq_savedstate);
	spin_unlock_irqrestore(&bcm->lock, flags);
}

/* Interrupt handler top-half */
static irqreturn_t bcm430x_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	struct bcm430x_private *bcm = dev_id;
	u32 reason, mask;

	if (!bcm)
		return IRQ_NONE;

	spin_lock(&bcm->lock);

	reason = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	if (reason == 0xffffffff) {
		/* irq not for us (shared irq) */
		spin_unlock(&bcm->lock);
		return IRQ_NONE;
	}
	mask = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_MASK);
	if (!(reason & mask)) {
		spin_unlock(&bcm->lock);
		return IRQ_HANDLED;
	}

	bcm->dma_reason[0] = bcm430x_read32(bcm, BCM430x_MMIO_DMA1_REASON)
			     & 0x0001dc00;
	bcm->dma_reason[1] = bcm430x_read32(bcm, BCM430x_MMIO_DMA2_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[2] = bcm430x_read32(bcm, BCM430x_MMIO_DMA3_REASON)
			     & 0x0000dc00;
	bcm->dma_reason[3] = bcm430x_read32(bcm, BCM430x_MMIO_DMA4_REASON)
			     & 0x0001dc00;

	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON,
			reason & mask);

	bcm430x_write32(bcm, BCM430x_MMIO_DMA1_REASON,
			bcm->dma_reason[0]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA2_REASON,
			bcm->dma_reason[1]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA3_REASON,
			bcm->dma_reason[2]);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA4_REASON,
			bcm->dma_reason[3]);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_PIO &&
	    bcm->current_core->rev < 3) {
		/* TODO */
	}

	/* disable all IRQs. They are enabled again in the bottom half. */
	bcm->irq_savedstate = bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);

	/* save the reason code and call our bottom half. */
	bcm->irq_reason = reason;
	tasklet_schedule(&bcm->isr_tasklet);

	spin_unlock(&bcm->lock);

	return IRQ_HANDLED;
}

static inline void bcm430x_write_microcode(struct bcm430x_private *bcm,
				    const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm430x_shm_control(bcm, BCM430x_SHM_UCODE + 0x0000);
	for (i = 0; i < len; i++) {
		bcm430x_shm_write32(bcm, be32_to_cpu(data[i]));
		udelay(10);
	}
}

static inline void bcm430x_write_pcm(struct bcm430x_private *bcm,
			      const u32 *data, const unsigned int len)
{
	unsigned int i;
	bcm430x_shm_control(bcm, BCM430x_SHM_PCM + 0x01ea);
	bcm430x_shm_write32(bcm, 0x00004000);
	bcm430x_shm_control(bcm, BCM430x_SHM_PCM + 0x01eb);
	for (i = 0; i < len; i++) {
		bcm430x_shm_write32(bcm, be32_to_cpu(data[i]));
		udelay(10);
	}
}

static int bcm430x_upload_microcode(struct bcm430x_private *bcm)
{
	const struct firmware *fw;
	char buf[22] = { 0 };

	snprintf(buf, ARRAY_SIZE(buf), "bcm430x_microcode%d.fw",
		 (bcm->current_core->rev >= 5 ? 5 : bcm->current_core->rev));
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX 
		       "Error: Microcode \"%s\" not available or load failed.\n",
		        buf);
		return -ENODEV;
	}
	bcm430x_write_microcode(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
#ifdef BCM430x_DEBUG
	bcm->ucode_size = fw->size;
#endif
	release_firmware(fw);

	snprintf(buf, ARRAY_SIZE(buf),
		 "bcm430x_pcm%d.fw", (bcm->current_core->rev < 5 ? 4 : 5));
	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX
		       "Error: PCM \"%s\" not available or load failed.\n",
		       buf);
		return -ENODEV;
	}
	bcm430x_write_pcm(bcm, (u32 *)fw->data, fw->size / sizeof(u32));
#ifdef BCM430x_DEBUG
	bcm->pcm_size = fw->size;
#endif
	release_firmware(fw);

	return 0;
}

static void bcm430x_write_initvals(struct bcm430x_private *bcm,
				   const struct bcm430x_initval *data,
				   const unsigned int len)
{
	u16 offset, size;
	u32 value;
	unsigned int i;

	for (i = 0; i < len; i++) {
		offset = be16_to_cpu(data[i].offset);
		size = be16_to_cpu(data[i].size);
		value = be32_to_cpu(data[i].value);

		if (size == 2)
			bcm430x_write16(bcm, offset, value);
		else if (size == 4)
			bcm430x_write32(bcm, offset, value);
		else
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
	}
}

static int bcm430x_upload_initvals(struct bcm430x_private *bcm)
{
	const struct firmware *fw;
	char buf[21] = { 0 };
	
	if ((bcm->current_core->rev == 2) || (bcm->current_core->rev == 4)) {
		switch (bcm->phy_type) {
			case BCM430x_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 3);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 1);
				break;
			default:
				goto out_noinitval;
		}
	
	} else if (bcm->current_core->rev >= 5) {
		switch (bcm->phy_type) {
			case BCM430x_PHYTYPE_A:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 7);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
				snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 5);
				break;
			default:
				goto out_noinitval;
		}
	} else
		goto out_noinitval;

	if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
		printk(KERN_ERR PFX 
		       "Error: InitVals \"%s\" not available or load failed.\n",
		        buf);
		return -ENODEV;
	}
	if (fw->size % sizeof(struct bcm430x_initval)) {
		printk(KERN_ERR PFX "InitVals fileformat error.\n");
		release_firmware(fw);
		return -ENODEV;
	}

	bcm430x_write_initvals(bcm, (struct bcm430x_initval *)fw->data,
			       fw->size / sizeof(struct bcm430x_initval));

	release_firmware(fw);

	if (bcm->current_core->rev >= 5) {
		switch (bcm->phy_type) {
			case BCM430x_PHYTYPE_A:
				bcm->sbtmstatehigh = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
				if (bcm->sbtmstatehigh & 0x00010000)
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 9);
				else
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 10);
				break;
			case BCM430x_PHYTYPE_B:
			case BCM430x_PHYTYPE_G:
					snprintf(buf, ARRAY_SIZE(buf), "bcm430x_initval%02d.fw", 6);
				break;
			default:
				goto out_noinitval;
		}
	
		if (request_firmware(&fw, buf, &bcm->pci_dev->dev) != 0) {
			printk(KERN_ERR PFX 
			       "Error: InitVals \"%s\" not available or load failed.\n",
		        	buf);
			return -ENODEV;
		}
		if (fw->size % sizeof(struct bcm430x_initval)) {
			printk(KERN_ERR PFX "InitVals fileformat error.\n");
			release_firmware(fw);
			return -ENODEV;
		}

		bcm430x_write_initvals(bcm, (struct bcm430x_initval *)fw->data,
				       fw->size / sizeof(struct bcm430x_initval));
	
		release_firmware(fw);
		
	}

printk(KERN_INFO PFX "InitVals written\n");
	return 0;

out_noinitval:
	printk(KERN_ERR PFX "Error: No InitVals available!\n");
	return -ENODEV;
}

static int bcm430x_initialize_irq(struct bcm430x_private *bcm)
{
	int res;
	unsigned int i;
	u32 data;

	res = request_irq(bcm->pci_dev->irq, bcm430x_interrupt_handler,
			  SA_SHIRQ, DRV_NAME, bcm);
	if (res) {
		printk(KERN_ERR PFX "Cannot register IRQ%d\n", bcm->pci_dev->irq);
		return -EFAULT;
	}
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, 0xffffffff);
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00020402);
	i = 0;
	while (1) {
		data = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
		if (data == 0x00000001)
			break;
		i++;
		if (i >= BCM430x_IRQWAIT_MAX_RETRIES) {
			printk(KERN_ERR PFX "Card IRQ register not responding. "
					    "Giving up.\n");
			free_irq(bcm->pci_dev->irq, bcm);
			return -ENODEV;
		}
		udelay(10);
	}
	// dummy read
	bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);

	return 0;
}

/* Keep this slim, as we're going to call it from within the interrupt tasklet! */
static void bcm430x_update_leds(struct bcm430x_private *bcm)
{
	int id;
	u16 value = bcm430x_read16(bcm, BCM430x_MMIO_GPIO_CONTROL);
	u16 state;

	for (id = 0; id < BCM430x_LED_COUNT; id++) {
		if (bcm->leds[id] == 0xFF)
			continue;

		state = 0;

		switch (bcm->leds[id] & ~BCM430x_LED_ACTIVELOW) {
		case BCM430x_LED_OFF:
			state = 0;
			break;
		case BCM430x_LED_ON:
			state = 1;
			break;
		case BCM430x_LED_RADIO_ALL:
			state = ((bcm->status & BCM430x_STAT_RADIOENABLED) ? 1 : 0);
			break;
		/*
		 * TODO: LED_ACTIVITY, _RADIO_A, _RADIO_B, MODE_BG, ASSOC
		 */
		default:
			break;
		};

		if (bcm->leds[id] & BCM430x_LED_ACTIVELOW)
			state = 0x0001 & (state ^ 0x0001);
		value &= ~(1 << id);
		value |= (state << id);
	}

	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_CONTROL, value);
}

/* Switch to the core used to write the GPIO register.
 * This is either the ChipCommon, or the PCI core.
 */
static inline int switch_to_gpio_core(struct bcm430x_private *bcm)
{
	int err;

	/* Where to find the GPIO register depends on the chipset.
	 * If it has a ChipCommon, its register at offset 0x6c is the GPIO
	 * control register. Otherwise the register at offset 0x6c in the
	 * PCI core is the GPIO control register.
	 */
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV) {
		err = bcm430x_switch_core(bcm, &bcm->core_pci);
		if (err == -ENODEV) {
			printk(KERN_ERR PFX "gpio error: "
			       "Neither ChipCommon nor PCI core available!\n");
			return -ENODEV;
		} else if (err != 0)
			return -ENODEV;
	} else if (err != 0)
		return -ENODEV;

	return 0;
}

/* Initialize the GPIOs
 * http://bcm-specs.sipsolutions.net/GPIO
 */
static int bcm430x_gpio_init(struct bcm430x_private *bcm)
{
	struct bcm430x_coreinfo *old_core;
	int err;
	u32 mask = 0x0000001F, value = 0x0000000F;

	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_CONTROL,
			bcm430x_read16(bcm, BCM430x_MMIO_GPIO_CONTROL) & 0xFFF0);
	bcm430x_write16(bcm, BCM430x_MMIO_GPIO_MASK,
			bcm430x_read16(bcm, BCM430x_MMIO_GPIO_MASK) | 0x000F);

	old_core = bcm->current_core;
	
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;

	if (bcm->current_core->rev >= 2){
		mask  |= 0x10;
		value |= 0x10;
	}
	if (bcm->chip_id == 0x4301) {
		mask  |= 0x60;
		value |= 0x60;
	}
	if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL) {
		mask  |= 0x200;
		value |= 0x200;
	}

	bcm430x_write32(bcm, BCM430x_GPIO_CONTROL,
	                (bcm430x_read32(bcm, BCM430x_GPIO_CONTROL) & mask) | value);

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* Turn off all GPIO stuff. Call this on module unload, for example. */
static int bcm430x_gpio_cleanup(struct bcm430x_private *bcm)
{
	struct bcm430x_coreinfo *old_core;
	int err;

	old_core = bcm->current_core;
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;
	bcm430x_write32(bcm, BCM430x_GPIO_CONTROL, 0x00000000);
	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* http://bcm-specs.sipsolutions.net/EnableMac */
static void bcm430x_mac_enable(struct bcm430x_private *bcm)
{
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			| BCM430x_SBF_MAC_ENABLED);
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, BCM430x_IRQ_READY);
	// dummy reads
	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	//FIXME: FuncPlaceholder (Set PS CTRL)
}

/* http://bcm-specs.sipsolutions.net/SuspendMAC */
static void bcm430x_mac_suspend(struct bcm430x_private *bcm)
{
	int i = 1000;
	//FIXME: FuncPlaceholder (Set PS CTRL)
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			& ~BCM430x_SBF_MAC_ENABLED);

	for ( ; i > 0; i--) {
		if (bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON) & BCM430x_IRQ_READY)
			i = -1;
	}
	if (!i)
		printk(KERN_ERR PFX "Failed to suspend mac!\n");
}

/* This is the opposite of bcm430x_chip_init() */
static void bcm430x_chip_cleanup(struct bcm430x_private *bcm)
{
	bcm430x_radio_turn_off(bcm);
	bcm430x_gpio_cleanup(bcm);
	free_irq(bcm->pci_dev->irq, bcm);
}

/* Initialize the chip
 * http://bcm-specs.sipsolutions.net/ChipInit
 */
static int bcm430x_chip_init(struct bcm430x_private *bcm)
{
	int err;
	int iw_mode = bcm->ieee->iw_mode;

	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00000404);
	err = bcm430x_upload_microcode(bcm);
	if (err)
		goto out;

	err = bcm430x_initialize_irq(bcm);
	if (err)
		goto out;

	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD) & 0xFFFF3FFF);
	
	err = bcm430x_gpio_init(bcm);
	if (err)
		goto err_free_irq;

	err = bcm430x_upload_initvals(bcm);
	if (err)
		goto err_gpio_cleanup;

	bcm430x_update_leds(bcm);

	err = bcm430x_radio_turn_on(bcm);
	if (err)
		goto err_gpio_cleanup;

	bcm430x_write16(bcm, 0x03E6, 0x0000);
	err = bcm430x_phy_init(bcm);
	if (err)
		goto err_radio_off;

	//FIXME: Calling with NONE for the time being...
	bcm430x_radio_calc_interference(bcm, BCM430x_RADIO_INTERFMODE_NONE);
	bcm430x_phy_set_antenna_diversity(bcm);
	bcm430x_radio_set_txantenna(bcm, BCM430x_RADIO_DEFAULT_ANTENNA);
	if (bcm->phy_type == BCM430x_PHYTYPE_B)
		bcm430x_write16(bcm, 0x005E,
		                bcm430x_read16(bcm, 0x005E) & 0x0004);
	bcm430x_write32(bcm, 0x0100, 0x01000000);
	bcm430x_write32(bcm, 0x010C, 0x01000000);
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			& ~BCM430x_SBF_MODE_AP);
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
	                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			| BCM430x_SBF_MODE_ADHOC);
	//FIXME: Check for promiscuous mode...
	if ((iw_mode & IW_MODE_MASTER))
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
		                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
				| 0x01000000);
	if (iw_mode & IW_MODE_MONITOR)
		bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
		                bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
				| 0x01000000
				| BCM430x_SBF_MODE_MONITOR);
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD,
			bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD)
			| BCM430x_SBF_MODE_PROMISC);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_PIO) {
		bcm430x_write16(bcm, 0x0210, 0x0100);
		bcm430x_write16(bcm, 0x0230, 0x0100);
		bcm430x_write16(bcm, 0x0250, 0x0100);
		bcm430x_write16(bcm, 0x0270, 0x0100);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0034);
		bcm430x_shm_write32(bcm, 0x00000000);
	}

	//FIXME: Probe Response Timeout Value??? (Is 16bit!)
	// Default to 0, has to be set by ioctl probably... :-/
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0074);
	bcm430x_shm_write16(bcm, 0x0000);
	//XXX: MMIO: 0x0608 is the work_mode register?
	if (!(iw_mode & IW_MODE_ADHOC) && !(iw_mode & IW_MODE_MASTER))
		if ((bcm->chip_id == 0x4306) && (bcm->chip_rev == 3))
			bcm430x_write16(bcm, 0x0608, 0x0064);
		else
			bcm430x_write16(bcm, 0x0608, 0x0032);
	else
		bcm430x_write16(bcm, 0x0608, 0x0002);
	//FIXME: FuncPlaceholder (Shortslot Enabled);
	if (bcm->current_core->rev < 3) {
		bcm430x_write16(bcm, 0x060E, 0x0000);
		bcm430x_write16(bcm, 0x0610, 0x8000);
		bcm430x_write16(bcm, 0x0604, 0x0000);
		bcm430x_write16(bcm, 0x0606, 0x0200);
	} else {
		bcm430x_write32(bcm, 0x0188, 0x80000000);
		bcm430x_write32(bcm, 0x018C, 0x02000000);
	}
	bcm430x_write32(bcm, BCM430x_MMIO_GEN_IRQ_REASON, 0x00004000);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA1_IRQ_MASK, 0x0001DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA2_IRQ_MASK, 0x0000DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA3_IRQ_MASK, 0x0000DC00);
	bcm430x_write32(bcm, BCM430x_MMIO_DMA4_IRQ_MASK, 0x0001DC00);
	
	bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW,
	                bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW) | 0x00100000);
	
	bcm430x_write16(bcm, BCM430x_MMIO_POWERUP_DELAY, bcm430x_pctl_powerup_delay(bcm));

	assert(err == 0);
printk(KERN_INFO PFX "Chip initialized\n");
out:
	return err;

err_radio_off:
	bcm430x_radio_turn_off(bcm);
err_gpio_cleanup:
	bcm430x_gpio_cleanup(bcm);
err_free_irq:
	free_irq(bcm->pci_dev->irq, bcm);
	goto out;
}
	
/* Validate chip access
 * http://bcm-specs.sipsolutions.net/ValidateChipAccess */
static int bcm430x_validate_chip(struct bcm430x_private *bcm)
{
	int err = 0;
	u32 status;
	u32 shm_backup;
	u16 phy_version;

	status = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	status |= 0x400; /* FIXME: Unknown SBF flag */
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, status);

	/* get phy version */
	phy_version = bcm430x_read16(bcm, BCM430x_MMIO_PHY_VER);

	bcm->phy_version = (phy_version & 0xF000) >> 12;
	bcm->phy_type = (phy_version & 0x0F00) >> 8;
	bcm->phy_rev = (phy_version & 0xF);

	printk(KERN_INFO PFX "phy UnkVer: %x, Type %x, Revision %x\n",
			bcm->phy_version, bcm->phy_type, bcm->phy_rev);

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	shm_backup = bcm430x_shm_read32(bcm);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	bcm430x_shm_write32(bcm, 0xAA5555AA);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	if (bcm430x_shm_read32(bcm) != 0xAA5555AA) {
		err = -ENODEV;
		printk(KERN_ERR PFX "SHM mismatch (1) validating chip.\n");
		goto out;
	}

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	bcm430x_shm_write32(bcm, 0x55AAAA55);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	if (bcm430x_shm_read32(bcm) != 0x55AAAA55) {
		err = -ENODEV;
		printk(KERN_ERR PFX "SHM mismatch (2) validating chip.\n");
		goto out;
	}

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED);
	bcm430x_shm_write32(bcm, shm_backup);

	if (bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON) != 0)
		printk(KERN_ERR PFX "Bad interrupt reason code while validating chip.\n");

	if (bcm->phy_type > 2)
		printk(KERN_ERR PFX "Unknown PHY Type: %x\n", bcm->phy_type);

	assert(err == 0);
out:
	return err;
}

static int bcm430x_probe_cores(struct bcm430x_private *bcm)
{
	int err, i;
	int original_core;
	int current_core;
	int core_vendor, core_id, core_rev, core_enabled;
	u32 sb_id_hi, chip_id_32 = 0;
	u16 pci_device, chip_id_16;

	/* save current core */
	err = _get_current_core(bcm, &original_core);
	if (err)
		goto out;

	/* map core 0 */
	err = _switch_core(bcm, 0);
	if (err)
		goto out;

	/* fetch sb_id_hi from core information registers */
	sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

	core_id = (sb_id_hi & 0xFFF0) >> 4;
	core_rev = (sb_id_hi & 0xF);
	core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;


	/* if present, chipcommon is always core 0; read the chipid from it */
	if (core_id == BCM430x_COREID_CHIPCOMMON) {
		chip_id_32 = bcm430x_read32(bcm, 0);
		chip_id_16 = chip_id_32 & 0xFFFF;
		bcm->core_chipcommon.flags |= BCM430x_COREFLAG_AVAILABLE;
		bcm->core_chipcommon.id = core_id;
		bcm->core_chipcommon.rev = core_rev;
		bcm->core_chipcommon.index = 0;
	} else {
		/* without a chipCommon, use a hard coded table. */
		pci_device = bcm->pci_dev->device;
		if (pci_device == 0x4301)
			chip_id_16 = 0x4301;
		else if ((pci_device >= 0x4305) && (pci_device <= 0x4307))
			chip_id_16 = 0x4307;
		else if ((pci_device >= 0x4402) && (pci_device <= 0x4403))
			chip_id_16 = 0x4402;
		else if ((pci_device >= 0x4610) && (pci_device <= 0x4615))
			chip_id_16 = 0x4610;
		else if ((pci_device >= 0x4710) && (pci_device <= 0x4715))
			chip_id_16 = 0x4710;
		else {
			/* Presumably devices not listed above are not
			 * put into the pci device table for this driver,
			 * so we should never get here */
			chip_id_16 = 0x2BAD;
		}
	}

	/* ChipCommon with Core Rev >=4 encodes number of cores,
	 * otherwise consult hardcoded table */
	if ((core_id == BCM430x_COREID_CHIPCOMMON) && (core_rev >= 4))
		bcm->core_count = (chip_id_32 & 0x0F000000) >> 24;
	else {
		switch (chip_id_16) {
			case 0x4610:
			case 0x4704:
			case 0x4710:
				bcm->core_count = 9;
				break;
			case 0x4310:
				bcm->core_count = 8;
				break;
			case 0x5365:
				bcm->core_count = 7;
				break;
			case 0x4306:
				bcm->core_count = 6;
				break;
			case 0x4301:
			case 0x4307:
				bcm->core_count = 5;
				break;
			case 0x4402:
				bcm->core_count = 3;
				break;
			default:
				/* SOL if we get here */
				bcm->core_count = 1;
		}
	}

	bcm->chip_id = chip_id_16;
	bcm->chip_rev = (chip_id_32 & 0x000f0000) >> 16;
	
	printk(KERN_INFO PFX "Chip ID 0x%x, rev 0x%x\n",
		bcm->chip_id, bcm->chip_rev);

	for (current_core = 1; current_core < bcm->core_count; current_core++) {
		struct bcm430x_coreinfo *core;

		err = _switch_core(bcm, current_core);
		if (err)
			goto out;
		/* Gather information */
		/* fetch sb_id_hi from core information registers */
		sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

		/* extract core_id, core_rev, core_vendor */
		core_id = (sb_id_hi & 0xFFF0) >> 4;
		core_rev = (sb_id_hi & 0xF);
		core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;
		
		core_enabled = bcm430x_core_enabled(bcm);

		printk(KERN_INFO PFX "Core %d: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
		       current_core, core_id, core_rev, core_vendor,
		       core_enabled ? "enabled" : "disabled" );

		core = 0;
		switch (core_id) {
		case BCM430x_COREID_PCI:
			core = &bcm->core_pci;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCI cores found.\n");
				continue;
			}
			break;
		case BCM430x_COREID_V90:
			core = &bcm->core_v90;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple V90 cores found.\n");
				continue;
			}
			break;
		case BCM430x_COREID_PCMCIA:
			core = &bcm->core_pcmcia;
			if (core->flags & BCM430x_COREFLAG_AVAILABLE) {
				printk(KERN_WARNING PFX "Multiple PCMCIA cores found.\n");
				continue;
			}
			break;
		case BCM430x_COREID_80211:
			for (i = 0; i < BCM430x_MAX_80211_CORES; i++) {
				core = &(bcm->core_80211[i]);
				if (!(core->flags & BCM430x_COREFLAG_AVAILABLE))
					break;
				core = 0;
			}
			if (!core) {
				printk(KERN_WARNING PFX "More than %d cores of type 802.11 found.\n",
				       BCM430x_MAX_80211_CORES);
				continue;
			}
			break;
		case BCM430x_COREID_CHIPCOMMON:
			printk(KERN_WARNING PFX "Multiple CHIPCOMMON cores found.\n");
			break;
		default:
			printk(KERN_WARNING PFX "Unknown core found (ID 0x%x)\n", core_id);
		}
		if (core) {
			core->flags |= BCM430x_COREFLAG_AVAILABLE;
			core->id = core_id;
			core->rev = core_rev;
			core->index = current_core;
		}
	}
	/* Again, was there a (meaningful) original core mapping? */
#if 1
	/* restore original core mapping */
	err = _switch_core(bcm, original_core);
	if (err)
		goto out;
#endif

	assert(err == 0);
out:
	return err;
}

static void bcm430x_dma_free(struct bcm430x_private *bcm)
{
	bcm430x_destroy_dmaring(bcm->rx_ring1);
	bcm->rx_ring1 = 0;
	bcm430x_destroy_dmaring(bcm->rx_ring0);
	bcm->rx_ring0 = 0;
	bcm430x_destroy_dmaring(bcm->tx_ring3);
	bcm->tx_ring3 = 0;
	bcm430x_destroy_dmaring(bcm->tx_ring2);
	bcm->tx_ring2 = 0;
	bcm430x_destroy_dmaring(bcm->tx_ring1);
	bcm->tx_ring1 = 0;
	bcm430x_destroy_dmaring(bcm->tx_ring0);
	bcm->tx_ring0 = 0;
}

static int bcm430x_dma_init(struct bcm430x_private *bcm)
{
	int err = -ENOMEM;

	/* setup TX DMA channels. */
	bcm->tx_ring0 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA1_BASE,
					      BCM430x_TXRING_SLOTS, 1);
	if (!bcm->tx_ring0)
		goto out;
	bcm->tx_ring1 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA2_BASE,
					      BCM430x_TXRING_SLOTS, 1);
	if (!bcm->tx_ring1)
		goto err_destroy_tx0;
	bcm->tx_ring2 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA3_BASE,
					      BCM430x_TXRING_SLOTS, 1);
	if (!bcm->tx_ring2)
		goto err_destroy_tx1;
	bcm->tx_ring3 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA4_BASE,
					      BCM430x_TXRING_SLOTS, 1);
	if (!bcm->tx_ring3)
		goto err_destroy_tx2;

	/* setup RX DMA channels. */
	bcm->rx_ring0 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA1_BASE,
					      BCM430x_RXRING_SLOTS, 0);
	if (!bcm->rx_ring0)
		goto err_destroy_tx3;
	if (bcm->current_core->rev < 5) {
		bcm->rx_ring1 = bcm430x_setup_dmaring(bcm, BCM430x_MMIO_DMA4_BASE,
						      BCM430x_RXRING_SLOTS, 0);
		if (!bcm->rx_ring1)
			goto err_destroy_rx0;
	}

printk(KERN_INFO PFX "DMA initialized.\n");
	err = 0;
out:
	return err;

err_destroy_rx1:
	bcm430x_destroy_dmaring(bcm->rx_ring1);
	bcm->rx_ring1 = 0;
err_destroy_rx0:
	bcm430x_destroy_dmaring(bcm->rx_ring0);
	bcm->rx_ring0 = 0;
err_destroy_tx3:
	bcm430x_destroy_dmaring(bcm->tx_ring3);
	bcm->tx_ring3 = 0;
err_destroy_tx2:
	bcm430x_destroy_dmaring(bcm->tx_ring2);
	bcm->tx_ring2 = 0;
err_destroy_tx1:
	bcm430x_destroy_dmaring(bcm->tx_ring1);
	bcm->tx_ring1 = 0;
err_destroy_tx0:
	bcm430x_destroy_dmaring(bcm->tx_ring0);
	bcm->tx_ring0 = 0;
	goto out;
}

static void bcm430x_80211_cleanup(struct bcm430x_private *bcm)
{
	bcm430x_chip_cleanup(bcm);
}

/* http://bcm-specs.sipsolutions.net/80211Init */
static int bcm430x_80211_init(struct bcm430x_private *bcm)
{
	u32 ucodeflags;
	int err = 0;

	//FIXME: FuncPlaceholder (SB CORE Fixup)
	bcm430x_phy_calibrate(bcm);
	bcm430x_chip_init(bcm);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0016);

	bcm430x_shm_write16(bcm, bcm->current_core->rev);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
	ucodeflags = bcm430x_shm_read32(bcm);

#if 0
	if ( 1 == 0)
		ucodeflags |= 0x00000010; //FIXME: Unknown ucode flag.
#endif
	if (bcm->phy_type == BCM430x_PHYTYPE_G) {
		ucodeflags |= BCM430x_UCODEFLAG_UNKBGPHY;
		if (bcm->phy_rev == 1)
			ucodeflags |= BCM430x_UCODEFLAG_UNKGPHY;
		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			ucodeflags |= BCM430x_UCODEFLAG_UNKPACTRL;
	} else if (bcm->phy_type == BCM430x_PHYTYPE_G) {
		ucodeflags |= BCM430x_UCODEFLAG_UNKBGPHY;
		if ((bcm->phy_rev >= 2) && ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000))
				ucodeflags &= ~BCM430x_UCODEFLAG_UNKGPHY;
	}

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
	if (ucodeflags != bcm430x_shm_read32(bcm)) {
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
		bcm430x_shm_write32(bcm, ucodeflags);
	}
	
	/* XXX: Using defaults as of http://bcm-specs.sipsolutions.net/SHM: 0x0002 */
	bcm430x_shm_control(bcm, BCM430x_SHM_WIRELESS + 0x0006);
	bcm430x_shm_write32(bcm, 7);
	bcm430x_shm_write32(bcm, 4);

	/* FIXME:
	SHM: 0x44, 0x46
	 */

	bcm430x_shm_control(bcm, BCM430x_SHM_WIRELESS + 0x0003);
	bcm430x_shm_write32(bcm, ((bcm->phy_type == BCM430x_PHYTYPE_B) ? 0x001F : 0x000F));
	bcm430x_shm_write32(bcm, 0x03FF);

	//TODO: Write MAC to template ram
	//TODO: Write BSSID to template ram
	
	if (bcm->current_core->rev >= 5)
		//XXX: Is this really 16bit wide? (No specs)
		bcm430x_write16(bcm, 0x043C, 0x000C);

	if (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA) {
		err = bcm430x_dma_init(bcm);
		if (err)
			goto err_chip_cleanup;
	}

out:
	return err;

err_chip_cleanup:
	bcm430x_chip_cleanup(bcm);
	goto out;
}

/* This is the opposite of bcm430x_init_board() */
static void bcm430x_free_board(struct bcm430x_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;
	int i;

	bcm->status &= ~BCM430x_STAT_BOARDINITDONE;

	bcm430x_radio_turn_off(bcm);
	bcm430x_dma_free(bcm);
	bcm430x_80211_cleanup(bcm);

	bcm430x_pctl_set_crystal(bcm, 0);
	iounmap(bcm->mmio_addr);
	bcm->mmio_addr = 0;

	bcm->current_core = 0;
	memset(&bcm->core_chipcommon, 0, sizeof(struct bcm430x_coreinfo));
	memset(&bcm->core_pci, 0, sizeof(struct bcm430x_coreinfo));
	memset(&bcm->core_v90, 0, sizeof(struct bcm430x_coreinfo));
	memset(&bcm->core_pcmcia, 0, sizeof(struct bcm430x_coreinfo));
	for (i = 0; i < BCM430x_MAX_80211_CORES; i++)
		memset(&bcm->core_80211[i], 0, sizeof(struct bcm430x_coreinfo));

	pci_release_regions(pci_dev);
	pci_disable_device(pci_dev);
}

static int bcm430x_init_board(struct bcm430x_private *bcm)
{
	struct net_device *net_dev = bcm->net_dev;
	struct pci_dev *pci_dev = bcm->pci_dev;
	void *ioaddr;
	unsigned long mmio_start, mmio_end, mmio_flags, mmio_len;
	int i, err;
	u16 pci_status;
	int num_80211_cores;

	err = pci_enable_device(pci_dev);
	if (err) {
		printk(KERN_ERR PFX "unable to wake up pci device (%i)\n", err);
		err = -ENODEV;
		goto out;
	}

	mmio_start = pci_resource_start(pci_dev, 0);
	mmio_end = pci_resource_end(pci_dev, 0);
	mmio_flags = pci_resource_flags(pci_dev, 0);
	mmio_len = pci_resource_len(pci_dev, 0);

	/* make sure PCI base addr is MMIO */
	if (!(mmio_flags & IORESOURCE_MEM)) {
		printk(KERN_ERR PFX
		       "%s, region #0 not an MMIO resource, aborting\n",
		       pci_name(pci_dev));
		err = -ENODEV;
		goto err_pci_disable;
	}
	if (mmio_len != BCM430x_IO_SIZE) {
		printk(KERN_ERR PFX
		       "%s: invalid PCI mem region size(s), aborting\n",
		       pci_name(pci_dev));
		err = -ENODEV;
		goto err_pci_disable;
	}

	err = pci_request_regions(pci_dev, DRV_NAME);
	if (err) {
		printk(KERN_ERR PFX
		       "could not access PCI resources (%i)\n", err);
		goto err_pci_disable;
	}

	/* enable PCI bus-mastering */
	pci_set_master(pci_dev);

	/* ioremap MMIO region */
	ioaddr = ioremap(mmio_start, mmio_len);
	if (!ioaddr) {
		printk(KERN_ERR PFX "%s: cannot remap MMIO, aborting\n",
		       pci_name(pci_dev));
		err = -EIO;
		goto err_pci_release;
	}

	net_dev->base_addr = (long)ioaddr;
	bcm->mmio_addr = ioaddr;
	bcm->mmio_len = mmio_len;

	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_VENDOR_ID,
	                           &bcm->board_vendor);
	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_SUBSYSTEM_ID,
	                           &bcm->board_type);

	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_CHIPCOMMON_CAPABILITIES,
	                           &bcm->chipcommon_capabilities);

	bcm430x_pctl_set_crystal(bcm, 1);
	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_STATUS, &pci_status);
	bcm430x_pci_write_config_16(bcm->pci_dev, PCI_STATUS, pci_status & ~PCI_STATUS_SIG_TARGET_ABORT);
	bcm430x_pctl_init(bcm);
	bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_FAST);
	err = bcm430x_probe_cores(bcm);
	if (err)
		goto err_iounmap;
	bcm430x_read_sprom(bcm);

	num_80211_cores = bcm430x_num_80211_cores(bcm);
	for (i = 0; i < num_80211_cores; i++) {
		err = bcm430x_switch_core(bcm, &bcm->core_80211[i]);
		assert(err != -ENODEV);
		if (err)
			goto err_iounmap;

		/* Enable the selected wireless core.
		 * Connect PHY only on the first core.
		 */
		bcm430x_wireless_core_reset(bcm, (i == 0));

		if (i != 0) {
			//TODO: make this 80211 core inactive.
		}

		bcm430x_read_radio_id(bcm);
		err = bcm430x_validate_chip(bcm);
		if (err)
			goto err_iounmap;

		switch (bcm->phy_type) {
		case BCM430x_PHYTYPE_A:
			err = ((bcm->phy_rev >= 4) ? -ENODEV : 0);
			break;
		case BCM430x_PHYTYPE_B:
			//XXX: What about rev == 5 ?
			if ((bcm->phy_rev != 2) && (bcm->phy_rev != 4) && (bcm->phy_rev != 6))
				err = -ENODEV;
			break;
		case BCM430x_PHYTYPE_G:
			err = ((bcm->phy_rev >= 3) ? -ENODEV : 0);
			break;
		default:
			err = -ENODEV;
		};
		if (err)
			goto err_iounmap;

		err = bcm430x_80211_init(bcm);
		if (err)
			goto err_iounmap;

		if (num_80211_cores >= 2) {
			bcm430x_mac_suspend(bcm);
			//      turn irqs off
			//      turn radio off
		}
	}
	if (num_80211_cores >= 2) {
		bcm430x_switch_core(bcm, &bcm->core_80211[0]);
		bcm430x_mac_enable(bcm);
	}
printk(KERN_INFO PFX "80211 cores initialized\n");
	//TODO: Set up LEDs
	//TODO: Initialize PIO (really here?)

	bcm430x_interrupt_enable(bcm, BCM430x_IRQ_INITIAL);

	bcm->status |= BCM430x_STAT_BOARDINITDONE;
	assert(err == 0);
out:
	return err;

err_radio_off:
	bcm430x_radio_turn_off(bcm);
err_iounmap:
	iounmap(bcm->mmio_addr);
err_pci_release:
	pci_release_regions(pci_dev);
err_pci_disable:
	pci_disable_device(pci_dev);
	goto out;
}

/* Do the Hardware IO operations to send the txb */
static inline int bcm430x_tx(struct bcm430x_private *bcm,
			     struct ieee80211_txb *txb)
{
	int err = -ENODEV;

	bcm->txb = txb;
	if (bcm->data_xfer_mode == BCM430x_DATAXFER_DMA)
		err = bcm430x_dma_transfer_txb(bcm->tx_ring1, txb);
	else
		; /* TODO: PIO transfer */

	return err;
}

/* set_security() callback in struct ieee80211_device */
static void bcm430x_ieee80211_set_security(struct net_device *net_dev,
					   struct ieee80211_security *sec)
{/*TODO*/
}

/* hard_start_xmit() callback in struct ieee80211_device */
static int bcm430x_ieee80211_hard_start_xmit(struct ieee80211_txb *txb,
					     struct net_device *net_dev,
					     int pri)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err;
	unsigned long flags;

	spin_lock_irqsave(&bcm->lock, flags);
	err = bcm430x_tx(bcm, txb);
	spin_unlock_irqrestore(&bcm->lock, flags);

	return err;
}

/* reset_port() callback in struct ieee80211_device */
static int bcm430x_ieee80211_reset_port(struct net_device *net_dev)
{/*TODO*/
	return 0;
}

/* handle_management_frame() callback in struct ieee80211_device */
static int bcm430x_ieee80211_handle_management_frame(struct net_device *net_dev,
						     struct ieee80211_network *network,
						     u16 type)
{/*TODO*/
	return 0;
}

static struct net_device_stats * bcm430x_net_get_stats(struct net_device *net_dev)
{
	return &(bcm430x_priv(net_dev)->ieee->stats);
}

static void bcm430x_net_tx_timeout(struct net_device *dev)
{/*TODO*/
}

static int bcm430x_net_open(struct net_device *net_dev)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	int err = 0;

	if (!(bcm->status & BCM430x_STAT_BOARDINITDONE)) {
		err = bcm430x_init_board(bcm);
		if (err)
			goto out;
	}
	
	assert(err == 0);
out:
	return err;
}

static int bcm430x_net_stop(struct net_device *net_dev)
{
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);
	unsigned long flags;

	/* make sure we don't receive more data from the device. */
	spin_lock_irqsave(&bcm->lock, flags);
	bcm430x_interrupt_disable(bcm, BCM430x_IRQ_ALL);
	spin_unlock_irqrestore(&bcm->lock, flags);
	tasklet_disable(&bcm->isr_tasklet);

	bcm430x_free_board(bcm);

	return 0;
}

static int __devinit bcm430x_init_one(struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct net_device *net_dev;
	struct bcm430x_private *bcm;
	int err;

#ifdef DEBUG_SINGLE_DEVICE_ONLY
	if (strcmp(pci_name(pdev), DEBUG_SINGLE_DEVICE_ONLY))
		return -ENODEV;
#endif

	net_dev = alloc_ieee80211(sizeof(*bcm));
	if (!net_dev) {
		printk(KERN_ERR PFX
		       "could not allocate ieee80211 device %s\n",
		       pci_name(pdev));
		err = -ENOMEM;
		goto out;
	}
	/* initialize the net_device struct */
	SET_MODULE_OWNER(net_dev);
	SET_NETDEV_DEV(net_dev, &pdev->dev);

	net_dev->open = bcm430x_net_open;
	net_dev->stop = bcm430x_net_stop;
	net_dev->get_stats = bcm430x_net_get_stats;
	net_dev->tx_timeout = bcm430x_net_tx_timeout;
	net_dev->wireless_handlers = &bcm430x_wx_handlers_def;
	net_dev->irq = pdev->irq;

/*FIXME: We disable scatter/gather IO until we figure out
 *       how to turn hardware checksumming on.
 */
#if 0
	net_dev->features |= NETIF_F_HW_CSUM;	/* hardware packet checksumming */
	net_dev->features |= NETIF_F_SG;	/* Scatter/gather IO. */
#endif

	/* initialize the bcm430x_private struct */
	bcm = bcm430x_priv(net_dev);
	bcm->ieee = netdev_priv(net_dev);
	bcm->pci_dev = pdev;
	bcm->net_dev = net_dev;
	spin_lock_init(&bcm->lock);
	tasklet_init(&bcm->isr_tasklet,
		     (void (*)(unsigned long))bcm430x_interrupt_tasklet,
		     (unsigned long)bcm);
	bcm->workqueue = create_workqueue(DRV_NAME "_wq");
	if (!bcm->workqueue) {
		err = -ENOMEM;
		goto err_free_netdev;
	}
	bcm->curr_channel = 0xFFFF;
	bcm->antenna_diversity = 0xFFFF;
	if (pio)
		bcm->data_xfer_mode = BCM430x_DATAXFER_PIO;
	else
		bcm->data_xfer_mode = BCM430x_DATAXFER_DMA;
assert(bcm->data_xfer_mode == BCM430x_DATAXFER_DMA); /*TODO: Implement complete support for PIO mode. */

	switch (mode) {
	case 1:
		bcm->ieee->iw_mode = IW_MODE_ADHOC;
		break;
	case 2:
		bcm->ieee->iw_mode = IW_MODE_MONITOR;
		break;
	default:
	case 0:
		bcm->ieee->iw_mode = IW_MODE_INFRA;
		break;
	}

	bcm->ieee->set_security = bcm430x_ieee80211_set_security;
	bcm->ieee->hard_start_xmit = bcm430x_ieee80211_hard_start_xmit;
	bcm->ieee->reset_port = bcm430x_ieee80211_reset_port;
	bcm->ieee->handle_management_frame = bcm430x_ieee80211_handle_management_frame;

	pci_set_drvdata(pdev, net_dev);

	err = register_netdev(net_dev);
	if (err) {
		printk(KERN_ERR PFX "Cannot register net device, "
		       "aborting.\n");
		err = -ENOMEM;
		goto err_destroy_wq;
	}

	bcm430x_debugfs_add_device(bcm);

	assert(err == 0);
out:
	return err;

err_destroy_wq:
	destroy_workqueue(bcm->workqueue);
err_free_netdev:
	free_netdev(net_dev);
	goto out;
}

static void __devexit bcm430x_remove_one(struct pci_dev *pdev)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	bcm430x_debugfs_remove_device(bcm);
	unregister_netdev(net_dev);
	destroy_workqueue(bcm->workqueue);
	free_netdev(net_dev);
}

#ifdef CONFIG_PM

static int bcm430x_suspend(struct pci_dev *pdev, pm_message_t state)
{/*TODO*/
	return 0;
}

static int bcm430x_resume(struct pci_dev *pdev)
{/*TODO*/
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
	printk(KERN_INFO BCM430x_DRIVER_NAME "\n");
	bcm430x_debugfs_init();
	return pci_module_init(&bcm430x_pci_driver);
}

static void __exit bcm430x_exit(void)
{
	pci_unregister_driver(&bcm430x_pci_driver);
	bcm430x_debugfs_exit();
}

module_param(pio, int, 0444);
MODULE_PARM_DESC(pio, "enable(1) / disable(0) PIO mode");

module_param(mode, int, 0444);
MODULE_PARM_DESC(mode, "network mode (0=BSS,1=IBSS,2=Monitor)");

module_init(bcm430x_init)
module_exit(bcm430x_exit)

/* vim: set ts=8 sw=8 sts=8: */
