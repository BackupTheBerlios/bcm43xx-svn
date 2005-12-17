/*

  Broadcom BCM43xx wireless driver

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

#include "bcm43xx.h"
#include "bcm43xx_phy.h"
#include "bcm43xx_main.h"
#include "bcm43xx_radio.h"
#include "bcm43xx_ilt.h"
#include "bcm43xx_power.h"


static const s8 bcm43xx_tssi2dbm_b_table[] = {
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

static const s8 bcm43xx_tssi2dbm_g_table[] = {
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

static void bcm43xx_phy_initg(struct bcm43xx_private *bcm);


void bcm43xx_phy_lock(struct bcm43xx_private *bcm)
{
	assert(!in_interrupt());
	if (bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD) == 0x00000000)
		return;
	if (bcm->current_core->rev < 3) {
		bcm43xx_mac_suspend(bcm);
		spin_lock(&bcm->current_core->phy->lock);
	} else {
		if (bcm->iw_mode == IW_MODE_MASTER)
			return;
		bcm43xx_power_saving_ctl_bits(bcm, -1, 1);
	}
}

void bcm43xx_phy_unlock(struct bcm43xx_private *bcm)
{
	assert(!in_interrupt());
	if (bcm->current_core->rev < 3) {
		if (!spin_is_locked(&bcm->current_core->phy->lock))
			return;
		spin_unlock(&bcm->current_core->phy->lock);
		bcm43xx_mac_enable(bcm);
	} else {
		if (bcm->iw_mode == IW_MODE_MASTER)
			return;
		bcm43xx_power_saving_ctl_bits(bcm, -1, -1);
	}
}

u16 bcm43xx_phy_read(struct bcm43xx_private *bcm, u16 offset)
{
	bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_CONTROL, offset);
	return bcm43xx_read16(bcm, BCM43xx_MMIO_PHY_DATA);
}

void bcm43xx_phy_write(struct bcm43xx_private *bcm, u16 offset, u16 val)
{
	bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_CONTROL, offset);
	bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_DATA, val);
}

void bcm43xx_phy_calibrate(struct bcm43xx_private *bcm)
{
	unsigned long flags;

	bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD); // Dummy read
	if (bcm->current_core->phy->calibrated)
		return;

	/* We do not want to be preempted while calibrating
	 * the hardware.
	 */
	local_irq_save(flags);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A)
		bcm43xx_radio_set_txpower_a(bcm, 0x0018);
	if ((bcm->current_core->phy->type == BCM43xx_PHYTYPE_G)
	    && (bcm->current_core->phy->rev == 1)) {
		bcm43xx_wireless_core_reset(bcm, 0);
		bcm43xx_phy_initg(bcm);
		bcm43xx_wireless_core_reset(bcm, 1);
	}
	bcm->current_core->phy->calibrated = 1;

	local_irq_restore(flags);
}

/* Connect the PHY 
 * http://bcm-specs.sipsolutions.net/SetPHY
 */
int bcm43xx_phy_connect(struct bcm43xx_private *bcm, int connect)
{
	u32 flags;

	if (bcm->current_core->rev < 5)
		return 0;

	flags = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATEHIGH);
	if (connect) {
		if (!(flags & 0x00010000))
			return -ENODEV;
		bcm->current_core->phy->connected = 1;

		flags = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
		flags |= (0x800 << 18);
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, flags);
		dprintk(KERN_INFO PFX "PHY connected\n");
	} else {
		if (!(flags & 0x00020000))
			return -ENODEV;
		bcm->current_core->phy->connected = 0;

		flags = bcm43xx_read32(bcm, BCM43xx_CIR_SBTMSTATELOW);
		flags &= ~(0x800 << 18);
		bcm43xx_write32(bcm, BCM43xx_CIR_SBTMSTATELOW, flags);
		dprintk(KERN_INFO PFX "PHY disconnected\n");
	}

	return 0;
}

/* intialize B PHY power control
 * as described in http://bcm-specs.sipsolutions.net/InitPowerControl
 */
static void bcm43xx_phy_init_pctl(struct bcm43xx_private *bcm)
{
	u16 t_batt = 0xFFFF;
	u16 t_ratt = 0xFFFF;
	u16 t_txatt = 0xFFFF;

	assert(bcm->current_core->phy->type != BCM43xx_PHYTYPE_A);

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) &&
	    (bcm->board_type == 0x0416))
		return;

	bcm43xx_write16(bcm, 0x03E6, bcm43xx_read16(bcm, 0x03E6) & 0xFFDF);
	bcm43xx_phy_write(bcm, 0x0028, 0x8018);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) {
		if (bcm->current_core->phy->connected)
			bcm43xx_phy_write(bcm, 0x047A, 0xC111);
		else
			return;
	}
	if (bcm->current_core->phy->savedpctlreg != 0xFFFF)
		return;

	if ((bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) &&
	    (bcm->current_core->phy->rev >= 2) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm43xx_radio_write16(bcm, 0x0076,
				      bcm43xx_radio_read16(bcm, 0x0076) | 0x0084);
	} else {
		t_batt = bcm->current_core->radio->txpower[0];
		t_ratt = bcm->current_core->radio->txpower[1];
		t_txatt = bcm->current_core->radio->txpower[2];
		bcm43xx_radio_set_txpower_bg(bcm, 0x000B, 0x0009, 0x0000);
	}

	bcm43xx_dummy_transmission(bcm);

	bcm->current_core->phy->savedpctlreg = bcm43xx_phy_read(bcm, BCM43xx_PHY_G_PCTL);

	if ((bcm->current_core->phy->type != BCM43xx_PHYTYPE_B) ||
	    (bcm->current_core->phy->rev < 2) ||
	    (bcm->current_core->radio->version != 0x2050))
		bcm43xx_radio_set_txpower_bg(bcm, t_batt, t_ratt, t_txatt);

	bcm43xx_radio_write16(bcm, 0x0076, bcm43xx_radio_read16(bcm, 0x0076) & 0xFF7B);

	bcm43xx_radio_clear_tssi(bcm);
}

static void bcm43xx_phy_agcsetup(struct bcm43xx_private *bcm)
{
	u16 offset = 0x0000;

	if (bcm->current_core->phy->rev == 1)
		offset = 0x4C00;

	bcm43xx_ilt_write16(bcm, offset, 0x00FE);
	bcm43xx_ilt_write16(bcm, offset + 1, 0x000D);
	bcm43xx_ilt_write16(bcm, offset + 2, 0x0013);
	bcm43xx_ilt_write16(bcm, offset + 3, 0x0019);

	if (bcm->current_core->phy->rev == 1) {
		bcm43xx_ilt_write16(bcm, 0x1800, 0x2710);
		bcm43xx_ilt_write16(bcm, 0x1801, 0x9B83);
		bcm43xx_ilt_write16(bcm, 0x1802, 0x9B83);
		bcm43xx_ilt_write16(bcm, 0x1803, 0x0F8D);
		bcm43xx_phy_write(bcm, 0x0455, 0x0004);
	}

	bcm43xx_phy_write(bcm, 0x04A5, (bcm43xx_phy_read(bcm, 0x04A5) & 0x00FF) | 0x5700);
	bcm43xx_phy_write(bcm, 0x041A, (bcm43xx_phy_read(bcm, 0x041A) & 0xFF80) | 0x000F);
	bcm43xx_phy_write(bcm, 0x041A, (bcm43xx_phy_read(bcm, 0x041A) & 0xC07F) | 0x2B80);
	bcm43xx_phy_write(bcm, 0x048C, (bcm43xx_phy_read(bcm, 0x048C) & 0xF0FF) | 0x0300);

	bcm43xx_radio_write16(bcm, 0x007A, bcm43xx_radio_read16(bcm, 0x007A) | 0x0008);

	bcm43xx_phy_write(bcm, 0x04A0, (bcm43xx_phy_read(bcm, 0x04A0) & 0xFFF0) | 0x0008);
	bcm43xx_phy_write(bcm, 0x04A1, (bcm43xx_phy_read(bcm, 0x04A1) & 0xF0FF) | 0x0600);
	bcm43xx_phy_write(bcm, 0x04A2, (bcm43xx_phy_read(bcm, 0x04A2) & 0xF0FF) | 0x0700);
	bcm43xx_phy_write(bcm, 0x04A0, (bcm43xx_phy_read(bcm, 0x04A0) & 0xF0FF) | 0x0100);

	if (bcm->current_core->phy->rev == 1)
		bcm43xx_phy_write(bcm, 0x04A2, (bcm43xx_phy_read(bcm, 0x04A2) & 0xFFF0) | 0x0007);

	bcm43xx_phy_write(bcm, 0x0488, (bcm43xx_phy_read(bcm, 0x0488) & 0xFF00) | 0x001C);
	bcm43xx_phy_write(bcm, 0x0488, (bcm43xx_phy_read(bcm, 0x0488) & 0xC0FF) | 0x0200);
	bcm43xx_phy_write(bcm, 0x0496, (bcm43xx_phy_read(bcm, 0x0496) & 0xFF00) | 0x001C);
	bcm43xx_phy_write(bcm, 0x0489, (bcm43xx_phy_read(bcm, 0x0489) & 0xFF00) | 0x0020);
	bcm43xx_phy_write(bcm, 0x0489, (bcm43xx_phy_read(bcm, 0x0489) & 0xC0FF) | 0x0200);
	bcm43xx_phy_write(bcm, 0x0482, (bcm43xx_phy_read(bcm, 0x0482) & 0xFF00) | 0x002E);
	bcm43xx_phy_write(bcm, 0x0496, (bcm43xx_phy_read(bcm, 0x0496) & 0x00FF) | 0x1A00);
	bcm43xx_phy_write(bcm, 0x0481, (bcm43xx_phy_read(bcm, 0x0481) & 0xFF00) | 0x0028);
	bcm43xx_phy_write(bcm, 0x0481, (bcm43xx_phy_read(bcm, 0x0481) & 0x00FF) | 0x2C00);

	if (bcm->current_core->phy->rev == 1) {
		bcm43xx_phy_write(bcm, 0x0430, 0x092B);
		bcm43xx_phy_write(bcm, 0x041B, (bcm43xx_phy_read(bcm, 0x041B) & 0xFFE1) | 0x0002);
	} else {
		bcm43xx_phy_write(bcm, 0x041B, bcm43xx_phy_read(bcm, 0x041B) & 0xFFE1);
		bcm43xx_phy_write(bcm, 0x041F, 0x287A);
		bcm43xx_phy_write(bcm, 0x0420, (bcm43xx_phy_read(bcm, 0x0420) & 0xFFF0) | 0x0004);
	}

	if (bcm->current_core->phy->rev > 2) {
		bcm43xx_phy_write(bcm, 0x0422, 0x287A);
		bcm43xx_phy_write(bcm, 0x0420, (bcm43xx_phy_read(bcm, 0x0420) & 0x0FFF) | 0x3000); 
	}
		
	bcm43xx_phy_write(bcm, 0x04A8, (bcm43xx_phy_read(bcm, 0x04A8) & 0x8080) | 0x7874);
	bcm43xx_phy_write(bcm, 0x048E, 0x1C00);

	if (bcm->current_core->phy->rev == 1) {
		bcm43xx_phy_write(bcm, 0x04AB, (bcm43xx_phy_read(bcm, 0x04AB) & 0xF0FF) | 0x0600);
		bcm43xx_phy_write(bcm, 0x048B, 0x005E);
		bcm43xx_phy_write(bcm, 0x048C, (bcm43xx_phy_read(bcm, 0x048C) & 0xFF00) | 0x001E);
		bcm43xx_phy_write(bcm, 0x048D, 0x0002);
	}

	bcm43xx_ilt_write16(bcm, offset + 0x0800, 0);
	bcm43xx_ilt_write16(bcm, offset + 0x0801, 7);
	bcm43xx_ilt_write16(bcm, offset + 0x0802, 16);
	bcm43xx_ilt_write16(bcm, offset + 0x0803, 28);
}

