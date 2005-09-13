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

#include <linux/types.h>
#include <linux/pci.h>

#include "bcm430x.h"
#include "bcm430x_phy.h"
#include "bcm430x_main.h"
#include "bcm430x_radio.h"

static void bcm430x_phy_initg(struct bcm430x_private *bcm);

u16 bcm430x_phy_read(struct bcm430x_private *bcm, u16 offset)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_PHY_DATA);
}

void bcm430x_phy_write(struct bcm430x_private *bcm, int offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_PHY_DATA, val);
}

void bcm430x_phy_calibrate(struct bcm430x_private *bcm)
{
	bcm430x_read32(bcm, BCM430x_MMIO_STATUS_BITFIELD); // Dummy read
	if (bcm->status & BCM430x_STAT_PHYCALIBRATED)
		return;
	if (bcm->phy_type == BCM430x_PHYTYPE_A)
		bcm430x_radio_set_txpower_a(bcm, 0x0018);
	else {
		//FIXME: (Only variables are set, most have unknown usage, working on it)
	}
	if ((bcm->phy_type == BCM430x_PHYTYPE_G) && (bcm->phy_rev == 1))
	{
		bcm->status &= ~BCM430x_STAT_PHYCONNECTED;
		//FIXME: which core to reset?
		//bcm430x_core_reset(bcm, ???);
		bcm430x_phy_initg(bcm);
		bcm->status |= BCM430x_STAT_PHYCONNECTED;
		//FIXME: which core to reset?
		//bcm430x_core_reset(bcm, ???);
	}
}

/* intialize B PHY power control
 * as described in http://bcm-specs.sipsolutions.net/InitPowerControl
 */
static void bcm430x_phy_init_pctl(struct bcm430x_private *bcm)
{
	u16 t_batt = 0xFFFF;
	u16 t_ratt = 0xFFFF;
	u16 t_txatt = 0xFFFF;
	u16 phy29;

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) && (bcm->board_type == 0x0416)) {
		return;
	}

	bcm430x_write16(bcm, 0x03E6, bcm430x_read16(bcm, 0x03E6) & 0xFFDF);
	bcm430x_phy_write(bcm, 0x0028, 0x8018);

	if (bcm->phy_type == BCM430x_PHYTYPE_G) {
		if (bcm->status & BCM430x_STAT_PHYCONNECTED) {
			bcm430x_phy_write(bcm, 0x047A, 0xC111);
		} else {
			return;
		}
	}
#if 0
	// FIXME: saved variable
	if (saved variable != 0) {
			return 0;
	}
#endif
	if ((bcm->phy_type == BCM430x_PHYTYPE_B) && (bcm->phy_rev >= 2) && ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_radio_write16(bcm, 0x0076, bcm430x_radio_read16(bcm, 0x0076) | 0x0084);
	} else {
		t_batt = bcm->radio_txpower[0];
		t_ratt = bcm->radio_txpower[1];
		t_txatt = bcm->radio_txpower[2];
		bcm430x_radio_set_txpower_b(bcm, 0x000B, 0x0009, 0x0000);
	}

	bcm430x_dummy_transmission(bcm);

	phy29 = bcm430x_phy_read(bcm, 0x0029);

	if ((bcm->phy_type != BCM430x_PHYTYPE_B) || (bcm->phy_rev < 2) || ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000)) {
		bcm430x_radio_set_txpower_b(bcm, t_batt, t_ratt, t_txatt);
	}

#if 0
	//FIXME: saved variable
	saved variable = phy29;
#endif
	bcm430x_radio_write16(bcm, 0x0076, bcm430x_radio_read16(bcm, 0x0076) & 0xFF7B);

	//FIXME: FuncPlaceHolder
}

static void bcm430x_phy_inita(struct bcm430x_private *bcm)
{
	//FIXME: APHYSetup
	if (bcm->phy_type != BCM430x_PHYTYPE_A) {
		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			bcm430x_phy_write(bcm, 0x046E, 0x03CF);
		return;
	}
	bcm430x_phy_write(bcm, 0x0029,
	                  (bcm430x_phy_read(bcm, 0x0029) & 0xF83C) | 0x340);
	//FIXME: FuncPlaceHolder x 3
	if (bcm->radio_id != BCM430x_RADIO_ID_NORF) {
		bcm430x_radio_init2060(bcm);

		if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM)
		    && ((bcm->board_type == 0x0416) || (bcm->board_type == 0x040A))) {
#if 0
			//FIXME: unk31A
			if (unk31A == -1) {
				//FIXME: FuncPlaceholder x 2
			} else {
				bcm430x_radio_write16(bcm, 0x001E, unk31A);
			}
#endif
		}
		bcm430x_phy_write(bcm, 0x007A, 0xF111);
