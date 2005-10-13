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
#include "bcm430x_ilt.h"


/* Frequencies are given as frequencies_bg[index] + 2.4GHz
 * Starting with channel 1
 */
static const u16 frequencies_bg[14] = {
        12, 17, 22, 27,
	32, 37, 42, 47,
	52, 57, 62, 67,
	72, 84,
};

/* Table for bcm430x_radio_calibrationvalue() */
static const u16 rcc_table[16] = {
	0x0002, 0x0003, 0x0001, 0x000F,
	0x0006, 0x0007, 0x0005, 0x000F,
	0x000A, 0x000B, 0x0009, 0x000F,
	0x000E, 0x000F, 0x000D, 0x000F,
};

/* Reverse the bits of a 4bit value.
 * Example:  1101 is flipped 1011
 */
static u16 flip_4bit(u16 value)
{
	u16 flipped = 0x0000;

	assert((value & ~0x000F) == 0x0000);

	flipped |= (value & 0x0001) << 3;
	flipped |= (value & 0x0002) << 1;
	flipped |= (value & 0x0004) >> 1;
	flipped |= (value & 0x0008) >> 3;

	return flipped;
}

u16 bcm430x_radio_read16(struct bcm430x_private *bcm, u16 offset)
{
	u32 radio_ver;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		offset |= 0x0040;
		break;
	case BCM430x_PHYTYPE_B:
		radio_ver = (bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK);
		if (radio_ver == 0x02053000) {
			if (offset < 0x70)
				offset += 0x80;
			else if (offset < 0x80)
				offset += 0x70;
		} else if (radio_ver == 0x02050000)
			offset |= 0x80;
		break;
	case BCM430x_PHYTYPE_G:
		offset |= 0x80;
		break;
	}

	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	return bcm430x_read16(bcm, BCM430x_MMIO_RADIO_DATA_LOW);
}

void bcm430x_radio_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_CONTROL, offset);
	bcm430x_write16(bcm, BCM430x_MMIO_RADIO_DATA_LOW, val);
}

static void bcm430x_set_all_gains(struct bcm430x_private *bcm,
				  s16 first, s16 second)
{
	u16 i;
	u16 start = 16, end = 32;
	u16 offset = 0x0400;

	if (bcm->current_core->phy->rev == 1) {
		offset = 0x5000;
		start = 8;
		end = 24;
	}
	
	for (i = 0; i < 4; i++)
		bcm430x_ilt_write16(bcm, offset + i, first);

	for (i = start; i < end; i++)
		bcm430x_ilt_write16(bcm, offset + i, first);

	if (second == -1)
		return;

	bcm430x_phy_write(bcm, 0x04A0,
	                  (bcm430x_phy_read(bcm, 0x04A0) & 0xBFBF) | 0x4040);
	bcm430x_phy_write(bcm, 0x04A1,
	                  (bcm430x_phy_read(bcm, 0x04A1) & 0xBFBF) | 0x4040);
	bcm430x_phy_write(bcm, 0x04A2,
	                  (bcm430x_phy_read(bcm, 0x04A2) & 0xBFBF) | 0x4000);
}

static void bcm430x_set_original_gains(struct bcm430x_private *bcm)
{
	u16 i, tmp;
	u16 offset = 0x0400;
	u16 start = 0x0008, end = 0x0018;

	if (bcm->current_core->phy->rev == 1) {
		offset = 0x5000;
		start = 0x0010;
		end = 0x0020;
	}

	for (i = 0; i < 4; i++) {
		tmp = (i & 0xFFFC);
		tmp |= (i & 0x0001) << 1;
		tmp |= (i & 0x0002) >> 1;

		bcm430x_ilt_write16(bcm, offset + i, tmp);
	}

	for (i = start; i < end; i++)
		bcm430x_ilt_write16(bcm, offset + i, i);

	bcm430x_phy_write(bcm, 0x04A0,
	                  (bcm430x_phy_read(bcm, 0x04A0) & 0xBFBF) | 0x4040);
	bcm430x_phy_write(bcm, 0x04A1,
	                  (bcm430x_phy_read(bcm, 0x04A1) & 0xBFBF) | 0x4040);
	bcm430x_phy_write(bcm, 0x04A2,
	                  (bcm430x_phy_read(bcm, 0x04A2) & 0xBFBF) | 0x4000);
}