static void bcm43xx_phy_setupg(struct bcm43xx_private *bcm)
{
	u16 i;

	assert(bcm->current_core->phy->type == BCM43xx_PHYTYPE_G);
	if (bcm->current_core->phy->rev == 1) {
		bcm43xx_phy_write(bcm, 0x0406, 0x4F19);
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
				  (bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0xFC3F) | 0x0340);
		bcm43xx_phy_write(bcm, 0x042C, 0x005A);
		bcm43xx_phy_write(bcm, 0x0427, 0x001A);

		for (i = 0; i < BCM43xx_ILT_FINEFREQG_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x5800 + i, bcm43xx_ilt_finefreqg[i]);
		for (i = 0; i < BCM43xx_ILT_NOISEG1_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1800 + i, bcm43xx_ilt_noiseg1[i]);
		for (i = 0; i < BCM43xx_ILT_ROTOR_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x2000 + i, bcm43xx_ilt_rotor[i]);
	} else {
		FIXME();//FIXME: 0xBA98 should be in 0-64, 0x7654 should be 6-bit!
		bcm43xx_nrssi_hw_write(bcm, 0xBA98, (s16)0x7654);
		
		if (bcm->current_core->phy->rev == 2) {
			bcm43xx_phy_write(bcm, 0x04C0, 0x1861);
			bcm43xx_phy_write(bcm, 0x04C1, 0x0271);
		} else if (bcm->current_core->phy->rev > 2) {
			bcm43xx_phy_write(bcm, 0x04C0, 0x0098);
			bcm43xx_phy_write(bcm, 0x04C1, 0x0070);
			bcm43xx_phy_write(bcm, 0x04C9, 0x0080);
		}

		bcm43xx_phy_write(bcm, 0x042B, bcm43xx_phy_read(bcm, 0x042B) | 0x800);

		for (i = 0; i < 64; i++)
			bcm43xx_ilt_write16(bcm, 0x4000 + i, i);
		for (i = 0; i < BCM43xx_ILT_NOISEG2_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1800 + i, bcm43xx_ilt_noiseg2[i]);
	}
	
	if (bcm->current_core->phy->rev <= 2)
		for (i = 0; i < BCM43xx_ILT_NOISESCALEG_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1400 + i, bcm43xx_ilt_noisescaleg1[i]);
	else if ((bcm->current_core->phy->rev == 7) && (bcm43xx_phy_read(bcm, 0x0449) & 0x0200))
		for (i = 0; i < BCM43xx_ILT_NOISESCALEG_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1400 + i, bcm43xx_ilt_noisescaleg3[i]);
	else
		for (i = 0; i < BCM43xx_ILT_NOISESCALEG_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1400 + i, bcm43xx_ilt_noisescaleg2[i]);
	
	if (bcm->current_core->phy->rev == 2)
		for (i = 0; i < BCM43xx_ILT_SIGMASQR_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x5000 + i, bcm43xx_ilt_sigmasqr1[i]);
	else if ((bcm->current_core->phy->rev > 2) && (bcm->current_core->phy->rev <= 7))
		for (i = 0; i < BCM43xx_ILT_SIGMASQR_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x5000 + i, bcm43xx_ilt_sigmasqr2[i]);
	
	if (bcm->current_core->phy->rev == 1) {
		for (i = 0; i < BCM43xx_ILT_RETARD_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x2400 + i, bcm43xx_ilt_retard[i]);
		for (i = 0; i < 4; i++) {
			bcm43xx_ilt_write16(bcm, 0x5404 + i, 0x0020);
			bcm43xx_ilt_write16(bcm, 0x5408 + i, 0x0020);
			bcm43xx_ilt_write16(bcm, 0x540C + i, 0x0020);
			bcm43xx_ilt_write16(bcm, 0x5410 + i, 0x0020);
		}
		bcm43xx_phy_agcsetup(bcm);
		
		if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) &&
		    (bcm->board_type == 0x0416) &&
		    (bcm->board_revision == 0x0017))
			return;
		
		bcm43xx_ilt_write16(bcm, 0x5001, 0x0002);
		bcm43xx_ilt_write16(bcm, 0x5002, 0x0001);
		
	} else {
		for (i = 0; i <= 0x2F; i++)
			bcm43xx_ilt_write16(bcm, 0x1000 + i, 0x0820);
		bcm43xx_phy_agcsetup(bcm);
		bcm43xx_phy_read(bcm, 0x0400); /* dummy read */
		bcm43xx_phy_write(bcm, 0x0403, 0x1000);
		bcm43xx_ilt_write16(bcm, 0x3C02, 0x000F);
		bcm43xx_ilt_write16(bcm, 0x3C03, 0x0014);

		if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) &&
		    (bcm->board_type == 0x0416) &&
		    (bcm->board_revision == 0x0017))
			return;

		bcm43xx_ilt_write16(bcm, 0x0401, 0x0002);
		bcm43xx_ilt_write16(bcm, 0x0402, 0x0001);
	}
}

/* Initialize the noisescaletable for APHY */
static void bcm43xx_phy_init_noisescaletbl(struct bcm43xx_private *bcm)
{
	int i;

	bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_CTRL, 0x1400);
	for (i = 0; i < 12; i++) {
		if (bcm->current_core->phy->rev == 2)
			bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x6767);
		else
			bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x2323);
	}
	if (bcm->current_core->phy->rev == 2)
		bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x6700);
	else
		bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x2300);
	for (i = 0; i < 11; i++) {
		if (bcm->current_core->phy->rev == 2)
			bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x6767);
		else
			bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x2323);
	}
	if (bcm->current_core->phy->rev == 2)
		bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x0067);
	else
		bcm43xx_phy_write(bcm, BCM43xx_PHY_ILT_A_DATA1, 0x0023);
}

