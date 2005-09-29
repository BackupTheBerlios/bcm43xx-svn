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
#include "bcm430x_main.h"
#include "bcm430x_phy.h"
#include "bcm430x_radio.h"

/* FlipMap */
static u16 bcm430x_flipmap[16] = {
	0x0000, 0x0008, 0x0004, 0x000C,
	0x0002, 0x000A, 0x0006, 0x000E,
	0x0001, 0x0009, 0x0005, 0x000D,
	0x0003, 0x000B, 0x0007, 0x000F,
};

void bcm430x_radio_calc_interference(struct bcm430x_private *bcm, u16 mode)
{
	u16 disable = (mode & BCM430x_RADIO_INTERFMODE_DISABLE);
	u16 *stack = bcm->current_core->radio->interfstack;
	u16 i = bcm->current_core->radio->interfsize;
	u16 fmapoffset;

	if (!(bcm->current_core->phy->type == BCM430x_PHYTYPE_G)
	    || (bcm->current_core->phy->rev == 0))
		return;
	if (!bcm->current_core->phy->connected)
		return;

	switch (mode & !BCM430x_RADIO_INTERFMODE_DISABLE) {
	case BCM430x_RADIO_INTERFMODE_NONE:
		if (bcm->current_core->radio->interfmode & BCM430x_RADIO_INTERFMODE_DISABLE)
			return;
		if (bcm->current_core->radio->interfmode != BCM430x_RADIO_INTERFMODE_NONE)
			bcm430x_radio_calc_interference(bcm,
		                                    bcm->current_core->radio->interfmode
						    | BCM430x_RADIO_INTERFMODE_DISABLE);
		break;
	case BCM430x_RADIO_INTERFMODE_NONWLAN:
		if (!disable) {
			if (bcm->current_core->phy->rev != 1) {
				bcm430x_phy_write(bcm, 0x042B,
				                  bcm430x_phy_read(bcm, 0x042B) & 0x0800);
				bcm430x_phy_write(bcm, 0x0429,
				                  bcm430x_phy_read(bcm, 0x0429) & ~0x4000);
				return;
			}
			fmapoffset = bcm430x_flipmap[(bcm430x_phy_read(bcm, 0x0078) & 0x001E) >> 1]; 
			if ( fmapoffset >= 4 )
				fmapoffset -= 3;
			bcm430x_phy_write(bcm, 0x0078, 2*bcm430x_flipmap[fmapoffset]);

			//FIXME: FuncPlaceholder (Set NRSSI Threshold)
			stack[i++] = bcm430x_phy_read(bcm, 0x04AC);
			stack[i++] = bcm430x_phy_read(bcm, 0x04AA);
			stack[i++] = bcm430x_phy_read(bcm, 0x0493);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A9);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A3);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A7);
			stack[i++] = bcm430x_phy_read(bcm, 0x04AB);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A8);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A2);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A1);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A0);
			if (bcm->current_core->rev < 5) {
				stack[i++] = bcm430x_phy_read(bcm, 0x0406);
				bcm430x_phy_write(bcm, 0x0406, 0x7E28);
			} else {
				stack[i++] = bcm430x_phy_read(bcm, 0x04C1);
				stack[i++] = bcm430x_phy_read(bcm, 0x04C0);
				bcm430x_phy_write(bcm, 0x04C0, 0x3E04);
				bcm430x_phy_write(bcm, 0x04C1, 0x0640);
			}
			bcm430x_phy_write(bcm, 0x042B,
			                  bcm430x_phy_read(bcm, 0x042B) | 0x0800);
			bcm430x_phy_write(bcm, 0x0401,
			                  bcm430x_phy_read(bcm, 0x0401) | 0x1000);

			bcm430x_phy_write(bcm, 0x04A0,
			                  (bcm430x_phy_read(bcm, 0x04A0) & 0xC0C0) | 0x0008);
			bcm430x_phy_write(bcm, 0x04A1,
			                  (bcm430x_phy_read(bcm, 0x04A1) & 0xC0C0) | 0x0008);
			bcm430x_phy_write(bcm, 0x04A2,
			                  (bcm430x_phy_read(bcm, 0x04A2) & 0xC0C0) | 0x0008);
			bcm430x_phy_write(bcm, 0x04A8,
			                  (bcm430x_phy_read(bcm, 0x04A8) & 0xC0C0) | 0x0008);
			bcm430x_phy_write(bcm, 0x04AB,
			                  (bcm430x_phy_read(bcm, 0x04AB) & 0xC0C0) | 0x0008);
						bcm430x_phy_write(bcm, 0x04A7, 0x0002);
			bcm430x_phy_write(bcm, 0x04A3, 0x287A);
			bcm430x_phy_write(bcm, 0x04A9, 0x2027);
			bcm430x_phy_write(bcm, 0x0493, 0x32F5);
			bcm430x_phy_write(bcm, 0x04AA, 0x2027);
						bcm430x_phy_write(bcm, 0x04AC, 0x32F5);
		} else { /* DISABLE */
			if (bcm->current_core->phy->rev != 1) {
				bcm430x_phy_write(bcm, 0x042B,
				                  bcm430x_phy_read(bcm, 0x042B) & ~0x0800);
				bcm430x_phy_write(bcm, 0x0429,
				                  bcm430x_phy_read(bcm, 0x0429) & 0x4000);
				return;
			}
			fmapoffset = bcm430x_flipmap[(bcm430x_phy_read(bcm, 0x0078) & 0x001E) >> 1]; 
			if ( fmapoffset >= 0x000C )
				fmapoffset += 3;
			bcm430x_phy_write(bcm, 0x0078, 2*bcm430x_flipmap[fmapoffset]);
			//FIXME: FuncPlaceholder(Set NRSSI Threshold)
			if (bcm->current_core->rev < 5)
				bcm430x_phy_write(bcm, 0x0406, stack[--i]);
			else {
				bcm430x_phy_write(bcm, 0x04C0, stack[--i]);
				bcm430x_phy_write(bcm, 0x04C1, stack[--i]);
			}
			bcm430x_phy_write(bcm, 0x042B,
			                  bcm430x_phy_read(bcm, 0x042B) & ~0x0800);
			/*FIXME:
			if (Bad frame preempt?)
				bcm430x_phy_write(bcm, 0x0401,
			                          bcm430x_phy_read(bcm, 0x0401) & 0x1000);
			*/
			bcm430x_phy_write(bcm, 0x0429,
				          bcm430x_phy_read(bcm, 0x0429) & 0x4000);
			bcm430x_phy_write(bcm, 0x04A0, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A1, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A2, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A8, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AB, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A7, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A3, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A9, stack[--i]);
			bcm430x_phy_write(bcm, 0x0493, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AA, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AC, stack[--i]);
		}
		break;
	case BCM430x_RADIO_INTERFMODE_MANUALWLAN:
		if (!disable) {
			if (bcm430x_phy_read(bcm, 0x0033) == 0x0800)
				return;
			//FIXME: (16c?)
			stack[i++] = bcm430x_phy_read(bcm, 0x04AB);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A8);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A2);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A0);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A1);
			stack[i++] = bcm430x_phy_read(bcm, 0x0493);
			stack[i++] = bcm430x_phy_read(bcm, 0x04AC);
			stack[i++] = bcm430x_phy_read(bcm, 0x04AA);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A9);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A3);
			stack[i++] = bcm430x_phy_read(bcm, 0x04A7);
			stack[i++] = bcm430x_phy_read(bcm, 0x0033);
			if (bcm->current_core->rev < 5)
				stack[i++] = bcm430x_phy_read(bcm, 0x0406);
			else {
				stack[i++] = bcm430x_phy_read(bcm, 0x04C1);
				stack[i++] = bcm430x_phy_read(bcm, 0x04C0);
			}
			stack[i++] = bcm430x_phy_read(bcm, 0x0429);
			stack[i++] = bcm430x_phy_read(bcm, 0x0401);
			bcm430x_phy_write(bcm, 0x0401,
			                  bcm430x_phy_read(bcm, 0x0401) & 0xEFFF);
			bcm430x_phy_write(bcm, 0x0429,
			                  (bcm430x_phy_read(bcm, 0x0429) & 0xEFFF) | 0x0002);
			bcm430x_phy_write(bcm, 0x04A7, 0x0800);
			bcm430x_phy_write(bcm, 0x04A3, 0x287A);
			bcm430x_phy_write(bcm, 0x04A9, 0x2027);
			bcm430x_phy_write(bcm, 0x0493, 0x32F5);
			bcm430x_phy_write(bcm, 0x04AA, 0x2027);
			bcm430x_phy_write(bcm, 0x04AC, 0x32F5);
			bcm430x_phy_write(bcm, 0x04A0,
			                  (bcm430x_phy_read(bcm, 0x04A0) & 0xFFC0) | 0x001A);
			if (bcm->current_core->rev < 5)
				bcm430x_phy_write(bcm, 0x0406, 0x280D);
			else {
				bcm430x_phy_write(bcm, 0x04C0, 0x0640);
				bcm430x_phy_write(bcm, 0x04C1, 0x00A9);
			}
			bcm430x_phy_write(bcm, 0x04A1,
			                  (bcm430x_phy_read(bcm, 0x04A1) & 0xC0FF) | 0x1800);
			bcm430x_phy_write(bcm, 0x04A1,
			                  (bcm430x_phy_read(bcm, 0x04A1) & 0xFFC0) | 0x0016);
			bcm430x_phy_write(bcm, 0x04A2,
			                  (bcm430x_phy_read(bcm, 0x04A2) & 0xF0FF) | 0x0900);
			bcm430x_phy_write(bcm, 0x04A0,
			                  (bcm430x_phy_read(bcm, 0x04A0) & 0xF0FF) | 0x0700);
			bcm430x_phy_write(bcm, 0x04A2,
			                  (bcm430x_phy_read(bcm, 0x04A2) & 0xFFF0) | 0x000D);
			bcm430x_phy_write(bcm, 0x04A8,
			                  (bcm430x_phy_read(bcm, 0x04A8) & 0xCFFF) | 0x1000);
			bcm430x_phy_write(bcm, 0x04A8,
			                  (bcm430x_phy_read(bcm, 0x04A8) & 0xF0FF) | 0x0A00);
			bcm430x_phy_write(bcm, 0x04AB,
			                  (bcm430x_phy_read(bcm, 0x04AB) & 0xCFFF) | 0x1000);
			bcm430x_phy_write(bcm, 0x04AB,
			                  (bcm430x_phy_read(bcm, 0x04AB) & 0xF0FF) | 0x0800);
			bcm430x_phy_write(bcm, 0x04AB,
			                  (bcm430x_phy_read(bcm, 0x04AB) & 0xFFCF) | 0x0010);
			bcm430x_phy_write(bcm, 0x04AB,
			                  (bcm430x_phy_read(bcm, 0x04AB) & 0xFFF0) | 0x0006);
			//FIXME: FuncPlaceholder (2050 NRSSI Slope Cal)

		} else { /* DISABLE */
			if (bcm430x_phy_read(bcm, 0x0033) != 0x0800)
				return;
			//FIXME: (16c?)
			bcm430x_phy_write(bcm, 0x0401, stack[--i]);
			bcm430x_phy_write(bcm, 0x0429, stack[--i]);
			if (bcm->current_core->rev < 5)
				bcm430x_phy_write(bcm, 0x0406, stack[--i]);
			else {
				bcm430x_phy_write(bcm, 0x04C0, stack[--i]);
				bcm430x_phy_write(bcm, 0x04C1, stack[--i]);
			}
			bcm430x_phy_write(bcm, 0x0033, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A7, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A3, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A9, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AA, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AC, stack[--i]);
			bcm430x_phy_write(bcm, 0x0493, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A1, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A0, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A2, stack[--i]);
			bcm430x_phy_write(bcm, 0x04A8, stack[--i]);
			bcm430x_phy_write(bcm, 0x04AB, stack[--i]);
			//FIXME: FuncPlaceholder (2050 NRSSI Slope Cal)
		}
		break;
	case BCM430x_RADIO_INTERFMODE_AUTOWLAN:
		//FIXME:
		//if (!disable) {
		//} else { /* DISABLE */
		//}
		break;
	default:
		printk(KERN_WARNING PFX "calc_interference(): Illegal mode %d\n", mode);
		break;
	};
	
