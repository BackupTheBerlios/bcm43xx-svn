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
#include <linux/pci.h>
#include <linux/types.h>

#include "bcm430x.h"
#include "bcm430x_phy.h"
#include "bcm430x_main.h"
#include "bcm430x_radio.h"
#include "bcm430x_ilt.h"
#include "bcm430x_power.h"


static const s8 bcm430x_tssi2dbm_b_table[] = {
	0x4D, 0x4C, 0x4B, 0x4A,
	0x4A, 0x49, 0x48, 0x47,
	0x47, 0x46, 0x45, 0x45,
	0x44, 0x43, 0x42, 0x42,
	0x41, 0x40, 0x3F, 0x3E,
	0x3D, 0x3C, 0x3B, 0x3A,
	0x39, 0x38, 0x37, 0x36,
	0x35, 0x34, 0x32, 0x31,
	0x30, 0x2F, 0x2D, 0x2C,
	0x2B, 0x29, 0x28, 0x26,
	0x25, 0x23, 0x21, 0x1F,
	0x1D, 0x1A, 0x17, 0x14,
	0x10, 0x0C, 0x06, 0x00,
	0xF9, 0xF9, 0xF9, 0xF9,
	0xF9, 0xF9, 0xF9, 0xF9,
	0xF9, 0xF9, 0xF9, 0xF9
};

static const s8 bcm430x_tssi2dbm_g_table[] = {
	0x4D, 0x4D, 0x4D, 0x4C,
	0x4C, 0x4C, 0x4B, 0x4B,
	0x4A, 0x4A, 0x49, 0x49,
	0x49, 0x48, 0x48, 0x47,
	0x47, 0x46, 0x46, 0x45,
	0x44, 0x44, 0x43, 0x43,
	0x42, 0x41, 0x41, 0x40,
	0x3F, 0x3F, 0x3E, 0x3D,
	0x3C, 0x3B, 0x3A, 0x39,
	0x38, 0x37, 0x36, 0x35,
	0x34, 0x32, 0x31, 0x2F,
	0x2D, 0x2B, 0x28, 0x25,
	0x21, 0x1C, 0x16, 0x0E,
	0x05, 0xF9, 0xEC, 0xEC,
	0xEC, 0xEC, 0xEC, 0xEC, 
	0xEC, 0xEC, 0xEC, 0xEC
};

static void bcm430x_phy_initg(struct bcm430x_private *bcm);


void bcm430x_phy_lock(struct bcm430x_private *bcm)
{
	assert(!in_interrupt());
	if (bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD) == 0x00000000)
		return;
	if (bcm->current_core->rev < 3) {
		bcm430x_mac_suspend(bcm);
		spin_lock(&bcm->current_core->phy->lock);
	} else {
		if (bcm->ieee->iw_mode == IW_MODE_MASTER)
			return;
		bcm430x_power_saving_ctl_bits(bcm, -1, 1);
	}
}

void bcm430x_phy_unlock(struct bcm430x_private *bcm)
{
	assert(!in_interrupt());
	if (bcm->current_core->rev < 3) {
		if (!spin_is_locked(&bcm->current_core->phy->lock))
			return;
		spin_unlock(&bcm->current_core->phy->lock);
		bcm430x_mac_enable(bcm);
	} else {
		if (bcm->ieee->iw_mode == IW_MODE_MASTER)
			return;
		bcm430x_power_saving_ctl_bits(bcm, -1, -1);
	}
}

u16 bcm430x_phy_read(struct bcm430x_private *bcm, u16 offset)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_PHY_DATA);
}

void bcm430x_phy_write(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_DATA, val);
}

void bcm430x_phy_calibrate(struct bcm430x_private *bcm)
{
	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD); // Dummy read
	if (bcm->current_core->phy->calibrated)
		return;
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
		bcm430x_radio_set_txpower_a(bcm, 0x0018);
	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_G)
	    && (bcm->current_core->phy->rev == 1)) {
		bcm430x_wireless_core_reset(bcm, 0);
		bcm430x_phy_initg(bcm);
		bcm430x_wireless_core_reset(bcm, 1);
	}
	bcm->current_core->phy->calibrated = 1;
}

/* Connect the PHY 
 * http://bcm-specs.sipsolutions.net/SetPHY
 */
int bcm430x_phy_connect(struct bcm430x_private *bcm, int connect)
{
	u32 flags;

	if (bcm->current_core->rev < 5)
		return 0;

	flags = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATEHIGH);
	if (connect) {
		if (!(flags & 0x00010000))
			return -ENODEV;
		bcm->current_core->phy->connected = 1;

		flags = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
		flags |= (0x800 << 18);
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, flags);
		dprintk(KERN_INFO PFX "PHY connected\n");
	} else {
		if (!(flags & 0x00020000))
			return -ENODEV;
		bcm->current_core->phy->connected = 0;

		flags = bcm430x_read32(bcm, BCM430x_CIR_SBTMSTATELOW);
		flags &= ~(0x800 << 18);
		bcm430x_write32(bcm, BCM430x_CIR_SBTMSTATELOW, flags);
		dprintk(KERN_INFO PFX "PHY disconnected\n");
	}

	return 0;
}

/* intialize B PHY power control
 * as described in http://bcm-specs.sipsolutions.net/InitPowerControl
 */
static void bcm430x_phy_init_pctl(struct bcm430x_private *bcm)
{
	u16 t_batt = 0xFFFF;
	u16 t_ratt = 0xFFFF;
	u16 t_txatt = 0xFFFF;

	assert(bcm->current_core->phy->type != BCM430x_PHYTYPE_A);

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) &&
	    (bcm->board_type == 0x0416))
		return;

	bcm430x_write16(bcm, 0x03E6, bcm430x_read16(bcm, 0x03E6) & 0xFFDF);
	bcm430x_phy_write(bcm, 0x0028, 0x8018);

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		if (bcm->current_core->phy->connected)
			bcm430x_phy_write(bcm, 0x047A, 0xC111);
		else
			return;
	}
	if (bcm->current_core->phy->savedpctlreg != 0xFFFF)
		return;

	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_B) &&
	    (bcm->current_core->phy->rev >= 2) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm430x_radio_write16(bcm, 0x0076,
				      bcm430x_radio_read16(bcm, 0x0076) | 0x0084);
	} else {
		t_batt = bcm->current_core->radio->txpower[0];
		t_ratt = bcm->current_core->radio->txpower[1];
		t_txatt = bcm->current_core->radio->txpower[2];
		bcm430x_radio_set_txpower_bg(bcm, 0x000B, 0x0009, 0x0000);
	}

	bcm430x_dummy_transmission(bcm);

	bcm->current_core->phy->savedpctlreg = bcm430x_phy_read(bcm, BCM430x_PHY_G_PCTL);

	if ((bcm->current_core->phy->type != BCM430x_PHYTYPE_B) ||
	    (bcm->current_core->phy->rev < 2) ||
	    (bcm->current_core->radio->version != 0x2050))
		bcm430x_radio_set_txpower_bg(bcm, t_batt, t_ratt, t_txatt);

	bcm430x_radio_write16(bcm, 0x0076, bcm430x_radio_read16(bcm, 0x0076) & 0xFF7B);

	bcm430x_radio_clear_tssi(bcm);
}