static void bcm43xx_phy_setupa(struct bcm43xx_private *bcm)
{
	u16 i;

	assert(bcm->current_core->phy->type == BCM43xx_PHYTYPE_A);
	switch (bcm->current_core->phy->rev) {
	case 2:
		bcm43xx_phy_write(bcm, 0x008E, 0x3800);
		bcm43xx_phy_write(bcm, 0x0035, 0x03FF);
		bcm43xx_phy_write(bcm, 0x0036, 0x0400);

		bcm43xx_ilt_write16(bcm, 0x3807, 0x0051);

		bcm43xx_phy_write(bcm, 0x001C, 0x0FF9);
		bcm43xx_phy_write(bcm, 0x0020, bcm43xx_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm43xx_ilt_write16(bcm, 0x3C0C, 0x07BF);
		bcm43xx_radio_write16(bcm, 0x0002, 0x07BF);

		bcm43xx_phy_write(bcm, 0x0024, 0x4680);
		bcm43xx_phy_write(bcm, 0x0020, 0x0003);
		bcm43xx_phy_write(bcm, 0x001D, 0x0F40);
		bcm43xx_phy_write(bcm, 0x001F, 0x1C00);

		bcm43xx_phy_write(bcm, 0x002A, (bcm43xx_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);
		bcm43xx_phy_write(bcm, 0x002B, bcm43xx_phy_read(bcm, 0x002B) & 0xFBFF);
		bcm43xx_phy_write(bcm, 0x008E, 0x58C1);

		bcm43xx_ilt_write16(bcm, 0x0803, 0x000F);
		bcm43xx_ilt_write16(bcm, 0x0804, 0x001F);
		bcm43xx_ilt_write16(bcm, 0x0805, 0x002A);
		bcm43xx_ilt_write16(bcm, 0x0805, 0x0030);
		bcm43xx_ilt_write16(bcm, 0x0807, 0x003A);

		bcm43xx_ilt_write16(bcm, 0x0000, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0001, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0002, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0003, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0004, 0x0015);
		bcm43xx_ilt_write16(bcm, 0x0005, 0x0015);
		bcm43xx_ilt_write16(bcm, 0x0006, 0x0019);

		bcm43xx_ilt_write16(bcm, 0x0404, 0x0003);
		bcm43xx_ilt_write16(bcm, 0x0405, 0x0003);
		bcm43xx_ilt_write16(bcm, 0x0406, 0x0007);

		for (i = 0; i < 16; i++)
			bcm43xx_ilt_write16(bcm, 0x4000 + i, (0x8 + i) & 0x000F);

		bcm43xx_ilt_write16(bcm, 0x3003, 0x1044);
		bcm43xx_ilt_write16(bcm, 0x3004, 0x7201);
		bcm43xx_ilt_write16(bcm, 0x3006, 0x0040);
		bcm43xx_ilt_write16(bcm, 0x3001, (bcm43xx_ilt_read16(bcm, 0x3001) & 0x0010) | 0x0008);

		for (i = 0; i < BCM43xx_ILT_FINEFREQA_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x5800 + i, bcm43xx_ilt_finefreqa[i]);
		for (i = 0; i < BCM43xx_ILT_NOISEA2_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1800 + i, bcm43xx_ilt_noisea2[i]);
		for (i = 0; i < BCM43xx_ILT_ROTOR_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x2000 + i, bcm43xx_ilt_rotor[i]);
		bcm43xx_phy_init_noisescaletbl(bcm);
		for (i = 0; i < BCM43xx_ILT_RETARD_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x2400 + i, bcm43xx_ilt_retard[i]);
		break;
	case 3:
		for (i = 0; i < 64; i++)
			bcm43xx_ilt_write16(bcm, 0x4000 + i, i);

		bcm43xx_ilt_write16(bcm, 0x3807, 0x0051);

		bcm43xx_phy_write(bcm, 0x001C, 0x0FF9);
		bcm43xx_phy_write(bcm, 0x0020, bcm43xx_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm43xx_radio_write16(bcm, 0x0002, 0x07BF);

		bcm43xx_phy_write(bcm, 0x0024, 0x4680);
		bcm43xx_phy_write(bcm, 0x0020, 0x0003);
		bcm43xx_phy_write(bcm, 0x001D, 0x0F40);
		bcm43xx_phy_write(bcm, 0x001F, 0x1C00);
		bcm43xx_phy_write(bcm, 0x002A, (bcm43xx_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);

		bcm43xx_ilt_write16(bcm, 0x3001, (bcm43xx_ilt_read16(bcm, 0x3001) & 0x0010) | 0x0008);
		for (i = 0; i < BCM43xx_ILT_NOISEA3_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x1800 + i, bcm43xx_ilt_noisea3[i]);
		bcm43xx_phy_init_noisescaletbl(bcm);
		for (i = 0; i < BCM43xx_ILT_SIGMASQR_SIZE; i++)
			bcm43xx_ilt_write16(bcm, 0x5000 + i, bcm43xx_ilt_sigmasqr1[i]);

		bcm43xx_phy_write(bcm, 0x0003, 0x1808);

		bcm43xx_ilt_write16(bcm, 0x0803, 0x000F);
		bcm43xx_ilt_write16(bcm, 0x0804, 0x001F);
		bcm43xx_ilt_write16(bcm, 0x0805, 0x002A);
		bcm43xx_ilt_write16(bcm, 0x0805, 0x0030);
		bcm43xx_ilt_write16(bcm, 0x0807, 0x003A);

		bcm43xx_ilt_write16(bcm, 0x0000, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0001, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0002, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0003, 0x0013);
		bcm43xx_ilt_write16(bcm, 0x0004, 0x0015);
		bcm43xx_ilt_write16(bcm, 0x0005, 0x0015);
		bcm43xx_ilt_write16(bcm, 0x0006, 0x0019);

		bcm43xx_ilt_write16(bcm, 0x0404, 0x0003);
		bcm43xx_ilt_write16(bcm, 0x0405, 0x0003);
		bcm43xx_ilt_write16(bcm, 0x0406, 0x0007);

		bcm43xx_ilt_write16(bcm, 0x3C02, 0x000F);
		bcm43xx_ilt_write16(bcm, 0x3C03, 0x0014);
		break;
	default:
		assert(0);
	}
}

/* Initialize APHY. This is also called for the GPHY in some cases. */
static void bcm43xx_phy_inita(struct bcm43xx_private *bcm)
{
	u16 tval;

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A) {
		bcm43xx_phy_setupa(bcm);
	} else {
		bcm43xx_phy_setupg(bcm);
		if (bcm->sprom.boardflags & BCM43xx_BFL_PACTRL)
			bcm43xx_phy_write(bcm, 0x046E, 0x03CF);
		return;
	}

	bcm43xx_phy_write(bcm, BCM43xx_PHY_A_CRS,
	                  (bcm43xx_phy_read(bcm, BCM43xx_PHY_A_CRS) & 0xF83C) | 0x0340);
	bcm43xx_phy_write(bcm, 0x0034, 0x0001);

	TODO();//TODO: RSSI AGC
	bcm43xx_phy_write(bcm, BCM43xx_PHY_A_CRS,
	                  bcm43xx_phy_read(bcm, BCM43xx_PHY_A_CRS) | (1 << 14));
	bcm43xx_radio_init2060(bcm);

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM)
	    && ((bcm->board_type == 0x0416) || (bcm->board_type == 0x040A))) {
		if (bcm->current_core->radio->lofcal == 0xFFFF) {
			TODO();//TODO: LOF Cal
			bcm43xx_radio_set_tx_iq(bcm);
		} else
			bcm43xx_radio_write16(bcm, 0x001E, bcm->current_core->radio->lofcal);
	}

	bcm43xx_phy_write(bcm, 0x007A, 0xF111);

	if (bcm->current_core->phy->savedpctlreg == 0xFFFF) {
		bcm43xx_radio_write16(bcm, 0x0019, 0x0000);
		bcm43xx_radio_write16(bcm, 0x0017, 0x0020);

		tval = bcm43xx_ilt_read16(bcm, 0x3001);
		if (bcm->current_core->phy->rev == 1) {
			bcm43xx_ilt_write16(bcm, 0x3001,
					    (bcm43xx_ilt_read16(bcm, 0x3001) & 0xFF87)
					    | 0x0058);
		} else {
			bcm43xx_ilt_write16(bcm, 0x3001,
					    (bcm43xx_ilt_read16(bcm, 0x3001) & 0xFFC3)
					    | 0x002C);
		}
		bcm43xx_dummy_transmission(bcm);
		bcm->current_core->phy->savedpctlreg = bcm43xx_phy_read(bcm, BCM43xx_PHY_A_PCTL);
		bcm43xx_ilt_write16(bcm, 0x3001, tval);

		bcm43xx_radio_set_txpower_a(bcm, 0x0018);
	}
	bcm43xx_radio_clear_tssi(bcm);
}

static void bcm43xx_phy_initb2(struct bcm43xx_private *bcm)
{
	u16 offset, val;

	bcm43xx_write16(bcm, 0x03EC, 0x3F22);
	bcm43xx_phy_write(bcm, 0x0020, 0x301C);
	bcm43xx_phy_write(bcm, 0x0026, 0x0000);
	bcm43xx_phy_write(bcm, 0x0030, 0x00C6);
	bcm43xx_phy_write(bcm, 0x0088, 0x3E00);
	val = 0x3C3D;
	for (offset = 0x0089; offset < 0x00A7; offset++) {
		bcm43xx_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm43xx_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->current_core->radio->channel == 0xFF)
		bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_BG, 0);
	else
		bcm43xx_radio_selectchannel(bcm, bcm->current_core->radio->channel, 0);
	if (bcm->current_core->radio->version != 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0075, 0x0080);
		bcm43xx_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
	bcm43xx_radio_write16(bcm, 0x0050, 0x0023);
	if (bcm->current_core->radio->version == 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
		bcm43xx_radio_write16(bcm, 0x005A, 0x0070);
		bcm43xx_radio_write16(bcm, 0x005B, 0x007B);
		bcm43xx_radio_write16(bcm, 0x005C, 0x00B0);
		bcm43xx_radio_write16(bcm, 0x007A, 0x000F);
		bcm43xx_phy_write(bcm, 0x0038, 0x0677);
		bcm43xx_radio_init2050(bcm);
	}
	bcm43xx_phy_write(bcm, 0x0014, 0x0080);
	bcm43xx_phy_write(bcm, 0x0032, 0x00CA);
	bcm43xx_phy_write(bcm, 0x0032, 0x00CC);
	bcm43xx_phy_write(bcm, 0x0035, 0x07C2);
	bcm43xx_phy_lo_b_measure(bcm);
	bcm43xx_phy_write(bcm, 0x0026, 0xCC00);
	if (bcm->current_core->radio->version != 0x2050)
		bcm43xx_phy_write(bcm, 0x0026, 0xCE00);
	bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, 0x1000);
	bcm43xx_phy_write(bcm, 0x002A, 0x88A3);
	if (bcm->current_core->radio->version != 0x2050)
		bcm43xx_phy_write(bcm, 0x002A, 0x88C2);
	bcm43xx_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm43xx_phy_init_pctl(bcm);
}

static void bcm43xx_phy_initb4(struct bcm43xx_private *bcm)
{
	u16 offset, val;

	bcm43xx_write16(bcm, 0x03EC, 0x3F22);
	bcm43xx_phy_write(bcm, 0x0020, 0x301C);
	bcm43xx_phy_write(bcm, 0x0026, 0x0000);
	bcm43xx_phy_write(bcm, 0x0030, 0x00C6);
	bcm43xx_phy_write(bcm, 0x0088, 0x3E00);
	val = 0x3C3D;
	for (offset = 0x0089; offset < 0x00A7; offset++) {
		bcm43xx_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm43xx_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->current_core->radio->channel == 0xFF)
		bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_BG, 0);
	else
		bcm43xx_radio_selectchannel(bcm, bcm->current_core->radio->channel, 0);
	if (bcm->current_core->radio->version != 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0075, 0x0080);
		bcm43xx_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
	bcm43xx_radio_write16(bcm, 0x0050, 0x0023);
	if (bcm->current_core->radio->version == 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
		bcm43xx_radio_write16(bcm, 0x005A, 0x0070);
		bcm43xx_radio_write16(bcm, 0x005B, 0x007B);
		bcm43xx_radio_write16(bcm, 0x005C, 0x00B0);
		bcm43xx_radio_write16(bcm, 0x007A, 0x000F);
		bcm43xx_phy_write(bcm, 0x0038, 0x0677);
		bcm43xx_radio_init2050(bcm);
	}
	bcm43xx_phy_write(bcm, 0x0014, 0x0080);
	bcm43xx_phy_write(bcm, 0x0032, 0x00CA);
	if (bcm->current_core->radio->version == 0x2050)
		bcm43xx_phy_write(bcm, 0x0032, 0x00E0);
	bcm43xx_phy_write(bcm, 0x0035, 0x07C2);

	bcm43xx_phy_lo_b_measure(bcm);

	bcm43xx_phy_write(bcm, 0x0026, 0xCC00);
	if (bcm->current_core->radio->version == 0x2050)
		bcm43xx_phy_write(bcm, 0x0026, 0xCE00);
	bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, 0x1100);
	bcm43xx_phy_write(bcm, 0x002A, 0x88A3);
	if (bcm->current_core->radio->version == 0x2050)
		bcm43xx_phy_write(bcm, 0x002A, 0x88C2);
	bcm43xx_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	if (bcm->sprom.boardflags & BCM43xx_BFL_RSSI) {
		bcm43xx_calc_nrssi_slope(bcm);
		bcm43xx_calc_nrssi_threshold(bcm);
	}
	bcm43xx_phy_init_pctl(bcm);
}

