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
	if (bcm->current_core->phy->calibrated)
		return;
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
		bcm430x_radio_set_txpower_a(bcm, 0x0018);
	else {
		//FIXME: (Only variables are set, most have unknown usage, working on it)
	}
	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_G)
	    && (bcm->current_core->phy->rev == 1)) {
		//XXX: Reseting active wireless core for the moment?
		bcm430x_wireless_core_reset(bcm, 0);
		bcm430x_phy_initg(bcm);
		//XXX: See above
		bcm430x_wireless_core_reset(bcm, 1);
	}
}

u16 bcm430x_phy_mls_r15_loop(struct bcm430x_private *bcm) {
	u16 sum = 0;
	int i;
	for (i=9; i>-1; i--){
		bcm430x_phy_write(bcm, 0x0015, 0xAFA0);
		udelay(1);
		bcm430x_phy_write(bcm, 0x0015, 0xEFA0);
		udelay(10);
		bcm430x_phy_write(bcm, 0x0015, 0xFFA0);
		udelay(40);
		sum += bcm430x_phy_read(bcm, 0x002C);
	}

	return sum;
}

void bcm430x_phy_measurelowsig(struct bcm430x_private *bcm)
{
	u16 phy_regs[10];
	u16 radio_regs[4];
	u16 mmio[2];
	u16 mls;
	u16 fval;
	u16 curr_channel;
	int i;
	int j;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_B:
		phy_regs[0] = bcm430x_phy_read(bcm, 0x0015);
		radio_regs[0] = bcm430x_radio_read16(bcm, 0x0052) & 0xFFF0;

		if (bcm->current_core->radio->id == 0x2053) {
			phy_regs[1] = bcm430x_phy_read(bcm, 0x000A);
			phy_regs[2] = bcm430x_phy_read(bcm, 0x002A);
			phy_regs[3] = bcm430x_phy_read(bcm, 0x0035);
			phy_regs[4] = bcm430x_phy_read(bcm, 0x0003);
			phy_regs[5] = bcm430x_phy_read(bcm, 0x0001);
			phy_regs[6] = bcm430x_phy_read(bcm, 0x0030);

			radio_regs[1] = bcm430x_radio_read16(bcm, 0x0043);
			radio_regs[2] = bcm430x_radio_read16(bcm, 0x007A);
			mmio[0] = bcm430x_read16(bcm, 0x03EC);
			radio_regs[3] = bcm430x_radio_read16(bcm, 0x0052) & 0x00F0;

			bcm430x_phy_write(bcm, 0x0030, 0x00FF);
			bcm430x_write16(bcm, 0x03EC, 0x3F3F);
			bcm430x_phy_write(bcm, 0x0035, phy_regs[3] & 0xFF7F);
			bcm430x_radio_write16(bcm, 0x007A, radio_regs[2] & 0xFFF0);
		}
		bcm430x_phy_write(bcm, 0x0015, 0xB000);
		bcm430x_phy_write(bcm, 0x002B, 0x0004);

		if (bcm->current_core->radio->id == 0x2053) {
			bcm430x_phy_write(bcm, 0x002B, 0x0203);
			bcm430x_phy_write(bcm, 0x002A, 0x08A3);
		}

		bcm->current_core->phy->minlowsig[0] = 0xFFFF;

		for (i=0; i<4; i++) {
			bcm430x_radio_write16(bcm, 0x0052, radio_regs[0] | i);
			bcm430x_phy_mls_r15_loop(bcm);
		}
		for (i=0; i<10; i++) {
			bcm430x_radio_write16(bcm, 0x0052, radio_regs[0] | i);
			mls = bcm430x_phy_mls_r15_loop(bcm)/10;
			if (mls < bcm->current_core->phy->minlowsig[0]) {
				bcm->current_core->phy->minlowsig[0] = mls;
				bcm->current_core->phy->minlowsigpos[0] = i;
			}
		}
		bcm430x_radio_write16(bcm, 0x0052, radio_regs[0] | bcm->current_core->phy->minlowsigpos[0]);

		bcm->current_core->phy->minlowsig[1] = 0xFFFF;

		for (i=-4;i<5;i+=2) {
			for (j=-4;j<5;j+=2) {
				if (j<0) {
					fval = (0x0100*i)+j+0x0100;
				} else {
					fval = (0x0100*i)+j;
				}
				bcm430x_phy_write(bcm, 0x002F, fval);
				mls = bcm430x_phy_mls_r15_loop(bcm)/10;
				if (mls < bcm->current_core->phy->minlowsig[1]) {
					bcm->current_core->phy->minlowsig[1] = mls;
					bcm->current_core->phy->minlowsigpos[1] = fval;
				}
			}
		}
		bcm430x_phy_write(bcm, 0x002F, bcm->current_core->phy->minlowsigpos[1]+0x0101);
		if (bcm->current_core->radio->id == 0x2053) {
			bcm430x_phy_write(bcm, 0x000A, phy_regs[1]);
			bcm430x_phy_write(bcm, 0x002A, phy_regs[2]);
			bcm430x_phy_write(bcm, 0x0035, phy_regs[3]);
			bcm430x_phy_write(bcm, 0x0003, phy_regs[4]);
			bcm430x_phy_write(bcm, 0x0001, phy_regs[5]);
			bcm430x_phy_write(bcm, 0x0030, phy_regs[6]);

			bcm430x_radio_write16(bcm, 0x0043, radio_regs[1]);
			bcm430x_radio_write16(bcm, 0x007A, radio_regs[2]);

			bcm430x_radio_write16(bcm, 0x0052, (bcm430x_radio_read16(bcm, 0x0052) & 0x000F) | radio_regs[3]);

			bcm430x_write16(bcm, 0x03EC, mmio[0]);
		}
		bcm430x_phy_write(bcm, 0x0015, phy_regs[0]);
		break;
	case BCM430x_PHYTYPE_G:
		phy_regs[0] = bcm430x_phy_read(bcm, 0x002A);
		phy_regs[1] = bcm430x_phy_read(bcm, 0x0015);
		phy_regs[2] = bcm430x_phy_read(bcm, 0x0035);
		phy_regs[3] = bcm430x_phy_read(bcm, 0x0060);

		radio_regs[0] = bcm430x_radio_read16(bcm, 0x0043);
		radio_regs[1] = bcm430x_radio_read16(bcm, 0x007A);

		mmio[0] = bcm430x_read16(bcm, 0x03F4);
		mmio[1] = bcm430x_read16(bcm, 0x03E2);

		radio_regs[3] = bcm430x_radio_read16(bcm, 0x0052) & 0x00F0;

		if (bcm->current_core->phy->connected) {
			phy_regs[4] = bcm430x_phy_read(bcm, 0x0811);
			phy_regs[5] = bcm430x_phy_read(bcm, 0x0812);
			phy_regs[6] = bcm430x_phy_read(bcm, 0x0814);
			phy_regs[7] = bcm430x_phy_read(bcm, 0x0815);
			phy_regs[8] = bcm430x_phy_read(bcm, 0x0429);
			phy_regs[9] = bcm430x_phy_read(bcm, 0x0802);
		} else {
			phy_regs[4] = 0;
			phy_regs[5] = 0;
			phy_regs[6] = 0;
			phy_regs[7] = 0;
			phy_regs[8] = 0;
			phy_regs[9] = 0;
		}

		curr_channel = bcm->current_core->radio->channel;
		bcm430x_radio_selectchannel(bcm, 6);

		if (!bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x0429, phy_regs[8] & 0x7FFF);
			bcm430x_phy_write(bcm, 0x0802, phy_regs[9] & 0xFFFC);
			bcm430x_dummy_transmission(bcm);
		}
		bcm430x_write16(bcm, 0x03E2, bcm430x_read16(bcm, 0x03E2) | 0x8000);
		bcm430x_radio_write16(bcm, 0x0043, 0x0006);

		if (bcm->current_core->phy->version == 1) {
			bcm430x_phy_write(bcm, 0x0060, (bcm430x_phy_read(bcm, 0x0060) & 0xFF87) | 0x0010);
		} else {
			bcm430x_phy_write(bcm, 0x0060, (bcm430x_phy_read(bcm, 0x0060) & 0xFFC3) | 0x0008);
		}

		bcm430x_write16(bcm, 0x03F4, 0x0000);
		bcm430x_phy_write(bcm, 0x002E, 0x007F);
		bcm430x_phy_write(bcm, 0x080F, 0x0078);
		bcm430x_phy_write(bcm, 0x0035, bcm430x_phy_read(bcm, 0x0035) & 0xFF7F);
		bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) & 0xFFF0);
		bcm430x_phy_write(bcm, 0x002B, 0x0203);
		bcm430x_phy_write(bcm, 0x002A, 0x08A3);

		if (bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x0814, bcm430x_phy_read(bcm, 0x0814) | 0x0003);
			bcm430x_phy_write(bcm, 0x0815, bcm430x_phy_read(bcm, 0x0815) & 0xFFFC);
			bcm430x_phy_write(bcm, 0x0811, 0x01B3);
			bcm430x_phy_write(bcm, 0x0812, 0x00B2);
		}

		//FIXME: Loop1
		bcm430x_phy_write(bcm, 0x080F, 0x8078);
		//FIXME: Loop2
		//FIXME: GPHY Complo
		bcm430x_phy_write(bcm, 0x002E, 0x807F);

		if (!bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x002F, 0x0101);
		} else {
			bcm430x_phy_write(bcm, 0x002F, 0x0202);
		}

		bcm430x_write16(bcm, 0x03F4, mmio[0]);
		bcm430x_write16(bcm, 0x03E2, mmio[1]);

		bcm430x_phy_write(bcm, 0x002A, phy_regs[0]);
		bcm430x_phy_write(bcm, 0x0015, phy_regs[1]);
		bcm430x_phy_write(bcm, 0x0035, phy_regs[2]);
		bcm430x_phy_write(bcm, 0x0060, phy_regs[3]);

		bcm430x_radio_write16(bcm, 0x0043, radio_regs[0]);
		bcm430x_radio_write16(bcm, 0x007A, radio_regs[1]);
		bcm430x_radio_write16(bcm, 0x0052, (bcm430x_radio_read16(bcm, 0x0052) & 0x000F) | radio_regs[3]);

		if (bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x0811, phy_regs[4]);
			bcm430x_phy_write(bcm, 0x0812, phy_regs[5]);
			bcm430x_phy_write(bcm, 0x0814, phy_regs[6]);
			bcm430x_phy_write(bcm, 0x0815, phy_regs[7]);
			bcm430x_phy_write(bcm, 0x0429, phy_regs[8]);
			bcm430x_phy_write(bcm, 0x0802, phy_regs[9]);
		}

		bcm430x_radio_selectchannel(bcm, curr_channel);
		break;
	}
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

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM) && (bcm->board_type == 0x0416)) {
		return;
	}

	bcm430x_write16(bcm, 0x03E6, bcm430x_read16(bcm, 0x03E6) & 0xFFDF);
	bcm430x_phy_write(bcm, 0x0028, 0x8018);

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		if (bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x047A, 0xC111);
		} else {
			return;
		}
	}

	if ( bcm->current_core->phy->savedpctlreg != 0xFFFF ) {
		return;
	}

	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_B)
	    && (bcm->current_core->phy->rev >= 2)
	    && ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_radio_write16(bcm, 0x0076, bcm430x_radio_read16(bcm, 0x0076) | 0x0084);
	} else {
		t_batt = bcm->current_core->radio->txpower[0];
		t_ratt = bcm->current_core->radio->txpower[1];
		t_txatt = bcm->current_core->radio->txpower[2];
		bcm430x_radio_set_txpower_b(bcm, 0x000B, 0x0009, 0x0000);
	}

	bcm430x_dummy_transmission(bcm);

	bcm->current_core->phy->savedpctlreg = bcm430x_phy_read(bcm, BCM430x_PHY_G_PCTL);

	if ((bcm->current_core->phy->type != BCM430x_PHYTYPE_B)
	    || (bcm->current_core->phy->rev < 2)
	    || ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000)) {
		bcm430x_radio_set_txpower_b(bcm, t_batt, t_ratt, t_txatt);
	}

	bcm430x_radio_write16(bcm, 0x0076, bcm430x_radio_read16(bcm, 0x0076) & 0xFF7B);

	bcm430x_radio_clear_tssi(bcm);
}