void bcm430x_calc_nrssi_slope(struct bcm430x_private *bcm)
{
	/*FIXME: We are not completely sure, if the nrssi values are really s16.
	 *       We have to check by testing, if the values and the u16 to s16 casts are correct.
	 */

	u16 backup[14];
	s16 slope = 0;
	s16 nrssi0;
	s16 nrssi1;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_B:
		backup[0] = bcm430x_radio_read16(bcm, 0x007A);
		backup[1] = bcm430x_radio_read16(bcm, 0x0052);
		backup[2] = bcm430x_radio_read16(bcm, 0x0043);
		backup[3] = bcm430x_phy_read(bcm, 0x0030);
		backup[4] = bcm430x_phy_read(bcm, 0x0026);
		backup[5] = bcm430x_phy_read(bcm, 0x0015);
		backup[6] = bcm430x_phy_read(bcm, 0x002A);
		backup[7] = bcm430x_phy_read(bcm, 0x0020);
		backup[8] = bcm430x_phy_read(bcm, 0x005A);
		backup[9] = bcm430x_phy_read(bcm, 0x0059);
		backup[10] = bcm430x_phy_read(bcm, 0x0058);
		backup[11] = bcm430x_read16(bcm, 0x03E2);
		backup[12] = bcm430x_read16(bcm, 0x03F4);
		backup[13] = bcm430x_read16(bcm, 0x03E6);

		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) & 0x000F);
		bcm430x_phy_write(bcm, 0x0030, 0x00FF);
		bcm430x_write16(bcm, 0x03EC, 0x3F3F);
		bcm430x_phy_write(bcm, 0x0026, 0x0000);
		bcm430x_phy_write(bcm, 0x0015,
				  bcm430x_phy_read(bcm, 0x0015) | 0x0020);
		bcm430x_phy_write(bcm, 0x002A, 0x08A3);
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) | 0x0080);

		nrssi0 = (s16)bcm430x_phy_read(bcm, 0x0027);
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) & 0x007F);
		if (bcm->current_core->phy->rev == 0) {
			bcm430x_write16(bcm, 0x03E6, 0x0122);
		} else {
			bcm430x_write16(bcm, 0x03F4,
					bcm430x_read16(bcm, 0x03F4) & 0x2000);
		}
		bcm430x_phy_write(bcm, 0x0020, 0x3F3F);
		bcm430x_phy_write(bcm, 0x0015, 0xF330);
		bcm430x_radio_write16(bcm, 0x0052, 0x0060);
		bcm430x_radio_write16(bcm, 0x0043, 0x0000);
		bcm430x_phy_write(bcm, 0x005A, 0x0480);
		bcm430x_phy_write(bcm, 0x0059, 0x0810);
		bcm430x_phy_write(bcm, 0x0058, 0x000D);
		udelay(20);

		nrssi1 = (s16)bcm430x_phy_read(bcm, 0x0027);
		bcm430x_phy_write(bcm, 0x0030, backup[3]);
		bcm430x_radio_write16(bcm, 0x007A, backup[0]);
		bcm430x_write16(bcm, 0x03E6, 0x0122);
		bcm430x_write16(bcm, 0x03E2, backup[11]);
		bcm430x_phy_write(bcm, 0x0026, backup[4]);
		bcm430x_phy_write(bcm, 0x0015, backup[5]);
		bcm430x_phy_write(bcm, 0x002A, backup[6]);
		if (bcm->current_core->phy->rev == 0)
			bcm430x_write16(bcm, 0x03E6, backup[13]);
		else
			bcm430x_write16(bcm, 0x03F4, backup[12]);
		bcm430x_phy_write(bcm, 0x0020, backup[7]);
		bcm430x_phy_write(bcm, 0x005A, backup[8]);
		bcm430x_phy_write(bcm, 0x0059, backup[9]);
		bcm430x_phy_write(bcm, 0x0058, backup[10]);
		bcm430x_radio_write16(bcm, 0x0052, backup[1]);
		bcm430x_radio_write16(bcm, 0x0043, backup[2]);

		slope = nrssi0 - nrssi1;
		if (slope == 0)
			slope = 64;
		bcm->current_core->radio->nrssislope = 0x400000 / slope;

		if (nrssi0 <= -4) {
			bcm->current_core->radio->nrssi[0] = nrssi0;
			bcm->current_core->radio->nrssi[1] = nrssi1;
		}
		break;
	case BCM430x_PHYTYPE_G:
		backup[0] = bcm430x_radio_read16(bcm, 0x007A);
		backup[1] = bcm430x_radio_read16(bcm, 0x0052);
		backup[2] = bcm430x_radio_read16(bcm, 0x0043);
		backup[3] = bcm430x_phy_read(bcm, 0x0015);
		backup[4] = bcm430x_phy_read(bcm, 0x005A);
		backup[5] = bcm430x_phy_read(bcm, 0x0059);
		backup[6] = bcm430x_phy_read(bcm, 0x0058);
		backup[7] = bcm430x_read16(bcm, 0x03E2);
		backup[8] = bcm430x_read16(bcm, 0x03F4);
		backup[9] = bcm430x_read16(bcm, 0x03E6);

		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) | 0x0070);
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
				  bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & ~0x8000);
		bcm430x_phy_write(bcm, 0x0802,
				  bcm430x_phy_read(bcm, 0x0802) & ~(0x0001 | 0x0002));
		bcm430x_phy_write(bcm, 0x03E2, 0x8000);

		bcm430x_set_all_gains(bcm, 0, 0);
		bcm430x_dummy_transmission(bcm);
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) & 0x00F7);
		if (bcm->current_core->phy->rev >= 2) {
			bcm430x_phy_write(bcm, 0x0812,
					  (bcm430x_phy_read(bcm, 0x0812) & 0xFFCF) | 0x0010);
			bcm430x_phy_write(bcm, 0x0811,
					  (bcm430x_phy_read(bcm, 0x0811) & 0xFFCF) | 0x0010);
		}
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) | 0x0080);
		udelay(20);

		nrssi0 = (s16)((bcm430x_phy_read(bcm, 0x047F) >> 8) & 0x003F);
		if (nrssi0 >= 0x0020)
			nrssi0 -= 0x0040;

		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) & 0x007F);
		if (bcm->current_core->phy->rev >= 2)
			bcm430x_write16(bcm, 0x03E6, 0x0040);
		bcm430x_write16(bcm, 0x03F4, 0x2000);
		bcm430x_radio_write16(bcm, 0x007A,
				      bcm430x_radio_read16(bcm, 0x007A) | 0x000F);
		bcm430x_phy_write(bcm, 0x0015, 0xF330);
		if (bcm->current_core->phy->rev >= 2) {
			bcm430x_phy_write(bcm, 0x0812,
					  (bcm430x_phy_read(bcm, 0x0812) & 0xFFCF) | 0x0020);
			bcm430x_phy_write(bcm, 0x0811,
					  (bcm430x_phy_read(bcm, 0x0811) & 0xFFCF) | 0x0020);
		}

		bcm430x_set_all_gains(bcm, 3, 1);
		bcm430x_dummy_transmission(bcm);
		bcm430x_radio_write16(bcm, 0x0052, 0x0060);
		bcm430x_radio_write16(bcm, 0x0043, 0x0000);
		bcm430x_phy_write(bcm, 0x005A, 0x0480);
		bcm430x_phy_write(bcm, 0x0059, 0x0810);
		bcm430x_phy_write(bcm, 0x0058, 0x000D);
		udelay(20);

		nrssi1 = (s16)((bcm430x_phy_read(bcm, 0x047F) >> 8) & 0x003F);
		if (nrssi1 >= 0x0020)
			nrssi1 -= 0x0040;
		slope = nrssi0 - nrssi1;
		if (slope == 0)
			slope = 64;
		bcm->current_core->radio->nrssislope = 0x400000 / slope;

		if (nrssi0 > -5) {
			bcm->current_core->radio->nrssi[0] = nrssi1;
			bcm->current_core->radio->nrssi[1] = nrssi0;
		}
		if (bcm->current_core->phy->rev >= 2) {
			bcm430x_phy_write(bcm, 0x0812,
					  bcm430x_phy_read(bcm, 0x0812) & 0xFFCF);
			bcm430x_phy_write(bcm, 0x0811,
					  bcm430x_phy_read(bcm, 0x0811) & 0xFFCF);
		}

		bcm430x_radio_write16(bcm, 0x007A, backup[0]);
		bcm430x_radio_write16(bcm, 0x0052, backup[1]);
		bcm430x_radio_write16(bcm, 0x0043, backup[2]);
		bcm430x_write16(bcm, 0x03E2, backup[7]);
		bcm430x_write16(bcm, 0x03E6, backup[9]);
		bcm430x_write16(bcm, 0x03F4, backup[8]);
		bcm430x_phy_write(bcm, 0x0015, backup[3]);
		bcm430x_phy_write(bcm, 0x005A, backup[4]);
		bcm430x_phy_write(bcm, 0x0059, backup[5]);
		bcm430x_phy_write(bcm, 0x0058, backup[6]);
		bcm430x_phy_write(bcm, 0x0802,
				  bcm430x_phy_read(bcm, 0x0802) | (0x0001 | 0x0002));

		bcm430x_set_original_gains(bcm);
		bcm430x_dummy_transmission(bcm);

		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
				  bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) | 0x8000);

		TODO();
		//TODO update inmem nrssi lookup table
		bcm430x_calc_nrssi_threshold(bcm);
		break;
	default:
		assert(0);
	}
}