static void bcm43xx_phy_initb5(struct bcm43xx_private *bcm)
{
	u16 offset;

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A)
				      | 0x0050);
	}

	if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM) &&
	    (bcm->board_type != 0x0416)) {
		for (offset = 0x00A8 ; offset < 0x00C7; offset++) {
			bcm43xx_phy_write(bcm, offset,
					  (bcm43xx_phy_read(bcm, offset) + 0x2020)
					  & 0x3F3F);
		}
	}

	bcm43xx_phy_write(bcm, 0x0035,
			  (bcm43xx_phy_read(bcm, 0x0035) & 0xF0FF)
			  | 0x0700);

	if (bcm->current_core->radio->version == 0x2050)
		bcm43xx_phy_write(bcm, 0x0038, 0x0667);

	if (bcm->current_core->phy->connected) {
		if (bcm->current_core->radio->version == 0x2050) {
			bcm43xx_radio_write16(bcm, 0x007A,
					      bcm43xx_radio_read16(bcm, 0x007A)
					      | 0x0020);
			bcm43xx_radio_write16(bcm, 0x0051,
					      bcm43xx_radio_read16(bcm, 0x0051)
					      | 0x0004);
		}

		bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_RADIO, 0x0000);

		bcm43xx_phy_write(bcm, 0x0802, bcm43xx_phy_read(bcm, 0x0802) | 0x0100);
		bcm43xx_phy_write(bcm, 0x042B, bcm43xx_phy_read(bcm, 0x042B) | 0x2000);

		bcm43xx_phy_write(bcm, 0x001C, 0x186A);

		bcm43xx_phy_write(bcm, 0x0013, (bcm43xx_phy_read(bcm, 0x0013) & 0x00FF) | 0x1900);
		bcm43xx_phy_write(bcm, 0x0035, (bcm43xx_phy_read(bcm, 0x0035) & 0xFFC0) | 0x0064);
		bcm43xx_phy_write(bcm, 0x005D, (bcm43xx_phy_read(bcm, 0x005D) & 0xFF80) | 0x000A);
	}

	if (bcm->bad_frames_preempt) {
		bcm43xx_phy_write(bcm, BCM43xx_PHY_RADIO_BITFIELD,
				  bcm43xx_phy_read(bcm, BCM43xx_PHY_RADIO_BITFIELD) | (1 << 11));
	}

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050)) {
		bcm43xx_phy_write(bcm, 0x0026, 0xCE00);
		bcm43xx_phy_write(bcm, 0x0021, 0x3763);
		bcm43xx_phy_write(bcm, 0x0022, 0x1BC3);
		bcm43xx_phy_write(bcm, 0x0023, 0x06F9);
		bcm43xx_phy_write(bcm, 0x0024, 0x037E);
	} else
		bcm43xx_phy_write(bcm, 0x0026, 0xCC00);

	bcm43xx_phy_write(bcm, 0x0030, 0x00C6);

	bcm43xx_write16(bcm, 0x03EC, 0x3F22);

	if ((bcm->current_core->phy->version == 1) &&
	    (bcm->current_core->radio->version == 0x2050))
		bcm43xx_phy_write(bcm, 0x0020, 0x3E1C);
	else
		bcm43xx_phy_write(bcm, 0x0020, 0x301C);

	if (bcm->current_core->phy->version == 0)
		bcm43xx_write16(bcm, 0x03E4, 0x3000);

	/* Force to channel 7, even if not supported. */
	bcm43xx_radio_selectchannel(bcm, 7, 0);

	if (bcm->current_core->radio->version != 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0075, 0x0080);
		bcm43xx_radio_write16(bcm, 0x0079, 0x0081);
	}

	bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
	bcm43xx_radio_write16(bcm, 0x0050, 0x0023);

	if (bcm->current_core->radio->version == 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
		bcm43xx_radio_write16(bcm, 0x005A, 0x0070);
	}

	bcm43xx_radio_write16(bcm, 0x005B, 0x007B);
	bcm43xx_radio_write16(bcm, 0x005C, 0x00B0);

	bcm43xx_radio_write16(bcm, 0x007A, bcm43xx_radio_read16(bcm, 0x007A) | 0x0007);

	bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_BG, 0);

	bcm43xx_phy_write(bcm, 0x0014, 0x0080);
	bcm43xx_phy_write(bcm, 0x0032, 0x00CA);
	bcm43xx_phy_write(bcm, 0x88A3, 0x002A);

	bcm43xx_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);

	if (bcm->current_core->radio->version == 0x2050)
		bcm43xx_radio_write16(bcm, 0x005D, 0x000D);

	bcm43xx_write16(bcm, 0x03E4, (bcm43xx_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
}

static void bcm43xx_phy_initb6(struct bcm43xx_private *bcm)
{
	u16 offset, val;

	bcm43xx_phy_write(bcm, 0x003E, 0x817A);
	bcm43xx_radio_write16(bcm, 0x007A,
	                      (bcm43xx_radio_read16(bcm, 0x007A) | 0x0058));
	if ((bcm->current_core->radio->manufact == 0x17F) &&
	    (bcm->current_core->radio->version == 0x2050) &&
	    (bcm->current_core->radio->revision == 3 ||
	     bcm->current_core->radio->revision == 4 ||
	     bcm->current_core->radio->revision == 5)) {
		bcm43xx_radio_write16(bcm, 0x0051, 0x001F);
		bcm43xx_radio_write16(bcm, 0x0052, 0x0040);
		bcm43xx_radio_write16(bcm, 0x0053, 0x005B);
		bcm43xx_radio_write16(bcm, 0x0054, 0x0098);
		bcm43xx_radio_write16(bcm, 0x005A, 0x0088);
		bcm43xx_radio_write16(bcm, 0x005B, 0x0088);
		bcm43xx_radio_write16(bcm, 0x005D, 0x0088);
		bcm43xx_radio_write16(bcm, 0x005E, 0x0088);
		bcm43xx_radio_write16(bcm, 0x007D, 0x0088);
	}
	if ((bcm->current_core->radio->manufact == 0x17F) &&
            (bcm->current_core->radio->version == 0x2050) &&
            (bcm->current_core->radio->revision == 6)) {
                bcm43xx_radio_write16(bcm, 0x0051, 0x0000);
                bcm43xx_radio_write16(bcm, 0x0052, 0x0040);
                bcm43xx_radio_write16(bcm, 0x0053, 0x00B7);
                bcm43xx_radio_write16(bcm, 0x0054, 0x0098);
                bcm43xx_radio_write16(bcm, 0x005A, 0x0088);
                bcm43xx_radio_write16(bcm, 0x005B, 0x008B);
		bcm43xx_radio_write16(bcm, 0x005C, 0x00B5);
                bcm43xx_radio_write16(bcm, 0x005D, 0x0088);
                bcm43xx_radio_write16(bcm, 0x005E, 0x0088);
                bcm43xx_radio_write16(bcm, 0x007D, 0x0088);
                bcm43xx_radio_write16(bcm, 0x007C, 0x0001);
                bcm43xx_radio_write16(bcm, 0x007E, 0x0008);
	}
        if ((bcm->current_core->radio->manufact == 0x17F) &&
            (bcm->current_core->radio->version == 0x2050) &&
            (bcm->current_core->radio->revision == 7)) {
                bcm43xx_radio_write16(bcm, 0x0051, 0x0000);
                bcm43xx_radio_write16(bcm, 0x0052, 0x0040);
                bcm43xx_radio_write16(bcm, 0x0053, 0x00B7);
                bcm43xx_radio_write16(bcm, 0x0054, 0x0098);
                bcm43xx_radio_write16(bcm, 0x005A, 0x0088);
                bcm43xx_radio_write16(bcm, 0x005B, 0x00A8);
                bcm43xx_radio_write16(bcm, 0x005C, 0x0075);
                bcm43xx_radio_write16(bcm, 0x005D, 0x00F5);
                bcm43xx_radio_write16(bcm, 0x005E, 0x00B8);
                bcm43xx_radio_write16(bcm, 0x007D, 0x00E8);
                bcm43xx_radio_write16(bcm, 0x007C, 0x0001);
                bcm43xx_radio_write16(bcm, 0x007E, 0x0008);
		bcm43xx_radio_write16(bcm, 0x007B, 0x0000);
        }
        if ((bcm->current_core->radio->manufact == 0x17F) &&
            (bcm->current_core->radio->version == 0x2050) &&
            (bcm->current_core->radio->revision == 8)) {
                bcm43xx_radio_write16(bcm, 0x0051, 0x0000);
                bcm43xx_radio_write16(bcm, 0x0052, 0x0040);
                bcm43xx_radio_write16(bcm, 0x0053, 0x00B7);
                bcm43xx_radio_write16(bcm, 0x0054, 0x0098);
                bcm43xx_radio_write16(bcm, 0x005A, 0x0088);
                bcm43xx_radio_write16(bcm, 0x005B, 0x006B);
                bcm43xx_radio_write16(bcm, 0x005C, 0x000F);
		if (bcm->sprom.boardflags & 0x8000) {
			bcm43xx_radio_write16(bcm, 0x005D, 0x00FA);
                        bcm43xx_radio_write16(bcm, 0x005E, 0x00D8);
		} else {
	                bcm43xx_radio_write16(bcm, 0x005D, 0x00F5);
        	        bcm43xx_radio_write16(bcm, 0x005E, 0x00B8);
		}
		bcm43xx_radio_write16(bcm, 0x0073, 0x0003);
                bcm43xx_radio_write16(bcm, 0x007D, 0x00A8);
                bcm43xx_radio_write16(bcm, 0x007C, 0x0001);
                bcm43xx_radio_write16(bcm, 0x007E, 0x0008);
        }
	val = 0x1E1F;
	for (offset = 0x0088; offset < 0x0098; offset++) {
		bcm43xx_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x3E3F;
	for (offset = 0x0098; offset < 0x00A8; offset++) {
		bcm43xx_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x2120;
	for (offset = 0x00A8; offset < 0x00C8; offset++) {
		bcm43xx_phy_write(bcm, offset, (val & 0x3F3F));
		val += 0x0202;
	}
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) {
		bcm43xx_radio_write16(bcm, 0x007A,
		                      bcm43xx_radio_read16(bcm, 0x007A) | 0x0020);
		bcm43xx_radio_write16(bcm, 0x0051,
		                      bcm43xx_radio_read16(bcm, 0x0051) | 0x0004);
		bcm43xx_phy_write(bcm, 0x0802,
		                  bcm43xx_phy_read(bcm, 0x0802) | 0x0100);
		bcm43xx_phy_write(bcm, 0x042B,
		                  bcm43xx_phy_read(bcm, 0x042B) | 0x2000);
	}

	/* Force to channel 7, even if not supported. */
	bcm43xx_radio_selectchannel(bcm, 7, 0);

	bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
	bcm43xx_radio_write16(bcm, 0x0050, 0x0023);
	udelay(40);
	bcm43xx_radio_write16(bcm, 0x007C, (bcm43xx_radio_read16(bcm, 0x007C) | 0x0002));
	bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
	if ((bcm->current_core->radio->manufact == 0x17F) &&
            (bcm->current_core->radio->version == 0x2050) &&
            (bcm->current_core->radio->revision == 2)) {
		bcm43xx_radio_write16(bcm, 0x0050, 0x0020);
		bcm43xx_radio_write16(bcm, 0x005A, 0x0070);
		bcm43xx_radio_write16(bcm, 0x005B, 0x007B);
		bcm43xx_radio_write16(bcm, 0x005C, 0x00B0);
	}
	bcm43xx_radio_write16(bcm, 0x007A,
	                      (bcm43xx_radio_read16(bcm, 0x007A) & 0x00F8) | 0x0007);

	bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_BG, 0);

	bcm43xx_phy_write(bcm, 0x0014, 0x0200);
	if (bcm->current_core->radio->version == 0x2050){
		if (bcm->current_core->radio->revision == 3 ||
		    bcm->current_core->radio->revision == 4 ||
		    bcm->current_core->radio->revision == 5)
			bcm43xx_phy_write(bcm, 0x002A, 0x8AC0);
		else
			bcm43xx_phy_write(bcm, 0x002A, 0x88C2);
	}
	bcm43xx_phy_write(bcm, 0x0038, 0x0668);
	bcm43xx_radio_set_txpower_bg(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	if (bcm->current_core->radio->version == 0x2050) {
		if (bcm->current_core->radio->revision == 3 ||
		    bcm->current_core->radio->revision == 4 ||
		    bcm->current_core->radio->revision == 5)
			bcm43xx_phy_write(bcm, 0x005D, bcm43xx_phy_read(bcm, 0x005D) | 0x0003);
		else if (bcm->current_core->radio->revision <= 2)
			bcm43xx_radio_write16(bcm, 0x005D, 0x000D);
	}
	
	if (bcm->current_core->phy->rev == 4)
		bcm43xx_phy_write(bcm, 0x0002, (bcm43xx_phy_read(bcm, 0x0002) & 0xFFC0) | 0x0004);
	else
		bcm43xx_write16(bcm, 0x03E4, 0x0009);
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
		bcm43xx_write16(bcm, 0x03E6, 0x8140);
		bcm43xx_phy_write(bcm, 0x0016, 0x5410);
		bcm43xx_phy_write(bcm, 0x0017, 0xA820);
		bcm43xx_phy_write(bcm, 0x0007, 0x0062);
		TODO();//TODO: calibrate stuff.
		bcm43xx_phy_init_pctl(bcm);
	} else
		bcm43xx_write16(bcm, 0x03E6, 0x0);
}

static void bcm43xx_phy_initg(struct bcm43xx_private *bcm)
{
	if (bcm->current_core->phy->rev == 1)
		bcm43xx_phy_initb5(bcm);
	else if (bcm->current_core->phy->rev >= 2 && bcm->current_core->phy->rev <= 7)
		bcm43xx_phy_initb6(bcm);
	if ((bcm->current_core->phy->rev >= 2) || bcm->current_core->phy->connected)
		bcm43xx_phy_inita(bcm);

	if (bcm->current_core->phy->rev >= 2) {
		bcm43xx_phy_write(bcm, 0x0814, 0x0000);
		bcm43xx_phy_write(bcm, 0x0815, 0x0000);
		if (bcm->current_core->phy->rev == 2)
			bcm43xx_phy_write(bcm, 0x0811, 0x0000);
		else if (bcm->current_core->phy->rev >= 3)
			bcm43xx_phy_write(bcm, 0x0811, 0x0400);
		bcm43xx_phy_write(bcm, 0x0015, 0x00C0);
		if ((bcm43xx_phy_read(bcm, 0x0400) & 0xFF) == 3) {
			bcm43xx_phy_write(bcm, 0x04C2, 0x1816);
			bcm43xx_phy_write(bcm, 0x04C3, 0x8606);
		} else if ((bcm43xx_phy_read(bcm, 0x0400) & 0xFF) <= 5) {
			bcm43xx_phy_write(bcm, 0x04C2, 0x1816);
			bcm43xx_phy_write(bcm, 0x04C3, 0x8006);
			bcm43xx_phy_write(bcm, 0x04CC, (bcm43xx_phy_read(bcm, 0x04CC)
					  & 0x00FF) | 0x1F00);
		}
	}
	if (bcm->current_core->radio->revision <= 3 &&
	    bcm->current_core->phy->connected)
		bcm43xx_phy_write(bcm, 0x047E, 0x0078);
	if ((bcm->current_core->radio->revision >= 6) &&
	    (bcm->current_core->radio->revision <= 8)) {
		bcm43xx_phy_write(bcm, 0x0801, bcm43xx_phy_read(bcm, 0x0801) | 0x0080);
		bcm43xx_phy_write(bcm, 0x043E, bcm43xx_phy_read(bcm, 0x043E) | 0x0004);
	}
	if (bcm->current_core->radio->initval == 0xFFFF) {
		bcm->current_core->radio->initval = bcm43xx_radio_init2050(bcm);
		bcm43xx_phy_lo_g_measure(bcm);
	} else {
		bcm43xx_radio_write16(bcm, 0x0078, bcm->current_core->radio->initval);
		bcm43xx_radio_write16(bcm, 0x0052,
				      (bcm43xx_radio_read16(bcm, 0x0052) & 0xFFF0)
				      | bcm->current_core->radio->txpower[3]);
	}

	if (bcm->current_core->phy->connected) {
		bcm43xx_phy_lo_adjust(bcm, 0);
		bcm43xx_phy_write(bcm, 0x080F, 0x8078);

		if (bcm->sprom.boardflags & BCM43xx_BFL_PACTRL)
			bcm43xx_phy_write(bcm, 0x002E, 0x807F);
		else
			bcm43xx_phy_write(bcm, 0x002E, 0x8075);

		if (bcm->current_core->phy->rev < 2)
			bcm43xx_phy_write(bcm, 0x002F, 0x0101);
		else
			bcm43xx_phy_write(bcm, 0x002F, 0x0202);
	}

	if ((bcm->sprom.boardflags & BCM43xx_BFL_RSSI) == 0) {
		FIXME();//FIXME: 0x7FFFFFFF should be 16-bit !
		bcm43xx_nrssi_hw_update(bcm, (u16)0x7FFFFFFF);
		bcm43xx_calc_nrssi_threshold(bcm);
	} else if (bcm->current_core->phy->connected) {
		if (bcm->current_core->radio->nrssi[0] == -1000) {
			assert(bcm->current_core->radio->nrssi[1] == -1000);
			bcm43xx_calc_nrssi_slope(bcm);
		} else
			bcm43xx_calc_nrssi_threshold(bcm);
	}
	bcm43xx_phy_init_pctl(bcm);
}

static u16 bcm43xx_phy_lo_b_r15_loop(struct bcm43xx_private *bcm)
{
	int i;
	u16 ret = 0;

	for (i = 0; i < 10; i++){
		bcm43xx_phy_write(bcm, 0x0015, 0xAFA0);
		udelay(1);
		bcm43xx_phy_write(bcm, 0x0015, 0xEFA0);
		udelay(10);
		bcm43xx_phy_write(bcm, 0x0015, 0xFFA0);
		udelay(40);
		ret += bcm43xx_phy_read(bcm, 0x002C);
	}

	return ret;
}

void bcm43xx_phy_lo_b_measure(struct bcm43xx_private *bcm)
{
	const int is_2053_radio = (bcm->current_core->radio->version == 0x2053);
	struct bcm43xx_phyinfo *phy = bcm->current_core->phy;
	u16 regstack[12] = { 0 };
	u16 mls;
	u16 fval;
	int i, j;

	regstack[0] = bcm43xx_phy_read(bcm, 0x0015);
	regstack[1] = bcm43xx_radio_read16(bcm, 0x0052) & 0xFFF0;

	if (is_2053_radio) {
		regstack[2] = bcm43xx_phy_read(bcm, 0x000A);
		regstack[3] = bcm43xx_phy_read(bcm, 0x002A);
		regstack[4] = bcm43xx_phy_read(bcm, 0x0035);
		regstack[5] = bcm43xx_phy_read(bcm, 0x0003);
		regstack[6] = bcm43xx_phy_read(bcm, 0x0001);
		regstack[7] = bcm43xx_phy_read(bcm, 0x0030);

		regstack[8] = bcm43xx_radio_read16(bcm, 0x0043);
		regstack[9] = bcm43xx_radio_read16(bcm, 0x007A);
		regstack[10] = bcm43xx_read16(bcm, 0x03EC);
		regstack[11] = bcm43xx_radio_read16(bcm, 0x0052) & 0x00F0;

		bcm43xx_phy_write(bcm, 0x0030, 0x00FF);
		bcm43xx_write16(bcm, 0x03EC, 0x3F3F);
		bcm43xx_phy_write(bcm, 0x0035, regstack[4] & 0xFF7F);
		bcm43xx_radio_write16(bcm, 0x007A, regstack[9] & 0xFFF0);
	}
	bcm43xx_phy_write(bcm, 0x0015, 0xB000);
	bcm43xx_phy_write(bcm, 0x002B, 0x0004);

	if (is_2053_radio) {
		bcm43xx_phy_write(bcm, 0x002B, 0x0203);
		bcm43xx_phy_write(bcm, 0x002A, 0x08A3);
	}

	phy->minlowsig[0] = 0xFFFF;

	for (i = 0; i < 4; i++) {
		bcm43xx_radio_write16(bcm, 0x0052, regstack[1] | i);
		bcm43xx_phy_lo_b_r15_loop(bcm);
	}
	for (i = 0; i < 10; i++) {
		bcm43xx_radio_write16(bcm, 0x0052, regstack[1] | i);
		mls = bcm43xx_phy_lo_b_r15_loop(bcm) / 10;
		if (mls < phy->minlowsig[0]) {
			phy->minlowsig[0] = mls;
			phy->minlowsigpos[0] = i;
		}
	}
	bcm43xx_radio_write16(bcm, 0x0052, regstack[1] | phy->minlowsigpos[0]);

	phy->minlowsig[1] = 0xFFFF;

	for (i = -4; i < 5; i += 2) {
		for (j = -4; j < 5; j += 2) {
			if (j < 0)
				fval = (0x0100 * i) + j + 0x0100;
			else
				fval = (0x0100 * i) + j;
			bcm43xx_phy_write(bcm, 0x002F, fval);
			mls = bcm43xx_phy_lo_b_r15_loop(bcm) / 10;
			if (mls < phy->minlowsig[1]) {
				phy->minlowsig[1] = mls;
				phy->minlowsigpos[1] = fval;
			}
		}
	}
	phy->minlowsigpos[1] += 0x0101;

	bcm43xx_phy_write(bcm, 0x002F, phy->minlowsigpos[1]);
	if (is_2053_radio) {
		bcm43xx_phy_write(bcm, 0x000A, regstack[2]);
		bcm43xx_phy_write(bcm, 0x002A, regstack[3]);
		bcm43xx_phy_write(bcm, 0x0035, regstack[4]);
		bcm43xx_phy_write(bcm, 0x0003, regstack[5]);
		bcm43xx_phy_write(bcm, 0x0001, regstack[6]);
		bcm43xx_phy_write(bcm, 0x0030, regstack[7]);

		bcm43xx_radio_write16(bcm, 0x0043, regstack[8]);
		bcm43xx_radio_write16(bcm, 0x007A, regstack[9]);

		bcm43xx_radio_write16(bcm, 0x0052,
		                      (bcm43xx_radio_read16(bcm, 0x0052) & 0x000F)
				      | regstack[11]);

		bcm43xx_write16(bcm, 0x03EC, regstack[10]);
	}
	bcm43xx_phy_write(bcm, 0x0015, regstack[0]);
}

static inline
u16 bcm43xx_phy_lo_g_deviation_subval(struct bcm43xx_private *bcm, u16 control)
{
	if (bcm->current_core->phy->connected) {
		bcm43xx_phy_write(bcm, 0x15, 0xE300);
		control <<= 8;
		bcm43xx_phy_write(bcm, 0x0812, control | 0x00B0);
		udelay(5);
		bcm43xx_phy_write(bcm, 0x0812, control | 0x00B2);
		udelay(2);
		bcm43xx_phy_write(bcm, 0x0812, control | 0x00B3);
		udelay(4);
		bcm43xx_phy_write(bcm, 0x0015, 0xF300);
		udelay(8);
	} else {
		bcm43xx_phy_write(bcm, 0x0015, control | 0xEFA0);
		udelay(2);
		bcm43xx_phy_write(bcm, 0x0015, control | 0xEFE0);
		udelay(4);
		bcm43xx_phy_write(bcm, 0x0015, control | 0xFFE0);
		udelay(8);
	}

	return bcm43xx_phy_read(bcm, 0x002D);
}

static u32 bcm43xx_phy_lo_g_singledeviation(struct bcm43xx_private *bcm, u16 control)
{
	int i;
	u32 ret = 0;

	for (i = 0; i < 8; i++)
		ret += bcm43xx_phy_lo_g_deviation_subval(bcm, control);

	return ret;
}

/* Write the LocalOscillator CONTROL */
static inline
void bcm43xx_lo_write(struct bcm43xx_private *bcm,
		      struct bcm43xx_lopair *pair)
{
	u16 value;

	value = (u8)(pair->low);
	value |= ((u8)(pair->high)) << 8;

#ifdef BCM43xx_DEBUG
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

	bcm43xx_phy_write(bcm, BCM43xx_PHY_G_LO_CONTROL, value);
}

static inline
struct bcm43xx_lopair * bcm43xx_find_lopair(struct bcm43xx_private *bcm,
					    u16 baseband_attenuation,
					    u16 radio_attenuation,
					    u16 tx)
{
	const u8 dict[10] = { 11, 10, 11, 12, 13, 12, 13, 12, 13, 12 };
	struct bcm43xx_phyinfo *phy = bcm->current_core->phy;

	if (baseband_attenuation > 6)
		baseband_attenuation = 6;
	assert(radio_attenuation < 10);
	assert(tx == 0 || tx == 3);

	if (tx == 3) {
		return bcm43xx_get_lopair(phy,
					  radio_attenuation,
					  baseband_attenuation);
	}
	return bcm43xx_get_lopair(phy, dict[radio_attenuation], baseband_attenuation);
}

static inline
struct bcm43xx_lopair * bcm43xx_current_lopair(struct bcm43xx_private *bcm)
{
	return bcm43xx_find_lopair(bcm,
				   bcm->current_core->radio->txpower[0],
				   bcm->current_core->radio->txpower[1],
				   bcm->current_core->radio->txpower[2]);
}

/* Adjust B/G LO */
void bcm43xx_phy_lo_adjust(struct bcm43xx_private *bcm, int fixed)
{
	struct bcm43xx_lopair *pair;

	if (fixed) {
		/* Use fixed values. Only for initialization. */
		pair = bcm43xx_find_lopair(bcm, 2, 3, 0);
	} else
		pair = bcm43xx_current_lopair(bcm);
	bcm43xx_lo_write(bcm, pair);
}

static inline
void bcm43xx_phy_lo_g_measure_txctl2(struct bcm43xx_private *bcm)
{
	u16 txctl2 = 0, i;
	u32 smallest, tmp;

	bcm43xx_radio_write16(bcm, 0x0052, 0x0000);
	udelay(10);
	smallest = bcm43xx_phy_lo_g_singledeviation(bcm, 0);
	for (i = 0; i < 16; i++) {
		bcm43xx_radio_write16(bcm, 0x0052, i);
		udelay(10);
		tmp = bcm43xx_phy_lo_g_singledeviation(bcm, 0);
		if (tmp < smallest) {
			smallest = tmp;
			txctl2 = i;
		}
	}
	bcm->current_core->radio->txpower[3] = txctl2;
}

static
void bcm43xx_phy_lo_g_state(struct bcm43xx_private *bcm,
			    const struct bcm43xx_lopair *in_pair,
			    struct bcm43xx_lopair *out_pair,
			    u16 r27)
{
	static const struct bcm43xx_lopair transitions[8] = {
		{ .high =  1,  .low =  1, },
		{ .high =  1,  .low =  0, },
		{ .high =  1,  .low = -1, },
		{ .high =  0,  .low = -1, },
		{ .high = -1,  .low = -1, },
		{ .high = -1,  .low =  0, },
		{ .high = -1,  .low =  1, },
		{ .high =  0,  .low =  1, },
	};
	struct bcm43xx_lopair lowest_transition = {
		.high = in_pair->high,
		.low = in_pair->low,
	};
	struct bcm43xx_lopair tmp_pair;
	struct bcm43xx_lopair transition;
	int i = 12;
	int state = 0;
	int found_lower;
	int j, begin, end;
	u32 lowest_deviation;
	u32 tmp;

	/* Note that in_pair and out_pair can point to the same pair. Be careful. */

#ifdef BCM43xx_DEBUG
	{
		/* Revert the poison values. We must begin at 0. */
		if (lowest_transition.low == -20) {
			assert(lowest_transition.high == -20);
			lowest_transition.low = 0;
			lowest_transition.high = 0;
		}
	}
#endif /* BCM43xx_DEBUG */

	bcm43xx_lo_write(bcm, &lowest_transition);
	lowest_deviation = bcm43xx_phy_lo_g_singledeviation(bcm, r27);
	do {
		found_lower = 0;
		assert(state >= 0 && state <= 8);
		if (state == 0) {
			begin = 1;
			end = 8;
		} else if (state % 2 == 0) {
			begin = state - 1;
			end = state + 1;
		} else {
			begin = state - 2;
			end = state + 2;
		}
		if (begin < 1)
			begin += 8;
		if (end > 8)
			end -= 8;

		j = begin;
		tmp_pair.high = lowest_transition.high;
		tmp_pair.low = lowest_transition.low;
		while (1) {
			assert(j >= 1 && j <= 8);
			transition.high = tmp_pair.high + transitions[j - 1].high;
			transition.low = tmp_pair.low + transitions[j - 1].low;
			if ((abs(transition.low) < 9) && (abs(transition.high) < 9)) {
				bcm43xx_lo_write(bcm, &transition);
				tmp = bcm43xx_phy_lo_g_singledeviation(bcm, r27);
				if (tmp < lowest_deviation) {
					lowest_deviation = tmp;
					state = j;
					found_lower = 1;

					lowest_transition.high = transition.high;
					lowest_transition.low = transition.low;
				}
			}
			if (j == end)
				break;
			if (j == 8)
				j = 1;
			else
				j++;
		}
	} while (i-- && found_lower);

	out_pair->high = lowest_transition.high;
	out_pair->low = lowest_transition.low;
}

/* Set the baseband attenuation value on chip. */
void bcm43xx_phy_set_baseband_attenuation(struct bcm43xx_private *bcm,
					  u16 baseband_attenuation)
{
	u16 value;

	if (bcm->current_core->phy->version == 0) {
		value = (bcm43xx_read16(bcm, 0x03E6) & 0xFFF0);
		value |= (baseband_attenuation & 0x000F);
		bcm43xx_write16(bcm, 0x03E6, value);
		return;
	}

	if (bcm->current_core->phy->version > 1) {
		value = bcm43xx_phy_read(bcm, 0x0060) & ~0x003C;
		value |= (baseband_attenuation << 2) & 0x003C;
	} else {
		value = bcm43xx_phy_read(bcm, 0x0060) & ~0x0078;
		value |= (baseband_attenuation << 3) & 0x0078;
	}
	bcm43xx_phy_write(bcm, 0x0060, value);
}

/* http://bcm-specs.sipsolutions.net/LocalOscillator/Measure */
void bcm43xx_phy_lo_g_measure(struct bcm43xx_private *bcm)
{
	struct bcm43xx_phyinfo *phy = bcm->current_core->phy;
	u16 h, i, oldi = 0, j;
	struct bcm43xx_lopair control;
	struct bcm43xx_lopair *tmp_control;
	const u8 pairorder[10] = { 3, 1, 5, 7, 9, 2, 0, 4, 6, 8 };
	u16 tmp;
	u16 regstack[16] = { 0 };
	u8 oldchannel;

	//XXX: What are these?
	u8 r27, r31;

	oldchannel = bcm->current_core->radio->channel;
	/* Setup */
	if (phy->connected) {
		regstack[0] = bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS);
		regstack[1] = bcm43xx_phy_read(bcm, 0x0802);
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS, regstack[0] & 0x7FFF);
		bcm43xx_phy_write(bcm, 0x0802, regstack[1] & 0xFFFC);
	}
	regstack[3] = bcm43xx_read16(bcm, 0x03E2);
	bcm43xx_write16(bcm, 0x03E2, regstack[3] | 0x8000);
	regstack[4] = bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT);
	regstack[5] = bcm43xx_phy_read(bcm, 0x15);
	regstack[6] = bcm43xx_phy_read(bcm, 0x2A);
	regstack[7] = bcm43xx_phy_read(bcm, 0x35);
	regstack[8] = bcm43xx_phy_read(bcm, 0x60);
	regstack[9] = bcm43xx_radio_read16(bcm, 0x43);
	regstack[10] = bcm43xx_radio_read16(bcm, 0x7A);
	regstack[11] = bcm43xx_radio_read16(bcm, 0x52);
	if (phy->connected) {
		regstack[12] = bcm43xx_phy_read(bcm, 0x0811);
		regstack[13] = bcm43xx_phy_read(bcm, 0x0812);
		regstack[14] = bcm43xx_phy_read(bcm, 0x0814);
		regstack[15] = bcm43xx_phy_read(bcm, 0x0815);
	}
	bcm43xx_radio_selectchannel(bcm, 6, 0);
	if (phy->connected) {
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS, regstack[0] & 0x7FFF);
		bcm43xx_phy_write(bcm, 0x0802, regstack[1] & 0xFFFC);
		bcm43xx_dummy_transmission(bcm);
	}
	bcm43xx_radio_write16(bcm, 0x0043, 0x0006);

	bcm43xx_phy_set_baseband_attenuation(bcm, 2);

	bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, 0x0000);
	bcm43xx_phy_write(bcm, 0x002E, 0x007F);
	bcm43xx_phy_write(bcm, 0x0078, 0x080F);
	bcm43xx_phy_write(bcm, 0x0035, regstack[7] & ~(1 << 7));
	bcm43xx_radio_write16(bcm, 0x007A, regstack[10] & 0xFFF0);
	bcm43xx_phy_write(bcm, 0x002B, 0x0203);
	bcm43xx_phy_write(bcm, 0x002A, 0x08A3);
	if (bcm->current_core->phy->connected) {
		bcm43xx_phy_write(bcm, 0x0814, regstack[14] | 0x0003);
		bcm43xx_phy_write(bcm, 0x0815, regstack[15] & 0xFFFC);
		bcm43xx_phy_write(bcm, 0x0811, 0x01B3);
		bcm43xx_phy_write(bcm, 0x0812, 0x00B2);
	}
	if (bcm43xx_is_initializing(bcm))
		bcm43xx_phy_lo_g_measure_txctl2(bcm);
	bcm43xx_phy_write(bcm, 0x080F, 0x8078);