#if 0
		//FIXME: unk154
		if (!unk154) {
			bcm430x_radio_write16(bcm, 0x0019, 0x0000);
			bcm430x_radio_write16(bcm, 0x0017, 0x0020);

			/* FIXME: Read 7273 offset 0x3001 into tmp */
			if (bcm->phy_rev == 1) {
				/* FIXME: MaskSet 7273 offset 0x3001, mask 0xFF87, set 0x58 */
			} else {
				/* FIXME: MaskSet 7273 offset 0x3001, mask 0xFFC3, set 0x2C */
			}

			bcm430x_dummy_transmission(bcm);
			unk154 = bcm430x_phy_read(bcm, 0x007B);

			/* FIXME: restore 7273 offsett 0x3001 */
			/* FIXME: FuncPlaceholder, not yet documented */
		}
#endif
		/* FIXME: FuncPlaceholder, not yet documented */
	}
	// no radio
}

static void bcm430x_phy_initb2(struct bcm430x_private *bcm)
{
	u16 offset, val = 0x3C3D;
	
	if (bcm->radio_id == BCM430x_RADIO_ID_NORF) {
		bcm430x_phy_write(bcm, 0x0046, 0x0007);
		bcm430x_phy_write(bcm, 0x0020, 0x281E);
		bcm430x_write16(bcm, 0x03EC, 0x3F22);
		return;
	}
	bcm430x_write16(bcm, 0x03EC, 0x3F22);
	bcm430x_phy_write(bcm, 0x0020, 0x301C);
	bcm430x_phy_write(bcm, 0x0026, 0x0000);
	bcm430x_phy_write(bcm, 0x0030, 0x00C6);
	bcm430x_phy_write(bcm, 0x0088, 0x3E00);
	for (offset = 0x0089; offset < 0x00a7; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm430x_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->curr_channel == 0xFFFF) {
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
	} else {
		bcm430x_radio_selectchannel(bcm, bcm->curr_channel);
	}
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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
	//FIXME: FuncPlaceholder (BPHY2_Measurelo)
	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	}
	bcm430x_write16(bcm, 0x03F4, 0x1000);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	}
	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb4(struct bcm430x_private *bcm)
{
	u16 offset, val = 0x3C3D;

	if (bcm->radio_id == BCM430x_RADIO_ID_NORF) {
		bcm430x_phy_write(bcm, 0x0020, 0x281E);
		bcm430x_write16(bcm, 0x03F4, 0x0100);
		return;
	}
	bcm430x_write16(bcm, 0x03EC, 0x3F22);
	bcm430x_phy_write(bcm, 0x0020, 0x301C);
	bcm430x_phy_write(bcm, 0x0026, 0x0000);
	bcm430x_phy_write(bcm, 0x0030, 0x00C6);
	bcm430x_phy_write(bcm, 0x0088, 0x3E00);
	for (offset = 0x0089; offset < 0x00a7; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	bcm430x_phy_write(bcm, 0x03E4, 0x3000);
	if (bcm->curr_channel == 0xFFFF) {
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
	} else {
		bcm430x_radio_selectchannel(bcm, bcm->curr_channel);
	}
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x0032, 0x00E0);
	bcm430x_phy_write(bcm, 0x0035, 0x07C2);
	//FIXME: FuncPlaceholder (BPHY2 Measurelo)
	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	bcm430x_write16(bcm, 0x03F4, 0x1100);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	if (bcm->sprom.boardflags & BCM430x_BFL_RSSI) {
		//FIXME: FuncPlaceholder
		//FIXME: FuncPlaceholder
	}
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb5(struct bcm430x_private *bcm)
{
	u16 offset;

	if ((bcm->phy_rev == 1) && ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0050);
	}

	if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM) && (bcm->board_type != 0x0416)) {
		for (offset = 0x00A8 ; offset < 0x00C7; offset++) {
			bcm430x_phy_write(bcm, offset, (bcm430x_phy_read(bcm, offset) + 0x2020) & 0x3F3F);
		}
	}

	bcm430x_phy_write(bcm, 0x0035, (bcm430x_phy_read(bcm, 0x0035) & 0xF0FF) | 0x0700);

	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_phy_write(bcm, 0x0038, 0x0667);
	}

	if (bcm->status & BCM430x_STAT_PHYCONNECTED) {
		if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
			bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0020);
			bcm430x_radio_write16(bcm, 0x0051, bcm430x_radio_read16(bcm, 0x0051) | 0x0004);
		}

		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO, 0x0000);

		bcm430x_phy_write(bcm, 0x0802, bcm430x_phy_read(bcm, 0x0802) | 0x0100);
		bcm430x_phy_write(bcm, 0x042B, bcm430x_phy_read(bcm, 0x042B) | 0x2000);

		bcm430x_phy_write(bcm, 0x001C, 0x186A);

		bcm430x_phy_write(bcm, 0x0013, (bcm430x_phy_read(bcm, 0x0013) & 0x00FF) | 0x1900);
		bcm430x_phy_write(bcm, 0x0035, (bcm430x_phy_read(bcm, 0x0035) & 0xFFC0) | 0x0064);
		bcm430x_phy_write(bcm, 0x005D, (bcm430x_phy_read(bcm, 0x005D) & 0xFF80) | 0x000A);
	}

	//FIXME: roam_delta not yet documented
	//if (roam_delta) {
	//	bcm430x_phy_write(bcm, 0x0401, (bcm430x_phy_read(bcm, 0x0401) & 0x0000) | 0x1000);
	//}

	if ((bcm->phy_rev == 1) && ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
		bcm430x_phy_write(bcm, 0x0021, 0x3763);
		bcm430x_phy_write(bcm, 0x0022, 0x1BC3);
		bcm430x_phy_write(bcm, 0x0023, 0x06F9);
		bcm430x_phy_write(bcm, 0x0024, 0x037E);
	} else {
		bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	}

	bcm430x_phy_write(bcm, 0x0030, 0x00C6);

	bcm430x_write16(bcm, 0x03EC, 0x3F22);

	if (bcm->radio_id == BCM430x_RADIO_ID_NORF) {
		bcm430x_phy_write(bcm, 0x0020, 0x281E);
		return;
	}

	if ((bcm->phy_rev == 1) && ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_phy_write(bcm, 0x0020, 0x3E1C);
	} else {
		bcm430x_phy_write(bcm, 0x0020, 0x301C);
	}

	if (bcm->phy_rev == 0) {
		bcm430x_write16(bcm, 0x03E4, 0x3000);
	}

	//XXX: Must be 7!
	bcm430x_radio_selectchannel(bcm, 7);

	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}

	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);

	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0050, 0x0020);
		bcm430x_radio_write16(bcm, 0x005A, 0x0070);
	}

	bcm430x_radio_write16(bcm, 0x005B, 0x007B);
	bcm430x_radio_write16(bcm, 0x005C, 0x00B0);

	bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0007);

	bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);

	bcm430x_phy_write(bcm, 0x0014, 0x0080);
	bcm430x_phy_write(bcm, 0x0032, 0x00CA);
	bcm430x_phy_write(bcm, 0x88A3, 0x88A3);

	/* FIXME: FuncPlaceholder, not yet documented */

	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_radio_write16(bcm, 0x005D, 0x000D);
	}

	bcm430x_write16(bcm, 0x03E4, (bcm430x_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
}

static void bcm430x_phy_initb6(struct bcm430x_private *bcm) {
	u16 offset, val = 0x1E1F;

	bcm430x_radio_write16(bcm, 0x007A,
	                      (bcm430x_radio_read16(bcm, 0x007A) | 0x0050));
	if (bcm->radio_id == 0x3205017F) {
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
	bcm430x_phy_write(bcm, 0x0088, 0x1E1F);
	for (offset = 0x0088; offset < 0x0098; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x3E3F;
	for (offset = 0x0098; offset < 0x00A9; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	val = 0x2120;
	for (offset = 0x00A8; offset < 0x00C9; offset++) {
		bcm430x_phy_write(bcm, offset, val);
		val -= 0x0202;
	}
	if (bcm->phy_type == BCM430x_PHYTYPE_G) {
		bcm430x_radio_write16(bcm, 0x007A,
		                      bcm430x_radio_read16(bcm, 0x007A) | 0x0020);
		bcm430x_radio_write16(bcm, 0x0051,
		                      bcm430x_radio_read16(bcm, 0x0051) | 0x0002);
		bcm430x_radio_write16(bcm, 0x0802,
		                      bcm430x_radio_read16(bcm, 0x0802) | 0x0100);
		bcm430x_radio_write16(bcm, 0x042B,
		                      bcm430x_radio_read16(bcm, 0x042B) | 0x2000);
	}
	if (bcm->radio_id == BCM430x_RADIO_ID_NORF) {
		bcm430x_write16(bcm, 0x03EC, 0x3206);
		bcm430x_phy_write(bcm, 0x0020, 0x281E);
		bcm430x_phy_write(bcm, 0x0026,
		                  bcm430x_phy_read(bcm, 0x0026) | 0x001A);
		return;
	}
	bcm430x_radio_selectchannel(bcm, 7);
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x005A, 0x0070);
	bcm430x_radio_write16(bcm, 0x005B, 0x007B);
	bcm430x_radio_write16(bcm, 0x005C, 0x00B0);
	bcm430x_radio_write16(bcm, 0x007A,
	                      (bcm430x_radio_read16(bcm, 0x007A) & 0x00F8) | 0x0007);
	bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
	bcm430x_phy_write(bcm, 0x0014, 0x0200);
	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm430x_radio_write16(bcm, 0x0052,
	                      (bcm430x_radio_read16(bcm, 0x0052) & 0x00F0) | 0x0009);
	bcm430x_radio_write16(bcm, 0x005D, 0x000D);
	bcm430x_write16(bcm, 0x03E4,
	                (bcm430x_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
	if (bcm->phy_type == BCM430x_PHYTYPE_B) {
		bcm430x_phy_write(bcm, 0x0016, 0x5410);
		bcm430x_phy_write(bcm, 0x0017, 0xA820);
		bcm430x_phy_write(bcm, 0x0007, 0x0062);
		bcm430x_phy_init_pctl(bcm);
	}
}

static void bcm430x_phy_initg(struct bcm430x_private *bcm)
{
	if (bcm->phy_rev == 1)
		bcm430x_phy_initb5(bcm);
	if (bcm->phy_rev == 2)
		bcm430x_phy_initb6(bcm);
	if ((bcm->phy_rev >= 2) || (bcm->status & BCM430x_STAT_PHYCONNECTED))
		bcm430x_phy_inita(bcm);
	if (bcm->phy_rev >= 2) {
		bcm430x_phy_write(bcm, 0x003E, 0x817A);
		bcm430x_phy_write(bcm, 0x0814, 0x0000);
		bcm430x_phy_write(bcm, 0x0815, 0x0000);
		bcm430x_phy_write(bcm, 0x0811, 0x0000);
		bcm430x_phy_write(bcm, 0x0015, 0x00C0);
		bcm430x_phy_write(bcm, 0x1816, 0x04C2);
		bcm430x_phy_write(bcm, 0x8606, 0x04C3);
	}
	if (bcm->radio_id == BCM430x_RADIO_ID_NORF)
		return;
	//FIXME: Add element to struct bcm430x_private for keeping retval
	//       of _radio_initXXXX()
#if 0
	if (retval == -1) {
		retval = bcm430x_radio_init2050(bcm);
		//FIXME: GPHYMeasureLo
	} else {
		bcm430x_radio_write16(bcm, 0x0078, retval);
	}
#endif

	if (bcm->status & BCM430x_STAT_PHYCONNECTED) {
		//FIXME: FuncPlaceholder
		bcm430x_phy_write(bcm, 0x080F, 0x8078);

		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL) {
			bcm430x_phy_write(bcm, 0x002E, 0x807F);
		} else {
			bcm430x_phy_write(bcm, 0x002E, 0x8075);
		}

		if (bcm->phy_rev < 2) {
			bcm430x_phy_write(bcm, 0x002F, 0x0101);
		} else {
			bcm430x_phy_write(bcm, 0x002F, 0x0202);
		}
	}

	if ((bcm->sprom.boardflags & BCM430x_BFL_RSSI) == 0) {
		//FIXME: FuncPlaceholder x 2
	} else {
#if 0
		if (phy_unkCC && phy_inkD0) {
			//FIXME: FuncPlaceholder
		} else {
			//FIXME: FuncPlaceholder
		}
#endif
	}
	//FIXME: FuncPlaceholder
}


int bcm430x_phy_init(struct bcm430x_private *bcm)
{
	int initialized = 0;

	switch (bcm->phy_type) {
	case BCM430x_PHYTYPE_A:
		if ((bcm->phy_rev == 2) || (bcm->phy_rev == 3)) {
			bcm430x_phy_inita(bcm);
			initialized = 1;
		}
		break;
	case BCM430x_PHYTYPE_B:
		switch (bcm->phy_rev) {
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
	u16 offset;
	u32 ucodeflags;

	if ((bcm->phy_type == BCM430x_PHYTYPE_A) && (bcm->phy_rev == 3)) {
		bcm->antenna_diversity = 0;
	}

	if (bcm->antenna_diversity == 0xFFFF) {
		bcm->antenna_diversity = 0x180;
	}

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
	ucodeflags = bcm430x_shm_read32(bcm);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
	bcm430x_shm_write32(bcm, ucodeflags & ~BCM430x_UCODEFLAG_AUTODIV);

	switch (bcm->phy_type) {
	case BCM430x_PHYTYPE_A:
		offset = 0x0000;
	case BCM430x_PHYTYPE_G:
		offset = 0x0400;

		if (bcm->antenna_diversity == 0x100) {
			bcm430x_phy_write(bcm, offset + 1, (bcm430x_phy_read(bcm, offset + 1) & 0x7E7F) | 0x0180);
		} else {
			bcm430x_phy_write(bcm, offset + 1, (bcm430x_phy_read(bcm, offset + 1) & 0x7E7F) | bcm->antenna_diversity);
		}

		if (bcm->antenna_diversity == 0x100) {
			bcm430x_phy_write(bcm, offset + 0x002B, (bcm430x_phy_read(bcm, offset + 0x002B) & 0xFEFF) | 0x0100);
		}

		if (bcm->antenna_diversity > 0x100) {
			bcm430x_phy_write(bcm, offset + 0x002B, bcm430x_phy_read(bcm, offset + 0x002B) & 0xFEFF);
		}

		if (!(bcm->status & BCM430x_STAT_PHYCONNECTED)) {
			if (bcm->antenna_diversity < 0x100) {
				bcm430x_phy_write(bcm, 0x048C, bcm430x_phy_read(bcm, 0x048C) & 0xDFFF);
			} else {
				bcm430x_phy_write(bcm, 0x048C, (bcm430x_phy_read(bcm, 0x048C) & 0xDFFF) | 0x2000);
			}

			if (bcm->phy_rev >= 2) {
				bcm430x_phy_write(bcm, 0x0461, bcm430x_phy_read(bcm, 0x0461) | 0x0010);
				bcm430x_phy_write(bcm, 0x04AD, (bcm430x_phy_read(bcm, 0x04AD) & 0xFF00) | 0x0015);
				bcm430x_phy_write(bcm, 0x0427, 0x0008);
			}
		}
		break;
	case BCM430x_PHYTYPE_B:
		if (bcm->current_core->rev == 2) {
			bcm430x_phy_write(bcm, 0x03E2, (bcm430x_phy_read(bcm, 0x03E2) & 0xFE7F) | 0x0180);
		} else {
			if (bcm->antenna_diversity == 0x100) {
				bcm430x_phy_write(bcm, 0x03E2, (bcm430x_phy_read(bcm, 0x03E2) & 0xFE7F) | 0x0180);
			} else {
				bcm430x_phy_write(bcm, 0x03E2, (bcm430x_phy_read(bcm, 0x03E2) & 0xFE7F) | bcm->antenna_diversity);
			}
		}
		break;
	default:
                printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return;
	}

	if (bcm->antenna_diversity >= 0x0100) {
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
		ucodeflags = bcm430x_shm_read32(bcm);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + BCM430x_UCODEFLAGS_OFFSET);
		bcm430x_shm_write32(bcm, ucodeflags |  BCM430x_UCODEFLAG_AUTODIV);
	}
}