void bcm430x_calc_nrssi_threshold(struct bcm430x_private *bcm)
{
	s16 threshold;
	s16 a, b;
	int tmp;
	u16 tmp16;

	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_B:
		if (bcm->current_core->phy->rev < 2)
			return;
		if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) != 0x02050000)
			return;
		if (!(bcm->sprom.boardflags & BCM430x_BFL_RSSI))
			return;

		if (bcm->current_core->radio->id == 0x3205017F
		    || bcm->current_core->radio->id == 0x4205017F
		    || bcm->current_core->radio->id == 0x52050175) {
			threshold = bcm->current_core->radio->nrssi[1];
		} else {
			threshold = (0x28 * bcm->current_core->radio->nrssi[0]
				     + 0x20 * (bcm->current_core->radio->nrssi[1]
					       - (bcm->current_core->radio->nrssi[0] + 0x14)));
			threshold /= 10;
		}
		threshold = limit_value(threshold, 0, 0x3E);
		bcm430x_phy_read(bcm, 0x0020); /* dummy read */
		bcm430x_phy_write(bcm, 0x0020, (((u16)threshold) << 8) | 0x001C);

		if (bcm->current_core->radio->id == 0x3205017F
		    || bcm->current_core->radio->id == 0x4205017F
		    || bcm->current_core->radio->id == 0x52050175) {
			bcm430x_phy_write(bcm, 0x0087, 0x0E0D);
			bcm430x_phy_write(bcm, 0x0086, 0x0C0B);
			bcm430x_phy_write(bcm, 0x0085, 0x0A09);
			bcm430x_phy_write(bcm, 0x0084, 0x0808);
			bcm430x_phy_write(bcm, 0x0083, 0x0808);
			bcm430x_phy_write(bcm, 0x0082, 0x0604);
			bcm430x_phy_write(bcm, 0x0081, 0x0302);
			bcm430x_phy_write(bcm, 0x0080, 0x0100);
		}
		break;
	case BCM430x_PHYTYPE_G:
		if (!bcm->current_core->phy->connected ||
		    !(bcm->sprom.boardflags & BCM430x_BFL_RSSI)) {
			TODO();//TODO
		} else {
			tmp = bcm->current_core->radio->interfmode;
			if (tmp == BCM430x_RADIO_INTERFMODE_NONWLAN) {
				a = -13;
				b = -17;
			} else if (tmp == BCM430x_RADIO_INTERFMODE_NONE
				   /*FIXME: && bit 2 of unk16c is not set*/) {
				a = -13;
				b = -10;
			} else {
				a = -4;
				b = -3;
			}
			a += 0x1B;
			a *= bcm->current_core->radio->nrssi[1] - bcm->current_core->radio->nrssi[0];
			a += bcm->current_core->radio->nrssi[0] * 0x40;
			//TODO limit_value()? specs incomplete
			b += 0x1B;
			b *= bcm->current_core->radio->nrssi[1] - bcm->current_core->radio->nrssi[0];
			b += bcm->current_core->radio->nrssi[0] * 0x40;
			//TODO limit_value()? specs incomplete
			TODO();

			a = limit_value(a, -31, 31);
			b = limit_value(b, -31, 31);

			tmp16 = bcm430x_phy_read(bcm, 0x048A) & 0xF000;
			tmp16 |= b & 0x003F;
			tmp16 |= (a << 6) & 0x0FC0;
			bcm430x_phy_write(bcm, 0x048A, tmp16);
		}
		break;
	default:
		assert(0);
	}
}