static void bcm430x_phy_agcsetup(struct bcm430x_private *bcm)
{
	u16 offset = 0x0000;

	if (bcm->current_core->phy->rev == 1)
		offset = 0x4C00;

	bcm430x_ilt_write16(bcm, offset, 0x00FE);
	bcm430x_ilt_write16(bcm, offset + 1, 0x000D);
	bcm430x_ilt_write16(bcm, offset + 2, 0x0013);
	bcm430x_ilt_write16(bcm, offset + 3, 0x0019);

	if (bcm->current_core->phy->rev == 1) {
		bcm430x_ilt_write16(bcm, 0x1800, 0x2710);
		bcm430x_ilt_write16(bcm, 0x1801, 0x9B83);
		bcm430x_ilt_write16(bcm, 0x1802, 0x9B83);
		bcm430x_ilt_write16(bcm, 0x1803, 0x0F8D);
		bcm430x_phy_write(bcm, 0x0455, 0x0004);
	}

	bcm430x_phy_write(bcm, 0x04A5, (bcm430x_phy_read(bcm, 0x04A5) & 0x00FF) | 0x5700);
	bcm430x_phy_write(bcm, 0x041A, (bcm430x_phy_read(bcm, 0x041A) & 0xFF80) | 0x000F);
	bcm430x_phy_write(bcm, 0x041A, (bcm430x_phy_read(bcm, 0x041A) & 0xC07F) | 0x2B80);
	bcm430x_phy_write(bcm, 0x048C, (bcm430x_phy_read(bcm, 0x048C) & 0xF0FF) | 0x0300);

	bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0008);

	bcm430x_phy_write(bcm, 0x04A0, (bcm430x_phy_read(bcm, 0x04A0) & 0xFFF0) | 0x0008);
	bcm430x_phy_write(bcm, 0x04A1, (bcm430x_phy_read(bcm, 0x04A1) & 0xF0FF) | 0x0600);
	bcm430x_phy_write(bcm, 0x04A2, (bcm430x_phy_read(bcm, 0x04A2) & 0xF0FF) | 0x0700);
	bcm430x_phy_write(bcm, 0x04A0, (bcm430x_phy_read(bcm, 0x04A0) & 0xF0FF) | 0x0100);

	if (bcm->current_core->phy->rev == 1)
		bcm430x_phy_write(bcm, 0x04A2, (bcm430x_phy_read(bcm, 0x04A2) & 0xFFF0) | 0x0007);

	bcm430x_phy_write(bcm, 0x0488, (bcm430x_phy_read(bcm, 0x0488) & 0xFF00) | 0x001C);
	bcm430x_phy_write(bcm, 0x0488, (bcm430x_phy_read(bcm, 0x0488) & 0xC0FF) | 0x0200);
	bcm430x_phy_write(bcm, 0x0496, (bcm430x_phy_read(bcm, 0x0496) & 0xFF00) | 0x001C);
	bcm430x_phy_write(bcm, 0x0489, (bcm430x_phy_read(bcm, 0x0489) & 0xFF00) | 0x0020);
	bcm430x_phy_write(bcm, 0x0489, (bcm430x_phy_read(bcm, 0x0489) & 0xC0FF) | 0x0200);
	bcm430x_phy_write(bcm, 0x0482, (bcm430x_phy_read(bcm, 0x0482) & 0xFF00) | 0x002E);
	bcm430x_phy_write(bcm, 0x0496, (bcm430x_phy_read(bcm, 0x0496) & 0x00FF) | 0x1A00);
	bcm430x_phy_write(bcm, 0x0481, (bcm430x_phy_read(bcm, 0x0481) & 0xFF00) | 0x0028);
	bcm430x_phy_write(bcm, 0x0481, (bcm430x_phy_read(bcm, 0x0481) & 0x00FF) | 0x2C00);

	if (bcm->current_core->phy->rev == 1) {
		bcm430x_phy_write(bcm, 0x0430, 0x092B);
		bcm430x_phy_write(bcm, 0x041B, (bcm430x_phy_read(bcm, 0x041B) & 0xFFE1) | 0x0002);
	} else {
		bcm430x_phy_write(bcm, 0x041B, bcm430x_phy_read(bcm, 0x041B) & 0xFFE1);
		bcm430x_phy_write(bcm, 0x041F, 0x287A);
		bcm430x_phy_write(bcm, 0x0420, (bcm430x_phy_read(bcm, 0x0420) & 0xFFF0) | 0x0004);
	}

	bcm430x_phy_write(bcm, 0x04A8, (bcm430x_phy_read(bcm, 0x04A8) & 0x8080) | 0x7874);
	bcm430x_phy_write(bcm, 0x048E, 0x1C00);

	if (bcm->current_core->phy->rev == 1) {
		bcm430x_phy_write(bcm, 0x04AB, (bcm430x_phy_read(bcm, 0x04AB) & 0xF0FF) | 0x0600);
		bcm430x_phy_write(bcm, 0x048B, 0x005E);
		bcm430x_phy_write(bcm, 0x048C, (bcm430x_phy_read(bcm, 0x048C) & 0xFF00) | 0x001E);
		bcm430x_phy_write(bcm, 0x048D, 0x0002);
	}

	bcm430x_ilt_write16(bcm, offset + 0x0800, 0);
	bcm430x_ilt_write16(bcm, offset + 0x0801, 7);
	bcm430x_ilt_write16(bcm, offset + 0x0802, 16);
	bcm430x_ilt_write16(bcm, offset + 0x0803, 28);
}

static void bcm430x_phy_setupg(struct bcm430x_private *bcm)
{
	u16 i;

	assert(bcm->current_core->phy->type == BCM430x_PHYTYPE_G);
	switch (bcm->current_core->phy->rev) {
	case 1:
		bcm430x_phy_write(bcm, 0x0406, 0x4F19);
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
				  (bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0xFC3F) | 0x0340);
		bcm430x_phy_write(bcm, 0x042C, 0x005A);
		bcm430x_phy_write(bcm, 0x0427, 0x001A);

		for (i = 0; i < BCM430x_ILT_FINEFREQG_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x5800 + i, bcm430x_ilt_finefreqg[i]);
		for (i = 0; i < BCM430x_ILT_NOISEG1_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1800 + i, bcm430x_ilt_noiseg1[i]);
		for (i = 0; i < BCM430x_ILT_ROTOR_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x2000 + i, bcm430x_ilt_rotor[i]);
		for (i = 0; i < BCM430x_ILT_NOISESCALEG_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1400 + i, bcm430x_ilt_noisescaleg[i]);
		for (i = 0; i < BCM430x_ILT_RETARD_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x2400 + i, bcm430x_ilt_retard[i]);
		for (i = 0; i < 4; i++) {
			bcm430x_ilt_write16(bcm, 0x5404 + i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x5408 + i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x540C + i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x5410 + i, 0x0020);
		}
		bcm430x_phy_agcsetup(bcm);
		break;
	case 2:
		//FIXME: 0xBA98 should be in 0-64, 0x7654 should be 6-bit!
		bcm430x_nrssi_hw_write(bcm, 0xBA98, 0x7654);

		bcm430x_phy_write(bcm, 0x04C0, 0x1861);
		bcm430x_phy_write(bcm, 0x04C1, 0x0271);

		for (i = 0; i < 64; i++)
			bcm430x_ilt_write16(bcm, 0x4000 + i, i);
		for (i = 0; i < BCM430x_ILT_NOISEG2_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1800 + i, bcm430x_ilt_noiseg2[i]);
		for (i = 0; i < BCM430x_ILT_NOISESCALEG_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1400 + i, bcm430x_ilt_noisescaleg[i]);
		for (i = 0; i < BCM430x_ILT_SIGMASQR_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x5000 + i, bcm430x_ilt_sigmasqr[i]);
		for (i = 0; i <= 0x2F; i++)
			bcm430x_ilt_write16(bcm, 0x1000 + i, 0x0820);
		bcm430x_phy_agcsetup(bcm);
		bcm430x_phy_write(bcm, 0x0403, 0x1000);
		bcm430x_ilt_write16(bcm, 0x3C02, 0x000F);
		bcm430x_ilt_write16(bcm, 0x3C03, 0x0014);

		if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) &&
		    (bcm->board_type == 0x0416) &&
		    (bcm->board_revision != 0x0017))
			return;

		if (bcm->current_core->phy->rev < 2) {
			bcm430x_ilt_write16(bcm, 0x5001, 0x0002);
			bcm430x_ilt_write16(bcm, 0x5002, 0x0001);
		} else {
			bcm430x_ilt_write16(bcm, 0x0401, 0x0002);
			bcm430x_ilt_write16(bcm, 0x0402, 0x0001);
		}
		break;
	default:
		assert(0);
	}
}

/* Initialize the noisescaletable for APHY */
static void bcm430x_phy_init_noisescaletbl(struct bcm430x_private *bcm)
{
	int i;

	bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_CTRL, 0x1400);
	for (i = 0; i < 12; i++) {
		if (bcm->current_core->phy->rev == 2)
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6767);
		else
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2323);
	}
	if (bcm->current_core->phy->rev == 2)
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6700);
	else
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2300);
	for (i = 0; i < 11; i++) {
		if (bcm->current_core->phy->rev == 2)
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6767);
		else
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2323);
	}
	if (bcm->current_core->phy->rev == 2)
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x0067);
	else
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x0023);
}