static void bcm430x_phy_agcsetup(struct bcm430x_private *bcm)
{
	u16 offset = 0x0000;

	if ( bcm->current_core->phy->rev == 1) {
		offset = 0x4C00;
	}

	bcm430x_ilt_write16(bcm, offset, 0x00FE);
	bcm430x_ilt_write16(bcm, offset+1, 0x000D);
	bcm430x_ilt_write16(bcm, offset+2, 0x0013);
	bcm430x_ilt_write16(bcm, offset+3, 0x0019);

	if ( bcm->current_core->phy->rev == 1) {
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

	if ( bcm->current_core->phy->rev == 1) {
		bcm430x_phy_write(bcm, 0x04A2, (bcm430x_phy_read(bcm, 0x04A2) & 0xFFF0) | 0x0007);
	}

	bcm430x_phy_write(bcm, 0x0488, (bcm430x_phy_read(bcm, 0x0488) & 0xFF00) | 0x001C);
	bcm430x_phy_write(bcm, 0x0488, (bcm430x_phy_read(bcm, 0x0488) & 0xC0FF) | 0x0200);
	bcm430x_phy_write(bcm, 0x0496, (bcm430x_phy_read(bcm, 0x0496) & 0xFF00) | 0x001C);
	bcm430x_phy_write(bcm, 0x0489, (bcm430x_phy_read(bcm, 0x0489) & 0xFF00) | 0x0020);
	bcm430x_phy_write(bcm, 0x0489, (bcm430x_phy_read(bcm, 0x0489) & 0xC0FF) | 0x0200);
	bcm430x_phy_write(bcm, 0x0482, (bcm430x_phy_read(bcm, 0x0482) & 0xFF00) | 0x002E);
	bcm430x_phy_write(bcm, 0x0496, (bcm430x_phy_read(bcm, 0x0496) & 0x00FF) | 0x1A00);
	bcm430x_phy_write(bcm, 0x0481, (bcm430x_phy_read(bcm, 0x0481) & 0xFF00) | 0x0028);
	bcm430x_phy_write(bcm, 0x0481, (bcm430x_phy_read(bcm, 0x0481) & 0x00FF) | 0x2C00);

	if ( bcm->current_core->phy->rev == 1) {
		bcm430x_phy_write(bcm, 0x0430, 0x092B);
		bcm430x_phy_write(bcm, 0x041B, (bcm430x_phy_read(bcm, 0x041B) & 0xFFE1) | 0x0002);
	} else {
		bcm430x_phy_write(bcm, 0x041B, bcm430x_phy_read(bcm, 0x041B) & 0xFFE1);
		bcm430x_phy_write(bcm, 0x041F, 0x287A);
		bcm430x_phy_write(bcm, 0x0420, (bcm430x_phy_read(bcm, 0x0420) & 0xFFF0) | 0x0004);
	}

	bcm430x_phy_write(bcm, 0x048A, (bcm430x_phy_read(bcm, 0x048A) & 0x8080) | 0x7874);
	bcm430x_phy_write(bcm, 0x048E, 0x1C00);

	if ( bcm->current_core->phy->rev == 1) {
		bcm430x_phy_write(bcm, 0x04AB, (bcm430x_phy_read(bcm, 0x04AB) & 0xF0FF) | 0x0600);
		bcm430x_phy_write(bcm, 0x048B, 0x005E);
		bcm430x_phy_write(bcm, 0x048C, (bcm430x_phy_read(bcm, 0x048C) & 0xFF00) | 0x001E);
		bcm430x_phy_write(bcm, 0x048D, 0x0002);
	}

	bcm430x_ilt_write16(bcm, offset + 0x0800, 0x0000);
	bcm430x_ilt_write16(bcm, offset + 0x0801, 0x0007);
	bcm430x_ilt_write16(bcm, offset + 0x0802, 0x0016);
	bcm430x_ilt_write16(bcm, offset + 0x0803, 0x0028);
}

static void bcm430x_phy_setupg(struct bcm430x_private *bcm)
{
	u16 i = 0x00;

	switch ( bcm->current_core->phy->rev ) {
	case 1:
		bcm430x_phy_write(bcm, 0x0406, 0x4F19);
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS, (bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0xFC3F) | 0x0340);
		bcm430x_phy_write(bcm, 0x042C, 0x005A);
		bcm430x_phy_write(bcm, 0x0427, 0x001A);

		for ( i = 0; i < BCM430x_ILT_FINEFREQG_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x5800+i, bcm430x_ilt_finefreqg[i]);
		}
		for ( i = 0; i < BCM430x_ILT_NOISEG1_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x1800+i, bcm430x_ilt_noiseg1[i]);
		}
		for ( i = 0; i < BCM430x_ILT_ROTOR_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x2000+i, bcm430x_ilt_rotor[i]);
		}
		for ( i = 0; i < BCM430x_ILT_NOISESCALEG_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x1400+i, bcm430x_ilt_noisescaleg[i]);
		}
		for ( i = 0; i < BCM430x_ILT_RETARD_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x2400+i, bcm430x_ilt_retard[i]);
		}
		for ( i = 0; i < 4; i++ ) {
			bcm430x_ilt_write16(bcm, 0x5404+i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x5408+i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x540C+i, 0x0020);
			bcm430x_ilt_write16(bcm, 0x5410+i, 0x0020);
		}
		bcm430x_phy_agcsetup(bcm);
		break;
	case 2:
		bcm430x_phy_write(bcm, BCM430x_PHY_NRSSILT_CTRL, 0xBA98);
		bcm430x_phy_write(bcm, BCM430x_PHY_NRSSILT_DATA, 0x7654);

		bcm430x_phy_write(bcm, 0x04C0, 0x1861);
		bcm430x_phy_write(bcm, 0x04C1, 0x0271);

		for (i = 0; i < 64; i++) {
			bcm430x_ilt_write16(bcm, 0x4000+i, i);
		}
		for (i = 0; i < BCM430x_ILT_NOISEG2_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x1800+i, bcm430x_ilt_noiseg2[i]);
		}
		for (i = 0; i < BCM430x_ILT_NOISESCALEG_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x1400+i, bcm430x_ilt_noisescaleg[i]);
		}
		for (i=0; i < BCM430x_ILT_SIGMASQR_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x5000+i, bcm430x_ilt_sigmasqr[i]);
		}
		for (i=0; i <= 0x2F; i++) {
			bcm430x_ilt_write16(bcm, 0x1000+i, 0x0820);
		}
		bcm430x_phy_agcsetup(bcm);
		bcm430x_phy_write(bcm, 0x0403, 0x1000);
		bcm430x_ilt_write16(bcm, 0x3C02, 0x000F);
		bcm430x_ilt_write16(bcm, 0x3C03, 0x0014);

		if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM)
			|| (bcm->board_type != 0x0416)
			|| (bcm->board_revision != 0x0017)) {
			if (bcm->current_core->phy->rev < 2) {
				bcm430x_ilt_write16(bcm, 0x5001, 0x0002);
				bcm430x_ilt_write16(bcm, 0x5002, 0x0001);
			} else {
				bcm430x_ilt_write16(bcm, 0x0401, 0x0002);
				bcm430x_ilt_write16(bcm, 0x0402, 0x0001);
			}
		}
		break;
	}
}