/* Helper macros to save on and restore values from the radio->interfstack */
#ifdef stack_save
# undef stack_save
#endif
#ifdef stack_restore
# undef stack_restore
#endif
#define stack_save(value)  \
	do {									\
	 	assert(i < ARRAY_SIZE(bcm->current_core->radio->interfstack));	\
		stack[i++] = (value);						\
	} while (0)

#define stack_restore()  \
	({									\
	 	assert(i < ARRAY_SIZE(bcm->current_core->radio->interfstack));	\
	 	stack[i++];							\
	})

static void
bcm430x_radio_interference_mitigation_enable(struct bcm430x_private *bcm,
					     int mode)
{
	int i = 0;
	u16 *stack = bcm->current_core->radio->interfstack;
	u16 tmp;

	switch (mode) {
	case BCM430x_RADIO_INTERFMODE_NONWLAN:
//TODO: review!
		if (bcm->current_core->phy->rev != 1) {
			bcm430x_phy_write(bcm, 0x042B,
			                  bcm430x_phy_read(bcm, 0x042B) & 0x0800);
			bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
			                  bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & ~0x4000);
			break;
		}
		tmp = ((bcm430x_phy_read(bcm, 0x0078) & 0x001E) >> 1);
		tmp = flip_4bit(tmp);
		if (tmp >= 4)
			tmp = flip_4bit(tmp - 3);
		bcm430x_phy_write(bcm, 0x0078, tmp << 1);

		bcm430x_calc_nrssi_threshold(bcm);

		if (bcm->current_core->rev < 5) {
			stack_save(bcm430x_phy_read(bcm, 0x0406));
			bcm430x_phy_write(bcm, 0x0406, 0x7E28);
		} else {
			stack_save(bcm430x_phy_read(bcm, 0x04C0));
			stack_save(bcm430x_phy_read(bcm, 0x04C1));
			bcm430x_phy_write(bcm, 0x04C0, 0x3E04);
			bcm430x_phy_write(bcm, 0x04C1, 0x0640);
		}

		bcm430x_phy_write(bcm, 0x042B,
		                  bcm430x_phy_read(bcm, 0x042B) | 0x0800);
		bcm430x_phy_write(bcm, BCM430x_PHY_RADIO_BITFIELD,
		                  bcm430x_phy_read(bcm, BCM430x_PHY_RADIO_BITFIELD) | 0x1000);

		stack_save(bcm430x_phy_read(bcm, 0x04A0));
		bcm430x_phy_write(bcm, 0x04A0,
		                  (bcm430x_phy_read(bcm, 0x04A0) & 0xC0C0) | 0x0008);
		stack_save(bcm430x_phy_read(bcm, 0x04A1));
		bcm430x_phy_write(bcm, 0x04A1,
				  (bcm430x_phy_read(bcm, 0x04A1) & 0xC0C0) | 0x0605);
		stack_save(bcm430x_phy_read(bcm, 0x04A2));
		bcm430x_phy_write(bcm, 0x04A2,
				  (bcm430x_phy_read(bcm, 0x04A2) & 0xC0C0) | 0x0204);
		stack_save(bcm430x_phy_read(bcm, 0x04A8));
		bcm430x_phy_write(bcm, 0x04A8,
				  (bcm430x_phy_read(bcm, 0x04A8) & 0xC0C0) | 0x0403);
		stack_save(bcm430x_phy_read(bcm, 0x04AB));
		bcm430x_phy_write(bcm, 0x04AB,
				  (bcm430x_phy_read(bcm, 0x04AB) & 0xC0C0) | 0x0504);

		stack_save(bcm430x_phy_read(bcm, 0x04A7));
		bcm430x_phy_write(bcm, 0x04A7, 0x0002);
		stack_save(bcm430x_phy_read(bcm, 0x04A3));
		bcm430x_phy_write(bcm, 0x04A3, 0x287A);
		stack_save(bcm430x_phy_read(bcm, 0x04A9));
		bcm430x_phy_write(bcm, 0x04A9, 0x2027);
		stack_save(bcm430x_phy_read(bcm, 0x0493));
		bcm430x_phy_write(bcm, 0x0493, 0x32F5);
		stack_save(bcm430x_phy_read(bcm, 0x04AA));
		bcm430x_phy_write(bcm, 0x04AA, 0x2027);
		stack_save(bcm430x_phy_read(bcm, 0x04AC));
		bcm430x_phy_write(bcm, 0x04AC, 0x32F5);
		break;
	case BCM430x_RADIO_INTERFMODE_MANUALWLAN:
//TODO: review!
		if (bcm430x_phy_read(bcm, 0x0033) == 0x0800)
			break;

		TODO();
		//TODO: (16c?)
		stack_save(bcm430x_phy_read(bcm, BCM430x_PHY_RADIO_BITFIELD));
		stack_save(bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS));
		if (bcm->current_core->rev < 5) {
			stack_save(bcm430x_phy_read(bcm, 0x0406));
		} else {
			stack_save(bcm430x_phy_read(bcm, 0x04C0));
			stack_save(bcm430x_phy_read(bcm, 0x04C1));
		}
		stack_save(bcm430x_phy_read(bcm, 0x0033));
		stack_save(bcm430x_phy_read(bcm, 0x04A7));
		stack_save(bcm430x_phy_read(bcm, 0x04A3));
		stack_save(bcm430x_phy_read(bcm, 0x04A9));
		stack_save(bcm430x_phy_read(bcm, 0x04AA));
		stack_save(bcm430x_phy_read(bcm, 0x04AC));
		stack_save(bcm430x_phy_read(bcm, 0x0493));
		stack_save(bcm430x_phy_read(bcm, 0x04A1));
		stack_save(bcm430x_phy_read(bcm, 0x04A0));
		stack_save(bcm430x_phy_read(bcm, 0x04A2));
		stack_save(bcm430x_phy_read(bcm, 0x048A));
		stack_save(bcm430x_phy_read(bcm, 0x04A8));
		stack_save(bcm430x_phy_read(bcm, 0x04AB));

		bcm430x_phy_write(bcm, BCM430x_PHY_RADIO_BITFIELD,
				  bcm430x_phy_read(bcm, BCM430x_PHY_RADIO_BITFIELD) & 0xEFFF);
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
				  (bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0xEFFF) | 0x0002);

		bcm430x_phy_write(bcm, 0x04A7, 0x0800);
		bcm430x_phy_write(bcm, 0x04A3, 0x287A);
		bcm430x_phy_write(bcm, 0x04A9, 0x2027);
		bcm430x_phy_write(bcm, 0x0493, 0x32F5);
		bcm430x_phy_write(bcm, 0x04AA, 0x2027);
		bcm430x_phy_write(bcm, 0x04AC, 0x32F5);

		bcm430x_phy_write(bcm, 0x04A0,
				  (bcm430x_phy_read(bcm, 0x04A0) & 0xFFC0) | 0x001A);
		if (bcm->current_core->rev < 5) {
			bcm430x_phy_write(bcm, 0x0406, 0x280D);
		} else {
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

		bcm430x_calc_nrssi_slope(bcm);
		break;
	default:
		assert(0);
	}
}