static void bcm430x_phy_setupa(struct bcm430x_private *bcm)
{
	u16 i;

	assert(bcm->current_core->phy->type == BCM430x_PHYTYPE_A);
	switch (bcm->current_core->phy->rev) {
	case 2:
		bcm430x_phy_write(bcm, 0x008E, 0x3800);
		bcm430x_phy_write(bcm, 0x0035, 0x03FF);
		bcm430x_phy_write(bcm, 0x0036, 0x0400);

		bcm430x_ilt_write16(bcm, 0x3807, 0x0051);

		bcm430x_phy_write(bcm, 0x001C, 0x0FF9);
		bcm430x_phy_write(bcm, 0x0020, bcm430x_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm430x_ilt_write16(bcm, 0x3C0C, 0x07BF);
		bcm430x_radio_write16(bcm, 0x0002, 0x07BF);

		bcm430x_phy_write(bcm, 0x0024, 0x4680);
		bcm430x_phy_write(bcm, 0x0020, 0x0003);
		bcm430x_phy_write(bcm, 0x001D, 0x0F40);
		bcm430x_phy_write(bcm, 0x001F, 0x1C00);

		bcm430x_phy_write(bcm, 0x002A, (bcm430x_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);
		bcm430x_phy_write(bcm, 0x002B, bcm430x_phy_read(bcm, 0x002B) & 0xFBFF);
		bcm430x_phy_write(bcm, 0x008E, 0x58C1);

		bcm430x_ilt_write16(bcm, 0x0803, 0x000F);
		bcm430x_ilt_write16(bcm, 0x0804, 0x001F);
		bcm430x_ilt_write16(bcm, 0x0805, 0x002A);
		bcm430x_ilt_write16(bcm, 0x0805, 0x0030);
		bcm430x_ilt_write16(bcm, 0x0807, 0x003A);

		bcm430x_ilt_write16(bcm, 0x0000, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0001, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0002, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0003, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0004, 0x0015);
		bcm430x_ilt_write16(bcm, 0x0005, 0x0015);
		bcm430x_ilt_write16(bcm, 0x0006, 0x0019);

		bcm430x_ilt_write16(bcm, 0x0404, 0x0003);
		bcm430x_ilt_write16(bcm, 0x0405, 0x0003);
		bcm430x_ilt_write16(bcm, 0x0406, 0x0007);

		FIXME();//FIXME: The specs are completely weird here. I think this code is wrong.
		for (i = 0; i < 16; i++)
			bcm430x_ilt_write16(bcm, 0x4000 + i, (i+8)%16);

		bcm430x_ilt_write16(bcm, 0x3003, 0x1044);
		bcm430x_ilt_write16(bcm, 0x3004, 0x7201);
		bcm430x_ilt_write16(bcm, 0x3006, 0x0040);
		bcm430x_ilt_write16(bcm, 0x3001, (bcm430x_ilt_read16(bcm, 0x3001) & 0x0010) | 0x0008);

		for (i = 0; i < BCM430x_ILT_FINEFREQA_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x5800 + i, bcm430x_ilt_finefreqa[i]);
		for (i = 0; i < BCM430x_ILT_NOISEA2_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1800 + i, bcm430x_ilt_noisea2[i]);
		for (i = 0; i < BCM430x_ILT_ROTOR_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x2000 + i, bcm430x_ilt_rotor[i]);
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i = 0; i < BCM430x_ILT_RETARD_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x2400 + i, bcm430x_ilt_retard[i]);
		break;
	case 3:
		for (i = 0; i < 64; i++)
			bcm430x_ilt_write16(bcm, 0x4000 + i, i);

		bcm430x_ilt_write16(bcm, 0x3807, 0x0051);

		bcm430x_phy_write(bcm, 0x001C, 0x0FF9);
		bcm430x_phy_write(bcm, 0x0020, bcm430x_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm430x_radio_write16(bcm, 0x0002, 0x07BF);

		bcm430x_phy_write(bcm, 0x0024, 0x4680);
		bcm430x_phy_write(bcm, 0x0020, 0x0003);
		bcm430x_phy_write(bcm, 0x001D, 0x0F40);
		bcm430x_phy_write(bcm, 0x001F, 0x1C00);
		bcm430x_phy_write(bcm, 0x002A, (bcm430x_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);

		bcm430x_ilt_write16(bcm, 0x3001, (bcm430x_ilt_read16(bcm, 0x3001) & 0x0010) | 0x0008);
		for (i = 0; i < BCM430x_ILT_NOISEA3_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x1800 + i, bcm430x_ilt_noisea3[i]);
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i = 0; i < BCM430x_ILT_SIGMASQR_SIZE; i++)
			bcm430x_ilt_write16(bcm, 0x5000 + i, bcm430x_ilt_sigmasqr[i]);

		bcm430x_phy_write(bcm, 0x0003, 0x1808);

		bcm430x_ilt_write16(bcm, 0x0803, 0x000F);
		bcm430x_ilt_write16(bcm, 0x0804, 0x001F);
		bcm430x_ilt_write16(bcm, 0x0805, 0x002A);
		bcm430x_ilt_write16(bcm, 0x0805, 0x0030);
		bcm430x_ilt_write16(bcm, 0x0807, 0x003A);

		bcm430x_ilt_write16(bcm, 0x0000, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0001, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0002, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0003, 0x0013);
		bcm430x_ilt_write16(bcm, 0x0004, 0x0015);
		bcm430x_ilt_write16(bcm, 0x0005, 0x0015);
		bcm430x_ilt_write16(bcm, 0x0006, 0x0019);

		bcm430x_ilt_write16(bcm, 0x0404, 0x0003);
		bcm430x_ilt_write16(bcm, 0x0405, 0x0003);
		bcm430x_ilt_write16(bcm, 0x0406, 0x0007);

		bcm430x_ilt_write16(bcm, 0x3C02, 0x000F);
		bcm430x_ilt_write16(bcm, 0x3C03, 0x0014);
		break;
	default:
		assert(0);
	}
}

/* Initialize APHY. This is also called for the GPHY in some cases. */
static void bcm430x_phy_inita(struct bcm430x_private *bcm)
{
	u16 tval;

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		bcm430x_phy_setupa(bcm);
	} else {
		bcm430x_phy_setupg(bcm);
		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			bcm430x_phy_write(bcm, 0x046E, 0x03CF);
		return;
	}

	bcm430x_phy_write(bcm, BCM430x_PHY_A_CRS,
	                  (bcm430x_phy_read(bcm, BCM430x_PHY_A_CRS) & 0xF83C) | 0x0340);
	bcm430x_phy_write(bcm, 0x0034, 0x0001);

	TODO();//TODO: RSSI AGC
	bcm430x_phy_write(bcm, BCM430x_PHY_A_CRS,
	                  bcm430x_phy_read(bcm, BCM430x_PHY_A_CRS) | (1 << 14));
	bcm430x_radio_init2060(bcm);

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM)
	    && ((bcm->board_type == 0x0416) || (bcm->board_type == 0x040A))) {
		if (bcm->current_core->radio->lofcal == 0xFFFF) {
			TODO();//TODO: LOF Cal
			//TODO: Set TX IQ based on VOS
		} else
			bcm430x_radio_write16(bcm, 0x001E, bcm->current_core->radio->lofcal);
	}

	bcm430x_phy_write(bcm, 0x007A, 0xF111);

	if (bcm->current_core->phy->savedpctlreg == 0xFFFF) {
		bcm430x_radio_write16(bcm, 0x0019, 0x0000);
		bcm430x_radio_write16(bcm, 0x0017, 0x0020);

		tval = bcm430x_ilt_read16(bcm, 0x3001);
		if (bcm->current_core->phy->rev == 1) {
			bcm430x_ilt_write16(bcm, 0x3001,
					    (bcm430x_ilt_read16(bcm, 0x3001) & 0xFF87)
					    | 0x0058);
		} else {
			bcm430x_ilt_write16(bcm, 0x3001,
					    (bcm430x_ilt_read16(bcm, 0x3001) & 0xFFC3)
					    | 0x002C);
		}
		bcm430x_dummy_transmission(bcm);
		bcm->current_core->phy->savedpctlreg = bcm430x_phy_read(bcm, BCM430x_PHY_A_PCTL);
		bcm430x_ilt_write16(bcm, 0x3001, tval);

		bcm430x_radio_set_txpower_a(bcm, 0x0018);
	}
	bcm430x_radio_clear_tssi(bcm);
}

