/*

  Broadcom BCM430x wireless driver

  Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
                     Stefano Brivio <st3@riseup.net>
                     Michael Buesch <mbuesch@freenet.de>

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

#include "bcm430x_main.h"
#include "bcm430x_ucode.h"
#include "bcm430x_initvals.h"
#include "bcm430x_debugfs.h"

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
	{ PCI_VENDOR_ID_BROADCOM, 0x4318, 0x1799, 0x7000, 0, 0, 0 }, /* Belkin F5D7000 PCI Card */

	/* Broadcom 4306 802.11b/g */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0001, 0, 0, 0 }, /* Dell TrueMobile 1300 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1028, 0x0003, 0, 0, 0 }, /* Dell TrueMobile 1350 Mini-PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1043, 0x100f, 0, 0, 0 }, /* Asus WL-100G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1057, 0x7025, 0, 0, 0 }, /* Motorola WN825G PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x106b, 0x004e, 0, 0, 0 }, /* Apple AirPort Extreme Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x14e4, 0x0013, 0, 0, 0 }, /* Linksys WMP54G PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1737, 0x0015, 0, 0, 0 }, /* Linksys WMP54GS PC Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7001, 0, 0, 0 }, /* Belkin F5D7001 PCI Card */
	{ PCI_VENDOR_ID_BROADCOM, 0x4320, 0x1799, 0x7010, 0, 0, 0 }, /* Belkin F5D7010 PC Card */

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


static u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset)
{
	u16 val;

	val = ioread16(bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "read 16  0x%04x  0x%04x\n", offset, val);
	return val;
}

static void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	iowrite16(val, bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "write 16  0x%04x  0x%04x\n", offset, val);
}

static u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset)
{
	u32 val;

	val = ioread32(bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "read 32  0x%04x  0x%08x\n", offset, val);
	return val;
}

static void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	iowrite32(val, bcm->mmio_addr + offset);
//	dprintk(KERN_INFO PFX "write 32  0x%04x  0x%08x\n", offset, val);
}

static u16 bcm430x_phy_read(struct bcm430x_private *bcm, u16 offset)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_PHY_DATA);
}

static void bcm430x_phy_write(struct bcm430x_private *bcm, int offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_DATA, val);
}

static void bcm430x_shm_control(struct bcm430x_private *bcm, u32 control)
{
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_CONTROL, control);
}

static u32 bcm430x_shm_read32(struct bcm430x_private *bcm)
{
	return bcm430x_read32(bcm, BCM430x_MMIO_SHM_DATA);
}

static void bcm430x_shm_write32(struct bcm430x_private *bcm, u32 val)
{
	bcm430x_write32(bcm, BCM430x_MMIO_SHM_DATA, val);
}

static int bcm430x_pci_read_config_8(struct pci_dev *pdev, u16 offset, u8 * val)
{
	int err;

	err = pci_read_config_byte(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 8  0x%04x  0x%02x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset,
				      u16 * val)
{
	int err;

	err = pci_read_config_word(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 16  0x%04x  0x%04x\n", offset, *val);
	return err;
}

static int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset,
				      u32 * val)
{
	int err;

	err = pci_read_config_dword(pdev, offset, val);
//	dprintk(KERN_INFO PFX "pci read 32  0x%04x  0x%08x\n", offset, *val);
	return err;
}

static int bcm430x_pci_write_config_8(struct pci_dev *pdev, int offset, u8 val)
{
//	dprintk(KERN_INFO PFX "pci write 8  0x%04x  0x%02x\n", offset, val);
	return pci_write_config_byte(pdev, offset, val);
}

static int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset,
				       u16 val)
{
//	dprintk(KERN_INFO PFX "pci write 16  0x%04x  0x%04x\n", offset, val);
	return pci_write_config_word(pdev, offset, val);
}

static int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset,
				       u32 val)
{
//	dprintk(KERN_INFO PFX "pci write 32  0x%04x  0x%08x\n", offset, val);
	return pci_write_config_dword(pdev, offset, val);
}

/* Read SPROM and fill the useful values in the net_device struct */
static void bcm430x_read_sprom(struct bcm430x_private *bcm)
{
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
}

