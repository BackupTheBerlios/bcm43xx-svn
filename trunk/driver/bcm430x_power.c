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

#include "bcm430x.h"
#include "bcm430x_power.h"
#include "bcm430x_main.h"


/* Get max/min slowclock frequency
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
static int bcm430x_pctl_clockfreqlimit(struct bcm430x_private *bcm,
				       int get_max)
{
	int limit = 0;
	int divisor;
	int selection;
	int err;
	u32 tmp;
	struct bcm430x_coreinfo *old_core;

	if (!(bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTL))
		goto out;
	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err)
		goto out;

	if (bcm->current_core->rev < 6) {
		if ((bcm->bustype == BCM430x_BUSTYPE_PCMCIA) ||
			(bcm->bustype == BCM430x_BUSTYPE_SB)) {
			selection = 1;
			divisor = 32;
		} else {
			err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &tmp);
			if (err) {
				printk(KERN_ERR PFX "clockfreqlimit pcicfg read failure\n");
				goto out_switchback;
			}
			if (tmp & 0x10) {
				/* PCI */
				selection = 2;
				divisor = 64;
			} else {
				/* XTAL */
				selection = 1;
				divisor = 32;
			}
		}
	} else if (bcm->current_core->rev < 10) {
		selection = (tmp & 0x07);
		if (selection) {
			tmp = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL);
			divisor = 4 * (1 + ((tmp & 0xFFFF0000) >> 16));
		} else
			divisor = 1;
	} else {
		tmp = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_SYSCLKCTL);
		divisor = 4 * (1 + ((tmp & 0xFFFF0000) >> 16));
		selection = 1;
	}
	
	switch (selection) {
	case 0:
		/* LPO */
		if (get_max)
			limit = 43000;
		else
			limit = 25000;
		break;
	case 1:
		/* XTAL */
		if (get_max)
			limit = 20200000;
		else
			limit = 19800000;
		break;
	case 2:
		/* PCI */
		if (get_max)
			limit = 34000000;
		else
			limit = 25000000;
		break;
	default:
		assert(0);
	}
	limit /= divisor;

out_switchback:
	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return limit;
}

/* init power control
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
int bcm430x_pctl_init(struct bcm430x_private *bcm)
{
	int err, maxfreq;
	struct bcm430x_coreinfo *old_core;

	if (!(bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTL))
		return 0;
	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV)
		return 0;
	if (err)
		goto out;

	maxfreq = bcm430x_pctl_clockfreqlimit(bcm, 1);
	bcm430x_write32(bcm, BCM430x_CHIPCOMMON_PLLONDELAY,
			(maxfreq * 150 + 999999) / 1000000);
	bcm430x_write32(bcm, BCM430x_CHIPCOMMON_FREFSELDELAY,
			(maxfreq * 15 + 999999) / 1000000);

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return err;
}

u16 bcm430x_pctl_powerup_delay(struct bcm430x_private *bcm)
{
	u16 delay = 0;
	int err;
	u32 pll_on_delay;
	struct bcm430x_coreinfo *old_core;
	int minfreq;

	if (bcm->bustype != BCM430x_BUSTYPE_PCI)
		goto out;
	if (!(bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTL))
		goto out;
	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV)
		goto out;

	minfreq = bcm430x_pctl_clockfreqlimit(bcm, 0);
	pll_on_delay = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_PLLONDELAY);
	delay = (((pll_on_delay + 2) * 1000000) + (minfreq - 1)) / minfreq;

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return delay;
}

/* set the powercontrol clock
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
int bcm430x_pctl_set_clock(struct bcm430x_private *bcm, u16 mode)
{
	int err;
	struct bcm430x_coreinfo *old_core;
	u32 tmp;

	if (!(bcm->core_chipcommon.flags & BCM430x_COREFLAG_AVAILABLE))
		/* No ChipCommon available. */
		return 0;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err)
		goto out;
	
	if (bcm->core_chipcommon.rev < 6) {
		if (mode == BCM430x_PCTL_CLK_FAST) {
			err = bcm430x_pctl_set_crystal(bcm, 1);
			if (err)
				goto out;
		}
	} else {
		if ((bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTL) &&
			(bcm->core_chipcommon.rev < 10)) {
			switch (mode) {
			case BCM430x_PCTL_CLK_FAST:
				tmp = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL);
				tmp = (tmp & ~BCM430x_PCTL_FORCE_SLOW) | BCM430x_PCTL_FORCE_PLL;
				bcm430x_write32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL, tmp);
				break;
			case BCM430x_PCTL_CLK_SLOW:
				tmp = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL);
				tmp |= BCM430x_PCTL_FORCE_SLOW;
				bcm430x_write32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL, tmp);
				break;
			case BCM430x_PCTL_CLK_DYNAMIC:
				tmp = bcm430x_read32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL);
				tmp &= ~BCM430x_PCTL_FORCE_SLOW;
				tmp |= BCM430x_PCTL_FORCE_PLL;
				tmp &= ~BCM430x_PCTL_DYN_XTAL;
				bcm430x_write32(bcm, BCM430x_CHIPCOMMON_SLOWCLKCTL, tmp);
			}
		}
	}
	
	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return err;
}