#ifdef BCM43xx_DEBUG
	{
		/* Poison all LOpairs. */
		for (i = 0; i < BCM43xx_LO_COUNT; i++) {
			tmp_control = bcm->current_core->phy->_lo_pairs + i;
			tmp_control->high = -20;
			tmp_control->low = -20;
		}
	}
#endif /* BCM43xx_DEBUG */

	/* Measure */
	control.low = 0;
	control.high = 0;
	for (h = 0; h < 10; h++) {
		/* Loop over each possible RadioAttenuation (0-9) */
		i = pairorder[h];
		if (bcm43xx_is_initializing(bcm)) {
			if (i == 3) {
				control.low = 0;
				control.high = 0;
			} else if (((i % 2 == 1) && (oldi % 2 == 1)) ||
				  ((i % 2 == 0) && (oldi % 2 == 0))) {
				control.low = bcm43xx_get_lopair(phy, oldi, 0)->low;
				control.high = bcm43xx_get_lopair(phy, oldi, 0)->high;
			} else {
				control.low = bcm43xx_get_lopair(phy, 3, 0)->low;
				control.high = bcm43xx_get_lopair(phy, 3, 0)->high;
			}
		}
		/* Loop over each possible BasebandAttenuation/2 */
		for (j = 0; j < 4; j++) {
			if (bcm43xx_is_initializing(bcm)) {
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
				control.low = bcm43xx_get_lopair(phy, i, j * 2)->low;
				control.high = bcm43xx_get_lopair(phy, i, j * 2)->high;
				r27 = 3;
				r31 = 1;
			}
			bcm43xx_radio_write16(bcm, 0x43, i);
			bcm43xx_radio_write16(bcm, 0x52,
					      bcm->current_core->radio->txpower[3]);
			udelay(10);

			bcm43xx_phy_set_baseband_attenuation(bcm, j * 2);

			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm43xx_radio_write16(bcm, 0x007A, tmp);

			tmp_control = bcm43xx_get_lopair(phy, i, j * 2);
			bcm43xx_phy_lo_g_state(bcm, &control, tmp_control, r27);
		}
		oldi = i;
	}
	/* Loop over each possible RadioAttenuation (10-13) */
	for (i = 10; i < 14; i++) {
		/* Loop over each possible BasebandAttenuation/2 */
		for (j = 0; j < 4; j++) {
			if (bcm43xx_is_initializing(bcm)) {
				control.low = bcm43xx_get_lopair(phy, i - 9, j * 2)->low;
				control.high = bcm43xx_get_lopair(phy, i - 9, j * 2)->high;
				tmp = (i - 9) * 2 + j - 5;
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
				control.low = bcm43xx_get_lopair(phy, i - 9, j * 2)->low;
				control.high = bcm43xx_get_lopair(phy, i - 9, j * 2)->high;
				r27 = 3;
				r31 = 1;
			}
			bcm43xx_radio_write16(bcm, 0x43, i - 9);
			bcm43xx_radio_write16(bcm, 0x52,
					      bcm->current_core->radio->txpower[3]
					      | (3/*txctl1*/ << 4));//FIXME: shouldn't txctl1 be zero here and 3 in the loop above?
			udelay(10);

			bcm43xx_phy_set_baseband_attenuation(bcm, j * 2);

			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm43xx_radio_write16(bcm, 0x7A, tmp);

			tmp_control = bcm43xx_get_lopair(phy, i, j * 2);
			bcm43xx_phy_lo_g_state(bcm, &control, tmp_control, r27);
		}
	}

	/* Restoration */
	if (phy->connected) {
		bcm43xx_phy_write(bcm, 0x0015, 0xE300);
		bcm43xx_phy_write(bcm, 0x0812, (r27 << 8) | 0xA0);
		udelay(5);
		bcm43xx_phy_write(bcm, 0x0812, (r27 << 8) | 0xA2);
		udelay(2);
		bcm43xx_phy_write(bcm, 0x0812, (r27 << 8) | 0xA3);
	} else
		bcm43xx_phy_write(bcm, 0x0015, r27 | 0xEFA0);
	bcm43xx_phy_lo_adjust(bcm, bcm43xx_is_initializing(bcm));
	bcm43xx_phy_write(bcm, 0x002E, 0x807F);
	if (phy->connected)
		bcm43xx_phy_write(bcm, 0x002F, 0x0202);
	else
		bcm43xx_phy_write(bcm, 0x002F, 0x0101);
	bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, regstack[4]);
	bcm43xx_phy_write(bcm, 0x0015, regstack[5]);
	bcm43xx_phy_write(bcm, 0x002A, regstack[6]);
	bcm43xx_phy_write(bcm, 0x0035, regstack[7]);
	bcm43xx_phy_write(bcm, 0x0060, regstack[8]);
	bcm43xx_radio_write16(bcm, 0x0043, regstack[9]);
	bcm43xx_radio_write16(bcm, 0x007A, regstack[10]);
	regstack[11] &= 0x00F0;
	regstack[11] |= (bcm43xx_radio_read16(bcm, 0x52) & 0x000F);
	bcm43xx_radio_write16(bcm, 0x52, regstack[11]);
	bcm43xx_write16(bcm, 0x03E2, regstack[3]);
	if (phy->connected) {
		bcm43xx_phy_write(bcm, 0x0811, regstack[12]);
		bcm43xx_phy_write(bcm, 0x0812, regstack[13]);
		bcm43xx_phy_write(bcm, 0x0814, regstack[14]);
		bcm43xx_phy_write(bcm, 0x0815, regstack[15]);
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS, regstack[0]);
		bcm43xx_phy_write(bcm, 0x0802, regstack[1]);
	}
	bcm43xx_radio_selectchannel(bcm, oldchannel, 1);