static void
bcm430x_radio_interference_mitigation_disable(struct bcm430x_private *bcm,
					      int mode)
{
	int i = 0;
	u16 *stack = bcm->current_core->radio->interfstack;
	u16 tmp;

	switch (mode) {
	case BCM430x_RADIO_INTERFMODE_NONWLAN:
//TODO: review!
		if (bcm->current_core->phy->rev != 1) {
			bcm430x_phy_write(bcm, 0x042B,
			                  bcm430x_phy_read(bcm, 0x042B) & ~0x0800);
			bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
			                  bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0x4000);
			break;
		}
		tmp = ((bcm430x_phy_read(bcm, 0x0078) & 0x001E) >> 1);
		tmp = flip_4bit(tmp);
		if (tmp >= 0x000C)
			tmp = flip_4bit(tmp + 3);
		bcm430x_phy_write(bcm, 0x0078, tmp << 1);

		bcm430x_calc_nrssi_threshold(bcm);

		if (bcm->current_core->rev < 5) {
			bcm430x_phy_write(bcm, 0x0406, stack_restore());
		} else {
			bcm430x_phy_write(bcm, 0x04C0, stack_restore());
			bcm430x_phy_write(bcm, 0x04C1, stack_restore());
		}
		bcm430x_phy_write(bcm, 0x042B,
				  bcm430x_phy_read(bcm, 0x042B) & ~0x0800);

		TODO();
		if (0 /*TODO: Bad frame preempt?*/) {
			bcm430x_phy_write(bcm, BCM430x_PHY_RADIO_BITFIELD,
					  bcm430x_phy_read(bcm, BCM430x_PHY_RADIO_BITFIELD) & 0x1000);
		}
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
				  bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0x4000);
		bcm430x_phy_write(bcm, 0x04A0, stack_restore());
		bcm430x_phy_write(bcm, 0x04A1, stack_restore());
		bcm430x_phy_write(bcm, 0x04A2, stack_restore());
		bcm430x_phy_write(bcm, 0x04A8, stack_restore());
		bcm430x_phy_write(bcm, 0x04AB, stack_restore());
		bcm430x_phy_write(bcm, 0x04A7, stack_restore());
		bcm430x_phy_write(bcm, 0x04A3, stack_restore());
		bcm430x_phy_write(bcm, 0x04A9, stack_restore());
		bcm430x_phy_write(bcm, 0x0493, stack_restore());
		bcm430x_phy_write(bcm, 0x04AA, stack_restore());
		bcm430x_phy_write(bcm, 0x04AC, stack_restore());
		break;
	case BCM430x_RADIO_INTERFMODE_MANUALWLAN:
//TODO: review!
		if (bcm430x_phy_read(bcm, 0x0033) != 0x0800)
			break;

		TODO();
		//TODO: (16c?)
		bcm430x_phy_write(bcm, BCM430x_PHY_RADIO_BITFIELD, stack_restore());
		bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS, stack_restore());
		if (bcm->current_core->rev < 5) {
			bcm430x_phy_write(bcm, 0x0406, stack_restore());
		} else {
			bcm430x_phy_write(bcm, 0x04C0, stack_restore());
			bcm430x_phy_write(bcm, 0x04C1, stack_restore());
		}
		bcm430x_phy_write(bcm, 0x0033, stack_restore());
		bcm430x_phy_write(bcm, 0x04A7, stack_restore());
		bcm430x_phy_write(bcm, 0x04A3, stack_restore());
		bcm430x_phy_write(bcm, 0x04A9, stack_restore());
		bcm430x_phy_write(bcm, 0x04AA, stack_restore());
		bcm430x_phy_write(bcm, 0x04AC, stack_restore());
		bcm430x_phy_write(bcm, 0x0493, stack_restore());
		bcm430x_phy_write(bcm, 0x04A1, stack_restore());
		bcm430x_phy_write(bcm, 0x04A0, stack_restore());
		bcm430x_phy_write(bcm, 0x04A2, stack_restore());
		bcm430x_phy_write(bcm, 0x04A8, stack_restore());
		bcm430x_phy_write(bcm, 0x04AB, stack_restore());

		bcm430x_calc_nrssi_slope(bcm);
		break;
	default:
		assert(0);
	}
}

#undef stack_save
#undef stack_restore

int bcm430x_radio_set_interference_mitigation(struct bcm430x_private *bcm,
					      int mode)
{
	int currentmode;

	if (bcm->current_core->phy->type != BCM430x_PHYTYPE_G)
		return -ENODEV;
	if (bcm->current_core->phy->rev == 0)
		return -ENODEV;
	if (!bcm->current_core->phy->connected)
		return -ENODEV;

	switch (mode) {
	case BCM430x_RADIO_INTERFMODE_AUTOWLAN:
		TODO();
		/*TODO: Either enable or disable MANUALWLAN mode. */
		if (1/*TODO*/)
			mode = BCM430x_RADIO_INTERFMODE_MANUALWLAN;
		else
			mode = BCM430x_RADIO_INTERFMODE_NONE;
		break;
	case BCM430x_RADIO_INTERFMODE_NONE:
	case BCM430x_RADIO_INTERFMODE_NONWLAN:
	case BCM430x_RADIO_INTERFMODE_MANUALWLAN:
		break;
	default:
		return -EINVAL;
	}

	currentmode = bcm->current_core->radio->interfmode;
	if (currentmode == mode)
		return 0;
	if (currentmode != BCM430x_RADIO_INTERFMODE_NONE)
		bcm430x_radio_interference_mitigation_disable(bcm, currentmode);

	if (mode != BCM430x_RADIO_INTERFMODE_NONE)
		bcm430x_radio_interference_mitigation_enable(bcm, mode);
	bcm->current_core->radio->interfmode = mode;

	return 0;
}

static u16 bcm430x_radio_calibrationvalue(struct bcm430x_private *bcm)
{
	u16 reg, index, ret;

	reg = bcm430x_radio_read16(bcm, 0x0060);
	index = (reg & 0x001E) >> 1;
	ret = rcc_table[index] << 1;
	ret |= (reg & 0x0001);
	ret |= 0x0020;

	return ret;
}