static void bcm430x_phy_initb2(struct bcm430x_private *bcm)
{
	u16 offset, val;

	bcm430x_write16(bcm, 0x03EC, 0x3F22);
	bcm430x_phy_write(bcm, 0x0020, 0x301C);
	bcm430x_phy_write(bcm, 0x0026, 0x0000);
	bcm430x_phy_write(bcm, 0x0030, 0x00C6);
	bcm430x_phy_write(bcm, 0x0088, 0x3E00);
	val = 0x3C3D;
	for (offset = 0x0089; offset < 0x00A7; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm430x_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->current_core->radio->channel == 0xFFFF)
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG, 0);
	else
		bcm430x_radio_selectchannel(bcm, bcm->current_core->radio->channel, 0);
	if (bcm->current_core->radio->version != 0x2050) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if (bcm->current_core->radio->version == 0x2050) {
		bcm430x_radio_write16(bcm, 0x0050, 0x0020);
		bcm430x_radio_write16(bcm, 0x005A, 0x0070);
		bcm430x_radio_write16(bcm, 0x005B, 0x007B);
		bcm430x_radio_write16(bcm, 0x005C, 0x00B0);
		bcm430x_radio_write16(bcm, 0x007A, 0x000F);
		bcm430x_phy_write(bcm, 0x0038, 0x0677);
		bcm430x_radio_init2050(bcm);
	}
	bcm430x_phy_write(bcm, 0x0014, 0x0080);
	bcm430x_phy_write(bcm, 0x0032, 0x00CA);
	bcm430x_phy_write(bcm, 0x0032, 0x00CC);
	bcm430x_phy_write(bcm, 0x0035, 0x07C2);
	bcm430x_phy_lo_b_measure(bcm);
	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if (bcm->current_core->radio->version != 0x2050)
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	bcm430x_write16(bcm, 0x03F4, 0x1000);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if (bcm->current_core->radio->version != 0x2050)
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	bcm430x_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb4(struct bcm430x_private *bcm)
{
	u16 offset, val;

	bcm430x_write16(bcm, 0x03EC, 0x3F22);
	bcm430x_phy_write(bcm, 0x0020, 0x301C);
	bcm430x_phy_write(bcm, 0x0026, 0x0000);
	bcm430x_phy_write(bcm, 0x0030, 0x00C6);
	bcm430x_phy_write(bcm, 0x0088, 0x3E00);
	val = 0x3C3D;
	for (offset = 0x0089; offset < 0x00A7; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm430x_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->current_core->radio->channel == 0xFFFF)
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG, 0);
	else
		bcm430x_radio_selectchannel(bcm, bcm->current_core->radio->channel, 0);
	if (bcm->current_core->radio->version != 0x2050) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if (bcm->current_core->radio->version == 0x2050) {
		bcm430x_radio_write16(bcm, 0x0050, 0x0020);
		bcm430x_radio_write16(bcm, 0x005A, 0x0070);
		bcm430x_radio_write16(bcm, 0x005B, 0x007B);
		bcm430x_radio_write16(bcm, 0x005C, 0x00B0);
		bcm430x_radio_write16(bcm, 0x007A, 0x000F);
		bcm430x_phy_write(bcm, 0x0038, 0x0677);
		bcm430x_radio_init2050(bcm);
	}
	bcm430x_phy_write(bcm, 0x0014, 0x0080);
	bcm430x_phy_write(bcm, 0x0032, 0x00CA);
	if (bcm->current_core->radio->version == 0x2050)
		bcm430x_phy_write(bcm, 0x0032, 0x00E0);
	bcm430x_phy_write(bcm, 0x0035, 0x07C2);

	bcm430x_phy_lo_b_measure(bcm);

	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if (bcm->current_core->radio->version == 0x2050)
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	bcm430x_write16(bcm, 0x03F4, 0x1100);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if (bcm->current_core->radio->version == 0x2050)
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	bcm430x_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	if (bcm->sprom.boardflags & BCM430x_BFL_RSSI) {
		bcm430x_calc_nrssi_slope(bcm);
		bcm430x_calc_nrssi_threshold(bcm);
	}
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb5(struct bcm430x_private *bcm)
{
	u16 offset;

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A)
				      | 0x0050);
	}

	if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM) &&
	    (bcm->board_type != 0x0416)) {
		for (offset = 0x00A8 ; offset < 0x00C7; offset++) {
			bcm430x_phy_write(bcm, offset,
					  (bcm430x_phy_read(bcm, offset) + 0x2020)
					  & 0x3F3F);
		}
	}

	bcm430x_phy_write(bcm, 0x0035,
			  (bcm430x_phy_read(bcm, 0x0035) & 0xF0FF)
			  | 0x0700);

	if (bcm->current_core->radio->version == 0x2050)
		bcm430x_phy_write(bcm, 0x0038, 0x0667);

	if (bcm->current_core->phy->connected) {
		if (bcm->current_core->radio->version == 0x2050) {
			bcm430x_radio_write16(bcm, 0x007A,
					      bcm430x_radio_read16(bcm, 0x007A)
					      | 0x0020);
			bcm430x_radio_write16(bcm, 0x0051,
					      bcm430x_radio_read16(bcm, 0x0051)
					      | 0x0004);
		}

		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO, 0x0000);

		bcm430x_phy_write(bcm, 0x0802, bcm430x_phy_read(bcm, 0x0802) | 0x0100);
		bcm430x_phy_write(bcm, 0x042B, bcm430x_phy_read(bcm, 0x042B) | 0x2000);

		bcm430x_phy_write(bcm, 0x001C, 0x186A);

		bcm430x_phy_write(bcm, 0x0013, (bcm430x_phy_read(bcm, 0x0013) & 0x00FF) | 0x1900);
		bcm430x_phy_write(bcm, 0x0035, (bcm430x_phy_read(bcm, 0x0035) & 0xFFC0) | 0x0064);
		bcm430x_phy_write(bcm, 0x005D, (bcm430x_phy_read(bcm, 0x005D) & 0xFF80) | 0x000A);
	}

	if (bcm->bad_frames_preempt) {
		bcm430x_phy_write(bcm, 0x0401,
				  bcm430x_phy_read(bcm, 0x0401) | (1 << 11));
	}

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
		bcm430x_phy_write(bcm, 0x0021, 0x3763);
		bcm430x_phy_write(bcm, 0x0022, 0x1BC3);
		bcm430x_phy_write(bcm, 0x0023, 0x06F9);
		bcm430x_phy_write(bcm, 0x0024, 0x037E);
	} else
		bcm430x_phy_write(bcm, 0x0026, 0xCC00);

	bcm430x_phy_write(bcm, 0x0030, 0x00C6);

	bcm430x_write16(bcm, 0x03EC, 0x3F22);

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050))
		bcm430x_phy_write(bcm, 0x0020, 0x3E1C);
	else
		bcm430x_phy_write(bcm, 0x0020, 0x301C);

	if (bcm->current_core->phy->version == 0)
		bcm430x_write16(bcm, 0x03E4, 0x3000);

	/* Force to channel 7, even if not supported. */
	bcm430x_radio_selectchannel(bcm, 7, 0);

	if (bcm->current_core->radio->version != 0x2050) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}

	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);

	if (bcm->current_core->radio->version == 0x2050) {
		bcm430x_radio_write16(bcm, 0x0050, 0x0020);
		bcm430x_radio_write16(bcm, 0x005A, 0x0070);
	}

	bcm430x_radio_write16(bcm, 0x005B, 0x007B);
	bcm430x_radio_write16(bcm, 0x005C, 0x00B0);

	bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0007);

	bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG, 0);

	bcm430x_phy_write(bcm, 0x0014, 0x0080);
	bcm430x_phy_write(bcm, 0x0032, 0x00CA);
	bcm430x_phy_write(bcm, 0x88A3, 0x002A);

	bcm430x_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);

	if (bcm->current_core->radio->version == 0x2050)
		bcm430x_radio_write16(bcm, 0x005D, 0x000D);

	bcm430x_write16(bcm, 0x03E4, (bcm430x_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
}

static void bcm430x_phy_initb6(struct bcm430x_private *bcm)
{
	u16 offset, val;

	bcm430x_radio_write16(bcm, 0x007A,
	                      (bcm430x_radio_read16(bcm, 0x007A) | 0x0050));
	if ((bcm->current_core->radio->manufact == 0x17F) &&
	    (bcm->current_core->radio->version == 0x2050) &&
	    (bcm->current_core->radio->revision == 3)) {
		bcm430x_radio_write16(bcm, 0x0051, 0x001F);
		bcm430x_radio_write16(bcm, 0x0052, 0x0040);
		bcm430x_radio_write16(bcm, 0x0053, 0x005B);
		bcm430x_radio_write16(bcm, 0x0054, 0x0098);
		bcm430x_radio_write16(bcm, 0x005A, 0x0088);
		bcm430x_radio_write16(bcm, 0x005B, 0x0088);
		bcm430x_radio_write16(bcm, 0x005D, 0x0088);
		bcm430x_radio_write16(bcm, 0x005E, 0x0088);
		bcm430x_radio_write16(bcm, 0x007D, 0x0088);
	}
	val = 0x1E1F;
	bcm430x_phy_write(bcm, 0x0088, val);
	for (offset = 0x0088; offset < 0x0098; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x3E3F;
	for (offset = 0x0098; offset < 0x00A8; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x2120;
	for (offset = 0x00A8; offset < 0x00C8; offset++) {
		bcm430x_phy_write(bcm, offset, (val & 0x3F3F));
		val += 0x0202;
	}
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		bcm430x_radio_write16(bcm, 0x007A,
		                      bcm430x_radio_read16(bcm, 0x007A) | 0x0020);
		bcm430x_radio_write16(bcm, 0x0051,
		                      bcm430x_radio_read16(bcm, 0x0051) | 0x0002);
		bcm430x_phy_write(bcm, 0x0802,
		                  bcm430x_phy_read(bcm, 0x0802) | 0x0100);
		bcm430x_phy_write(bcm, 0x042B,
		                  bcm430x_phy_read(bcm, 0x042B) | 0x2000);
	}

	/* Force to channel 7, even if not supported. */
	bcm430x_radio_selectchannel(bcm, 7, 0);

	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x005A, 0x0070);
	bcm430x_radio_write16(bcm, 0x005B, 0x007B);
	bcm430x_radio_write16(bcm, 0x005C, 0x00B0);
	bcm430x_radio_write16(bcm, 0x007A,
	                      (bcm430x_radio_read16(bcm, 0x007A) & 0x00F8) | 0x0007);

	bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG, 0);

	bcm430x_phy_write(bcm, 0x0014, 0x0200);
	bcm430x_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm430x_radio_write16(bcm, 0x0052,
	                      (bcm430x_radio_read16(bcm, 0x0052) & 0x00F0) | 0x0009);
	bcm430x_radio_write16(bcm, 0x005D, 0x000D);
	bcm430x_write16(bcm, 0x03E4,
	                (bcm430x_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		bcm430x_phy_write(bcm, 0x0016, 0x5410);
		bcm430x_phy_write(bcm, 0x0017, 0xA820);
		bcm430x_phy_write(bcm, 0x0007, 0x0062);
		bcm430x_phy_init_pctl(bcm);
	}
}

static void bcm430x_phy_initg(struct bcm430x_private *bcm)
{
	if (bcm->current_core->phy->rev == 1)
		bcm430x_phy_initb5(bcm);
	else if (bcm->current_core->phy->rev == 2)
		bcm430x_phy_initb6(bcm);
	if ((bcm->current_core->phy->rev >= 2) || bcm->current_core->phy->connected)
		bcm430x_phy_inita(bcm);

	if (bcm->current_core->phy->rev >= 2) {
		bcm430x_phy_write(bcm, 0x003E, 0x817A);
		bcm430x_phy_write(bcm, 0x0814, 0x0000);
		bcm430x_phy_write(bcm, 0x0815, 0x0000);
		bcm430x_phy_write(bcm, 0x0811, 0x0000);
		bcm430x_phy_write(bcm, 0x0015, 0x00C0);
		bcm430x_phy_write(bcm, 0x04C2, 0x1816);
		bcm430x_phy_write(bcm, 0x04C3, 0x8606);
	}
	if (bcm->current_core->radio->initval == 0xFFFF) {
		bcm->current_core->radio->initval = bcm430x_radio_init2050(bcm);
		bcm430x_phy_lo_g_measure(bcm);
	} else {
		bcm430x_radio_write16(bcm, 0x0078, bcm->current_core->radio->initval);
		bcm430x_radio_write16(bcm, 0x0052,
				      (bcm430x_radio_read16(bcm, 0x0052) & 0xFFF0)
				      | bcm->current_core->radio->txpower[3]);
	}

	if (bcm->current_core->phy->connected) {
		bcm430x_phy_lo_adjust(bcm, 0);
		bcm430x_phy_write(bcm, 0x080F, 0x8078);

		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			bcm430x_phy_write(bcm, 0x002E, 0x807F);
		else
			bcm430x_phy_write(bcm, 0x002E, 0x8075);

		if (bcm->current_core->phy->rev < 2)
			bcm430x_phy_write(bcm, 0x002F, 0x0101);
		else
			bcm430x_phy_write(bcm, 0x002F, 0x0202);
	}

	if ((bcm->sprom.boardflags & BCM430x_BFL_RSSI) == 0) {
		FIXME();//FIXME: 0x7FFFFFFF should be 16-bit !
		bcm430x_nrssi_hw_update(bcm, (u16)0x7FFFFFFF);
		bcm430x_calc_nrssi_threshold(bcm);
	} else if (bcm->current_core->phy->connected) {
		if ((bcm->current_core->radio->nrssi[0] == -1000) &&
		    (bcm->current_core->radio->nrssi[1] == -1000))
			bcm430x_calc_nrssi_slope(bcm);
		else
			bcm430x_calc_nrssi_threshold(bcm);
	}
	bcm430x_phy_init_pctl(bcm);
}

static inline
u16 bcm430x_phy_lo_b_r15_loop(struct bcm430x_private *bcm)
{
	int i;
	u16 ret = 0;

	FIXME();
	//FIXME: Why do we loop from 9 to 0 here, instead of from 0 to 9?
	for (i = 9; i > -1; i--){
		bcm430x_phy_write(bcm, 0x0015, 0xAFA0);
		udelay(1);
		bcm430x_phy_write(bcm, 0x0015, 0xEFA0);
		udelay(10);
		bcm430x_phy_write(bcm, 0x0015, 0xFFA0);
		udelay(40);
		ret += bcm430x_phy_read(bcm, 0x002C);
	}

	return ret;
}

void bcm430x_phy_lo_b_measure(struct bcm430x_private *bcm)
{
	const int is_2053_radio = (bcm->current_core->radio->version == 0x2053);
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;
	u16 regstack[12] = { 0 };
	u16 mls;
	u16 fval;
	int i, j;

	regstack[0] = bcm430x_phy_read(bcm, 0x0015);
	regstack[1] = bcm430x_radio_read16(bcm, 0x0052) & 0xFFF0;

	if (is_2053_radio) {
		regstack[2] = bcm430x_phy_read(bcm, 0x000A);
		regstack[3] = bcm430x_phy_read(bcm, 0x002A);
		regstack[4] = bcm430x_phy_read(bcm, 0x0035);
		regstack[5] = bcm430x_phy_read(bcm, 0x0003);
		regstack[6] = bcm430x_phy_read(bcm, 0x0001);
		regstack[7] = bcm430x_phy_read(bcm, 0x0030);

		regstack[8] = bcm430x_radio_read16(bcm, 0x0043);
		regstack[9] = bcm430x_radio_read16(bcm, 0x007A);
		regstack[10] = bcm430x_read16(bcm, 0x03EC);
		regstack[11] = bcm430x_radio_read16(bcm, 0x0052) & 0x00F0;

		bcm430x_phy_write(bcm, 0x0030, 0x00FF);
		bcm430x_write16(bcm, 0x03EC, 0x3F3F);
		bcm430x_phy_write(bcm, 0x0035, regstack[4] & 0xFF7F);
		bcm430x_radio_write16(bcm, 0x007A, regstack[9] & 0xFFF0);
	}
	bcm430x_phy_write(bcm, 0x0015, 0xB000);
	bcm430x_phy_write(bcm, 0x002B, 0x0004);

	if (is_2053_radio) {
		bcm430x_phy_write(bcm, 0x002B, 0x0203);
		bcm430x_phy_write(bcm, 0x002A, 0x08A3);
	}

	phy->minlowsig[0] = 0xFFFF;

	for (i = 0; i < 4; i++) {
		bcm430x_radio_write16(bcm, 0x0052, regstack[1] | i);
		bcm430x_phy_lo_b_r15_loop(bcm);
	}
	for (i = 0; i < 10; i++) {
		bcm430x_radio_write16(bcm, 0x0052, regstack[1] | i);
		mls = bcm430x_phy_lo_b_r15_loop(bcm) / 10;
		if (mls < phy->minlowsig[0]) {
			phy->minlowsig[0] = mls;
			phy->minlowsigpos[0] = i;
		}
	}
	bcm430x_radio_write16(bcm, 0x0052, regstack[1] | phy->minlowsigpos[0]);

	phy->minlowsig[1] = 0xFFFF;

	for (i = -4; i < 5; i += 2) {
		for (j = -4; j < 5; j += 2) {
			if (j < 0)
				fval = (0x0100 * i) + j + 0x0100;
			else
				fval = (0x0100 * i) + j;
			bcm430x_phy_write(bcm, 0x002F, fval);
			mls = bcm430x_phy_lo_b_r15_loop(bcm) / 10;
			if (mls < phy->minlowsig[1]) {
				phy->minlowsig[1] = mls;
				phy->minlowsigpos[1] = fval;
			}
		}
	}
	phy->minlowsigpos[1] += 0x0101;

	bcm430x_phy_write(bcm, 0x002F, phy->minlowsigpos[1]);
	if (is_2053_radio) {
		bcm430x_phy_write(bcm, 0x000A, regstack[2]);
		bcm430x_phy_write(bcm, 0x002A, regstack[3]);
		bcm430x_phy_write(bcm, 0x0035, regstack[4]);
		bcm430x_phy_write(bcm, 0x0003, regstack[5]);
		bcm430x_phy_write(bcm, 0x0001, regstack[6]);
		bcm430x_phy_write(bcm, 0x0030, regstack[7]);

		bcm430x_radio_write16(bcm, 0x0043, regstack[8]);
		bcm430x_radio_write16(bcm, 0x007A, regstack[9]);

		bcm430x_radio_write16(bcm, 0x0052,
		                      (bcm430x_radio_read16(bcm, 0x0052) & 0x000F)
				      | regstack[11]);

		bcm430x_write16(bcm, 0x03EC, regstack[10]);
	}
	bcm430x_phy_write(bcm, 0x0015, regstack[0]);
}

static inline
u16 bcm430x_phy_lo_g_deviation_subval(struct bcm430x_private *bcm, u16 control)
{
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x15, 0xE300);
		control <<= 8;
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B0);
		udelay(5);
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B2);
		udelay(2);
		bcm430x_phy_write(bcm, 0x0812, control | 0x00B4);
		udelay(4);
		bcm430x_phy_write(bcm, 0x0015, 0xF300);
		udelay(8);
	} else {
		bcm430x_phy_write(bcm, 0x0015, control | 0xEFA0);
		udelay(2);
		bcm430x_phy_write(bcm, 0x0015, control | 0xEFE0);
		udelay(4);
		bcm430x_phy_write(bcm, 0x0015, control | 0xFFE0);
		udelay(8);
	}

	return bcm430x_phy_read(bcm, 0x002D);
}