#ifdef BCM43xx_DEBUG
	{
		/* Sanity check for all lopairs. */
		for (i = 0; i < BCM43xx_LO_COUNT; i++) {
			tmp_control = bcm->current_core->phy->_lo_pairs + i;
			if (tmp_control->low < -8 || tmp_control->low > 8 ||
			    tmp_control->high < -8 || tmp_control->high > 8) {
				printk(KERN_WARNING PFX
				       "WARNING: Invalid LOpair (low: %d, high: %d, index: %d)\n",
				       tmp_control->low, tmp_control->high, i);
			}
		}
	}
#endif /* BCM43xx_DEBUG */
}

static
void bcm43xx_phy_lo_mark_current_used(struct bcm43xx_private *bcm)
{
	struct bcm43xx_lopair *pair;

	pair = bcm43xx_current_lopair(bcm);
	pair->used = 1;
}

void bcm43xx_phy_lo_mark_all_unused(struct bcm43xx_private *bcm)
{
	struct bcm43xx_lopair *pair;
	int i;

	for (i = 0; i < BCM43xx_LO_COUNT; i++) {
		pair = bcm->current_core->phy->_lo_pairs + i;
		pair->used = 0;
	}
}

/* http://bcm-specs.sipsolutions.net/EstimatePowerOut
 * This function converts a TSSI value to dBm.
 */