static void bcm430x_clr_target_abort(struct bcm430x_private *bcm)
{
	u16 pci_status;

	bcm430x_pci_read_config_16(bcm->pci_dev, PCI_STATUS, &pci_status);
	pci_status &= ~ PCI_STATUS_SIG_TARGET_ABORT;
	bcm430x_pci_write_config_16(bcm->pci_dev, PCI_STATUS, pci_status);
}

static void bcm430x_pctl_set_crystal(struct bcm430x_private *bcm, int on)
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
}

/* Puts the index of the current core into user supplied core variable */
static int _get_current_core(struct bcm430x_private *bcm, int *core)
{
	int err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_REG_ACTIVE_CORE, core);
	*core = (*core - 0x18000000) / 0x1000;
	return err;
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

static int bcm430x_switch_core(struct bcm430x_private *bcm, struct bcm430x_coreinfo *new_core)
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

static irqreturn_t bcm430x_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{/*TODO*/
	struct bcm430x_private *bcm = dev_id;
	u32 reason;

	assert(bcm);
	reason = bcm430x_read32(bcm, BCM430x_MMIO_GEN_IRQ_REASON);
	if (reason == 0xffffffff)
		return IRQ_NONE; // irq not for us (shared irq)

printk(KERN_INFO PFX "We got an interrupt! Reason: 0x%08x\n", reason);

	return IRQ_HANDLED;
}

static void bcm430x_write_microcode(struct bcm430x_private *bcm,
				    const u32 *data, const unsigned int len)
{
	unsigned int i;

	bcm430x_shm_control(bcm, BCM430x_SHM_UCODE + 0x0000);
	for (i = 0; i < len; i++) {
		bcm430x_shm_write32(bcm, data[i]);
		udelay(10);
	}
}

static void bcm430x_write_pcm(struct bcm430x_private *bcm,
			      const u32 *data, const unsigned int len)
{
	unsigned int i;

	bcm430x_shm_control(bcm, BCM430x_SHM_PCM + 0x01ea);
	bcm430x_shm_write32(bcm, 0x00004000);
	bcm430x_shm_control(bcm, BCM430x_SHM_PCM + 0x01eb);
	for (i = 0; i < len; i++) {
		bcm430x_shm_write32(bcm, data[i]);
		udelay(10);
	}
}

static int bcm430x_upload_microcode(struct bcm430x_private *bcm)
{
	if (bcm->core_80211.rev == 2) {
		bcm430x_write_microcode(bcm, bcm430x_ucode2_data,
					bcm430x_ucode2_size);
	} else if (bcm->core_80211.rev == 4) {
		bcm430x_write_microcode(bcm, bcm430x_ucode4_data,
					bcm430x_ucode4_size);
	} else if (bcm->core_80211.rev >= 5) {
		bcm430x_write_microcode(bcm, bcm430x_ucode5_data,
					bcm430x_ucode5_size);
	} else {
		printk(KERN_ERR PFX "Error: No microcode available.\n");
		return -ENODEV;
	}

	if (bcm->core_80211.rev < 5) {
		bcm430x_write_pcm(bcm, bcm430x_pcm4_data,
				  bcm430x_pcm4_size);
	} else {
		bcm430x_write_pcm(bcm, bcm430x_pcm5_data,
				  bcm430x_pcm5_size);
	}

	return 0;
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
	/* TODO? */
	return 0;
}

/* Switch to the core used to write the GPIO register.
 * This is either the ChipCommon, or the PCI core.
 */
static int switch_to_gpio_core(struct bcm430x_private *bcm)
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
	u32 value = 0x00000000;

	old_core = bcm->current_core;
	err = switch_to_gpio_core(bcm);
	if (err)
		return err;

	if (bcm->core_80211.rev <= 2)
		value |= 0x10;
	/*FIXME: Need to set up some LED flags here? */
	if (bcm->chip_id == 0x4301)
		value |= 0x60;
	if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
		value |= 0x200;

	bcm430x_write32(bcm, BCM430x_GPIO_CONTROL, value);

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

/* Initialize the chip
 * http://bcm-specs.sipsolutions.net/ChipInit
 */