#ifdef BCM430x_DEBUG
	// make sure i only 0 when we disable!
	assert( ~((disable == 0) ^ (i == 0)) );
#endif
	bcm->current_core->radio->interfsize = i;
	bcm->current_core->radio->interfmode = mode;
}

static u16 bcm430x_radio_calibrationvalue(struct bcm430x_private *bcm)
{
	u16 values[16] = {
		0x0002, 0x0003, 0x0001, 0x000F,
		0x0006, 0x0007, 0x0005, 0x000F,
		0x000A, 0x000B, 0x0009, 0x000F,
		0x000E, 0x000F, 0x000D, 0x000F,
	};
	u16 reg, index, ret;

	reg    = bcm430x_radio_read16(bcm, 0x0060);
	index  = (reg & 0x001E) / 2;
	ret    = values[index] * 2;
	ret   |= (reg & 0x0001);
	ret   |= 0x0020;

	return ret;
}

u16 bcm430x_radio_init2050(struct bcm430x_private *bcm)
{
	u16 stack[20];
	u16 index = 0, reg78, ret;
	int i, j, tmp1 = 0, tmp2 = 0;
	
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		stack[index++] = bcm430x_read16(bcm, 0x03EC);
		stack[index++] = bcm430x_phy_read(bcm, 0x0030);
		bcm430x_phy_write(bcm, 0x0030, 0x00FF);
		bcm430x_write16(bcm, 0x03EC, 0x3F3F);
	} else {
		if (bcm->current_core->phy->connected) {
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
		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO,
		                (bcm430x_read16(bcm, BCM430x_MMIO_PHY_RADIO) | 0x8000));
	}
	stack[index++] = bcm430x_phy_read(bcm, 0x0035);
	bcm430x_phy_write(bcm, 0x0035,
	                  (bcm430x_phy_read(bcm, 0x0035) & 0xFF7F));
	stack[index++] = bcm430x_read16(bcm, 0x03F4);
	stack[index++] = bcm430x_read16(bcm, 0x03E6);
	stack[index++] = bcm430x_radio_read16(bcm, 0x0043);
	stack[index++] = bcm430x_phy_read(bcm, 0x0015);
	
	// Initialization
	if (bcm->current_core->phy->version == 0)
		bcm430x_write16(bcm, 0x03E6, 0x0122);
	else {
		if (bcm->current_core->phy->version >= 2)
			bcm430x_write16(bcm, 0x03E6, 0x0040);
		bcm430x_write16(bcm, 0x03F4,
                                (bcm430x_read16(bcm, 0x03F4) | 0x2000));
	}

	ret = bcm430x_radio_calibrationvalue(bcm);
	
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B)
		bcm430x_radio_write16(bcm, 0x0078, 0x0003);
	
	bcm430x_radio_write16(bcm, 0x0015, 0xBFAF);
	bcm430x_radio_write16(bcm, 0x002B, 0x1403);
	if (!bcm->current_core->phy->connected)
		bcm430x_phy_write(bcm, 0x0812, 0x00B2);
	bcm430x_phy_write(bcm, 0x0015, 0xBFA0);
	bcm430x_radio_write16(bcm, 0x0051,
	                      (bcm430x_read16(bcm, 0x0051) | 0x0004));
	bcm430x_radio_write16(bcm, 0x0052, 0x0000);
	bcm430x_radio_write16(bcm, 0x0043, 0x0009);
	bcm430x_radio_write16(bcm, 0x0058, 0x0000);
	
	for (i = 0; i < 16; i++) {
		bcm430x_phy_write(bcm, 0x005A, 0x0480);
		bcm430x_phy_write(bcm, 0x0059, 0x6810);
		bcm430x_phy_write(bcm, 0x0058, 0x000D);
		if (bcm->current_core->phy->connected)
			bcm430x_phy_write(bcm, 0x0812, 0x30B2);
		bcm430x_phy_write(bcm, 0x0015, 0xAFB0);
		udelay(10);
		if (bcm->current_core->phy->connected)
			bcm430x_phy_write(bcm, 0x0812, 0x30B2);
		bcm430x_phy_write(bcm, 0x0015, 0xEFB0);
		udelay(10);
		if (bcm->current_core->phy->connected)
			bcm430x_phy_write(bcm, 0x0812, 0x30B2);
		bcm430x_phy_write(bcm, 0x0015, 0xFFF0);
		udelay(10);
		tmp1 += bcm430x_phy_read(bcm, 0x002D);
		bcm430x_phy_write(bcm, 0x0058, 0x0000);
		if (bcm->current_core->phy->connected)
			bcm430x_phy_write(bcm, 0x0812, 0x30B2);
		bcm430x_phy_write(bcm, 0x0015, 0xAFB0);
	}

	tmp1++;
	tmp1 = (tmp1 >> 9);
	udelay(10);
	bcm430x_radio_write16(bcm, 0x0058, 0x0000);
	
	for (i = 0; i < 16; i++) {
		bcm430x_phy_write(bcm, 0x0078, bcm430x_flipmap[i] | 0x0020);
		reg78 = bcm430x_phy_read(bcm, 0x0078);
		udelay(10);
		for (j = 0; j < 16; j++) {
			bcm430x_phy_write(bcm, 0x005A, 0x0D80);
			bcm430x_phy_write(bcm, 0x0059, 0xC810);
			bcm430x_phy_write(bcm, 0x0058, 0x000D);
			if (bcm->current_core->phy->connected)
				bcm430x_phy_write(bcm, 0x0812, 0x30B2);
			bcm430x_phy_write(bcm, 0x0015, 0xAFB0);
			udelay(10);
			if (bcm->current_core->phy->connected)
				bcm430x_phy_write(bcm, 0x0812, 0x30B2);
			bcm430x_phy_write(bcm, 0x0015, 0xEFB0);
			udelay(10);
			if (bcm->current_core->phy->connected)
				bcm430x_phy_write(bcm, 0x0812, 0x30B2);
			bcm430x_phy_write(bcm, 0x0015, 0xFFF0);
			udelay(10);
			tmp2 += bcm430x_phy_read(bcm, 0x002D);
			bcm430x_phy_write(bcm, 0x0058, 0x0000);
			if (bcm->current_core->phy->connected)
				bcm430x_phy_write(bcm, 0x0812, 0x30B2);
			bcm430x_phy_write(bcm, 0x0015, 0xAFB0);
		}
		tmp2++;
		tmp2 = (tmp2 >> 8);
		if (tmp1 < tmp2) {
#ifdef BCM430x_DEBUG
printk(KERN_INFO PFX "Broke loop 2 in radio_init2050: i=%d, j=%d, tmp1=%d, tmp2=%d\n", i, j, tmp1, tmp2);
#endif
			break;
		}
	}
	
	// Restore data from stack
	bcm430x_phy_write(bcm, 0x0015, stack[--index]);
	bcm430x_radio_write16(bcm, 0x0051,
	                      (bcm430x_radio_read16(bcm, 0x0051) & 0xFFFB));
	bcm430x_radio_write16(bcm, 0x0051, 0x0009);
	bcm430x_radio_write16(bcm, 0x0043, stack[--index]);
	bcm430x_write16(bcm, 0x003E6, stack[--index]);
	--index;
	if (!bcm->current_core->phy->version == 0)
		bcm430x_write16(bcm, 0x003E6, stack[index]);
	bcm430x_phy_write(bcm, 0x0035, stack[--index]);
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		bcm430x_phy_write(bcm, 0x0030, stack[--index]);
		bcm430x_write16(bcm, 0x03EC, stack[--index]);
	} else {
		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO,
		                (bcm430x_read16(bcm, BCM430x_MMIO_PHY_RADIO) & 0x7FFF));
		if (bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x0811, stack[--index]);
			bcm430x_phy_write(bcm, 0x0812, stack[--index]);
			bcm430x_phy_write(bcm, 0x0814, stack[--index]);
			bcm430x_phy_write(bcm, 0x0815, stack[--index]);
			bcm430x_phy_write(bcm, 0x0429, stack[--index]);
			bcm430x_phy_write(bcm, 0x0802, stack[--index]);
		}
	}
	if (i >= 15)
		ret = bcm430x_radio_read16(bcm, 0x0078);

	return ret;
}

