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
	int err;
	u32 out, slow_clk_ctl;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);

	if (!err && (bcm->current_core->flags & BCM430x_COREFLAG_AVAILABLE) && (bcm->current_core->id == BCM430x_COREID_CHIPCOMMON)) {
		if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
			if (bcm->current_core->rev < 6) {
				bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
				if (out & 0x10) {
					/* PCI */
					return 25000000/64;
				} else {
					/* XTAL */
					return 19800000/32;
				}
			} else {
				bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &slow_clk_ctl);
				switch (slow_clk_ctl & 0x7) {
				case 0:
				case 1:
					/* LPO */
					return 25000;
					break;
				case 2:
					/* XTAL */
					return 19800000/(4*(1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
					break;
				case 3:
				default:
					/* PCI */
					return 25000000/(4*(1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
					break;
				}
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return -1;
}

/* get max slowclock frequency
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
static int bcm430x_pctl_get_maxslowclk(struct bcm430x_private *bcm)
{
	int err;
	u32 out;
	u32 slow_clk_ctl = 0;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);

	if (!err && (bcm->current_core->flags & BCM430x_COREFLAG_AVAILABLE) && (bcm->current_core->id == BCM430x_COREID_CHIPCOMMON)) {
		if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
			if (bcm->current_core->rev < 6) {
				bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
				if (out & 0x10) {
					/* PCI */
					return 34000000/64;
				} else {
					/* XTAL */
					return 20200000/32;
				}
			} else {
				bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &slow_clk_ctl);
				switch (slow_clk_ctl & 0x7) {
				case 0:
				case 1:
					/* LPO */
					return 43000;
					break;
				case 2:
					/* XTAL */
					return 20200000/(4*(1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
					break;
				case 3:
				default:
					/* PCI */
					return 34000000/(4*(1 + ((slow_clk_ctl & 0xFFFF0000) >> 16)));
					break;
				}
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return -1;
}

/* init power control
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
void bcm430x_pctl_init(struct bcm430x_private *bcm)
{
	int err, maxfreq;
	struct bcm430x_coreinfo *old_core;

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);

	if (!err && (bcm->current_core->flags & BCM430x_COREFLAG_AVAILABLE) && (bcm->current_core->id == BCM430x_COREID_CHIPCOMMON)) {
		if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
			maxfreq = bcm430x_pctl_get_maxslowclk(bcm);
			bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_IN, (maxfreq*250+999999)/1000000);
			bcm430x_pci_write_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, (maxfreq*250+999999)/1000000);
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);
}

u16 bcm430x_pctl_powerup_delay(struct bcm430x_private *bcm)
{
	int err;
	u32 pll_on_delay;
	struct bcm430x_coreinfo *old_core;
	int minfreq;

	//FIXME: ensure PCI

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);

	if (!err && (bcm->current_core->flags & BCM430x_COREFLAG_AVAILABLE) && (bcm->current_core->id == BCM430x_COREID_CHIPCOMMON)) {
		if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
			minfreq = bcm430x_pctl_get_minslowclk(bcm);
			bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_IN, &pll_on_delay);
			return (((pll_on_delay+2)*1000000)+(minfreq-1))/minfreq;
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);

	return 0;
}

/* set the powercontrol clock
 * as described in http://bcm-specs.sipsolutions.net/PowerControl
 */
void bcm430x_pctl_set_clock(struct bcm430x_private *bcm, u16 mode)
{
	int err;
	u16 oldmode;
	struct bcm430x_coreinfo *old_core;

	//TODO: return early, if we are setting to slow clock and the board does not implement it (boardflags)

	//FIXME: ensure PCI

	old_core = bcm->current_core;
	err = bcm430x_switch_core(bcm, &bcm->core_chipcommon);

	if (!err && (bcm->current_core->flags & BCM430x_COREFLAG_AVAILABLE) && (bcm->current_core->id == BCM430x_COREID_CHIPCOMMON)) {
		if (bcm->chipcommon_capabilities & BCM430x_CAPABILITIES_PCTLMASK) {
			if (bcm->current_core->rev < 6) {
				if (mode == BCM430x_PCTL_CLK_FAST)
					bcm430x_pctl_set_crystal(bcm, 1);
			} else {
				switch (mode) {
				case BCM430x_PCTL_CLK_FAST:
					bcm430x_pci_read_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &oldmode);
					oldmode = (oldmode & ~BCM430x_PCTL_FORCE_SLOW) | BCM430x_PCTL_FORCE_PLL;
					bcm430x_pci_write_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, oldmode);
					break;
				case BCM430x_PCTL_CLK_SLOW:
					bcm430x_pci_read_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &oldmode);
					oldmode |= BCM430x_PCTL_FORCE_SLOW;
					bcm430x_pci_write_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, oldmode);
					break;
				case BCM430x_PCTL_CLK_DYNAMIC:
					bcm430x_pci_read_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &oldmode);
					oldmode = ((oldmode & ~BCM430x_PCTL_FORCE_SLOW) & ~BCM430x_PCTL_FORCE_PLL) | BCM430x_PCTL_DYN_XTAL;
					bcm430x_pci_write_config_16(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, oldmode);
					break;
				}
			}
		}
	}

	err = bcm430x_switch_core(bcm, old_core);
	assert(err == 0);
}

void bcm430x_pctl_set_crystal(struct bcm430x_private *bcm, int on)
{
	u32 in, out, outenable;

	/* All the code in this function is derived from
	 * http://bcm-specs.sipsolutions.net/PowerControl */

	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_IN, &in);
	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUT, &out);
	bcm430x_pci_read_config_32(bcm->pci_dev, BCM430x_PCTL_OUTENABLE, &outenable);

	outenable |= (BCM430x_PCTL_XTAL_POWERUP | BCM430x_PCTL_PLL_POWERDOWN);

	if (on) {
		if (in & 0x40)
			return;

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