static int bcm430x_chip_init(struct bcm430x_private *bcm)
{
	int err;

	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, 0x00000404);
	err = bcm430x_upload_microcode(bcm);
	if (err)
		goto out;
	err = bcm430x_initialize_irq(bcm);
	if (err)
		goto out;
	/*TODO*/
	err = bcm430x_gpio_init(bcm);
	if (err)
		goto out;
	/*TODO*/
	assert(err == 0);
printk(KERN_INFO PFX "Chip initialized\n");
out:
	return err;
}

static void write_initvals_array(struct bcm430x_private *bcm,
				 const struct bcm430x_initval *data)
{
	unsigned int i = 0;

	while (data[i].size != 0) {
		if (data[i].size == 4) {
			bcm430x_write32(bcm, data[i].offset, data[i].value);
		} else {
			assert(data[i].size == 2);
			bcm430x_write16(bcm, data[i].offset, (u16)(data[i].value));
		}
		i++;
	}
}

/* Write the initial values
 * http://bcm-specs.sipsolutions.net/InitialValues
 */
static int bcm430x_write_initvals(struct bcm430x_private *bcm)
{
	if (bcm->core_80211.rev == 2 || bcm->core_80211.rev == 4) {
		switch (bcm->phy_type) {
		case BCM430x_PHYTYPE_A:
			write_initvals_array(bcm, bcm430x_initvals_core24_aphy);
			break;
		case BCM430x_PHYTYPE_B:
		case BCM430x_PHYTYPE_G:
			write_initvals_array(bcm, bcm430x_initvals_core24_bgphy);
			break;
		default:
			goto out_noinitval;
		}
	} else if (bcm->core_80211.rev >= 5) {
		switch (bcm->phy_type) {
		case BCM430x_PHYTYPE_A:
			write_initvals_array(bcm, bcm430x_initvals_core5_aphy);
			/* FIXME: The expression in the following if statement is pseudo code,
			 *        as I don't know what SB_CoreFlagsHI is.
			 *        See http://bcm-specs.sipsolutions.net/APHYBSInitVal5
			 */
#if 0
			if ( SB_CoreFlagsHI & 0x10000 )
				write_initvals_array(bcm, bcm430x_bsinitvals_core5_aphy_2);
			else
				write_initvals_array(bcm, bcm430x_bsinitvals_core5_aphy_1);
#endif
			break;
		case BCM430x_PHYTYPE_B:
		case BCM430x_PHYTYPE_G:
			write_initvals_array(bcm, bcm430x_initvals_core5_bgphy);
			write_initvals_array(bcm, bcm430x_bsinitvals_core5_bgphy);
			break;
		default:
			goto out_noinitval;
		}
	} else
		goto out_noinitval;

printk(KERN_INFO PFX "InitVals written\n");
	return 0;

out_noinitval:
	printk(KERN_ERR PFX "Error: No InitVals available!\n");
	return -ENODEV;
}

/* Validate chip access
 * http://bcm-specs.sipsolutions.net/ValidateChipAccess */
static int bcm430x_validate_chip(struct bcm430x_private *bcm)
{
	int err;
	u32 status;
	u32 shm_backup;
	u16 phy_version;

	/* some magic from http://bcm-specs.sipsolutions.net/DeviceInitialization */

	/* select and enable 80211 core */
	err = bcm430x_switch_core(bcm, &bcm->core_80211);
	if (err)
		goto out;
	err = bcm430x_core_enable(bcm, 0x20040000);
	if (err)
		goto out;

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

	if (bcm430x_read32(bcm, 0x128) != 0)
		printk(KERN_ERR PFX "Bad interrupt reason code (?) validating chip.\n");

	if (bcm->phy_type > 2)
		printk(KERN_ERR PFX "Unknown PHY Type: %x\n", bcm->phy_type);

	assert(err == 0);
out:
	return err;
}