int bcm430x_pctl_set_crystal(struct bcm430x_private *bcm, int on)
{
	int err;
	u32 in, out, outenable;

	err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_IN, &in);
	if (err)
		goto err_pci;
	err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
	if (err)
		goto err_pci;
	err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &outenable);
	if (err)
		goto err_pci;

	outenable |= (BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN);

	if (on) {
		if (in & 0x40)
			return 0;

		out |= (BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN);

		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);
		if (err)
			goto err_pci;
		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, outenable);
		if (err)
			goto err_pci;
		udelay(1000);

		out &= ~BCM430x_PCTL_PLL_POWERDOWN;
		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);
		if (err)
			goto err_pci;
		udelay(5000);
	} else {
		if ((bcm->current_core->rev > 4) &&
			!(bcm430x_read32(bcm, BCM430x_MMIO_RADIO_HWENABLED_HI) & (1 << 16)) &&
			!(bcm->sprom.boardflags & BCM430x_BFL_XTAL_NOSLOW)) {
			err = bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_SLOW);
			if (err)
				goto out;
		}
		out &= ~BCM430x_PCTL_XTAL_POWERUP;
		out |= BCM430x_PCTL_PLL_POWERDOWN;
		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, out);
		if (err)
			goto err_pci;
		err = bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, outenable);
		if (err)
			goto err_pci;
	}

out:
	return err;

err_pci:
	printk(KERN_ERR PFX "Error: pctl_set_clock() could not access PCI config space!\n");
	err = -EBUSY;
	goto out;
}

/* Set the PowerSavingControlBits.
 * Bitvalues:
 *   0  => unset the bit
 *   1  => set the bit
 *   -1 => calculate the bit
 */
void bcm430x_power_saving_ctl_bits(struct bcm430x_private *bcm,
				   int bit25, int bit26)
{
	int i;
	u32 status;

//FIXME: Force 25 to off and 26 to on for now:
bit25 = 0;
bit26 = 1;

	if (bit25 == -1) {
		//TODO: If powersave is not off and FIXME is not set and we are not in adhoc
		//	and thus is not an AP and we are associated, set bit 25
	}
	if (bit26 == -1) {
		//TODO: If the device is awake or this is an AP, or FIXME, or FIXME,
		//	or we are associated, or FIXME, or FIXME, set bit26
	}
	status = bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD);
	if (bit25)
		status |= BCM430x_SBF_PS1;
	else
		status &= ~BCM430x_SBF_PS1;
	if (bit26)
		status |= BCM430x_SBF_PS2;
	else
		status &= ~BCM430x_SBF_PS2;
	bcm430x_write32(bcm, BCM430x_MMIO_STATUS_BITFIELD, status);
	if (bit26 && bcm->current_core->rev >= 5) {
		for (i = 0; i < 100; i++) {
			if (bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0040) != 4)
				break;
			udelay(10);
		}
	}
}
