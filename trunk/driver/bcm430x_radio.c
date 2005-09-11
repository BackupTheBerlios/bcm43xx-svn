/*

  Broadcom BCM430x wireless driver

  Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
                     Stefano Brivio <st3@riseup.net>
                     Michael Buesch <mbuesch@freenet.de>
                     Danny van Dyk <kugelfang@gentoo.org>

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
#include "bcm430x_main.h"
#include "bcm430x_phy.h"
#include "bcm430x_radio.h"

static u16 bcm430x_radio_calibrationvalue(struct bcm430x_private *bcm) {
	u16 values[16] = {
		0x0002, 0x0003, 0x0001, 0x000F,
		0x0006, 0x0007, 0x0005, 0x000F,
		0x000A, 0x000B, 0x0009, 0x000F,
		0x000E, 0x000F, 0x000D, 0x000F,
	};
	u16 reg, index, ret;

	reg    = bcm430x_radio_read16(bcm, 0x0060);
	index  = (reg & 0x003E) / 2;
	ret    = values[index] * 2;
	ret   |= (reg & 0x0001);
	ret   |= 0x0020;

	return ret;
}

u16 bcm430x_radio_init2050(struct bcm430x_private *bcm) {
	u16 stack[20];
	u16 index = 0, ret;

	// Backup data to stack
	if (bcm->phy_type == BCM430x_PHYTYPE_B) {
		stack[index++] = bcm430x_read16(bcm, 0x03EC);
		stack[index++] = bcm430x_phy_read(bcm, 0x0030);
	} else {
		if (bcm->status && BCM430x_STAT_PHYCONNECTED) {
			stack[index++] = bcm430x_phy_read(bcm, 0x0802);
			stack[index++] = bcm430x_phy_read(bcm, 0x0429);
			stack[index++] = bcm430x_phy_read(bcm, 0x0815);
			stack[index++] = bcm430x_phy_read(bcm, 0x0814);
			stack[index++] = bcm430x_phy_read(bcm, 0x0812);
			stack[index++] = bcm430x_phy_read(bcm, 0x0811);
			bcm430x_phy_write(bcm, 0x0814,
                                          (bcm430x_phy_read(bcm, 0x0814) | 0x0003));
			bcm430x_phy_write(bcm, 0x0815,
                                          (bcm430x_phy_read(bcm, 0x0815) & 0xFFFC));	
			bcm430x_phy_write(bcm, 0x0429,
			                  (bcm430x_phy_read(bcm, 0x0429) & 0x7FFC));
			bcm430x_phy_write(bcm, 0x0802,
			                  (bcm430x_phy_read(bcm, 0x0802) & 0xFFFC));
			bcm430x_phy_write(bcm, 0x0811, 0x01B3);
			bcm430x_phy_write(bcm, 0x0812, 0x0fb2);
		}
		bcm430x_write16(bcm, 0x03E2,
		                (bcm430x_read16(bcm, 0x03E2) | 0x8000));
	}
	stack[index++] = bcm430x_phy_read(bcm, 0x0035);
	bcm430x_phy_write(bcm, 0x0035,
	                  (bcm430x_phy_read(bcm, 0x0035) & 0xFF7F));
	stack[index++] = bcm430x_read16(bcm, 0x03F4);
	stack[index++] = bcm430x_read16(bcm, 0x03E6);
	stack[index++] = bcm430x_radio_read16(bcm, 0x0043);
	stack[index++] = bcm430x_phy_read(bcm, 0x0015);
	
	// Initialization
	if (bcm->phy_version == 0)
		bcm430x_write16(bcm, 0x03E6, 0x0122);
	else {
		if (bcm->phy_version >= 2)
			bcm430x_write16(bcm, 0x03E6, 0x0040);
		bcm430x_write16(bcm, 0x03F4,
                                (bcm430x_read16(bcm, 0x03F4) | 0x2000));
	}

	ret = bcm430x_radio_calibrationvalue(bcm);
	
	//FIXME: what is 'flipmap'? Is it the RCC value?
	//if (bcm->phy_type == BCM430x_PHYTYPE_B)
	//	bcm430x_radio_write16(bcm, 0x0078, 'flipmap');
	
	bcm430x_radio_write16(bcm, 0x0015, 0xBFAF);
	bcm430x_radio_write16(bcm, 0x002B, 0x1403);
	if (!(bcm->status && BCM430x_STAT_PHYCONNECTED))
		bcm430x_phy_write(bcm, 0x0812, 0x00B2);
	bcm430x_phy_write(bcm, 0x0015, 0xBFA0);
	bcm430x_radio_write16(bcm, 0x0051,
	                      (bcm430x_read16(bcm, 0x0051) | 0x0004));
	bcm430x_radio_write16(bcm, 0x0052, 0x0000);
	bcm430x_radio_write16(bcm, 0x0043, 0x0009);
	bcm430x_radio_write16(bcm, 0x0058, 0x0000);
	
	//FIXME: Loop 1
	
	udelay(10);
	bcm430x_radio_write16(bcm, 0x0058, 0x0000);
	
	//FIXME: Loop 2
	
	// Restore data from stack
	// Restoring data that specs says we should not backup (PHY:0x15, radio:0x43)
	bcm430x_phy_write(bcm, 0x0015, stack[--index]);
	bcm430x_radio_write16(bcm, 0x0051,
	                      (bcm430x_radio_read16(bcm, 0x0051) & 0xFFFB));
	bcm430x_radio_write16(bcm, 0x0051, 0x0009);
	bcm430x_radio_write16(bcm, 0x0043, stack[--index]);
	bcm430x_write16(bcm, 0x003E6, stack[--index]);
	--index;
	if (!bcm->phy_version == 0)
		bcm430x_write16(bcm, 0x003E6, stack[index]);
	bcm430x_phy_write(bcm, 0x0035, stack[--index]);
	if (bcm->phy_type == BCM430x_PHYTYPE_B) {
		bcm430x_phy_write(bcm, 0x0030, stack[--index]);
		bcm430x_write16(bcm, 0x03EC, stack[--index]);
	} else {
		bcm430x_write16(bcm, 0x03E2,
		                (bcm430x_read16(bcm, 0x03E2) & 0x7FFF));
		if (bcm->status & BCM430x_STAT_PHYCONNECTED) {
			bcm430x_phy_write(bcm, 0x0811, stack[--index]);
			bcm430x_phy_write(bcm, 0x0812, stack[--index]);
			bcm430x_phy_write(bcm, 0x0814, stack[--index]);
			bcm430x_phy_write(bcm, 0x0815, stack[--index]);
			bcm430x_phy_write(bcm, 0x0429, stack[--index]);
			bcm430x_phy_write(bcm, 0x0802, stack[--index]);
		}
	}
	//FIXME:
	//if (Loop 2 was gone through _completely_)
	//	ret = bcm430x_radio_read16(bcm, 0x0078);

	return ret;
}

u16 bcm430x_radio_init2060(struct bcm430x_private *bcm) {
	bcm430x_radio_write16(bcm, 0x0004, 0x00C0);
	bcm430x_radio_write16(bcm, 0x0005, 0x0008);
	bcm430x_phy_write(bcm, 0x0010, bcm430x_phy_read(bcm, 0x0010) & 0xFFF7);
	bcm430x_phy_write(bcm, 0x0011, bcm430x_phy_read(bcm, 0x0011) & 0xFFF7);
	bcm430x_radio_write16(bcm, 0x0004, 0x00C0);
	bcm430x_radio_write16(bcm, 0x0005, 0x0008);
	bcm430x_radio_write16(bcm, 0x0009, 0x0040);
	bcm430x_radio_write16(bcm, 0x0005, 0x00AA);
	bcm430x_radio_write16(bcm, 0x0032, 0x008F);
	bcm430x_radio_write16(bcm, 0x0006, 0x008F);
	bcm430x_radio_write16(bcm, 0x0034, 0x008F);
	bcm430x_radio_write16(bcm, 0x002C, 0x0007);
	bcm430x_radio_write16(bcm, 0x0082, 0x0080);
	bcm430x_radio_write16(bcm, 0x0080, 0x0000);
	bcm430x_radio_write16(bcm, 0x003F, 0x00DA);
	bcm430x_radio_write16(bcm, 0x0005, bcm430x_radio_read16(bcm, 0x0005) & ~0x0008);
	bcm430x_radio_write16(bcm, 0x0081, bcm430x_radio_read16(bcm, 0x0081) & ~0x0010);
	bcm430x_radio_write16(bcm, 0x0081, bcm430x_radio_read16(bcm, 0x0081) & ~0x0020);
	bcm430x_radio_write16(bcm, 0x0081, bcm430x_radio_read16(bcm, 0x0081) & ~0x0020);
	udelay(400);
	
	bcm430x_radio_write16(bcm, 0x0081, (bcm430x_radio_read16(bcm, 0x0081) & ~0x0020) | 0x0010);
	udelay(400);
	
	bcm430x_radio_write16(bcm, 0x0005, (bcm430x_radio_read16(bcm, 0x0005) & ~0x0008) | 0x0008);
	bcm430x_radio_write16(bcm, 0x0085, bcm430x_radio_read16(bcm, 0x0085) & ~0x0010);
	bcm430x_radio_write16(bcm, 0x0005, bcm430x_radio_read16(bcm, 0x0005) & ~0x0008);
	bcm430x_radio_write16(bcm, 0x0081, bcm430x_radio_read16(bcm, 0x0081) & ~0x0040);
	bcm430x_radio_write16(bcm, 0x0081, (bcm430x_radio_read16(bcm, 0x0081) & ~0x0040) | 0x0040);
	bcm430x_radio_write16(bcm, 0x0005, (bcm430x_radio_read16(bcm, 0x0081) & ~0x0008) | 0x0008);
	bcm430x_phy_write(bcm, 0x0063, 0xDDC6);
	bcm430x_phy_write(bcm, 0x0069, 0x07BE);
	bcm430x_phy_write(bcm, 0x006A, 0x0000);
	
	//FIXME: What is the default channel for PHYTYPE_A
	//bcm430x_radio_selectchannel(bcm, BCM430x_PHYTYPE_A_DEFAULT_CHANNEL);
printk(KERN_WARNING PFX "FIXME: radio_init2060(), what is the 802.11a default channel?\n");
	udelay(1000);

	return 0;
}

u16 bcm430x_radio_read16(struct bcm430x_private *bcm, u16 offset) {
	if (bcm->phy_type == BCM430x_PHYTYPE_A)
		offset |= 0x40;
	else if (bcm->phy_type == BCM430x_PHYTYPE_B) {
		switch (bcm->radio_id & 0x0FFFF000) {
			case 0x02053000:
				if (offset < 0x70)
					offset += 0x80;
				else if (offset < 0x80)
					offset += 0x70;
			break;
			case 0x02050000:
				offset |= 0x80;
			break;
		}
	} else if (bcm->phy_type == BCM430x_PHYTYPE_G)
		offset |= 0x80;
	
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA);
}

int bcm430x_radio_selectchannel(struct bcm430x_private *bcm,
                                       u8 channel) {
	// Frequencies are given as differences to 2.4GHz
	// starting with channel 1
	static u16 frequencies_bg[14] = {
	        12, 17, 22, 27,
		32, 37, 42, 47,
		52, 57, 62, 67,
		72, 84,
	};

        switch (bcm->phy_type) {
        case BCM430x_PHYTYPE_A:
                //FIXME: Specs incomplete 2005/09/09
		break;
        case BCM430x_PHYTYPE_B:
        case BCM430x_PHYTYPE_G:
                if ((channel == 0) || (channel >= 15))
                        return -1;
                bcm430x_write16(bcm, BCM430x_MMIO_CHANNEL,
		                frequencies_bg[channel - 1]);
                break;
        default:
                printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
                return -1;
        }
	return 0;
}

void bcm430x_radio_set_txpower_a(struct bcm430x_private *bcm, u16 txpower)
{
	/* TODO */