static u32 bcm430x_phy_lo_g_singledeviation(struct bcm430x_private *bcm, u16 control)
{
	int i;
	u32 ret = 0;

	for (i = 0; i < 8; i++)
		ret += bcm430x_phy_lo_g_deviation_subval(bcm, control);

	return ret;
}

/* Write the LocalOscillator CONTROL */
static inline
void bcm430x_lo_write(struct bcm430x_private *bcm,
		      struct bcm430x_lopair *pair)
{
	u16 value;

	value = (u8)(pair->low);
	value |= ((u8)(pair->high)) << 8;

#ifdef BCM430x_DEBUG
	/* Sanity check. */
	if (pair->low < -8 || pair->low > 8 ||
	    pair->high < -8 || pair->high > 8) {
		printk(KERN_WARNING PFX
		       "WARNING: Writing invalid LOpair "
		       "(low: %d, high: %d, index: %lu)\n",
		       pair->low, pair->high,
		       (unsigned long)(pair - bcm->current_core->phy->_lo_pairs));
		dump_stack();
	}
#endif

	bcm430x_phy_write(bcm, BCM430x_PHY_G_LO_CONTROL, value);
}

static inline
struct bcm430x_lopair * bcm430x_find_lopair(struct bcm430x_private *bcm,
					    u16 baseband_attenuation,
					    u16 radio_attenuation,
					    u16 tx)
{
	const u8 dict[10] = { 11, 10, 11, 12, 13, 12, 13, 12, 13, 12 };
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;

	if (baseband_attenuation > 6)
		baseband_attenuation = 6;
	assert(radio_attenuation < 10);
	assert(tx == 0 || tx == 3);

	if (tx == 3) {
		return bcm430x_get_lopair(phy,
					  radio_attenuation,
					  baseband_attenuation);
	}
	return bcm430x_get_lopair(phy, dict[radio_attenuation], baseband_attenuation);
}