static int bcm430x_probe_cores(struct bcm430x_private *bcm)
{
	int err;
	int original_core, current_core, core_count;
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

	printk(KERN_INFO PFX "Core 0: ID 0x%x, rev 0x%x, vendor 0x%x\n", core_id,
	       core_rev, core_vendor);

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
	if (core_id == BCM430x_COREID_CHIPCOMMON && core_rev >= 4)
		core_count = (chip_id_32 & 0x0F000000) >> 24;
	else {
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

	bcm->chip_id = chip_id_16;
	bcm->chip_rev = (chip_id_32 & 0x000f0000) >> 16;

	for (current_core = 1; current_core < core_count; current_core++) {
		err = _switch_core(bcm, current_core);
		if (err)
			goto out;

		/* fetch sb_id_hi from core information registers */
		sb_id_hi = bcm430x_read32(bcm, BCM430x_CIR_SB_ID_HI);

		/* extract core_id, core_rev, core_vendor */
		core_id = (sb_id_hi & 0xFFF0) >> 4;
		core_rev = (sb_id_hi & 0xF);
		core_vendor = (sb_id_hi & 0xFFFF0000) >> 16;
		
		core_enabled = bcm430x_core_enabled(bcm);

		switch (core_id) {
		case BCM430x_COREID_PCI:
			bcm->core_pci.flags |= BCM430x_COREFLAG_AVAILABLE;
			bcm->core_pci.id = core_id;
			bcm->core_pci.rev = core_rev;
			bcm->core_pci.index = current_core;
			break;
		case BCM430x_COREID_V90:
			bcm->core_v90.flags |= BCM430x_COREFLAG_AVAILABLE;
			bcm->core_v90.id = core_id;
			bcm->core_v90.rev = core_rev;
			bcm->core_v90.index = current_core;
			break;
		case BCM430x_COREID_PCMCIA:
			bcm->core_pcmcia.flags |= BCM430x_COREFLAG_AVAILABLE;
			bcm->core_pcmcia.id = core_id;
			bcm->core_pcmcia.rev = core_rev;
			bcm->core_pcmcia.index = current_core;
			break;
		case BCM430x_COREID_80211:
			bcm->core_80211.flags |= BCM430x_COREFLAG_AVAILABLE;
			bcm->core_80211.id = core_id;
			bcm->core_80211.rev = core_rev;
			bcm->core_80211.index = current_core;
			break;
		case BCM430x_COREID_CHIPCOMMON:
			printk(KERN_WARNING PFX "Strange, another chipcommon core found.\n");
			break;
		default:
			printk(KERN_WARNING PFX "Unknown core found (ID 0x%x)\n", core_id);
		}

		printk(KERN_INFO PFX "Core %d: ID 0x%x, rev 0x%x, vendor 0x%x, %s\n",
		       current_core, core_id, core_rev, core_vendor,
		       core_enabled ? "enabled" : "disabled" );
	}

	/* restore original core mapping */
	err = _switch_core(bcm, original_core);
	if (err)
		goto out;

	assert(err == 0);
out:
	return err;
}

static struct net_device_stats * bcm430x_net_get_stats(struct net_device *dev)
{
	return &(bcm430x_priv(dev)->ieee->stats);
}

static void bcm430x_net_tx_timeout(struct net_device *dev)
{/*TODO*/
}

static int bcm430x_net_open(struct net_device *dev)
{/*TODO*/
	return 0;
}

static int bcm430x_net_stop(struct net_device *dev)
{/*TODO*/
	return 0;
}

static int bcm430x_init_board(struct pci_dev *pdev, struct bcm430x_private **bcm_out)
{
	void *ioaddr;
	struct net_device *net_dev;
	struct bcm430x_private *bcm;
	unsigned long mmio_start, mmio_end, mmio_flags, mmio_len;
	int err;

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
	net_dev->irq = pdev->irq;

	/* initialize the bcm430x_private struct */
	bcm = bcm430x_priv(net_dev);
	bcm->ieee = netdev_priv(net_dev);
	bcm->pci_dev = pdev;
	bcm->net_dev = net_dev;
	spin_lock_init(&bcm->lock);

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

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR PFX "unable to wake up pci device (%i)\n", err);
		goto err_free_ieee;
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
		goto err_free_ieee;
	}
	if (mmio_len != BCM430x_IO_SIZE) {
		printk(KERN_ERR PFX
		       "%s: invalid PCI mem region size(s), aborting\n",
		       pci_name(pdev));
		err = -ENODEV;
		goto err_free_ieee;
	}

	err = pci_request_regions(pdev, DRV_NAME);
	if (err) {
		printk(KERN_ERR PFX
		       "could not access PCI resources (%i)\n", err);
		goto err_free_ieee;
	}

	/* enable PCI bus-mastering */
	pci_set_master(pdev);

	/* ioremap MMIO region */
	ioaddr = ioremap(mmio_start, mmio_len);
	if (!ioaddr) {
		printk(KERN_ERR PFX "%s: cannot remap MMIO, aborting\n",
		       pci_name(pdev));
		err = -EIO;
		goto err_pci_release;
	}

	net_dev->base_addr = (long)ioaddr;
	bcm->mmio_addr = ioaddr;
	bcm->mmio_len = mmio_len;

	bcm430x_pctl_set_crystal(bcm, 1);
	bcm430x_clr_target_abort(bcm);
	err = bcm430x_probe_cores(bcm);
	if (err)
		goto err_pci_release;
	err = bcm430x_validate_chip(bcm);
	if (err)
		goto err_pci_release;
	err = bcm430x_chip_init(bcm);
	if (err)
		goto err_pci_release;
	err = bcm430x_write_initvals(bcm);
	if (err)
		goto err_pci_release; /* FIXME: Maybe we need to de-init the chip? Also in rmmod code... */

	*bcm_out = bcm;
	assert(err == 0);