static void bcm430x_phy_init_noisescaletbl(struct bcm430x_private *bcm)
{
	u16 i = 0x00;

	bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_CTRL, 0x1400);
	for (i=0; i < 12; i++) {
		if (bcm->current_core->phy->rev == 2) {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6767);
		} else {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2323);
		}
	}
	if (bcm->current_core->phy->rev == 2) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6700);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2300);
	}
	for (i=0; i < 11; i++) {
		if (bcm->current_core->phy->rev == 2) {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x6767);
		} else {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x2323);
		}
	}
	if (bcm->current_core->phy->rev == 2) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x0067);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILT_A_DATA1, 0x0023);
	}
}

static void bcm430x_phy_setupa(struct bcm430x_private *bcm)
{
	u16 i = 0x00;

	switch ( bcm->current_core->phy->rev ) {
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

		for (i = 0; i < 16; i++) {
			bcm430x_ilt_write16(bcm, 0x4000+i, (i+8)%16);
		}
		
		bcm430x_ilt_write16(bcm, 0x3003, 0x1044);
		bcm430x_ilt_write16(bcm, 0x3004, 0x7201);
		bcm430x_ilt_write16(bcm, 0x3006, 0x0040);
		bcm430x_ilt_write16(bcm, 0x3001, (bcm430x_ilt_read16(bcm, 0x3001) & 0x0010) | 0x0008);

		for (i = 0; i < BCM430x_ILT_FINEFREQA_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x5800+i, bcm430x_ilt_finefreqa[i]);
		}
		for ( i = 0; i < BCM430x_ILT_NOISEA2_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x1800+i, bcm430x_ilt_noisea2[i]);
		}
		for (i = 0; i < BCM430x_ILT_ROTOR_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x2000+i, bcm430x_ilt_rotor[i]);
		}
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i = 0; i < BCM430x_ILT_RETARD_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x2400+i, bcm430x_ilt_retard[i]);
		}
		break;
	case 3:
		for (i = 0; i < 64; i++) {
			bcm430x_ilt_write16(bcm, 0x4000+i, i);
		}

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
		for ( i = 0; i < BCM430x_ILT_NOISEA3_SIZE; i++ ) {
			bcm430x_ilt_write16(bcm, 0x1800+i, bcm430x_ilt_noisea3[i]);
		}
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i=0; i < BCM430x_ILT_SIGMASQR_SIZE; i++) {
			bcm430x_ilt_write16(bcm, 0x5000+i, bcm430x_ilt_sigmasqr[i]);
		}
		
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
	}
}