u16 bcm430x_radio_init2050(struct bcm430x_private *bcm)
{
	u16 backup[13];
	u16 ret;
	u16 i, j;
	u32 tmp1 = 0, tmp2 = 0;

	backup[0] = bcm430x_radio_read16(bcm, 0x0043);
	backup[1] = bcm430x_radio_read16(bcm, 0x0015);
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		backup[2] = bcm430x_phy_read(bcm, 0x0030);
		backup[3] = bcm430x_read16(bcm, 0x03EC);
		bcm430x_phy_write(bcm, 0x0030, 0x00FF);
		bcm430x_write16(bcm, 0x03EC, 0x3F3F);
	} else {
		if (bcm->current_core->phy->connected) {
			backup[4] = bcm430x_phy_read(bcm, 0x0811);
			backup[5] = bcm430x_phy_read(bcm, 0x0812);
			backup[6] = bcm430x_phy_read(bcm, 0x0814);
			backup[7] = bcm430x_phy_read(bcm, 0x0815);
			backup[8] = bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS);
			backup[9] = bcm430x_phy_read(bcm, 0x0802);
			bcm430x_phy_write(bcm, 0x0814,
                                          (bcm430x_phy_read(bcm, 0x0814) | 0x0003));
			bcm430x_phy_write(bcm, 0x0815,
                                          (bcm430x_phy_read(bcm, 0x0815) & 0xFFFC));	
			bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS,
			                  (bcm430x_phy_read(bcm, BCM430x_PHY_G_CRS) & 0x7FFF));
			bcm430x_phy_write(bcm, 0x0802,
			                  (bcm430x_phy_read(bcm, 0x0802) & 0xFFFC));
			bcm430x_phy_write(bcm, 0x0811, 0x01B3);
			bcm430x_phy_write(bcm, 0x0812, 0x0FB2);
		}
		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO,
		                (bcm430x_read16(bcm, BCM430x_MMIO_PHY_RADIO) | 0x8000));
	}
	backup[10] = bcm430x_phy_read(bcm, 0x0035);
	bcm430x_phy_write(bcm, 0x0035,
	                  (bcm430x_phy_read(bcm, 0x0035) & 0xFF7F));
	backup[11] = bcm430x_read16(bcm, 0x03E6);
	backup[12] = bcm430x_read16(bcm, 0x03F4);

	// Initialization
	if (bcm->current_core->phy->version == 0) {
		bcm430x_write16(bcm, 0x03E6, 0x0122);
	} else {
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
	tmp1 >>= 9;
	udelay(10);
	bcm430x_radio_write16(bcm, 0x0058, 0x0000);

	for (i = 0; i < 16; i++) {
		bcm430x_phy_write(bcm, 0x0078, flip_4bit(i) | 0x0020);
		backup[13] = bcm430x_phy_read(bcm, 0x0078);
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
				bcm430x_phy_write(bcm, 0x0812, 0x30B3); /* 0x30B3 is not a typo */
			bcm430x_phy_write(bcm, 0x0015, 0xFFF0);
			udelay(10);
			tmp2 += bcm430x_phy_read(bcm, 0x002D);
			bcm430x_phy_write(bcm, 0x0058, 0x0000);
			if (bcm->current_core->phy->connected)
				bcm430x_phy_write(bcm, 0x0812, 0x30B2);
			bcm430x_phy_write(bcm, 0x0015, 0xAFB0);
		}
		tmp2++;
		tmp2 >>= 8;
		if (tmp1 < tmp2)
			break;
	}

	/* Restore the registers */
	bcm430x_phy_write(bcm, 0x0015, backup[1]);
	bcm430x_radio_write16(bcm, 0x0051,
	                      (bcm430x_radio_read16(bcm, 0x0051) & 0xFFFB));
	bcm430x_radio_write16(bcm, 0x0052, 0x0009);
	bcm430x_radio_write16(bcm, 0x0043, backup[0]);
	bcm430x_write16(bcm, 0x03E6, backup[11]);
	if (bcm->current_core->phy->version != 0)
		bcm430x_write16(bcm, 0x03F4, backup[12]);
	bcm430x_phy_write(bcm, 0x0035, backup[10]);
	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_B) {
		bcm430x_phy_write(bcm, 0x0030, backup[2]);
		bcm430x_write16(bcm, 0x03EC, backup[3]);
	} else {
		bcm430x_write16(bcm, BCM430x_MMIO_PHY_RADIO,
				(bcm430x_read16(bcm, BCM430x_MMIO_PHY_RADIO) & 0x7FFF));
		if (bcm->current_core->phy->connected) {
			bcm430x_phy_write(bcm, 0x0811, backup[4]);
			bcm430x_phy_write(bcm, 0x0812, backup[5]);
			bcm430x_phy_write(bcm, 0x0814, backup[6]);
			bcm430x_phy_write(bcm, 0x0815, backup[7]);
			bcm430x_phy_write(bcm, BCM430x_PHY_G_CRS, backup[8]);
			bcm430x_phy_write(bcm, 0x0802, backup[9]);
		}
	}
	if (i >= 15)
		ret = bcm430x_radio_read16(bcm, 0x0078);

	return ret;
}

void bcm430x_radio_init2060(struct bcm430x_private *bcm)
{
	int err;

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

	err = bcm430x_radio_selectchannel(bcm, BCM430x_RADIO_DEFAULT_CHANNEL_A);
	assert(err == 0);
	udelay(1000);
}

int bcm430x_radio_selectchannel(struct bcm430x_private *bcm,
				u8 channel)
{
        switch (bcm->current_core->phy->type) {
        case BCM430x_PHYTYPE_A:
		if (!(bcm->current_core->radio->id == 0x1206017F))
			return -ENODEV;
		if (channel > 200)
			return -EINVAL;

		TODO(); //FIXME: 1. Workaround here doesn't make sense (Specs)
		bcm430x_write16(bcm, BCM430x_MMIO_CHANNEL, 5000 + 5 * channel);
		bcm430x_phy_write(bcm, 0x0008, 0x0000);
		TODO(); //FIXME: 4.-13.
		bcm430x_radio_write16(bcm, 0x0029,
		                      (bcm430x_radio_read16(bcm, 0x0029) & 0xFF0F) | 0x000B);
		bcm430x_radio_write16(bcm, 0x0035, 0x00AA);
		bcm430x_radio_write16(bcm, 0x0036, 0x0085);
		TODO(); //FIXME: 17.
		bcm430x_radio_write16(bcm, 0x003D,
		                      bcm430x_radio_read16(bcm, 0x003D) & 0x00FF);
		bcm430x_radio_write16(bcm, 0x0081,
		                      (bcm430x_radio_read16(bcm, 0x0081) & 0xFF7F) | 0x0080);
		bcm430x_radio_write16(bcm, 0x0035,
		                      bcm430x_radio_read16(bcm, 0x0035) & 0xFFEF);
		bcm430x_radio_write16(bcm, 0x0035,
		                      (bcm430x_radio_read16(bcm, 0x0035) & 0xFFEF) | 0x0010 );
		TODO(); //FIXME: 22.-25.
		bcm430x_phy_xmitpower(bcm);

		break;
        case BCM430x_PHYTYPE_B:
        case BCM430x_PHYTYPE_G:
		if ((channel < 1) || (channel > 14))
			return -EINVAL;
                bcm430x_write16(bcm, BCM430x_MMIO_CHANNEL,
		                frequencies_bg[channel - 1]);
                break;
        default:
		assert(0);
        }

	bcm->current_core->radio->channel = channel;
	
	//XXX: Using the longer of 2 timeouts (8000 vs 2000 usecs). Specs states
	//     that 2000 usecs might suffice.
	udelay(8000);

	return 0;
}