static inline
struct bcm430x_lopair * bcm430x_current_lopair(struct bcm430x_private *bcm)
{
	return bcm430x_find_lopair(bcm,
				   bcm->current_core->radio->txpower[0],
				   bcm->current_core->radio->txpower[1],
				   bcm->current_core->radio->txpower[2]);
}

/* Adjust B/G LO */
void bcm430x_phy_lo_adjust(struct bcm430x_private *bcm, int fixed)
{
	struct bcm430x_lopair *pair;

	if (fixed) {
		/* Use fixed values. Only for initialization. */
		pair = bcm430x_find_lopair(bcm, 2, 3, 0);
	} else
		pair = bcm430x_current_lopair(bcm);
	bcm430x_lo_write(bcm, pair);
}

static inline
void bcm430x_phy_lo_g_measure_txctl2(struct bcm430x_private *bcm)
{
	u16 txctl2 = 0, i;
	u32 smallest, tmp;

	bcm430x_radio_write16(bcm, 0x0052, 0x0000);
	udelay(10);
	smallest = bcm430x_phy_lo_g_singledeviation(bcm, 0);
	for (i = 0; i < 16; i++) {
		bcm430x_radio_write16(bcm, 0x0052, i);
		udelay(10);
		tmp = bcm430x_phy_lo_g_singledeviation(bcm, 0);
		if (tmp < smallest) {
			smallest = tmp;
			txctl2 = i;
		}
	}
	bcm->current_core->radio->txpower[3] = txctl2;
}

static
void bcm430x_phy_lo_g_state(struct bcm430x_private *bcm,
			    const struct bcm430x_lopair *in_pair,
			    struct bcm430x_lopair *out_pair,
			    u16 r27)
{
	struct bcm430x_lopair transitions[8] = {
		{ .high =  1,  .low =  1, },
		{ .high =  1,  .low =  0, },
		{ .high =  1,  .low = -1, },
		{ .high =  0,  .low = -1, },
		{ .high = -1,  .low = -1, },
		{ .high = -1,  .low =  0, },
		{ .high = -1,  .low =  1, },
		{ .high =  0,  .low =  1, },
	};
	struct bcm430x_lopair transition;
	struct bcm430x_lopair result = {
		.high = in_pair->high,
		.low = in_pair->low,
	};
	int i = 12, j, lowered = 1, state = 0;
	int index;
	u32 deviation, tmp;

	/* Note that in_pair and out_pair can point to the same pair. Be careful. */

	deviation = bcm430x_phy_lo_g_singledeviation(bcm, r27);
	while ((i--) && (lowered == 1)) {
		lowered = 0;
		assert(state >= 0 && state <= 8);
		if (state == 0) {
			/* Initial state */
			for (j = 0; j < 8; j++) {
				index = j;
				transition.high = in_pair->high + transitions[index].high;
				transition.low = in_pair->low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}
		} else if (state % 2 == 0) {
			for (j = -1; j < 2; j += 2) {
				index = state + j;
				assert(index >= 1 && index <= 9);
				if (index > 8)
					index = 1;
				index -= 1;
				transition.high = in_pair->high + transitions[index].high;
				transition.low = in_pair->low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}
		} else {
			for (j = -2; j < 3; j += 4) {
				index = state + j;
				assert(index >= -1 && index <= 9);
				if (index > 8)
					index = 1;
				else if (index < 1)
					index = 7;
				index -= 1;
				transition.high = in_pair->high + transitions[index].high;
				transition.low = in_pair->low + transitions[index].low;
				if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
					bcm430x_lo_write(bcm, &transition);
					tmp = bcm430x_phy_lo_g_singledeviation(bcm, r27);
					if (tmp < deviation) {
						deviation = tmp;
						state = index + 1;
						lowered = 1;

						result.high = transition.high;
						result.low = transition.low;
					}
				}
			}	
		}
	}
	out_pair->high = result.high;
	out_pair->low = result.low;
}

/* Set the baseband attenuation value on chip. */
void bcm430x_phy_set_baseband_attenuation(struct bcm430x_private *bcm,
					  u16 baseband_attenuation)
{
	u16 value;

	if (bcm->current_core->phy->version == 0) {
		value = (bcm430x_read16(bcm, 0x03E6) & 0xFFF0);
		value |= (baseband_attenuation & 0x000F);
		bcm430x_write16(bcm, 0x03E6, value);
		return;
	}

	if (bcm->current_core->phy->version > 1) {
		value = bcm430x_phy_read(bcm, 0x0060) & ~0x003C;
		value |= (baseband_attenuation << 2) & 0x003C;
	} else {
		value = bcm430x_phy_read(bcm, 0x0060) & ~0x0078;
		value |= (baseband_attenuation << 3) & 0x0078;
	}
	bcm430x_phy_write(bcm, 0x0060, value);
}

/* http://bcm-specs.sipsolutions.net/LocalOscillator/Measure */
void bcm430x_phy_lo_g_measure(struct bcm430x_private *bcm)
{
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;
	u16 h, i, oldi, j;
	const struct bcm430x_lopair *control;
	struct bcm430x_lopair *tmp_control;
	const u8 pairorder[10] = { 3, 1, 5, 7, 9, 2, 0, 4, 6, 8 };
	u16 tmp;
	u16 regstack[16] = { 0 };
	u8 oldchannel;

	//XXX: What are these?
	u8 r27, r31;

	oldchannel = bcm->current_core->radio->channel;
	/* Setup */
	if (phy->connected) {
		regstack[0] = bcm430x_phy_read(bcm, 0x0429);
		regstack[1] = bcm430x_phy_read(bcm, 0x0802);
		bcm430x_phy_write(bcm, 0x0429, regstack[0] & 0x3FFF);
		bcm430x_phy_write(bcm, 0x0802, regstack[1] & 0xFFFC);
	}
	regstack[3] = bcm430x_read16(bcm, 0x03E2);
	bcm430x_write16(bcm, 0x03E2, regstack[3] | 0x8000);
	regstack[4] = bcm430x_read16(bcm, 0x03F4);
	regstack[5] = bcm430x_phy_read(bcm, 0x15);
	regstack[6] = bcm430x_phy_read(bcm, 0x2A);
	regstack[7] = bcm430x_phy_read(bcm, 0x35);
	regstack[8] = bcm430x_phy_read(bcm, 0x60);
	regstack[9] = bcm430x_radio_read16(bcm, 0x43);
	regstack[10] = bcm430x_radio_read16(bcm, 0x7A);
	regstack[11] = bcm430x_radio_read16(bcm, 0x52);
	if (phy->connected) {
		regstack[12] = bcm430x_phy_read(bcm, 0x0811);
		regstack[13] = bcm430x_phy_read(bcm, 0x0812);
		regstack[14] = bcm430x_phy_read(bcm, 0x0814);
		regstack[15] = bcm430x_phy_read(bcm, 0x0815);
	}
	bcm430x_radio_selectchannel(bcm, 6, 0);
	if (phy->connected) {
		bcm430x_phy_write(bcm, 0x0429, regstack[0] & 0x7FFF);
		bcm430x_phy_write(bcm, 0x0802, regstack[1] & 0xFFFC);
		bcm430x_dummy_transmission(bcm);
	}
	bcm430x_radio_write16(bcm, 0x0043, 0x0006);

	bcm430x_phy_set_baseband_attenuation(bcm, 2);

	bcm430x_write16(bcm, 0x03F4, 0x0000);
	bcm430x_phy_write(bcm, 0x002E, 0x007F);
	bcm430x_phy_write(bcm, 0x080F, 0x0078);
	bcm430x_phy_write(bcm, 0x0035, regstack[7] & ~(1 << 7));
	bcm430x_radio_write16(bcm, 0x007A, regstack[10] & 0xFFF0);
	bcm430x_phy_write(bcm, 0x002B, 0x0203);
	bcm430x_phy_write(bcm, 0x002A, 0x08A3);
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x0814, regstack[14] | 0x0003);
		bcm430x_phy_write(bcm, 0x0815, regstack[15] & 0xFFFC);
		bcm430x_phy_write(bcm, 0x0811, 0x01B3);
		bcm430x_phy_write(bcm, 0x0812, 0x00B2);
	}
	if (bcm430x_is_initializing(bcm))
		bcm430x_phy_lo_g_measure_txctl2(bcm);
	bcm430x_phy_write(bcm, 0x080F, 0x8078);

	/* Measure */
	for (h = 0; h < 10; h++) {
		/* Loop over each possible RadioAttenuation (0-9) */
		i = pairorder[h];
		if (bcm430x_is_initializing(bcm)) {//FIXME: This all seems very useless, as it is overridden at the j loop.
			if (i == 3)
				control = bcm430x_get_lopair(phy, 0, 0);
			else if (((i % 2 == 1) && (oldi % 2 == 1)) ||
				 ((i % 2 == 0) && (oldi % 2 == 0)))//FIXME: what for the oldi uninitialized case?
				control = bcm430x_get_lopair(phy, i, 0);
			else
				control = bcm430x_get_lopair(phy, 3, 0);
		}
		/* Loop over each possible BasebandAttenuation/2 */
		for (j = 0; j < 4; j++) {
			control = bcm430x_get_lopair(phy, i, j * 2);
			if (bcm430x_is_initializing(bcm)) {
				tmp = i * 2 + j;
				r27 = 0;
				if (tmp > 14) {
					r31 = 1;
					if (tmp > 17)
						r27 = 1;
					if (tmp > 19)
						r27 = 2;
				} else
					r31 = 0;
			} else {
				r27 = 3;
				r31 = 1;
			}
			bcm430x_radio_write16(bcm, 0x43, i);
			bcm430x_radio_write16(bcm, 0x52,
					      bcm->current_core->radio->txpower[3]);
			udelay(10);

			bcm430x_phy_set_baseband_attenuation(bcm, j * 2);

			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm430x_radio_write16(bcm, 0x007A, tmp);

			tmp_control = bcm430x_get_lopair(phy, i, j * 2);
			bcm430x_phy_lo_g_state(bcm, control, tmp_control, r27);
		}
		oldi = i;
	}
	/* Loop over each possible RadioAttenuation (10-13) */
	for (i = 10; i < 14; i++) {
		/* Loop over each possible BasebandAttenuation/2 */
		for (j = 0; j < 4; j++) {
			if (bcm430x_is_initializing(bcm)) {
				tmp = i * 2 + j - 5;
				r27 = 0;
				r31 = 0;
				if (tmp > 14) {
					r31 = 1;
					if (tmp > 17)
						r27 = 1;
					if (tmp > 19)
						r27 = 2;
				}
			} else {
				control = bcm430x_get_lopair(phy, i, j * 2);
				r27 = 3;
				r31 = 1;
			}
			bcm430x_radio_write16(bcm, 0x43, i);
			bcm430x_radio_write16(bcm, 0x52,
					      bcm->current_core->radio->txpower[3]
					      | (3/*txctl1*/ << 4));//FIXME: shouldn't txctl1 be zero here and 3 in the loop above?
			udelay(10);

			bcm430x_phy_set_baseband_attenuation(bcm, j * 2);

			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm430x_radio_write16(bcm, 0x7A, tmp);

			tmp_control = bcm430x_get_lopair(phy, i, j * 2);
			bcm430x_phy_lo_g_state(bcm, control, tmp_control, r27);
		}
	}

	/* Restoration */
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x0015, 0xE300);
		bcm430x_phy_write(bcm, 0x0812, (r27 << 8) | 0xA0);
		udelay(5);
		bcm430x_phy_write(bcm, 0x0812, (r27 << 8) | 0xA2);
		udelay(2);
		bcm430x_phy_write(bcm, 0x0812, (r27 << 8) | 0xA3);
	} else
		bcm430x_phy_write(bcm, 0x0015, r27 | 0xEFA0);
	bcm430x_phy_lo_adjust(bcm, bcm430x_is_initializing(bcm));
	bcm430x_phy_write(bcm, 0x002E, 0x807F);
	if (phy->connected)
		bcm430x_phy_write(bcm, 0x002F, 0x0202);
	else
		bcm430x_phy_write(bcm, 0x002F, 0x0101);
	bcm430x_write16(bcm, 0x03F4, regstack[4]);
	bcm430x_phy_write(bcm, 0x0015, regstack[5]);
	bcm430x_phy_write(bcm, 0x002A, regstack[6]);
	bcm430x_phy_write(bcm, 0x0035, regstack[7]);
	bcm430x_phy_write(bcm, 0x0060, regstack[8]);
	bcm430x_radio_write16(bcm, 0x0043, regstack[9]);
	bcm430x_radio_write16(bcm, 0x007A, regstack[10]);
	regstack[11] &= 0x00F0;
	regstack[11] |= (bcm430x_radio_read16(bcm, 0x52) & 0x000F);
	bcm430x_radio_write16(bcm, 0x52, regstack[11]);
	bcm430x_write16(bcm, 0x03E3, regstack[3]);
	if (phy->connected) {
		bcm430x_phy_write(bcm, 0x0811, regstack[12]);
		bcm430x_phy_write(bcm, 0x0812, regstack[13]);
		bcm430x_phy_write(bcm, 0x0814, regstack[14]);
		bcm430x_phy_write(bcm, 0x0815, regstack[15]);
		bcm430x_phy_write(bcm, 0x0429, regstack[0]);
		bcm430x_phy_write(bcm, 0x0802, regstack[1]);
	}
	bcm430x_radio_selectchannel(bcm, oldchannel, 1);