static void bcm430x_phy_inita(struct bcm430x_private *bcm)
{
	u16 tval;
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A) {
		bcm430x_phy_setupa(bcm);
	} else {
		bcm430x_phy_setupg(bcm);
	}
	if (bcm->current_core->phy->type != BCM430x_PHYTYPE_A) {
		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL)
			bcm430x_phy_write(bcm, 0x046E, 0x03CF);
		return;
	}
	bcm430x_phy_write(bcm, BCM430x_PHY_A_CRS,
	                  (bcm430x_phy_read(bcm, BCM430x_PHY_A_CRS) & 0xF83C) | 0x340);
	bcm430x_phy_write(bcm, 0x0034, 0x0001);

	//FIXME: RSSI AGC
	bcm430x_phy_write(bcm, BCM430x_PHY_A_CRS,
	                  bcm430x_phy_read(bcm, BCM430x_PHY_A_CRS) | (0x1 << 14));
	bcm430x_radio_init2060(bcm);

	if ((bcm->board_vendor == PCI_VENDOR_ID_BROADCOM)
	    && ((bcm->board_type == 0x0416) || (bcm->board_type == 0x040A))) {
		if (bcm->current_core->radio->lofcal == 0xFFFF) {
			//FIXME: LOF Cal
			//FIXME: Set TX IQ based on VOS
		} else {
			bcm430x_radio_write16(bcm, 0x001E, bcm->current_core->radio->lofcal);
		}
	}

	bcm430x_phy_write(bcm, 0x007A, 0xF111);

	if (bcm->current_core->phy->savedpctlreg == 0xFFFF) {
		bcm430x_radio_write16(bcm, 0x0019, 0x0000);
		bcm430x_radio_write16(bcm, 0x0017, 0x0020);

		tval = bcm430x_ilt_read16(bcm, 0x3001);
		if (bcm->current_core->phy->rev == 1) {
			bcm430x_ilt_write16(bcm, 0x3001, (bcm430x_ilt_read16(bcm, 0x3001) & 0xFF87) | 0x0058);
		} else {
			bcm430x_ilt_write16(bcm, 0x3001, (bcm430x_ilt_read16(bcm, 0x3001) & 0xFFC3) | 0x002C);
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
	u16 offset, val = 0x3C3D;
	
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
	if (bcm->current_core->radio->channel == 0xFFFF) {
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
	} else {
		bcm430x_radio_selectchannel(bcm, bcm->current_core->radio->channel);
	}
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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
	bcm430x_phy_measurelowsig(bcm);
	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	}
	bcm430x_write16(bcm, 0x03F4, 0x1000);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	}
	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb4(struct bcm430x_private *bcm)
{
	u16 offset, val = 0x3C3D;

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
	if (bcm->current_core->radio->channel == 0xFFFF) {
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
	} else {
		bcm430x_radio_selectchannel(bcm, bcm->current_core->radio->channel);
	}
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}
	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x0032, 0x00E0);
	bcm430x_phy_write(bcm, 0x0035, 0x07C2);

	bcm430x_phy_measurelowsig(bcm);

	bcm430x_phy_write(bcm, 0x0026, 0xCC00);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x0026, 0xCE00);
	bcm430x_write16(bcm, 0x03F4, 0x1100);
	bcm430x_phy_write(bcm, 0x002A, 0x88A3);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_phy_write(bcm, 0x002A, 0x88C2);
	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);
	if (bcm->sprom.boardflags & BCM430x_BFL_RSSI) {
		bcm430x_calc_nrssi_slope(bcm);
		bcm430x_calc_nrssi_threshold(bcm);
	}
	bcm430x_phy_init_pctl(bcm);
}