static s8 bcm43xx_phy_estimate_power_out(struct bcm43xx_private *bcm, s8 tssi)
{
	s8 tssi2dbm = 0;
	s32 tmp;

	tmp = bcm->current_core->phy->idle_tssi;
	tmp += tssi;
	tmp -= (s8)(bcm->current_core->phy->savedpctlreg);

	switch (bcm->current_core->phy->type) {
		case BCM43xx_PHYTYPE_A:
			tmp += 0x80;
			tmp = limit_value(tmp, 0x00, 0xFF);
			tssi2dbm = bcm->current_core->phy->tssi2dbm[tmp];
			TODO(); //TODO: There's a FIXME on the specs
			break;
		case BCM43xx_PHYTYPE_B:
		case BCM43xx_PHYTYPE_G:
			tmp = limit_value(tmp, 0x00, 0x3F);
			tssi2dbm = bcm->current_core->phy->tssi2dbm[tmp];
			break;
		default:
			assert(0);
	}

	return tssi2dbm;
}

/* http://bcm-specs.sipsolutions.net/RecalculateTransmissionPower */
void bcm43xx_phy_xmitpower(struct bcm43xx_private *bcm)
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
	if ((bcm->board_type == 0x0416) &&
	    (bcm->board_vendor == PCI_VENDOR_ID_BROADCOM))
		return;

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:

		TODO(); //TODO: Nothing for A PHYs yet :-/

		break;
	case BCM43xx_PHYTYPE_B:
	case BCM43xx_PHYTYPE_G:

		tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x0058);
		v0 = (s8)(tmp & 0x00FF);
		v1 = (s8)((tmp & 0xFF00) >> 8);
		tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x005A);
		v2 = (s8)(tmp & 0x00FF);
		v3 = (s8)((tmp & 0xFF00) >> 8);
		tmp = 0;
		if (v0 == 0x7F || v1 == 0x7F || v2 == 0x7F || v3 == 0x7F) {
			tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x0070);
			v0 = (s8)(tmp & 0x00FF);
			v1 = (s8)((tmp & 0xFF00) >> 8);
			tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x0072);
			v2 = (s8)(tmp & 0x00FF);
			v3 = (s8)((tmp & 0xFF00) >> 8);
			if (v0 == 0x7F || v1 == 0x7F || v2 == 0x7F || v3 == 0x7F)
				return;
			v0 = (v0 + 0x20) & 0x3F;
			v1 = (v1 + 0x20) & 0x3F;
			v2 = (v2 + 0x20) & 0x3F;
			v3 = (v3 + 0x20) & 0x3F;
			tmp = 1;
		}
		bcm43xx_radio_clear_tssi(bcm);

		average = (v0 + v1 + v2 + v3 + 2) / 4;

		if (tmp && (bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x005E) & 0x8))
			average -= 13;
		estimated_pwr = bcm43xx_phy_estimate_power_out(bcm, average);

		max_pwr = bcm->sprom.maxpower_bgphy;
		
		if ((bcm->sprom.boardflags & BCM43xx_BFL_PACTRL)
		    && (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G))
			max_pwr -= 0x3;
		
		max_pwr -= bcm->sprom.antennagain_bgphy + 0x6;
		
		//TODO: Limit max_pwr as per the regulatory domain.
		
		/*TODO: Get desired_pwr from wx_handlers or the stack
		limit_value(desired_pwr, 0, max_pwr);
		*/

		desired_pwr = max_pwr; /* remove this when we have a real desired_pwr */
		
		pwr_adjust = estimated_pwr - desired_pwr;
		radio_att_delta = -(pwr_adjust + 7) >> 3;
		baseband_att_delta = -(pwr_adjust >> 1) - (4 * radio_att_delta);
		if ((radio_att_delta == 0) && (baseband_att_delta == 0)) {
			bcm43xx_phy_lo_mark_current_used(bcm);
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
		while (baseband_attenuation < 0 && radio_attenuation > 0) {
			radio_attenuation--;
			baseband_attenuation += 4;
		}
		/* adjust maximum values */
		while (baseband_attenuation > 11 && radio_attenuation < 9) {
			baseband_attenuation -= 4;
			radio_attenuation++;
		}
		if (radio_attenuation < 0)
			radio_attenuation = 0;

		txpower = bcm->current_core->radio->txpower[2];
		if ((bcm->current_core->radio->version == 0x2050) &&
		    !((bcm->current_core->radio->revision > 2) &&
		      (bcm->current_core->radio->revision < 6))) {
			if (radio_attenuation <= 1) {
				if (txpower == 0) {
					txpower = 3;
					radio_attenuation += 2;
					baseband_attenuation += 2;
				} else if (bcm->sprom.boardflags & BCM43xx_BFL_PACTRL) {
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

		bcm43xx_phy_lock(bcm);
		bcm43xx_radio_lock(bcm);
		bcm43xx_radio_set_txpower_bg(bcm, baseband_attenuation,
					     radio_attenuation, txpower);
		bcm43xx_phy_lo_mark_current_used(bcm);
		bcm43xx_radio_unlock(bcm);
		bcm43xx_phy_unlock(bcm);
		break;
	default:
		assert(0);
	}
}

/* http://bcm-specs.sipsolutions.net/TSSI_to_DBM_Table */
int bcm43xx_phy_init_tssi2dbm_table(struct bcm43xx_private *bcm)
{
	s16 pab0, pab1, pab2;

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A) {
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
		bcm->current_core->phy->tssi2dbm = bcm43xx_tssi2dbm_b_table;
		return 0;
	}

	if (pab0 != 0 && pab1 != 0 && pab2 != 0 &&
	    pab0 != -1 && pab1 != -1 && pab2 != -1) {
		/* The pabX values are set in SPROM. Use them. */
		if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A)
			bcm->current_core->phy->idle_tssi = (s8)(bcm->sprom.idle_tssi_tgt_aphy);
		else
			bcm->current_core->phy->idle_tssi = (s8)(bcm->sprom.idle_tssi_tgt_bgphy);
		if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
			TODO(); //TODO: incomplete specs.
		}
		TODO(); //TODO: incomplete specs.
		return -ENODEV;
	} else {
		/* pabX values not set in SPROM. */
		switch (bcm->current_core->phy->type) {
		case BCM43xx_PHYTYPE_A:
			/* APHY needs a generated table. */
			bcm->current_core->phy->tssi2dbm = NULL;
			printk(KERN_ERR PFX "Could not generate tssi2dBm "
					    "table (wrong SPROM info)!\n");
			return -ENODEV;
		case BCM43xx_PHYTYPE_B:
			bcm->current_core->phy->idle_tssi = 0x34;
			bcm->current_core->phy->tssi2dbm = bcm43xx_tssi2dbm_b_table;
			break;
		case BCM43xx_PHYTYPE_G:
			bcm->current_core->phy->idle_tssi = 0x34;
			bcm->current_core->phy->tssi2dbm = bcm43xx_tssi2dbm_g_table;
			break;
		}
	}

	return 0;
}