u16 bcm430x_radio_init2060(struct bcm430x_private *bcm)
{
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
	bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_A);
printk(KERN_WARNING PFX "FIXME: radio_init2060(), what is the 802.11a default channel?\n");
	udelay(1000);

	return 0;
}

u16 bcm430x_radio_read16(struct bcm430x_private *bcm, u16 offset)
{
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_A)
		offset |= 0x40;
	else if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		switch (bcm->current_core->radio->id & 0x0FFFF000) {
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
	} else if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G)
		offset |= 0x80;
	
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA);
}

int bcm430x_radio_selectchannel(struct bcm430x_private *bcm,
				u8 channel)
{
	// Frequencies are given as differences to 2.4GHz
	// starting with channel 1
	static u16 frequencies_bg[14] = {
	        12, 17, 22, 27,
		32, 37, 42, 47,
		52, 57, 62, 67,
		72, 84,
	};

        switch (bcm->current_core->phy->type) {
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

void bcm430x_radio_set_txantenna(struct bcm430x_private *bcm, u32 val)
{
	u32 tmp;

	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0022);
	tmp = bcm430x_shm_read32(bcm) & 0xFFFFFCFF;
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0022);
	bcm430x_shm_write32(bcm, tmp | val);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x03A8);
	tmp = bcm430x_shm_read32(bcm) & 0xFFFFFCFF;
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x03A8);
	bcm430x_shm_write32(bcm, tmp | val);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0054);
	tmp = bcm430x_shm_read32(bcm) & 0xFFFFFCFF;
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0054);
	bcm430x_shm_write32(bcm, tmp | val);
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
		baseband_attenuation = bcm->current_core->radio->txpower[0];
	else
		bcm->current_core->radio->txpower[0] = baseband_attenuation;
	if (attenuation == 0xFFFF)
		attenuation = bcm->current_core->radio->txpower[1];
	else
		baseband_attenuation = attenuation;
	if (txpower == 0xFFFF)
		txpower = bcm->current_core->radio->txpower[2];
	else
		bcm->current_core->radio->txpower[2] = txpower;
	
	if (bcm->current_core->phy->version == 0) {
		reg = 0x03E6;
		tmp = (bcm430x_phy_read(bcm, reg) & 0xFFF0) | baseband_attenuation;
	} else if (bcm->current_core->phy->version == 1) {
		tmp  = bcm430x_phy_read(bcm, reg) & ~0x0078;
		tmp |= (baseband_attenuation << 3) & 0x0078;
	} else {
		tmp  = bcm430x_phy_read(bcm, reg) & ~0x003C;
		tmp |= (baseband_attenuation << 2) & 0x003C;
	}
	bcm430x_phy_write(bcm, reg, tmp);
	bcm430x_write16(bcm, 0x0043, attenuation);
	bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0064);
	bcm430x_shm_write16(bcm, attenuation);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000)
		bcm430x_radio_write16(bcm, 0x0052,
		                      (bcm430x_radio_read16(bcm, 0x0052) & 0xFF8F) | txpower);
	//FIXME: Set GPHY CompLo
}