static void bcm430x_phy_initb5(struct bcm430x_private *bcm)
{
	u16 offset;

	if ((bcm->current_core->phy->rev == 1)
	    && ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_radio_write16(bcm, 0x007A, bcm430x_radio_read16(bcm, 0x007A) | 0x0050);
	}

	if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM) && (bcm->board_type != 0x0416)) {
		for (offset = 0x00A8 ; offset < 0x00C7; offset++) {
			bcm430x_phy_write(bcm, offset, (bcm430x_phy_read(bcm, offset) + 0x2020) & 0x3F3F);
		}
	}

	bcm430x_phy_write(bcm, 0x0035, (bcm430x_phy_read(bcm, 0x0035) & 0xF0FF) | 0x0700);

	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_phy_write(bcm, 0x0038, 0x0667);
	}

	if (bcm->current_core->phy->connected) {
		if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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

	//FIXME: roam_delta
	//if (bcm->roam_delta != 0) {
	//	bcm430x_phy_write(bcm, 0x0401, (bcm430x_phy_read(bcm, 0x0401) & 0x0000) | 0x1000);
	//}

	if ((bcm->current_core->phy->rev == 1)
	    && ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
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

	if ((bcm->current_core->phy->rev == 1)
	    && ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)) {
		bcm430x_phy_write(bcm, 0x0020, 0x3E1C);
	} else {
		bcm430x_phy_write(bcm, 0x0020, 0x301C);
	}

	if (bcm->current_core->phy->rev == 0) {
		bcm430x_write16(bcm, 0x03E4, 0x3000);
	}

	//XXX: Must be 7!
	bcm430x_radio_selectchannel(bcm, 7);

	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0075, 0x0080);
		bcm430x_radio_write16(bcm, 0x0079, 0x0081);
	}

	bcm430x_radio_write16(bcm, 0x0050, 0x0020);
	bcm430x_radio_write16(bcm, 0x0050, 0x0023);

	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
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

	bcm430x_radio_set_txpower_b(bcm, 0xFFFF, 0xFFFF, 0xFFFF);

	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_radio_write16(bcm, 0x005D, 0x000D);
	}

	bcm430x_write16(bcm, 0x03E4, (bcm430x_read16(bcm, 0x03E4) & 0xFFC0) | 0x0004);
}

static void bcm430x_phy_initb6(struct bcm430x_private *bcm) {
	u16 offset, val = 0x1E1F;

	bcm430x_radio_write16(bcm, 0x007A,
	                      (bcm430x_radio_read16(bcm, 0x007A) | 0x0050));
	if (bcm->current_core->radio->id == 0x3205017F) {
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
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G) {
		bcm430x_radio_write16(bcm, 0x007A,
		                      bcm430x_radio_read16(bcm, 0x007A) | 0x0020);
		bcm430x_radio_write16(bcm, 0x0051,
		                      bcm430x_radio_read16(bcm, 0x0051) | 0x0002);
		bcm430x_radio_write16(bcm, 0x0802,
		                      bcm430x_radio_read16(bcm, 0x0802) | 0x0100);
		bcm430x_radio_write16(bcm, 0x042B,
		                      bcm430x_radio_read16(bcm, 0x042B) | 0x2000);
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
	if (bcm->current_core->phy->rev == 2)
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
		//FIXME: init2050 gives OOPS
		//bcm->current_core->radio->initval = bcm430x_radio_init2050(bcm);
		bcm430x_phy_measurelowsig(bcm);
	} else {
		//bcm430x_radio_write16(bcm, 0x0078, bcm->current_core->radio->initval);
		//FIXME: take the saved value from measurelowsig for G PHY as mask
		// bcm430x_radio_write16(bcm, 0x0052, (bcm430x_radio_read16(0x0052) & 0xFFF0) | ???);
	}

	if (bcm->current_core->phy->connected) {
		//FIXME: Set GPHY CompLo
		bcm430x_phy_write(bcm, 0x080F, 0x8078);

		if (bcm->sprom.boardflags & BCM430x_BFL_PACTRL) {
			bcm430x_phy_write(bcm, 0x002E, 0x807F);
		} else {
			bcm430x_phy_write(bcm, 0x002E, 0x8075);
		}

		if (bcm->current_core->phy->rev < 2) {
			bcm430x_phy_write(bcm, 0x002F, 0x0101);
		} else {
			bcm430x_phy_write(bcm, 0x002F, 0x0202);
		}
	}

	if ((bcm->sprom.boardflags & BCM430x_BFL_RSSI) == 0) {
		//FIXME: Update hardware NRSSI Table
		bcm430x_calc_nrssi_threshold(bcm);
	} else {
		if (bcm->current_core->phy->connected) {
			if ((bcm->current_core->radio->nrssi[0] == -1000) && (bcm->current_core->radio->nrssi[1] == -1000)) {
				bcm430x_calc_nrssi_slope(bcm);
			} else {
				bcm430x_calc_nrssi_threshold(bcm);
			}
		}
	}
	bcm430x_phy_init_pctl(bcm);
}