int bcm43xx_phy_init(struct bcm43xx_private *bcm)
{
	int err = -ENODEV;
	unsigned long flags;

	/* We do not want to be preempted while calibrating
	 * the hardware.
	 */
	local_irq_save(flags);

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		if ((bcm->current_core->phy->rev == 2) || (bcm->current_core->phy->rev == 3)) {
			bcm43xx_phy_inita(bcm);
			err = 0;
		}
		break;
	case BCM43xx_PHYTYPE_B:
		switch (bcm->current_core->phy->rev) {
		case 2:
			bcm43xx_phy_initb2(bcm);
			err = 0;
			break;
		case 4:
			bcm43xx_phy_initb4(bcm);
			err = 0;
			break;
		case 5:
			bcm43xx_phy_initb5(bcm);
			err = 0;
			break;
		case 6:
			bcm43xx_phy_initb6(bcm);
			err = 0;
			break;
		}
		break;
	case BCM43xx_PHYTYPE_G:
		bcm43xx_phy_initg(bcm);
		err = 0;
		break;
	}
	if (err)
		printk(KERN_WARNING PFX "Unknown PHYTYPE found!\n");
	local_irq_restore(flags);

	return err;
}

void bcm43xx_phy_set_antenna_diversity(struct bcm43xx_private *bcm)
{
	u16 antennadiv;
	u16 offset;
	u16 value;
	u32 ucodeflags;

	antennadiv = bcm->current_core->phy->antenna_diversity;

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A &&
	    bcm->current_core->phy->rev == 3)
		antennadiv = 0;
	if (antennadiv == 0xFFFF)
		antennadiv = 3;

	assert(antennadiv <= 3);

	ucodeflags = bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED,
					BCM43xx_UCODEFLAGS_OFFSET);
	bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED,
			    BCM43xx_UCODEFLAGS_OFFSET,
			    ucodeflags & ~BCM43xx_UCODEFLAG_AUTODIV);

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
	case BCM43xx_PHYTYPE_G:
		if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_A)
			offset = 0x0000;
		else
			offset = 0x0400;

		if (antennadiv == 2)
			value = (3/*automatic*/ << 7);
		else
			value = (antennadiv << 7);
		bcm43xx_phy_write(bcm, offset + 1,
				  (bcm43xx_phy_read(bcm, offset + 1)
				   & 0x7E7F) | value);

		if (antennadiv >= 2) {
			if (antennadiv == 2)
				value = (antennadiv << 7);
			else
				value = (0/*force0*/ << 7);
			bcm43xx_phy_write(bcm, offset + 0x2B,
					  (bcm43xx_phy_read(bcm, offset + 0x2B)
					   & 0xFEFF) | value);
		}

		if (!bcm->current_core->phy->connected) {
			if (antennadiv < 2)
				value = 0;
			else
				value = 0x2000;
			bcm43xx_phy_write(bcm, 0x048C,
					  (bcm43xx_phy_read(bcm, 0x048C)
					   & 0xDFFF) | value);
			if (bcm->current_core->phy->rev >= 2) {
				bcm43xx_phy_write(bcm, 0x0461,
						  bcm43xx_phy_read(bcm, 0x0461) | 0x0010);
				bcm43xx_phy_write(bcm, 0x04AD,
						  (bcm43xx_phy_read(bcm, 0x04AD) & 0xFF00) | 0x0015);
				bcm43xx_phy_write(bcm, 0x0427, 0x0008);
			}
		}
		break;
	case BCM43xx_PHYTYPE_B:
		if (bcm->current_core->rev == 2 || antennadiv == 2)
			value = (3/*automatic*/ << 7);
		else
			value = (antennadiv << 7);
		bcm43xx_phy_write(bcm, 0x03E2,
				  (bcm43xx_phy_read(bcm, 0x03E2)
				   & 0xFE7F) | value);
		break;
	default:
		assert(0);
	}

	if (antennadiv >= 2) {
		ucodeflags = bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED,
						BCM43xx_UCODEFLAGS_OFFSET);
		bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED,
				    BCM43xx_UCODEFLAGS_OFFSET,
				    ucodeflags | BCM43xx_UCODEFLAG_AUTODIV);
	}

	bcm->current_core->phy->antenna_diversity = antennadiv;
}
