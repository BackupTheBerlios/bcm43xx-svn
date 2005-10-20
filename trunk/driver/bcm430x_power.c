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

/* get min slowclock frequency
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
static int bcm430x_pctl_get_minslowclk(struct bcm430x_private *bcm)
{
	int minslowclk = 0;
	int err;
	u32 out, slow_clk_ctl;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err)
		goto out;

	if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
		if (bcm->current_core->rev < 6) {
			bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
			if (out & 0x10) {
				/* PCI */
				minslowclk = 25000000 / 64;
			} else {
				/* XTAL */
				minslowclk = 19800000 / 32;
			}
		} else {
			bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &slow_clk_ctl);
			switch (slow_clk_ctl & 0x7) {
			case 0:
			case 1:
				/* LPO */
				minslowclk = 25000;
				break;
			case 2:
				/* XTAL */
				minslowclk = 19800000 / (4 * (1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
				break;
			case 3:
			default:
				/* PCI */
				minslowclk = 25000000 / (4 * (1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
				break;
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return minslowclk;
}

/* get max slowclock frequency
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
static int bcm430x_pctl_get_maxslowclk(struct bcm430x_private *bcm)
{
	int maxslowclk = 0;
	int err;
	u32 out;
	u32 slow_clk_ctl = 0;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err)
		goto out;

	if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
		if (bcm->current_core->rev < 6) {
			bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
			if (out & 0x10) {
				/* PCI */
				maxslowclk = 34000000 / 64;
			} else {
				/* XTAL */
				maxslowclk = 20200000 / 32;
			}
		} else {
			bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &slow_clk_ctl);
			switch (slow_clk_ctl & 0x7) {
			case 0:
			case 1:
				/* LPO */
				maxslowclk = 43000;
				break;
			case 2:
				/* XTAL */
				maxslowclk = 20200000 / (4 * (1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
				break;
			case 3:
			default:
				/* PCI */
				maxslowclk = 34000000 / (4 * (1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
				break;
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return maxslowclk;
}

/* init power control
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
int bcm430x_pctl_init(struct bcm430x_private *bcm)
{
	int err, maxfreq;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV)
		return 0;
	if (err)
		goto out;

	if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
		maxfreq = bcm430x_pctl_get_maxslowclk(bcm);
		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_IN,
					    (maxfreq * 250 + 999999) / 1000000);
		bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT,
					    (maxfreq * 250 + 999999) / 1000000);
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return err;
}

u16 bcm430x_pctl_powerup_delay(struct bcm430x_private *bcm)
{
	u16 delay = 0;
	int err;
	u32 pll_on_delay = 0;
	struct bcm430x_coreinfo *old_core;
	int minfreq;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV)
		goto out;

	if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
		minfreq = bcm430x_pctl_get_minslowclk(bcm);
		err = bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_IN, &pll_on_delay);
		if (err)
			printk(KERN_WARNING PFX "Could not read pll_on_delay\n");
		delay = (((pll_on_delay + 2) * 1000000) + (minfreq - 1)) / minfreq;
	}

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
	u16 oldmode;
	struct bcm430x_coreinfo *old_core;

	if (mode == BCM430x_PCTL_CLK_SLOW &&
	    !(bcm->sprom.boardflags & BCM430x_BFL_XTAL)) {
		/* Slow clock not supported by chip. */
		return 0;
	}

	/* FIXME: Current driver doesn't support anything but PCI, so
	 * we don't need to 'Ensure PCI' here right now.
	 */
	
	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);
	if (err == -ENODEV) {
		/* No ChipCommon available. */
		return 0;
	}
	if (err)
		goto out;

	if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
		if (bcm->current_core->rev < 6) {
			if (mode == BCM430x_PCTL_CLK_FAST) {
				err = bcm430x_pctl_set_crystal(bcm, 1);
				if (err)
					goto out;
			}
		} else {
			switch (mode) {
			case BCM430x_PCTL_CLK_FAST:
				err = bcm430x_pci_read_config_16(bcm->pci_dev,
								 BCM430x_PCTL_OUTENABLE,
								 &oldmode);
				if (err)
					goto err_pci;
				oldmode = (oldmode & ~BCM430x_PCTL_FORCE_SLOW) | BCM430x_PCTL_FORCE_PLL;
				err = bcm430x_pci_write_config_16(bcm->pci_dev,
								  BCM430x_PCTL_OUTENABLE,
								  oldmode);
				if (err)
					goto err_pci;
				break;
			case BCM430x_PCTL_CLK_SLOW:
				err = bcm430x_pci_read_config_16(bcm->pci_dev,
								 BCM430x_PCTL_OUTENABLE,
								 &oldmode);
				if (err)
					goto err_pci;
				oldmode |= BCM430x_PCTL_FORCE_SLOW;
				err = bcm430x_pci_write_config_16(bcm->pci_dev,
								  BCM430x_PCTL_OUTENABLE,
								  oldmode);
				if (err)
					goto err_pci;
				break;
			case BCM430x_PCTL_CLK_DYNAMIC:
				err = bcm430x_pci_read_config_16(bcm->pci_dev,
								 BCM430x_PCTL_OUTENABLE,
								 &oldmode);
				if (err)
					goto err_pci;
				oldmode = ((oldmode & ~BCM430x_PCTL_FORCE_SLOW) & ~BCM430x_PCTL_FORCE_PLL) | BCM430x_PCTL_DYN_XTAL;
				err = bcm430x_pci_write_config_16(bcm->pci_dev,
								  BCM430x_PCTL_OUTENABLE,
								  oldmode);
				if (err)
					goto err_pci;
				break;
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

out:
	return err;

err_pci:
	printk(KERN_ERR PFX "Error: pctl_set_clock() could not access PCI config space!\n");
	err = -EBUSY;
	goto out;
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
		//TODO: if the radio is hardware-disabled, the core rev is < 5 or BFL_XTAL is set, do nothing.
		err = bcm430x_pctl_set_clock(bcm, BCM430x_PCTL_CLK_SLOW);
		if (err)
			goto out;
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