static inline
u16 bcm430x_phy_lo_singlevalue(struct bcm430x_private *bcm, u16 control) {
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x15, 0xE300);
		control = (control << 8);
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
		bcm430x_phy_write(bcm, 0x0015, control | 0xEFE0);
		udelay(8);
	}

	return bcm430x_phy_read(bcm, 0x002D);
}

static inline
u16 bcm430x_phy_lo_singledeviation(struct bcm430x_private *bcm, u16 control) {
	u16 ret = 0;
	int i;

	for (i = 0; i < 8; i++) {
		ret += bcm430x_phy_lo_singlevalue(bcm, control);
	}

	return ret;
}


static inline
u16 bcm430x_phy_lo_pair(struct bcm430x_private *bcm, u16 baseband, u16 radio, u16 tx)
{
	u8 dict[10] = { 11, 10, 11, 12, 13, 12, 13, 12, 13, 12 };

	if (baseband > 12)
		baseband = 6;
	else
		baseband /= 2;

	if (tx == 3) {
		return bcm->current_core->phy->lo_pairs[radio + 14 * baseband].value;
	if (tx != 3)
		assert(radio < 10);
		radio = dict[radio];
	}
	return bcm->current_core->phy->lo_pairs[radio + 14 * baseband].value;
}

static inline
void bcm430x_phy_lo_adjust(struct bcm430x_private *bcm)
{
	struct bcm430x_radioinfo *r = bcm->current_core->radio;
	bcm430x_phy_write(bcm, 0x0810,
	                  bcm430x_phy_lo_pair(bcm, r->txpower[0], r->txpower[1], r->txpower[2]));
}

static inline
u8 bcm430x_phy_lo_unk16(struct bcm430x_private *bcm)
{
	/* phy_info_unk16 */
	u8 ret = 0, i;
	u16 deviation = 0, tmp;

	bcm430x_radio_write16(bcm, 0x52, 0x0000);
	udelay(10);
	deviation = bcm430x_phy_lo_singledeviation(bcm, 0);
	for (i = 0; i < 16; i++) {
		bcm430x_radio_write16(bcm, 0x52, i);
		udelay(10);
		tmp = bcm430x_phy_lo_singledeviation(bcm, 0);
		if (tmp < deviation) {
			deviation = tmp;
			ret = i;
		}
	}

	return ret;
}

static inline
u16 bcm430x_phy_lo_state(struct bcm430x_private *bcm, u16 pair, u16 r27)
{
	union bcm430x_lopair transitions[8] = {
		{ .items = { +1, +1 } }, { .items = { +1, 0 } },
		{ .items = { +1, -1 } }, { .items = { 0, -1 } },
		{ .items = { -1, -1 } }, { .items = { -1, 0 } },
		{ .items = { -1, +1 } }, { .items = { 0, +1 } }
	};
	union bcm430x_lopair transition;
	int i = 12, j;
	u8 state = 0, done = 0;
	u16 deviation = bcm430x_phy_lo_singledeviation(bcm, r27), tmp, ret;

	while ((i--) && (!done)) {
		if (state == 0) {
			for (j = 0; j < 8; j++) {
				transition.value = pair;
				transition.items[0] += transitions[j].items[0];
				transition.items[0] += transitions[j].items[0];
				tmp = 0xFFFF;
				if ((abs(transition.items[0]) < 9) && (abs(transition.items[1]) < 9)) {
					bcm430x_phy_write(bcm, 0x0810, transition.value);
					tmp = bcm430x_phy_lo_singledeviation(bcm, r27);
				}
				if (tmp < deviation) {
					deviation = tmp;
					state = j + 1;
					ret = transition.value;
				}
			}
		} else if (state % 2 == 0) {
			for (j = -1; j < 2; j++) {
				if (!j)
					continue;
				transition.value = pair;
				transition.items[0] += transitions[((state - 1) % 8) + j].items[0];
				transition.items[1] += transitions[((state - 1) % 8) + j].items[1];
				tmp = 0xFFFF;
				if ((abs(transition.items[0]) < 9) && (abs(transition.items[1]) < 9)) {
					bcm430x_phy_write(bcm, 0x0810, transition.value);
					tmp = bcm430x_phy_lo_singledeviation(bcm, r27);
				}
				if (tmp < deviation) {
					deviation = tmp;
					state = ((state - 1) % 8) + j + 1;
					ret = transition.value;
				}
			}
		} else {
			for (j = -2; j < 3; j++) {
				if (!j)
					continue;
				transition.value = pair;
				transition.items[0] += transitions[((state - 1) % 8) + j].items[0];
				transition.items[1] += transitions[((state - 1) % 8) + j].items[1];
				tmp = 0xFFFF;
				if ((abs(transition.items[0]) < 9) && (abs(transition.items[1]) < 9)) {
					bcm430x_phy_write(bcm, 0x0810, transition.value);
					tmp = bcm430x_phy_lo_singledeviation(bcm, r27);
				}
				if (tmp < deviation) {
					deviation = tmp;
					state = ((state - 1) % 8) + j + 1;
					ret = transition.value;
				}
			}	
		}
	}

	return ret;
}