out:
	return err;

err_pci_release:
	pci_release_regions(pdev);
err_free_ieee:
	free_netdev(net_dev);
	goto out;
}

/* This is the opposite of bcm430x_chip_init() */
static void bcm430x_chip_cleanup(struct bcm430x_private *bcm)
{
	bcm430x_gpio_cleanup(bcm);
}

/* This is the opposite of bcm430x_init_board() */
static void bcm430x_free_board(struct bcm430x_private *bcm)
{
	struct pci_dev *pci_dev = bcm->pci_dev;

	bcm430x_chip_cleanup(bcm);

	bcm430x_pctl_set_crystal(bcm, 0);
	iounmap(bcm->mmio_addr);
	pci_release_regions(pci_dev);
	free_irq(pci_dev->irq, bcm);
}

static int __devinit bcm430x_init_one(struct pci_dev *pdev,
				      const struct pci_device_id *ent)
{
	struct net_device *net_dev = NULL;
	struct bcm430x_private *bcm;
	int err;
	u8 pci_rev;

#ifdef DEBUG_SINGLE_DEVICE_ONLY
	if (strcmp(pci_name(pdev), DEBUG_SINGLE_DEVICE_ONLY))
		return -ENODEV;
#endif
	/* TODO: Almost everything here (except net_device registration)
	 *       should be done at bcm430x_net_open() time.
	 *       For now we do it here for easier development.
	 */

	bcm430x_pci_read_config_8(pdev, PCI_REVISION_ID, &pci_rev);

	err = bcm430x_init_board(pdev, &bcm);
	if (err)
		goto out;
	net_dev = bcm->net_dev;
	err = register_netdev(net_dev);
	if (err) {
		printk(KERN_ERR PFX "Cannot register net device, "
		       "aborting.\n");
		goto err_freeboard;
	}

	bcm430x_read_sprom(bcm);
	pci_set_drvdata(pdev, net_dev);

	bcm430x_debugfs_add_device(bcm);

	assert(err == 0);
out:
	return err;

err_freeboard:
	bcm430x_free_board(bcm);
	goto out;
}

static void __devexit bcm430x_remove_one(struct pci_dev *pdev)
{
	struct net_device *net_dev = pci_get_drvdata(pdev);
	struct bcm430x_private *bcm = bcm430x_priv(net_dev);

	bcm430x_debugfs_remove_device(bcm);

	bcm430x_free_board(bcm);
	if (net_dev) {
		unregister_netdev(net_dev);
		free_netdev(net_dev);
	}
	pci_set_drvdata(pdev, NULL);
	pci_disable_device(pdev);
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

module_param(mode, int, 0444);
MODULE_PARM_DESC(mode, "network mode (0=BSS,1=IBSS,2=Monitor)");

module_init(bcm430x_init)
module_exit(bcm430x_exit)

/* vim: set ts=8 sw=8 sts=8: */