void bcm430x_radio_set_txantenna(struct bcm430x_private *bcm, u32 val)
{
	u32 tmp;

	tmp = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0022) & 0xFFFFFCFF;
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0022, tmp | val);
	tmp = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x03a8) & 0xFFFFFCFF;
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x03a8, tmp | val);
	tmp = bcm430x_shm_read32(bcm, BCM430x_SHM_SHARED, 0x0054) & 0xFFFFFCFF;
	bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0054, tmp | val);
}

/* http://bcm-specs.sipsolutions.net/TX_Gain_Base_Band */
static u16 bcm430x_get_txgain_base_band(u16 txpower)
{
	u16 ret;

	assert(txpower <= 63);

	if (txpower >= 54)
		ret = 2;
	else if (txpower >= 49)
		ret = 4;
	else if (txpower >= 44)
		ret = 5;
	else
		ret = 6;

	return ret;
}

/* http://bcm-specs.sipsolutions.net/TX_Gain_Radio_Frequency_Power_Amplifier */
static u16 bcm430x_get_txgain_freq_power_amp(u16 txpower)
{
	u16 ret;

	assert(txpower <= 63);

	if (txpower >= 32)
		ret = 0;
	else if (txpower >= 25)
		ret = 1;
	else if (txpower >= 20)
		ret = 2;
	else if (txpower >= 12)
		ret = 3;
	else
		ret = 4;

	return ret;
}

/* http://bcm-specs.sipsolutions.net/TX_Gain_Digital_Analog_Converter */
static u16 bcm430x_get_txgain_dac(u16 txpower)
{
	u16 ret;

	assert(txpower <= 63);

	if (txpower >= 54)
		ret = txpower - 53;
	else if (txpower >= 49)
		ret = txpower - 42;
	else if (txpower >= 44)
		ret = txpower - 37;
	else if (txpower >= 32)
		ret = txpower - 32;
	else if (txpower >= 25)
		ret = txpower - 20;
	else if (txpower >= 20)
		ret = txpower - 13;
	else if (txpower >= 12)
		ret = txpower - 8;
	else
		ret = txpower;

	return ret;
}

void bcm430x_radio_set_txpower_a(struct bcm430x_private *bcm, u16 txpower)
{
	u16 pamp, base, dac, ilt;

	txpower = limit_value(txpower, 0, 63);

	pamp = bcm430x_get_txgain_freq_power_amp(txpower);
	pamp <<= 5;
	pamp &= 0x00E0;
	bcm430x_phy_write(bcm, 0x0019, pamp);

	base = bcm430x_get_txgain_base_band(txpower);
	base &= 0x000F;
	bcm430x_phy_write(bcm, 0x0017, base | 0x0020);

	ilt = bcm430x_ilt_read16(bcm, 0x3001);
	ilt &= 0x0007;

	dac = bcm430x_get_txgain_dac(txpower);
	dac <<= 3;
	dac |= ilt;

	bcm430x_ilt_write16(bcm, 0x3001, dac);

	bcm->current_core->radio->txpower[0] = txpower;

	TODO();
	//TODO: FuncPlaceholder (Adjust BB loft cancel)
}

void bcm430x_radio_set_txpower_bg(struct bcm430x_private *bcm,
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
	bcm430x_radio_write16(bcm, 0x0043, attenuation);
	bcm430x_shm_write16(bcm, BCM430x_SHM_SHARED, 0x0064, attenuation);
	if ((bcm->current_core->radio->id & BCM430x_RADIO_ID_VERSIONMASK) == 0x02050000) {
		bcm430x_radio_write16(bcm, 0x0052,
		                      (bcm430x_radio_read16(bcm, 0x0052) & 0xFF8F) | txpower);
	}

	if (bcm->current_core->phy->type == BCM430x_PHYTYPE_G)
		bcm430x_phy_lo_adjust(bcm);
}


int bcm430x_radio_turn_on(struct bcm430x_private *bcm)
{
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
		assert(0);
	}
	dprintk(KERN_INFO PFX "Radio turned on\n");

	bcm->current_core->radio->enabled = 1;

	return 0;
}
	
int bcm430x_radio_turn_off(struct bcm430x_private *bcm)
{
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
		assert(0);
	}
	dprintk(KERN_INFO PFX "Radio turned off\n");

	bcm->current_core->radio->enabled = 0;

	return 0;
}

void bcm430x_radio_clear_tssi(struct bcm430x_private *bcm)
{
	switch (bcm->current_core->phy->type) {
	case BCM430x_PHYTYPE_A:
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0068, 0x7F7F);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x006a, 0x7F7F);
		break;
	case BCM430x_PHYTYPE_B:
	case BCM430x_PHYTYPE_G:
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0058, 0x7F7F);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x005a, 0x7F7F);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0070, 0x7F7F);
		bcm430x_shm_write32(bcm, BCM430x_SHM_SHARED, 0x0072, 0x7F7F);
		break;
	}
}