/* http://bcm-specs.sipsolutions.net/LocalOscillator/Measure */
void bcm430x_phy_lo_measure(struct bcm430x_private *bcm)
{
	struct bcm430x_phyinfo *phy = bcm->current_core->phy;
	int h, i, oldi, j;
	union bcm430x_lopair control = { .value = 0 };
	u8 pairorder[10] = { 3, 1, 5, 7, 9, 2, 0, 4, 6, 8 };
	u16 tmp, reg;
	u16 regstack[16] = { 0 };

	//XXX: What are these?
	u8 r27, r31;

	/* Setup */
	if (phy->desired_power[0] < 0) {
		phy->info_unk16 = 0xFFFF;
		FIXME();
	}
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
	bcm430x_radio_selectchannel(bcm, 6);
	if (phy->connected) {
		bcm430x_phy_write(bcm, 0x0429, regstack[0] & 0x7FFF);
		bcm430x_phy_write(bcm, 0x0802, regstack[1] & 0xFFFE);
		bcm430x_dummy_transmission(bcm);
	}
	bcm430x_radio_write16(bcm, 0x43, 6);
	tmp = ((bcm->current_core->radio->txpower[0] & 0x000F) << 2);
	if (bcm->current_core->phy->rev > 1)
		tmp <<= 1;
	tmp |= bcm430x_phy_read(bcm, 0x60);
	bcm430x_phy_write(bcm, 0x60, tmp);
	bcm430x_write16(bcm, 0x03F4, 0x0000);
	bcm430x_phy_write(bcm, 0x2E, 0x007F);
	bcm430x_phy_write(bcm, 0x080F, 0x0078);
	bcm430x_phy_write(bcm, 0x35, regstack[7] & 0xFFBF);
	bcm430x_radio_write16(bcm, 0x7A, regstack[10] & 0xFFF0);
	bcm430x_phy_write(bcm, 0x2B, 0x0203);
	bcm430x_phy_write(bcm, 0x2A, 0x08A3);
	if (bcm->current_core->phy->connected) {
		bcm430x_phy_write(bcm, 0x0814, regstack[14] | 0x0003);
		bcm430x_phy_write(bcm, 0x0815, regstack[15] & 0xFFFC);
		bcm430x_phy_write(bcm, 0x0811, 0x01B3);
		bcm430x_phy_write(bcm, 0x0812, 0x00B2);
	}
	if (phy->info_unk16 == 0xFFFF)
		phy->info_unk16 = bcm430x_phy_lo_unk16(bcm);
	bcm430x_phy_write(bcm, 0x8078, 0x080F);

	/* Measure */
	for (h = 0; h < 10; h++) {
		i = pairorder[h];
		if (phy->info_unk16 != 0xFFFF) {
			if (i == 3)
				control.value = 0;
			else if ((i % 2 == 1) && (oldi % 2 == 1))
				control = phy->lo_pairs[i] ;
			else
				control = phy->lo_pairs[3];
		}
		for (j = 0; j < 4; j++) {
			if (phy->info_unk16 == 0xFFFF) {
				control = phy->lo_pairs[i + 14 * j];
				tmp = i * 2 + j;
				r27 = 0;
				if (tmp > 14) {
					r31 = 1;
					if (tmp > 17)
						r27 = 1;
					if (tmp > 19)
						r27 = 2;
				} else {
					r31 = 0;
				}
			} else {
				if (phy->desired_power[i + 14 * j] == 0)
					continue;
				control = phy->lo_pairs[i + 14 * j];
				r27 = 3;
				r31 = 1;
			}
			bcm430x_phy_write(bcm, 0x43, i);
			bcm430x_phy_write(bcm, 0x52, phy->info_unk16);
			reg = 0x0060;
			if (bcm->current_core->phy->version == 0) {
				reg = 0x03E6;
				tmp = (bcm430x_phy_read(bcm, reg) & 0xFFF0) | (j * 2);
			} else if (bcm->current_core->phy->version == 1) {
				tmp  = bcm430x_phy_read(bcm, reg) & ~0x0078;
				tmp |= ((j * 2) << 3) & 0x0078;
			} else {
				tmp  = bcm430x_phy_read(bcm, reg) & ~0x003C;
				tmp |= ((j * 2) << 2) & 0x003C;
			}
			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm430x_radio_write16(bcm, 0x7A, tmp);
			phy->lo_pairs[i + 14 * i].value = bcm430x_phy_lo_state(bcm,
			                                                       phy->lo_pairs[i + 14 * j].value,
									       r27);
		}
		oldi = i;
	}
	for (i = 10; i < 14; i++) {
		for (j = 0; j < 4; j++) {
			if (phy->info_unk16 == 0xFFFF) {
				control = phy->lo_pairs[i + 14 * j];
				tmp = i * 2 + j;
				r27 = 0;
				if (tmp > 14) {
					r31 = 1;
					if (tmp > 17)
						r27 = 1;
					if (tmp > 19)
						r27 = 2;
				} else {
					r31 = 0;
				}
			} else {
				if (phy->desired_power[i + 14 * j] == 0)
					continue;
				control = phy->lo_pairs[i + 14 * j];
				r27 = 3;
				r31 = 1;
			}
			bcm430x_phy_write(bcm, 0x43, i);
			bcm430x_phy_write(bcm, 0x52, phy->info_unk16 + 0x30);
			reg = 0x0060;
			if (bcm->current_core->phy->version == 0) {
				reg = 0x03E6;
				tmp = (bcm430x_phy_read(bcm, reg) & 0xFFF0) | (j * 2);
			} else if (bcm->current_core->phy->version == 1) {
				tmp  = bcm430x_phy_read(bcm, reg) & ~0x0078;
				tmp |= ((j * 2) << 3) & 0x0078;
			} else {
				tmp  = bcm430x_phy_read(bcm, reg) & ~0x003C;
				tmp |= ((j * 2) << 2) & 0x003C;
			}
			bcm430x_phy_write(bcm, reg, tmp);
			tmp = (regstack[10] & 0xFFF0);
			if (r31)
				tmp |= 0x0008;
			bcm430x_radio_write16(bcm, 0x7A, tmp);
			phy->lo_pairs[i + 14 * j].value = bcm430x_phy_lo_state(bcm,
			                                     	               phy->lo_pairs[i + 14 * j].value,
								               r27);
		}
	}

	/* Restoration */
	bcm430x_phy_lo_adjust(bcm);
	bcm430x_phy_write(bcm, 0x2E, 0x807F);
	if (phy->connected)
		bcm430x_phy_write(bcm, 0x2F, 0x0202);
	else
		bcm430x_phy_write(bcm, 0x2F, 0x0101);
	bcm430x_write16(bcm, 0x03F4, regstack[4]);
	bcm430x_phy_write(bcm, 0x15, regstack[5]);
	bcm430x_phy_write(bcm, 0x2A, regstack[6]);
	bcm430x_phy_write(bcm, 0x35, regstack[7]);
	bcm430x_phy_write(bcm, 0x60, regstack[8]);
	bcm430x_radio_write16(bcm, 0x43, regstack[9]);
	bcm430x_radio_write16(bcm, 0x7A, regstack[10]);
	regstack[11] &= 0xF0;
	regstack[11] |= (bcm430x_radio_read16(bcm, 0x52) & 0x0F);
	bcm430x_radio_write16(bcm, 0x52, regstack[11]);
	if (phy->connected) {
		bcm430x_phy_write(bcm, 0x0811, regstack[12]);
		bcm430x_phy_write(bcm, 0x0812, regstack[13]);
		bcm430x_phy_write(bcm, 0x0814, regstack[14]);
		bcm430x_phy_write(bcm, 0x0815, regstack[15]);
		bcm430x_phy_write(bcm, 0x0429, regstack[0]);
		bcm430x_phy_write(bcm, 0x0802, regstack[1]);
	}
	TODO(); // FuncPlaceholder
	bcm430x_radio_selectchannel(bcm, bcm->current_core->radio->channel);
}