#ifdef BCM430x_DEBUG
	{
		/* Sanity check for all lopairs. */
		for (i = 0; i < BCM430x_LO_COUNT; i++) {
			control = bcm->current_core->phy->_lo_pairs + i;
			if (control->low < -8 || control->low > 8 ||
			    control->high < -8 || control->high > 8) {
				printk(KERN_WARNING PFX
				       "WARNING: Invalid LOpair (low: %d, high: %d, index: %d)\n",
				       control->low, control->high, i);
			}
		}
	}
#endif
}

static
void bcm430x_phy_lo_mark_current_used(struct bcm430x_private *bcm)
{
	struct bcm430x_lopair *pair;

	pair = bcm430x_current_lopair(bcm);
	pair->used = 1;
}

void bcm430x_phy_lo_mark_all_unused(struct bcm430x_private *bcm)
{
	struct bcm430x_lopair *pair;
	int i;

	for (i = 0; i < BCM430x_LO_COUNT; i++) {
		pair = bcm->current_core->phy->_lo_pairs + i;
		pair->used = 0;
	}
}

/* http://bcm-specs.sipsolutions.net/EstimatePowerOut
 * This function converts a TSSI value to dBm.
 */
static s8 bcm430x_phy_estimate_power_out(struct bcm430x_private *bcm, s8 tssi)
{
	s8 tssi2dbm = 0;
	s32 tmp;

	tmp = bcm->current_core->phy->idle_tssi;
	tmp += tssi;
	tmp -= (s8)(bcm->current_core->phy->savedpctlreg);

	switch (bcm->current_core->phy->type) {
		case BCM430x_PHYTYPE_A:
			tmp += 0x80;
			tmp = limit_value(tmp, 0x00, 0xFF);
			tssi2dbm = bcm->current_core->phy->tssi2dbm[tmp];
			TODO(); //TODO: There's a FIXME on the specs
			break;
		case BCM430x_PHYTYPE_B:
		case BCM430x_PHYTYPE_G:
			tmp = limit_value(tmp, 0x00, 0x3F);
			tssi2dbm = bcm->current_core->phy->tssi2dbm[tmp];
			break;
		default:
			assert(0);
	}

	return tssi2dbm;
}

/* http://bcm-specs.sipsolutions.net/RecalculateTransmissionPower */
void bcm430x_phy_xmitpower(struct bcm430x_private *bcm)
{
	u16 tmp;
	u16 txpower;
	s8 v0, v1, v2, v3;
	s8 average;
	u8 max_pwr;
	s16 desired_pwr, estimated_pwr, pwr_adjust;
	s16 radio_att_delta, baseband_att_delta;
	s16 radio_attenuation, baseband_attenuation;

	if (bcm->current_core->phy->savedpctlreg == 0xFFFF)
		return;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:

		TODO(); //TODO: Nothing for A PHYs yet :-/

		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		if ((bcm->board_type == 0x0416) &&
		    (bcm->board_vendor == 0x106B/*FIXME: Board Vendor Broadcom*/))
			return;

		tmp = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x0058);
		v0 = (s8)(tmp & 0x00FF);
		v1 = (s8)((tmp & 0xFF00) >> 8);
		tmp = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x005A);
		v2 = (s8)(tmp & 0x00FF);
		v3 = (s8)((tmp & 0xFF00) >> 8);
		if ((v0 == 0x7F && v1 == 0x7F) || (v2 == 0x7F && v3 == 0x7F)) {
			tmp = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x0070);
			v0 = (s8)(tmp & 0x00FF);
			v1 = (s8)((tmp & 0xFF00) >> 8);
			tmp = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x0072);
			v2 = (s8)(tmp & 0x00FF);
			v3 = (s8)((tmp & 0xFF00) >> 8);
			if ((v0 == 0x7F && v1 == 0x7F) || (v2 == 0x7F && v3 == 0x7F))
				return;
			v0 = (v0 + 0x20) & 0x3F;
			v1 = (v1 + 0x20) & 0x3F;
			v2 = (v2 + 0x20) & 0x3F;
			v3 = (v3 + 0x20) & 0x3F;
		}
		bcm430x_radio_clear_tssi(bcm);

		average = (v0 + v1 + v2 + v3 + 2) / 4;
		//TODO: If FIXME, substract 13
		estimated_pwr = bcm430x_phy_estimate_power_out(bcm, average);

		//FIXME: Is this adjustment correct?
		max_pwr = bcm->sprom.maxpower_bgphy;
		if (!(bcm->sprom.boardflags & BCM430x_BFL_PACTRL))
			max_pwr -= 0xC;
		desired_pwr = min((u16)(estimated_pwr + (estimated_pwr * bcm->sprom.antennagain_bgphy)),
				  (u16)max_pwr);

		pwr_adjust = estimated_pwr - desired_pwr;
		radio_att_delta = -(pwr_adjust + 7) / 8;
		baseband_att_delta = -(pwr_adjust / 2) - (4 * radio_att_delta);
		if ((radio_att_delta == 0) && (baseband_att_delta == 0)) {
			bcm430x_phy_lo_mark_current_used(bcm);
			return;
		}

		/* Calculate the new attenuation values. */
		baseband_attenuation = bcm->current_core->radio->txpower[0];
		baseband_attenuation += baseband_att_delta;
		radio_attenuation = bcm->current_core->radio->txpower[1];
		radio_attenuation += radio_att_delta;

		/* baseband_attenuation affects the lower level 4 times as
		 * much as radio attenuation. So adjust them.
		 */
		while (baseband_attenuation < 1 && radio_attenuation > 0) {
			radio_attenuation--;
			baseband_attenuation += 4;
		}
		/* adjust maximum values */
		while (baseband_attenuation > 5 && radio_attenuation < 9) {
			baseband_attenuation -= 4;
			radio_attenuation++;
		}
		if (radio_attenuation < 0)
			radio_attenuation = 0;

		txpower = bcm->current_core->radio->txpower[2];
		if (bcm->current_core->radio->version == 0x2050) {
			if (radio_attenuation == 0) {
				if (txpower == 0) {
					txpower = 3;
					radio_attenuation += 2;
					baseband_attenuation += 2;
				} else if (!(bcm->sprom.boardflags & BCM430x_BFL_PACTRL)) {
					baseband_attenuation += 4 * (radio_attenuation - 2);
					radio_attenuation = 2;
				}
			} else if (radio_attenuation > 4 && txpower != 0) {
				txpower = 0;
				if (baseband_attenuation < 3) {
					radio_attenuation -= 3;
					baseband_attenuation += 2;
				} else {
					radio_attenuation -= 2;
					baseband_attenuation -= 2;
				}
			}
		}
		baseband_attenuation = limit_value(baseband_attenuation, 0, 11);
		radio_attenuation = limit_value(radio_attenuation, 0, 9);

		bcm430x_phy_lock(bcm);
		bcm430x_radio_lock(bcm);
		bcm430x_radio_set_txpower_bg(bcm, baseband_attenuation,
					     radio_attenuation, txpower);
		bcm430x_phy_lo_mark_current_used(bcm);
		bcm430x_radio_unlock(bcm);
		bcm430x_phy_unlock(bcm);
		break;
	default:
		assert(0);
	}
}