printk(KERN_WARNING PFX "TODO: set_txpower_a()\n");
}

void bcm430x_radio_set_txpower_b(struct bcm430x_private *bcm,
                                 u16 baseband_attenuation, u16 attenuation,
			         u16 txpower)
{
	u16 reg = 0x0060, tmp;

	if (baseband_attenuation == 0xFFFF)
		baseband_attenuation = bcm->radio_txpower[0];
	else
		bcm->radio_txpower[0] = baseband_attenuation;
	if (attenuation == 0xFFFF)
		attenuation = bcm->radio_txpower[1];
	else
		baseband_attenuation = attenuation;
	if (txpower == 0xFFFF)
		txpower = bcm->radio_txpower[2];
	else
		bcm->radio_txpower[2] = txpower;
	
	if (bcm->phy_version == 0) {
		reg = 0x03E6;
		tmp = (bcm430x_phy_read(bcm, reg) & 0xFFF0) | baseband_attenuation;
	} else if (bcm->phy_version == 1) {
		tmp  = bcm430x_phy_read(bcm, reg) & ~0x0078;
		tmp |= (baseband_attenuation << 3) & 0x0078;
	} else {
		tmp  = bcm430x_phy_read(bcm, reg) & ~0x003C;
		tmp |= (baseband_attenuation << 2) & 0x003C;
	}
	bcm430x_phy_write(bcm, reg, tmp);
	bcm430x_write16(bcm, 0x0043, attenuation);
	bcm430x_shm_control(bcm, 0x0064);
	bcm430x_shm_write16(bcm, attenuation);
	if ((bcm->radio_id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_radio_write16(bcm, 0x0052,
		                      (bcm430x_radio_read16(bcm, 0x0052) & 0xFF8F) | txpower);
	//FIXME: FuncPlaceholder
}


int bcm430x_radio_turn_on(struct bcm430x_private *bcm)
{
	if (bcm->radio_id == BCM430x_RADIO_ID_NORF) {
		printk(KERN_ERR PFX "Error: No radio device found on chip!\n");
		return -ENODEV;
	}

	switch (bcm->phy_type) {
	case BCM430x_PHYTYPE_A:
		bcm430x_radio_write16(bcm, 0x0004, 0x00C0);
		bcm430x_radio_write16(bcm, 0x0005, 0x0008);
		bcm430x_phy_write(bcm, 0x0010, bcm430x_phy_read(bcm, 0x0010) & 0xFFF7);
		bcm430x_phy_write(bcm, 0x0011, bcm430x_phy_read(bcm, 0x0011) & 0xFFF7);
		bcm430x_radio_init2060(bcm);	
		break;
        case BCM430x_PHYTYPE_B:
        case BCM430x_PHYTYPE_G:
		bcm430x_phy_write(bcm, 0x0015, 0x8000);
		bcm430x_phy_write(bcm, 0x0015, 0xCC00);
		bcm430x_phy_write(bcm, 0x0015, ((bcm->status & BCM430x_STAT_PHYCONNECTED) ? 0x00C0 : 0x0000));
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_BG_DEFAULTCHANNEL);
		break;
	default:
		printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return -1;
	}
printk(KERN_INFO PFX "radio turned on\n");

	return 0;
}
	
int bcm430x_radio_turn_off(struct bcm430x_private *bcm)
{
	if (bcm->radio_id == BCM430x_RADIO_ID_NORF)
		return -ENODEV;

	switch (bcm->phy_type) {
	case BCM430x_PHYTYPE_A:
		bcm430x_radio_write16(bcm, 0x0004, 0x00FF);
		bcm430x_radio_write16(bcm, 0x0005, 0x00FB);
		bcm430x_phy_write(bcm, 0x0010, (bcm430x_phy_read(bcm, 0x0010) & 0xFFFF) | 0x0008);
		bcm430x_phy_write(bcm, 0x0011, (bcm430x_phy_read(bcm, 0x0011) & 0xFFFF) | 0x0008);
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		if (bcm->chip_rev < 5)
			bcm430x_phy_write(bcm, 0x0015, 0xAA00);
		else {
			bcm430x_phy_write(bcm, 0x0811, (bcm430x_phy_read(bcm, 0x0811) & 0xFFFF) | 0x008C);
			bcm430x_phy_write(bcm, 0x0812, bcm430x_phy_read(bcm, 0x0812) & 0xFF73);
			/*FIXME: 'set of 0', as above */
		}
		break;
	default:
		printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return -1;
	}
	
	return 0;
}

void bcm430x_radio_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_DATA, val);
}