int bcm430x_radio_turn_on(struct bcm430x_private *bcm)
{
	if (bcm->current_core->radio->id == BCM430x_RADIO_ID_NORF) {
		printk(KERN_ERR PFX "Error: No radio device found on chip!\n");
		return -ENODEV;
	}

	if (bcm->current_core->radio->enabled)
		return 0;

	switch (bcm->current_core->phy->type) {
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
		bcm430x_phy_write(bcm, 0x0015, ((bcm->current_core->phy->connected) ? 0x00C0 : 0x0000));
		bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_BG);
		break;
	default:
		printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return -1;
	}
	dprintk(KERN_INFO PFX "Radio turned on\n");

	bcm->current_core->radio->enabled = 1;

	return 0;
}
	
int bcm430x_radio_turn_off(struct bcm430x_private *bcm)
{
	if (bcm->current_core->radio->id == BCM430x_RADIO_ID_NORF)
		return -ENODEV;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		bcm430x_radio_write16(bcm, 0x0004, 0x00FF);
		bcm430x_radio_write16(bcm, 0x0005, 0x00FB);
		bcm430x_phy_write(bcm, 0x0010, bcm430x_phy_read(bcm, 0x0010) | 0x0008);
		bcm430x_phy_write(bcm, 0x0011, bcm430x_phy_read(bcm, 0x0011) | 0x0008);
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		if (bcm->chip_rev < 5)
			bcm430x_phy_write(bcm, 0x0015, 0xAA00);
		else {
			bcm430x_phy_write(bcm, 0x0811, bcm430x_phy_read(bcm, 0x0811) | 0x008C);
			bcm430x_phy_write(bcm, 0x0812, bcm430x_phy_read(bcm, 0x0812) & 0xFF73);
		}
		break;
	default:
		printk(KERN_WARNING PFX "Unknown PHY Type found.\n");
		return -1;
	}
	dprintk(KERN_INFO PFX "Radio turned off\n");

	bcm->current_core->radio->enabled = 0;

	return 0;
}

void bcm430x_radio_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_DATA, val);
}

void bcm430x_radio_clear_tssi(struct bcm430x_private *bcm)
{
	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0068);
		bcm430x_shm_write32(bcm, 0x7F7F);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x006A);
		bcm430x_shm_write32(bcm, 0x7F7F);
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0058);
		bcm430x_shm_write32(bcm, 0x7F7F);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x005A);
		bcm430x_shm_write32(bcm, 0x7F7F);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0070);
		bcm430x_shm_write32(bcm, 0x7F7F);
		bcm430x_shm_control(bcm, BCM430x_SHM_SHARED + 0x0072);
		bcm430x_shm_write32(bcm, 0x7F7F);
		break;
	}
}