/* http://bcm-specs.sipsolutions.net/TSSI_to_DBM_Table */
int bcm430x_phy_init_tssi2dbm_table(struct bcm430x_private *bcm)
{
	s16 pab0, pab1, pab2;

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		pab0 = (s16)(bcm->sprom.pa0b0);
		pab1 = (s16)(bcm->sprom.pa0b1);
		pab2 = (s16)(bcm->sprom.pa0b2);
	} else {
		pab0 = (s16)(bcm->sprom.pa1b0);
		pab1 = (s16)(bcm->sprom.pa1b1);
		pab2 = (s16)(bcm->sprom.pa1b2);
	}

	if ((bcm->chip_id == 0x4301) && (bcm->current_core->radio->version != 0x2050)) {
		bcm->current_core->phy->idle_tssi = 0x34;
		bcm->current_core->phy->tssi2dbm = bcm430x_tssi2dbm_b_table;
		return 0;
	}
	if ((bcm->chip_id == 0x4306) || (bcm->chip_id == 0x4301)) {
		if (pab0 != 0 && pab1 != 0 && pab2 != 0 &&
		    pab0 != -1 && pab1 != -1 && pab2 != -1) {
			/* The pabX values are set in SPROM. Use them. */
			if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
				bcm->current_core->phy->idle_tssi = (s8)(bcm->sprom.idle_tssi_tgt_aphy);
			else
				bcm->current_core->phy->idle_tssi = (s8)(bcm->sprom.idle_tssi_tgt_bgphy);
			if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
				TODO(); //TODO: incomplete specs.
			}
			TODO(); //TODO: incomplete specs.
			return -ENODEV;
		} else {
			/* pabX values not set in SPROM. */
			switch (bcm->current_core->phy->type) {
			case BCM430x_PHYTYPE_A:
				/* APHY needs a generated table. */
				bcm->current_core->phy->tssi2dbm = NULL;
				printk(KERN_ERR PFX "Could not generate tssi2dBm "
						    "table (wrong SPROM info)!\n");
				return -ENODEV;
			case BCM430x_PHYTYPE_B:
				bcm->current_core->phy->idle_tssi = 0x34;
				bcm->current_core->phy->tssi2dbm = bcm430x_tssi2dbm_b_table;
				break;
			case BCM430x_PHYTYPE_G:
				bcm->current_core->phy->idle_tssi = 0x34;
				bcm->current_core->phy->tssi2dbm = bcm430x_tssi2dbm_g_table;
				break;
			}
		}
	}

	return 0;
}

int bcm430x_phy_init(struct bcm430x_private *bcm)
{
	int initialized = 0;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		if ((bcm->current_core->phy->rev == 2) || (bcm->current_core->phy->rev == 3)) {
			bcm430x_phy_inita(bcm);
			initialized = 1;
		}
		break;
	case BCM430x_PHYTYPE_B:
		switch (bcm->current_core->phy->rev) {
		case 2:
			bcm430x_phy_initb2(bcm);
			initialized = 1;
			break;
		case 4:
			bcm430x_phy_initb4(bcm);
			initialized = 1;
			break;
		case 5:
			bcm430x_phy_initb5(bcm);
			initialized = 1;
			break;
		case 6:
			bcm430x_phy_initb6(bcm);
			initialized = 1;
			break;
		}
		break;
	case BCM430x_PHYTYPE_G:
		bcm430x_phy_initg(bcm);
		initialized = 1;
		break;
	}

	if (!initialized) {
		printk(KERN_WARNING PFX "Unknown PHYTYPE found!\n");
		return -1;
	}

	return 0;
}

void bcm430x_phy_set_antenna_diversity(struct bcm430x_private *bcm)
{
	u16 antennadiv;
	u16 offset;
	u32 ucodeflags;

	antennadiv = bcm->current_core->phy->antenna_diversity;

	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_A) && antennadiv == 3)
		antennadiv = 0;
	if (antennadiv == 0xFFFF)
		antennadiv = 3;

	assert(antennadiv <= 3);

	ucodeflags = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED,
					BCM430x_UCODEFLAGS_OFFSET);
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED,
			    BCM430x_UCODEFLAGS_OFFSET,
			    ucodeflags & ~BCM430x_UCODEFLAG_AUTODIV);

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
	case BCM430x_PHYTYPE_G:
		if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
			offset = 0x0000;
		else
			offset = 0x0400;

		if (antennadiv == 2) {
			bcm430x_phy_write(bcm, offset + 1,
					  (bcm430x_phy_read(bcm, offset + 1)
					   & 0x7E7F) | (antennadiv << 7));
		} else {
			bcm430x_phy_write(bcm, offset + 1,
					  (bcm430x_phy_read(bcm, offset + 1)
					   & 0x7E7F) | (3/*automatic*/ << 7));
		}
		if (antennadiv == 2) {
			bcm430x_phy_write(bcm, offset + 0x2B,
					  (bcm430x_phy_read(bcm, offset + 0x2B)
					   & 0xFEFF) | (antennadiv << 7));
		} else if (antennadiv > 2) {
			bcm430x_phy_write(bcm, offset + 0x2B,
					  (bcm430x_phy_read(bcm, offset + 0x2B)
					   & 0xFEFF) | 0/*force0*/);
		}

		if (!bcm->current_core->phy->connected) {
			if (antennadiv < 2) {
				bcm430x_phy_write(bcm, 0x048C,
						  bcm430x_phy_read(bcm, 0x048C) & 0xDFFF);
			} else {
				bcm430x_phy_write(bcm, 0x048C,
						  (bcm430x_phy_read(bcm, 0x048C)
						   & 0xDFFF) | 0x2000);
			}
			if (bcm->current_core->phy->rev >= 2) {
				bcm430x_phy_write(bcm, 0x0461,
						  bcm430x_phy_read(bcm, 0x0461) | 0x0010);
				bcm430x_phy_write(bcm, 0x04AD,
						  (bcm430x_phy_read(bcm, 0x04AD) & 0xFF00) | 0x0015);
				bcm430x_phy_write(bcm, 0x0427, 0x0008);
			}
		}
		break;
	case BCM430x_PHYTYPE_B:
		if (bcm->current_core->rev == 2) {
			bcm430x_phy_write(bcm, 0x03E2,
					  (bcm430x_phy_read(bcm, 0x03E2)
					   & 0xFE7F) | (3/*automatic*/ << 7));
		} else {
			if (antennadiv == 2) {
				bcm430x_phy_write(bcm, 0x03E2,
						  (bcm430x_phy_read(bcm, 0x03E2)
						   & 0xFE7F) | (3/*automatic*/ << 7));
			} else {
				bcm430x_phy_write(bcm, 0x03E2,
						  (bcm430x_phy_read(bcm, 0x03E2)
						   & 0xFE7F) | (antennadiv << 7));
			}
		}
		break;
	default:
                printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return;
	}

	if (antennadiv >= 2) {
		ucodeflags = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED,
						BCM430x_UCODEFLAGS_OFFSET);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED,
				    BCM430x_UCODEFLAGS_OFFSET,
				    ucodeflags |  BCM430x_UCODEFLAG_AUTODIV);
	}

	bcm->current_core->phy->antenna_diversity = antennadiv;
}