/* http://bcm-specs.sipsolutions.net/RecalculateTransmissionPower */
void bcm430x_phy_xmitpower(struct bcm430x_private *bcm)
{
	u16 saved[2] = { 0 };

	if (bcm->current_core->phy->savedpctlreg == 0xFFFF)
		return;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		if (((bcm->board_type == 0x0416) || (bcm->board_type == 0x040A))
		    && (bcm->board_vendor == PCI_VENDOR_ID_BROADCOM))
			return;

		FIXME(); //FIXME: Nothing for A PHYs yet :-/
		break;

	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		//XXX: What is board_type 0x0416?
		if ((bcm->board_type == 0x0416) && (bcm->board_vendor == PCI_VENDOR_ID_BROADCOM))
			return;

		saved[0] = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x00F8);
		saved[1] = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x005A);
		if ((saved[0] == 0x7F7F) || (saved[1] == 0x7F7F)) {
			saved[0] = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x0070);
			saved[1] = bcm430x_shm_read16(bcm, BCM430x_SHM_SHARED, 0x0072);
			if ((saved[0] == 0x7F7F) || (saved[1] == 0x7F7F))
				return;

			saved[0] = (saved[0] + 0x2020) & 0x3F3F;
			saved[1] = (saved[1] + 0x2020) & 0x3F3F;
		}
		bcm430x_radio_clear_tssi(bcm);

		TODO(); //TODO: 'Continues'
		break;
	}
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
	u16 offset;
	u32 ucodeflags;

	if ((bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
	    && (bcm->current_core->phy->rev == 3)) {
		bcm->current_core->phy->antenna_diversity = 0;
	}

	if (bcm->current_core->phy->antenna_diversity == 0xFFFF) {
		bcm->current_core->phy->antenna_diversity = 0x180;
	}

	ucodeflags = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED,
					BCM430x_UCODEFLAGS_OFFSET);
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED,
			    BCM430x_UCODEFLAGS_OFFSET,
			    ucodeflags & ~BCM430x_UCODEFLAG_AUTODIV);

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		offset = 0x0000;
	case BCM430x_PHYTYPE_G:
		offset = 0x0400;

		if (bcm->current_core->phy->antenna_diversity == 0x100) {
			bcm430x_phy_write(bcm, offset + 1, (bcm430x_phy_read(bcm, offset + 1) & 0x7E7F) | 0x0180);
		} else {
			bcm430x_phy_write(bcm, offset + 1, (bcm430x_phy_read(bcm, offset + 1) & 0x7E7F) | bcm->current_core->phy->antenna_diversity);
		}

		if (bcm->current_core->phy->antenna_diversity == 0x100) {
			bcm430x_phy_write(bcm, offset + 0x002B, (bcm430x_phy_read(bcm, offset + 0x002B) & 0xFEFF) | 0x0100);
		}

		if (bcm->current_core->phy->antenna_diversity > 0x100) {
			bcm430x_phy_write(bcm, offset + 0x002B, bcm430x_phy_read(bcm, offset + 0x002B) & 0xFEFF);
		}

		if (!bcm->current_core->phy->connected) {
			if (bcm->current_core->phy->antenna_diversity < 0x100) {
				bcm430x_phy_write(bcm, 0x048C, bcm430x_phy_read(bcm, 0x048C) & 0xDFFF);
			} else {
				bcm430x_phy_write(bcm, 0x048C, (bcm430x_phy_read(bcm, 0x048C) & 0xDFFF) | 0x2000);
			}

			if (bcm->current_core->phy->rev >= 2) {
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
			if (bcm->current_core->phy->antenna_diversity == 0x100) {
				bcm430x_phy_write(bcm, 0x03E2, (bcm430x_phy_read(bcm, 0x03E2) & 0xFE7F) | 0x0180);
			} else {
				bcm430x_phy_write(bcm, 0x03E2, (bcm430x_phy_read(bcm, 0x03E2) & 0xFE7F) | bcm->current_core->phy->antenna_diversity);
			}
		}
		break;
	default:
                printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return;
	}

	if (bcm->current_core->phy->antenna_diversity >= 0x0100) {
		ucodeflags = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED,
						BCM430x_UCODEFLAGS_OFFSET);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED,
				    BCM430x_UCODEFLAGS_OFFSET,
				    ucodeflags |  BCM430x_UCODEFLAG_AUTODIV);
	}
}
