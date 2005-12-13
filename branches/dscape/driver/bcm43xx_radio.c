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

#include "bcm43xx.h"
#include "bcm43xx_main.h"
#include "bcm43xx_phy.h"
#include "bcm43xx_radio.h"
#include "bcm43xx_ilt.h"


/* Table for bcm43xx_radio_calibrationvalue() */
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

static void bcm43xx_set_all_gains(struct bcm43xx_private *bcm,
				  s16 first, s16 second, s16 third);
static void bcm43xx_set_original_gains(struct bcm43xx_private *bcm);

void bcm43xx_radio_lock(struct bcm43xx_private *bcm)
{
	u32 status;

	status = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	status |= BCM43xx_SBF_RADIOREG_LOCK;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, status);
	udelay(10);
}

void bcm43xx_radio_unlock(struct bcm43xx_private *bcm)
{
	u32 status;

	bcm43xx_read16(bcm, BCM43xx_MMIO_PHY_VER); /* dummy read */
	status = bcm43xx_read32(bcm, BCM43xx_MMIO_STATUS_BITFIELD);
	status &= ~BCM43xx_SBF_RADIOREG_LOCK;
	bcm43xx_write32(bcm, BCM43xx_MMIO_STATUS_BITFIELD, status);
}

u16 bcm43xx_radio_read16(struct bcm43xx_private *bcm, u16 offset)
{
	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		offset |= 0x0040;
		break;
	case BCM43xx_PHYTYPE_B:
		if (bcm->current_core->radio->version == 0x2053) {
			if (offset < 0x70)
				offset += 0x80;
			else if (offset < 0x80)
				offset += 0x70;
		} else if (bcm->current_core->radio->version == 0x2050)
			offset |= 0x80;
		break;
	case BCM43xx_PHYTYPE_G:
		offset |= 0x80;
		break;
	}

	bcm43xx_write16(bcm, BCM43xx_MMIO_RADIO_CONTROL, offset);
	return bcm43xx_read16(bcm, BCM43xx_MMIO_RADIO_DATA_LOW);
}

void bcm43xx_radio_write16(struct bcm43xx_private *bcm, u16 offset, u16 val)
{
	bcm43xx_write16(bcm, BCM43xx_MMIO_RADIO_CONTROL, offset);
	bcm43xx_write16(bcm, BCM43xx_MMIO_RADIO_DATA_LOW, val);
}

u8 bcm43xx_radio_aci_detect(struct bcm43xx_private *bcm, u8 channel)
{
	u8 ret = 0;
	u16 saved = bcm43xx_phy_read(bcm, 0x0403), rssi, temp;
	int i, j = 0;
	bcm43xx_radio_selectchannel(bcm, channel, 0);
	bcm43xx_phy_write(bcm, 0x0403, (saved & 0xFFF8) | 5);
	if (bcm->current_core->radio->aci_hw_rssi)
		rssi = bcm43xx_phy_read(bcm, 0x048A) & 0x3F;
	else
		rssi = saved & 0x3F;
	/* clamp temp to signed 5bit */
	if (rssi > 32)
		rssi -= 64;
	for (i = 0;i < 100; i++) {
		temp = (bcm43xx_phy_read(bcm, 0x047F) >> 8) & 0x3F;
		if (temp > 32)
			temp -= 64;
		if (temp < rssi)
			j++;
		if (j >= 20)
			ret = 1;
	}
	bcm43xx_phy_write(bcm, 0x0403, saved);

	return ret;
}

u8 bcm43xx_radio_aci_scan(struct bcm43xx_private *bcm)
{
	u8 ret[13];
	unsigned int channel = bcm->current_core->radio->channel;
	unsigned int i, j, start, end;

	if (!((bcm->current_core->phy->type == BCM43xx_PHYTYPE_G) && (bcm->current_core->phy->rev > 0)))
		return 0;

	bcm43xx_phy_lock(bcm);
	bcm43xx_radio_lock(bcm);
	bcm43xx_phy_write(bcm, 0x0802,
	                  bcm43xx_phy_read(bcm, 0x0802) & 0xFFFC);
	bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
	                  bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0x7FFF);
	bcm43xx_set_all_gains(bcm, 3, 8, 1);

	start = (channel - 5 > 0) ? channel - 5 : 1;
	end = (channel + 5 < 14) ? channel + 5 : 13;

	for (i = start; i <= end; i++) {
		if (abs(channel - i) > 2)
			ret[i-1] = bcm43xx_radio_aci_detect(bcm, i);
	}
	bcm43xx_radio_selectchannel(bcm, channel, 0);
	bcm43xx_phy_write(bcm, 0x0802,
	                  (bcm43xx_phy_read(bcm, 0x0802) & 0xFFFC) | 0x0003);
	bcm43xx_phy_write(bcm, 0x0403,
	                  bcm43xx_phy_read(bcm, 0x0403) & 0xFFF8);
	bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
	                  bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) | 0x8000);
	bcm43xx_set_original_gains(bcm);
	for (i = 0; i < 13; i++) {
		if (!ret[i])
			continue;
		end = (i + 5 < 13) ? i + 5 : 13;
		for (j = i; j < end; j++)
			ret[j] = 1;
	}
	bcm43xx_radio_unlock(bcm);
	bcm43xx_phy_unlock(bcm);
	return ret[channel - 1];
}

static void bcm43xx_set_all_gains(struct bcm43xx_private *bcm,
				  s16 first, s16 second, s16 third)
{
	u16 i;
	u16 start = 0x08, end = 0x18;
	u16 offset = 0x0400;
	u16 tmp;

	if (bcm->current_core->phy->rev <= 1) {
		offset = 0x5000;
		start = 0x10;
		end = 0x20;
	}

	for (i = 0; i < 4; i++)
		bcm43xx_ilt_write16(bcm, offset + i, first);

	for (i = start; i < end; i++)
		bcm43xx_ilt_write16(bcm, offset + i, second);

	if (third != -1) {
		tmp = ((u16)third << 14) | ((u16)third << 6);
		bcm43xx_phy_write(bcm, 0x04A0,
		                  (bcm43xx_phy_read(bcm, 0x04A0) & 0xBFBF) | tmp);
		bcm43xx_phy_write(bcm, 0x04A1,
		                  (bcm43xx_phy_read(bcm, 0x04A1) & 0xBFBF) | tmp);
		bcm43xx_phy_write(bcm, 0x04A2,
		                  (bcm43xx_phy_read(bcm, 0x04A2) & 0xBFBF) | tmp);
	}
	bcm43xx_dummy_transmission(bcm);
}

static void bcm43xx_set_original_gains(struct bcm43xx_private *bcm)
{
	u16 i, tmp;
	u16 offset = 0x0400;
	u16 start = 0x0008, end = 0x0018;

	if (bcm->current_core->phy->rev <= 1) {
		offset = 0x5000;
		start = 0x0010;
		end = 0x0020;
	}

	for (i = 0; i < 4; i++) {
		tmp = (i & 0xFFFC);
		tmp |= (i & 0x0001) << 1;
		tmp |= (i & 0x0002) >> 1;

		bcm43xx_ilt_write16(bcm, offset + i, tmp);
	}

	for (i = start; i < end; i++)
		bcm43xx_ilt_write16(bcm, offset + i, i - start);

	bcm43xx_phy_write(bcm, 0x04A0,
	                  (bcm43xx_phy_read(bcm, 0x04A0) & 0xBFBF) | 0x4040);
	bcm43xx_phy_write(bcm, 0x04A1,
	                  (bcm43xx_phy_read(bcm, 0x04A1) & 0xBFBF) | 0x4040);
	bcm43xx_phy_write(bcm, 0x04A2,
	                  (bcm43xx_phy_read(bcm, 0x04A2) & 0xBFBF) | 0x4000);
	bcm43xx_dummy_transmission(bcm);
}

/* http://bcm-specs.sipsolutions.net/NRSSILookupTable */
void bcm43xx_nrssi_hw_write(struct bcm43xx_private *bcm, u16 offset, s16 val)
{
	bcm43xx_phy_write(bcm, BCM43xx_PHY_NRSSILT_CTRL, offset);
	bcm43xx_phy_write(bcm, BCM43xx_PHY_NRSSILT_DATA, (u16)val);
}

/* http://bcm-specs.sipsolutions.net/NRSSILookupTable */
s16 bcm43xx_nrssi_hw_read(struct bcm43xx_private *bcm, u16 offset)
{
	u16 val;

	bcm43xx_phy_write(bcm, BCM43xx_PHY_NRSSILT_CTRL, offset);
	val = bcm43xx_phy_read(bcm, BCM43xx_PHY_NRSSILT_DATA);

	return (s16)val;
}

/* http://bcm-specs.sipsolutions.net/NRSSILookupTable */
void bcm43xx_nrssi_hw_update(struct bcm43xx_private *bcm, u16 val)
{
	u16 i;
	s16 tmp;

	for (i = 0; i < 64; i++) {
		tmp = bcm43xx_nrssi_hw_read(bcm, i);
		tmp -= val;
		tmp = limit_value(tmp, -32, 31);
		bcm43xx_nrssi_hw_write(bcm, i, tmp);
	}
}

/* http://bcm-specs.sipsolutions.net/NRSSILookupTable */
void bcm43xx_nrssi_mem_update(struct bcm43xx_private *bcm)
{
	s16 i, delta;
	s32 tmp;

	delta = 0x1F - bcm->current_core->radio->nrssi[0];
	for (i = 0; i < 64; i++) {
		tmp = (i - delta) * bcm->current_core->radio->nrssislope;
		tmp /= 0x10000;
		tmp += 0x3A;
		tmp = limit_value(tmp, 0, 0x3F);
		bcm->current_core->radio->nrssi_lt[i] = tmp;
	}
}

void bcm43xx_calc_nrssi_slope(struct bcm43xx_private *bcm)
{
	/*FIXME: We are not completely sure, if the nrssi values are really s16.
	 *       We have to check by testing, if the values and the u16 to s16 casts are correct.
	 */

	u16 backup[14];
	s16 slope = 0;
	s16 nrssi0;
	s16 nrssi1;

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_B:
		backup[0] = bcm43xx_radio_read16(bcm, 0x007A);
		backup[1] = bcm43xx_radio_read16(bcm, 0x0052);
		backup[2] = bcm43xx_radio_read16(bcm, 0x0043);
		backup[3] = bcm43xx_phy_read(bcm, 0x0030);
		backup[4] = bcm43xx_phy_read(bcm, 0x0026);
		backup[5] = bcm43xx_phy_read(bcm, 0x0015);
		backup[6] = bcm43xx_phy_read(bcm, 0x002A);
		backup[7] = bcm43xx_phy_read(bcm, 0x0020);
		backup[8] = bcm43xx_phy_read(bcm, 0x005A);
		backup[9] = bcm43xx_phy_read(bcm, 0x0059);
		backup[10] = bcm43xx_phy_read(bcm, 0x0058);
		backup[11] = bcm43xx_read16(bcm, 0x03E2);
		backup[12] = bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT);
		backup[13] = bcm43xx_read16(bcm, 0x03E6);

		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) & 0x000F);
		bcm43xx_phy_write(bcm, 0x0030, 0x00FF);
		bcm43xx_write16(bcm, 0x03EC, 0x3F3F);
		bcm43xx_phy_write(bcm, 0x0026, 0x0000);
		bcm43xx_phy_write(bcm, 0x0015,
				  bcm43xx_phy_read(bcm, 0x0015) | 0x0020);
		bcm43xx_phy_write(bcm, 0x002A, 0x08A3);
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) | 0x0080);

		nrssi0 = (s16)bcm43xx_phy_read(bcm, 0x0027);
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) & 0x007F);
		if (bcm->current_core->phy->rev == 0) {
			bcm43xx_write16(bcm, 0x03E6, 0x0122);
		} else {
			bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT,
					bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT) & 0x2000);
		}
		bcm43xx_phy_write(bcm, 0x0020, 0x3F3F);
		bcm43xx_phy_write(bcm, 0x0015, 0xF330);
		bcm43xx_radio_write16(bcm, 0x0052, 0x0060);
		bcm43xx_radio_write16(bcm, 0x0043, 0x0000);
		bcm43xx_phy_write(bcm, 0x005A, 0x0480);
		bcm43xx_phy_write(bcm, 0x0059, 0x0810);
		bcm43xx_phy_write(bcm, 0x0058, 0x000D);
		udelay(20);

		nrssi1 = (s16)bcm43xx_phy_read(bcm, 0x0027);
		bcm43xx_phy_write(bcm, 0x0030, backup[3]);
		bcm43xx_radio_write16(bcm, 0x007A, backup[0]);
		bcm43xx_write16(bcm, 0x03E6, 0x0122);
		bcm43xx_write16(bcm, 0x03E2, backup[11]);
		bcm43xx_phy_write(bcm, 0x0026, backup[4]);
		bcm43xx_phy_write(bcm, 0x0015, backup[5]);
		bcm43xx_phy_write(bcm, 0x002A, backup[6]);
		if (bcm->current_core->phy->rev == 0)
			bcm43xx_write16(bcm, 0x03E6, backup[13]);
		else
			bcm43xx_write16(bcm, 0x03F4, backup[12]);
		bcm43xx_phy_write(bcm, 0x0020, backup[7]);
		bcm43xx_phy_write(bcm, 0x005A, backup[8]);
		bcm43xx_phy_write(bcm, 0x0059, backup[9]);
		bcm43xx_phy_write(bcm, 0x0058, backup[10]);
		bcm43xx_radio_write16(bcm, 0x0052, backup[1]);
		bcm43xx_radio_write16(bcm, 0x0043, backup[2]);

		slope = nrssi0 - nrssi1;
		if (slope == 0)
			slope = 64;
		bcm->current_core->radio->nrssislope = 0x400000 / slope;

		if (nrssi0 <= -4) {
			bcm->current_core->radio->nrssi[0] = nrssi0;
			bcm->current_core->radio->nrssi[1] = nrssi1;
		}
		break;
	case BCM43xx_PHYTYPE_G:
		backup[0] = bcm43xx_radio_read16(bcm, 0x007A);
		backup[1] = bcm43xx_radio_read16(bcm, 0x0052);
		backup[2] = bcm43xx_radio_read16(bcm, 0x0043);
		backup[3] = bcm43xx_phy_read(bcm, 0x0015);
		backup[4] = bcm43xx_phy_read(bcm, 0x005A);
		backup[5] = bcm43xx_phy_read(bcm, 0x0059);
		backup[6] = bcm43xx_phy_read(bcm, 0x0058);
		backup[7] = bcm43xx_read16(bcm, 0x03E2);
		backup[8] = bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT);
		backup[9] = bcm43xx_read16(bcm, 0x03E6);

		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) | 0x0070);
		bcm43xx_phy_write(bcm, 0x0802,
				  bcm43xx_phy_read(bcm, 0x0802) & ~0x8000);
		bcm43xx_phy_write(bcm, 0x0802,
				  bcm43xx_phy_read(bcm, 0x0802) & ~(0x0001 | 0x0002));
		bcm43xx_phy_write(bcm, 0x03E2, 0x8000);

		bcm43xx_set_all_gains(bcm, 0, 8, 0);
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) & 0x00F7);
		if (bcm->current_core->phy->rev >= 2) {
			bcm43xx_phy_write(bcm, 0x0812,
					  (bcm43xx_phy_read(bcm, 0x0812) & 0xFFCF) | 0x0010);
			bcm43xx_phy_write(bcm, 0x0811,
					  (bcm43xx_phy_read(bcm, 0x0811) & 0xFFCF) | 0x0010);
		}
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) | 0x0080);
		udelay(20);

		nrssi0 = (s16)((bcm43xx_phy_read(bcm, 0x047F) >> 8) & 0x003F);
		if (nrssi0 >= 0x0020)
			nrssi0 -= 0x0040;

		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) & 0x007F);
		if (bcm->current_core->phy->rev >= 2)
			bcm43xx_write16(bcm, 0x03E6, 0x0040);
		bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, 0x2000);
		bcm43xx_radio_write16(bcm, 0x007A,
				      bcm43xx_radio_read16(bcm, 0x007A) | 0x000F);
		bcm43xx_phy_write(bcm, 0x0015, 0xF330);
		if (bcm->current_core->phy->rev >= 2) {
			bcm43xx_phy_write(bcm, 0x0812,
					  (bcm43xx_phy_read(bcm, 0x0812) & 0xFFCF) | 0x0020);
			bcm43xx_phy_write(bcm, 0x0811,
					  (bcm43xx_phy_read(bcm, 0x0811) & 0xFFCF) | 0x0020);
		}

		bcm43xx_set_all_gains(bcm, 3, 0, 1);
		bcm43xx_radio_write16(bcm, 0x0052, 0x0060);
		bcm43xx_radio_write16(bcm, 0x0043, 0x0000);
		bcm43xx_phy_write(bcm, 0x005A, 0x0480);
		bcm43xx_phy_write(bcm, 0x0059, 0x0810);
		bcm43xx_phy_write(bcm, 0x0058, 0x000D);
		udelay(20);

		nrssi1 = (s16)((bcm43xx_phy_read(bcm, 0x047F) >> 8) & 0x003F);
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
			bcm43xx_phy_write(bcm, 0x0812,
					  bcm43xx_phy_read(bcm, 0x0812) & 0xFFCF);
			bcm43xx_phy_write(bcm, 0x0811,
					  bcm43xx_phy_read(bcm, 0x0811) & 0xFFCF);
		}

		bcm43xx_radio_write16(bcm, 0x007A, backup[0]);
		bcm43xx_radio_write16(bcm, 0x0052, backup[1]);
		bcm43xx_radio_write16(bcm, 0x0043, backup[2]);
		bcm43xx_write16(bcm, 0x03E2, backup[7]);
		bcm43xx_write16(bcm, 0x03E6, backup[9]);
		bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, backup[8]);
		bcm43xx_phy_write(bcm, 0x0015, backup[3]);
		bcm43xx_phy_write(bcm, 0x005A, backup[4]);
		bcm43xx_phy_write(bcm, 0x0059, backup[5]);
		bcm43xx_phy_write(bcm, 0x0058, backup[6]);
		bcm43xx_phy_write(bcm, 0x0802,
				  bcm43xx_phy_read(bcm, 0x0802) | (0x0001 | 0x0002));

		bcm43xx_set_original_gains(bcm);

		bcm43xx_phy_write(bcm, 0x0802,
				  bcm43xx_phy_read(bcm, 0x0802) | 0x8000);

		bcm43xx_nrssi_mem_update(bcm);
		bcm43xx_calc_nrssi_threshold(bcm);
		break;
	default:
		assert(0);
	}
}

void bcm43xx_calc_nrssi_threshold(struct bcm43xx_private *bcm)
{
	s16 threshold;
	s32 a, b;
	int tmp;
	s16 tmp16;
	u16 tmp_u16;

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_B:
		if (bcm->current_core->phy->rev < 2)
			return;
		if (bcm->current_core->radio->version != 0x2050)
			return;
		if (!(bcm->sprom.boardflags & BCM43xx_BFL_RSSI))
			return;

		if (bcm->current_core->radio->_id == 0x3205017F
		    || bcm->current_core->radio->_id == 0x4205017F
		    || bcm->current_core->radio->_id == 0x52050175) {
			threshold = bcm->current_core->radio->nrssi[1] - 5;
		} else {
			threshold = 40 * bcm->current_core->radio->nrssi[0];
			threshold += 33 * (bcm->current_core->radio->nrssi[1]
					   - bcm->current_core->radio->nrssi[0]);
			threshold += 20;
			threshold /= 10;
		}
		threshold = limit_value(threshold, 0, 0x3E);
		bcm43xx_phy_read(bcm, 0x0020); /* dummy read */
		bcm43xx_phy_write(bcm, 0x0020, (((u16)threshold) << 8) | 0x001C);

		if (bcm->current_core->radio->_id == 0x3205017F
		    || bcm->current_core->radio->_id == 0x4205017F
		    || bcm->current_core->radio->_id == 0x52050175) {
			bcm43xx_phy_write(bcm, 0x0087, 0x0E0D);
			bcm43xx_phy_write(bcm, 0x0086, 0x0C0B);
			bcm43xx_phy_write(bcm, 0x0085, 0x0A09);
			bcm43xx_phy_write(bcm, 0x0084, 0x0808);
			bcm43xx_phy_write(bcm, 0x0083, 0x0808);
			bcm43xx_phy_write(bcm, 0x0082, 0x0604);
			bcm43xx_phy_write(bcm, 0x0081, 0x0302);
			bcm43xx_phy_write(bcm, 0x0080, 0x0100);
		}
		break;
	case BCM43xx_PHYTYPE_G:
		if (!bcm->current_core->phy->connected ||
		    !(bcm->sprom.boardflags & BCM43xx_BFL_RSSI)) {
			tmp16 = bcm43xx_nrssi_hw_read(bcm, 0x20);
			if (tmp16 >= 0x20)
				tmp16 -= 0x40;
			if (tmp16 < 3) {
				bcm43xx_phy_write(bcm, 0x048A,
						  (bcm43xx_phy_read(bcm, 0x048A)
						   & 0xF000) | 0x09EB);
			} else {
				bcm43xx_phy_write(bcm, 0x048A,
						  (bcm43xx_phy_read(bcm, 0x048A)
						   & 0xF000) | 0x0AED);
			}
		} else {
			tmp = bcm->current_core->radio->interfmode;
			if (tmp == BCM43xx_RADIO_INTERFMODE_NONWLAN) {
				a = -13;
				b = -17;
			} else if (tmp == BCM43xx_RADIO_INTERFMODE_NONE &&
				   !bcm->current_core->radio->aci_enable) {
				a = -13;
				b = -10;
			} else {
				a = -8;
				b = -9;
			}
			a += 0x1B;
			a *= bcm->current_core->radio->nrssi[1] - bcm->current_core->radio->nrssi[0];
			a += bcm->current_core->radio->nrssi[0] * 0x40;
			a /= 64;
			b += 0x1B;
			b *= bcm->current_core->radio->nrssi[1] - bcm->current_core->radio->nrssi[0];
			b += bcm->current_core->radio->nrssi[0] * 0x40;
			b /= 64;

			a = limit_value(a, -31, 31);
			b = limit_value(b, -31, 31);

			tmp_u16 = bcm43xx_phy_read(bcm, 0x048A) & 0xF000;
			tmp_u16 |= ((u32)a & 0x003F);
			tmp_u16 |= (((u32)b & 0x003F) << 6);
			bcm43xx_phy_write(bcm, 0x048A, tmp_u16);
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
bcm43xx_radio_interference_mitigation_enable(struct bcm43xx_private *bcm,
					     int mode)
{
	int i = 0;
	u16 *stack = bcm->current_core->radio->interfstack;
	u16 tmp;

	switch (mode) {
	case BCM43xx_RADIO_INTERFMODE_NONWLAN:
		if (bcm->current_core->phy->rev != 1) {
			bcm43xx_phy_write(bcm, 0x042B,
			                  bcm43xx_phy_read(bcm, 0x042B) & 0x0800);
			bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
			                  bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & ~0x4000);
			break;
		}
		tmp = ((bcm43xx_phy_read(bcm, 0x0078) & 0x001E) >> 1);
		tmp = flip_4bit(tmp);
		if (tmp >= 4)
			tmp = flip_4bit(tmp - 3);
		bcm43xx_phy_write(bcm, 0x0078, tmp << 1);

		bcm43xx_calc_nrssi_threshold(bcm);

		if (bcm->current_core->rev < 5) {
			stack_save(bcm43xx_phy_read(bcm, 0x0406));
			bcm43xx_phy_write(bcm, 0x0406, 0x7E28);
		} else {
			stack_save(bcm43xx_phy_read(bcm, 0x04C0));
			stack_save(bcm43xx_phy_read(bcm, 0x04C1));
			bcm43xx_phy_write(bcm, 0x04C0, 0x3E04);
			bcm43xx_phy_write(bcm, 0x04C1, 0x0640);
		}

		bcm43xx_phy_write(bcm, 0x042B,
		                  bcm43xx_phy_read(bcm, 0x042B) | 0x0800);
		bcm43xx_phy_write(bcm, BCM43xx_PHY_RADIO_BITFIELD,
		                  bcm43xx_phy_read(bcm, BCM43xx_PHY_RADIO_BITFIELD) | 0x1000);

		stack_save(bcm43xx_phy_read(bcm, 0x04A0));
		bcm43xx_phy_write(bcm, 0x04A0,
		                  (bcm43xx_phy_read(bcm, 0x04A0) & 0xC0C0) | 0x0008);
		stack_save(bcm43xx_phy_read(bcm, 0x04A1));
		bcm43xx_phy_write(bcm, 0x04A1,
				  (bcm43xx_phy_read(bcm, 0x04A1) & 0xC0C0) | 0x0605);
		stack_save(bcm43xx_phy_read(bcm, 0x04A2));
		bcm43xx_phy_write(bcm, 0x04A2,
				  (bcm43xx_phy_read(bcm, 0x04A2) & 0xC0C0) | 0x0204);
		stack_save(bcm43xx_phy_read(bcm, 0x04A8));
		bcm43xx_phy_write(bcm, 0x04A8,
				  (bcm43xx_phy_read(bcm, 0x04A8) & 0xC0C0) | 0x0403);
		stack_save(bcm43xx_phy_read(bcm, 0x04AB));
		bcm43xx_phy_write(bcm, 0x04AB,
				  (bcm43xx_phy_read(bcm, 0x04AB) & 0xC0C0) | 0x0504);

		stack_save(bcm43xx_phy_read(bcm, 0x04A7));
		bcm43xx_phy_write(bcm, 0x04A7, 0x0002);
		stack_save(bcm43xx_phy_read(bcm, 0x04A3));
		bcm43xx_phy_write(bcm, 0x04A3, 0x287A);
		stack_save(bcm43xx_phy_read(bcm, 0x04A9));
		bcm43xx_phy_write(bcm, 0x04A9, 0x2027);
		stack_save(bcm43xx_phy_read(bcm, 0x0493));
		bcm43xx_phy_write(bcm, 0x0493, 0x32F5);
		stack_save(bcm43xx_phy_read(bcm, 0x04AA));
		bcm43xx_phy_write(bcm, 0x04AA, 0x2027);
		stack_save(bcm43xx_phy_read(bcm, 0x04AC));
		bcm43xx_phy_write(bcm, 0x04AC, 0x32F5);
		break;
	case BCM43xx_RADIO_INTERFMODE_MANUALWLAN:
		if (bcm43xx_phy_read(bcm, 0x0033) == 0x0800)
			break;

		bcm->current_core->radio->aci_enable = 1;

		stack_save(bcm43xx_phy_read(bcm, BCM43xx_PHY_RADIO_BITFIELD));
		stack_save(bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS));
		if (bcm->current_core->rev < 5) {
			stack_save(bcm43xx_phy_read(bcm, 0x0406));
		} else {
			stack_save(bcm43xx_phy_read(bcm, 0x04C0));
			stack_save(bcm43xx_phy_read(bcm, 0x04C1));
		}
		stack_save(bcm43xx_phy_read(bcm, 0x0033));
		stack_save(bcm43xx_phy_read(bcm, 0x04A7));
		stack_save(bcm43xx_phy_read(bcm, 0x04A3));
		stack_save(bcm43xx_phy_read(bcm, 0x04A9));
		stack_save(bcm43xx_phy_read(bcm, 0x04AA));
		stack_save(bcm43xx_phy_read(bcm, 0x04AC));
		stack_save(bcm43xx_phy_read(bcm, 0x0493));
		stack_save(bcm43xx_phy_read(bcm, 0x04A1));
		stack_save(bcm43xx_phy_read(bcm, 0x04A0));
		stack_save(bcm43xx_phy_read(bcm, 0x04A2));
		stack_save(bcm43xx_phy_read(bcm, 0x048A));
		stack_save(bcm43xx_phy_read(bcm, 0x04A8));
		stack_save(bcm43xx_phy_read(bcm, 0x04AB));

		bcm43xx_phy_write(bcm, BCM43xx_PHY_RADIO_BITFIELD,
				  bcm43xx_phy_read(bcm, BCM43xx_PHY_RADIO_BITFIELD) & 0xEFFF);
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
				  (bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0xEFFF) | 0x0002);

		bcm43xx_phy_write(bcm, 0x04A7, 0x0800);
		bcm43xx_phy_write(bcm, 0x04A3, 0x287A);
		bcm43xx_phy_write(bcm, 0x04A9, 0x2027);
		bcm43xx_phy_write(bcm, 0x0493, 0x32F5);
		bcm43xx_phy_write(bcm, 0x04AA, 0x2027);
		bcm43xx_phy_write(bcm, 0x04AC, 0x32F5);

		bcm43xx_phy_write(bcm, 0x04A0,
				  (bcm43xx_phy_read(bcm, 0x04A0) & 0xFFC0) | 0x001A);
		if (bcm->current_core->rev < 5) {
			bcm43xx_phy_write(bcm, 0x0406, 0x280D);
		} else {
			bcm43xx_phy_write(bcm, 0x04C0, 0x0640);
			bcm43xx_phy_write(bcm, 0x04C1, 0x00A9);
		}

		bcm43xx_phy_write(bcm, 0x04A1,
		                  (bcm43xx_phy_read(bcm, 0x04A1) & 0xC0FF) | 0x1800);
		bcm43xx_phy_write(bcm, 0x04A1,
		                  (bcm43xx_phy_read(bcm, 0x04A1) & 0xFFC0) | 0x0016);
		bcm43xx_phy_write(bcm, 0x04A2,
		                  (bcm43xx_phy_read(bcm, 0x04A2) & 0xF0FF) | 0x0900);
		bcm43xx_phy_write(bcm, 0x04A0,
		                  (bcm43xx_phy_read(bcm, 0x04A0) & 0xF0FF) | 0x0700);
		bcm43xx_phy_write(bcm, 0x04A2,
		                  (bcm43xx_phy_read(bcm, 0x04A2) & 0xFFF0) | 0x000D);
		bcm43xx_phy_write(bcm, 0x04A8,
		                  (bcm43xx_phy_read(bcm, 0x04A8) & 0xCFFF) | 0x1000);
		bcm43xx_phy_write(bcm, 0x04A8,
		                  (bcm43xx_phy_read(bcm, 0x04A8) & 0xF0FF) | 0x0A00);
		bcm43xx_phy_write(bcm, 0x04AB,
		                  (bcm43xx_phy_read(bcm, 0x04AB) & 0xCFFF) | 0x1000);
		bcm43xx_phy_write(bcm, 0x04AB,
		                  (bcm43xx_phy_read(bcm, 0x04AB) & 0xF0FF) | 0x0800);
		bcm43xx_phy_write(bcm, 0x04AB,
		                  (bcm43xx_phy_read(bcm, 0x04AB) & 0xFFCF) | 0x0010);
		bcm43xx_phy_write(bcm, 0x04AB,
		                  (bcm43xx_phy_read(bcm, 0x04AB) & 0xFFF0) | 0x0006);

		bcm43xx_calc_nrssi_slope(bcm);
		break;
	default:
		assert(0);
	}
}

static void
bcm43xx_radio_interference_mitigation_disable(struct bcm43xx_private *bcm,
					      int mode)
{
	int i = 0;
	u16 *stack = bcm->current_core->radio->interfstack;
	u16 tmp;

	switch (mode) {
	case BCM43xx_RADIO_INTERFMODE_NONWLAN:
		if (bcm->current_core->phy->rev != 1) {
			bcm43xx_phy_write(bcm, 0x042B,
			                  bcm43xx_phy_read(bcm, 0x042B) & ~0x0800);
			bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
			                  bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0x4000);
			break;
		}
		tmp = ((bcm43xx_phy_read(bcm, 0x0078) & 0x001E) >> 1);
		tmp = flip_4bit(tmp);
		if (tmp >= 0x000C)
			tmp = flip_4bit(tmp + 3);
		bcm43xx_phy_write(bcm, 0x0078, tmp << 1);

		bcm43xx_calc_nrssi_threshold(bcm);

		if (bcm->current_core->rev < 5) {
			bcm43xx_phy_write(bcm, 0x0406, stack_restore());
		} else {
			bcm43xx_phy_write(bcm, 0x04C0, stack_restore());
			bcm43xx_phy_write(bcm, 0x04C1, stack_restore());
		}
		bcm43xx_phy_write(bcm, 0x042B,
				  bcm43xx_phy_read(bcm, 0x042B) & ~0x0800);

		if (!bcm->bad_frames_preempt)
			bcm43xx_phy_write(bcm, BCM43xx_PHY_RADIO_BITFIELD,
					  bcm43xx_phy_read(bcm, BCM43xx_PHY_RADIO_BITFIELD) & ~(1 << 11));
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
				  bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0x4000);
		bcm43xx_phy_write(bcm, 0x04A0, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A1, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A2, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A8, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AB, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A7, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A3, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A9, stack_restore());
		bcm43xx_phy_write(bcm, 0x0493, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AA, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AC, stack_restore());
		break;
	case BCM43xx_RADIO_INTERFMODE_MANUALWLAN:
		if (bcm43xx_phy_read(bcm, 0x0033) != 0x0800)
			break;

		bcm->current_core->radio->aci_enable = 0;

		bcm43xx_phy_write(bcm, BCM43xx_PHY_RADIO_BITFIELD, stack_restore());
		bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS, stack_restore());
		if (bcm->current_core->rev < 5) {
			bcm43xx_phy_write(bcm, 0x0406, stack_restore());
		} else {
			bcm43xx_phy_write(bcm, 0x04C0, stack_restore());
			bcm43xx_phy_write(bcm, 0x04C1, stack_restore());
		}
		bcm43xx_phy_write(bcm, 0x0033, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A7, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A3, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A9, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AA, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AC, stack_restore());
		bcm43xx_phy_write(bcm, 0x0493, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A1, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A0, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A2, stack_restore());
		bcm43xx_phy_write(bcm, 0x04A8, stack_restore());
		bcm43xx_phy_write(bcm, 0x04AB, stack_restore());

		bcm43xx_calc_nrssi_slope(bcm);
		break;
	default:
		assert(0);
	}
}

#undef stack_save
#undef stack_restore

int bcm43xx_radio_set_interference_mitigation(struct bcm43xx_private *bcm,
					      int mode)
{
	int currentmode;

	if (bcm->current_core->phy->type != BCM43xx_PHYTYPE_G)
		return -ENODEV;
	if (bcm->current_core->phy->rev == 0)
		return -ENODEV;
	if (!bcm->current_core->phy->connected)
		return -ENODEV;

	bcm->current_core->radio->aci_wlan_automatic = 0;
	switch (mode) {
	case BCM43xx_RADIO_INTERFMODE_AUTOWLAN:
		bcm->current_core->radio->aci_wlan_automatic = 1;
		if (bcm->current_core->radio->aci_enable)
			mode = BCM43xx_RADIO_INTERFMODE_MANUALWLAN;
		else
			mode = BCM43xx_RADIO_INTERFMODE_NONE;
		break;
	case BCM43xx_RADIO_INTERFMODE_NONE:
	case BCM43xx_RADIO_INTERFMODE_NONWLAN:
	case BCM43xx_RADIO_INTERFMODE_MANUALWLAN:
		break;
	default:
		return -EINVAL;
	}

	currentmode = bcm->current_core->radio->interfmode;
	if (currentmode == mode)
		return 0;
	if (currentmode != BCM43xx_RADIO_INTERFMODE_NONE)
		bcm43xx_radio_interference_mitigation_disable(bcm, currentmode);

	if (mode == BCM43xx_RADIO_INTERFMODE_NONE) {
		bcm->current_core->radio->aci_enable = 0;
		bcm->current_core->radio->aci_hw_rssi = 0;
	} else
		bcm43xx_radio_interference_mitigation_enable(bcm, mode);
	bcm->current_core->radio->interfmode = mode;

	return 0;
}

static u16 bcm43xx_radio_calibrationvalue(struct bcm43xx_private *bcm)
{
	u16 reg, index, ret;

	reg = bcm43xx_radio_read16(bcm, 0x0060);
	index = (reg & 0x001E) >> 1;
	ret = rcc_table[index] << 1;
	ret |= (reg & 0x0001);
	ret |= 0x0020;

	return ret;
}

u16 bcm43xx_radio_init2050(struct bcm43xx_private *bcm)
{
	u16 backup[19] = { 0 };
	u16 ret;
	u16 i, j;
	u32 tmp1 = 0, tmp2 = 0;

	backup[0] = bcm43xx_radio_read16(bcm, 0x0043);
	backup[14] = bcm43xx_radio_read16(bcm, 0x0051);
	backup[15] = bcm43xx_radio_read16(bcm, 0x0052);
	backup[1] = bcm43xx_phy_read(bcm, 0x0015);
	backup[16] = bcm43xx_phy_read(bcm, 0x005A);
	backup[17] = bcm43xx_phy_read(bcm, 0x0059);
	backup[18] = bcm43xx_phy_read(bcm, 0x0058);
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
		backup[2] = bcm43xx_phy_read(bcm, 0x0030);
		backup[3] = bcm43xx_read16(bcm, 0x03EC);
		bcm43xx_phy_write(bcm, 0x0030, 0x00FF);
		bcm43xx_write16(bcm, 0x03EC, 0x3F3F);
	} else {
		if (bcm->current_core->phy->connected) {
			backup[4] = bcm43xx_phy_read(bcm, 0x0811);
			backup[5] = bcm43xx_phy_read(bcm, 0x0812);
			backup[6] = bcm43xx_phy_read(bcm, 0x0814);
			backup[7] = bcm43xx_phy_read(bcm, 0x0815);
			backup[8] = bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS);
			backup[9] = bcm43xx_phy_read(bcm, 0x0802);
			bcm43xx_phy_write(bcm, 0x0814,
                                          (bcm43xx_phy_read(bcm, 0x0814) | 0x0003));
			bcm43xx_phy_write(bcm, 0x0815,
                                          (bcm43xx_phy_read(bcm, 0x0815) & 0xFFFC));	
			bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS,
			                  (bcm43xx_phy_read(bcm, BCM43xx_PHY_G_CRS) & 0x7FFF));
			bcm43xx_phy_write(bcm, 0x0802,
			                  (bcm43xx_phy_read(bcm, 0x0802) & 0xFFFC));
			bcm43xx_phy_write(bcm, 0x0811, 0x01B3);
			bcm43xx_phy_write(bcm, 0x0812, 0x0FB2);
		}
		bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_RADIO,
		                (bcm43xx_read16(bcm, BCM43xx_MMIO_PHY_RADIO) | 0x8000));
	}
	backup[10] = bcm43xx_phy_read(bcm, 0x0035);
	bcm43xx_phy_write(bcm, 0x0035,
	                  (bcm43xx_phy_read(bcm, 0x0035) & 0xFF7F));
	backup[11] = bcm43xx_read16(bcm, 0x03E6);
	backup[12] = bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT);

	// Initialization
	if (bcm->current_core->phy->version == 0) {
		bcm43xx_write16(bcm, 0x03E6, 0x0122);
	} else {
		if (bcm->current_core->phy->version >= 2)
			bcm43xx_write16(bcm, 0x03E6, 0x0040);
		bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT,
                                (bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT) | 0x2000));
	}

	ret = bcm43xx_radio_calibrationvalue(bcm);

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B)
		bcm43xx_radio_write16(bcm, 0x0078, 0x0003);

	bcm43xx_phy_write(bcm, 0x0015, 0xBFAF);
	bcm43xx_phy_write(bcm, 0x002B, 0x1403);
	if (bcm->current_core->phy->connected)
		bcm43xx_phy_write(bcm, 0x0812, 0x00B2);
	bcm43xx_phy_write(bcm, 0x0015, 0xBFA0);
	bcm43xx_radio_write16(bcm, 0x0051,
	                      (bcm43xx_radio_read16(bcm, 0x0051) | 0x0004));
	bcm43xx_radio_write16(bcm, 0x0052, 0x0000);
	bcm43xx_radio_write16(bcm, 0x0043,
			      bcm43xx_radio_read16(bcm, 0x0043) | 0x0009);
	bcm43xx_phy_write(bcm, 0x0058, 0x0000);

	for (i = 0; i < 16; i++) {
		bcm43xx_phy_write(bcm, 0x005A, 0x0480);
		bcm43xx_phy_write(bcm, 0x0059, 0xC810);
		bcm43xx_phy_write(bcm, 0x0058, 0x000D);
		if (bcm->current_core->phy->connected)
			bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
		bcm43xx_phy_write(bcm, 0x0015, 0xAFB0);
		udelay(10);
		if (bcm->current_core->phy->connected)
			bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
		bcm43xx_phy_write(bcm, 0x0015, 0xEFB0);
		udelay(10);
		if (bcm->current_core->phy->connected)
			bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
		bcm43xx_phy_write(bcm, 0x0015, 0xFFF0);
		udelay(10);
		tmp1 += bcm43xx_phy_read(bcm, 0x002D);
		bcm43xx_phy_write(bcm, 0x0058, 0x0000);
		if (bcm->current_core->phy->connected)
			bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
		bcm43xx_phy_write(bcm, 0x0015, 0xAFB0);
	}

	tmp1++;
	tmp1 >>= 9;
	udelay(10);
	bcm43xx_phy_write(bcm, 0x0058, 0x0000);

	for (i = 0; i < 16; i++) {
		bcm43xx_radio_write16(bcm, 0x0078, (flip_4bit(i) << 1) | 0x0020);
		backup[13] = bcm43xx_radio_read16(bcm, 0x0078);
		udelay(10);
		for (j = 0; j < 16; j++) {
			bcm43xx_phy_write(bcm, 0x005A, 0x0D80);
			bcm43xx_phy_write(bcm, 0x0059, 0xC810);
			bcm43xx_phy_write(bcm, 0x0058, 0x000D);
			if (bcm->current_core->phy->connected)
				bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
			bcm43xx_phy_write(bcm, 0x0015, 0xAFB0);
			udelay(10);
			if (bcm->current_core->phy->connected)
				bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
			bcm43xx_phy_write(bcm, 0x0015, 0xEFB0);
			udelay(10);
			if (bcm->current_core->phy->connected)
				bcm43xx_phy_write(bcm, 0x0812, 0x30B3); /* 0x30B3 is not a typo */
			bcm43xx_phy_write(bcm, 0x0015, 0xFFF0);
			udelay(10);
			tmp2 += bcm43xx_phy_read(bcm, 0x002D);
			bcm43xx_phy_write(bcm, 0x0058, 0x0000);
			if (bcm->current_core->phy->connected)
				bcm43xx_phy_write(bcm, 0x0812, 0x30B2);
			bcm43xx_phy_write(bcm, 0x0015, 0xAFB0);
		}
		tmp2++;
		tmp2 >>= 8;
		if (tmp1 < tmp2)
			break;
	}

	/* Restore the registers */
	bcm43xx_phy_write(bcm, 0x0015, backup[1]);
	bcm43xx_radio_write16(bcm, 0x0051, backup[14]);
	bcm43xx_radio_write16(bcm, 0x0052, backup[15]);
	bcm43xx_radio_write16(bcm, 0x0043, backup[0]);
	bcm43xx_phy_write(bcm, 0x005A, backup[16]);
	bcm43xx_phy_write(bcm, 0x0059, backup[17]);
	bcm43xx_phy_write(bcm, 0x0058, backup[18]);
	bcm43xx_write16(bcm, 0x03E6, backup[11]);
	if (bcm->current_core->phy->version != 0)
		bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT, backup[12]);
	bcm43xx_phy_write(bcm, 0x0035, backup[10]);
	bcm43xx_radio_selectchannel(bcm, bcm->current_core->radio->channel, 1);
	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_B) {
		bcm43xx_phy_write(bcm, 0x0030, backup[2]);
		bcm43xx_write16(bcm, 0x03EC, backup[3]);
	} else {
		bcm43xx_write16(bcm, BCM43xx_MMIO_PHY_RADIO,
				(bcm43xx_read16(bcm, BCM43xx_MMIO_PHY_RADIO) & 0x7FFF));
		if (bcm->current_core->phy->connected) {
			bcm43xx_phy_write(bcm, 0x0811, backup[4]);
			bcm43xx_phy_write(bcm, 0x0812, backup[5]);
			bcm43xx_phy_write(bcm, 0x0814, backup[6]);
			bcm43xx_phy_write(bcm, 0x0815, backup[7]);
			bcm43xx_phy_write(bcm, BCM43xx_PHY_G_CRS, backup[8]);
			bcm43xx_phy_write(bcm, 0x0802, backup[9]);
		}
	}
	if (i >= 15)
		ret = backup[13];

	return ret;
}

void bcm43xx_radio_init2060(struct bcm43xx_private *bcm)
{
	int err;

	bcm43xx_radio_write16(bcm, 0x0004, 0x00C0);
	bcm43xx_radio_write16(bcm, 0x0005, 0x0008);
	bcm43xx_radio_write16(bcm, 0x0009, 0x0040);
	bcm43xx_radio_write16(bcm, 0x0005, 0x00AA);
	bcm43xx_radio_write16(bcm, 0x0032, 0x008F);
	bcm43xx_radio_write16(bcm, 0x0006, 0x008F);
	bcm43xx_radio_write16(bcm, 0x0034, 0x008F);
	bcm43xx_radio_write16(bcm, 0x002C, 0x0007);
	bcm43xx_radio_write16(bcm, 0x0082, 0x0080);
	bcm43xx_radio_write16(bcm, 0x0080, 0x0000);
	bcm43xx_radio_write16(bcm, 0x003F, 0x00DA);
	bcm43xx_radio_write16(bcm, 0x0005, bcm43xx_radio_read16(bcm, 0x0005) & ~0x0008);
	bcm43xx_radio_write16(bcm, 0x0081, bcm43xx_radio_read16(bcm, 0x0081) & ~0x0010);
	bcm43xx_radio_write16(bcm, 0x0081, bcm43xx_radio_read16(bcm, 0x0081) & ~0x0020);
	bcm43xx_radio_write16(bcm, 0x0081, bcm43xx_radio_read16(bcm, 0x0081) & ~0x0020);
	udelay(400);

	bcm43xx_radio_write16(bcm, 0x0081, (bcm43xx_radio_read16(bcm, 0x0081) & ~0x0020) | 0x0010);
	udelay(400);

	bcm43xx_radio_write16(bcm, 0x0005, (bcm43xx_radio_read16(bcm, 0x0005) & ~0x0008) | 0x0008);
	bcm43xx_radio_write16(bcm, 0x0085, bcm43xx_radio_read16(bcm, 0x0085) & ~0x0010);
	bcm43xx_radio_write16(bcm, 0x0005, bcm43xx_radio_read16(bcm, 0x0005) & ~0x0008);
	bcm43xx_radio_write16(bcm, 0x0081, bcm43xx_radio_read16(bcm, 0x0081) & ~0x0040);
	bcm43xx_radio_write16(bcm, 0x0081, (bcm43xx_radio_read16(bcm, 0x0081) & ~0x0040) | 0x0040);
	bcm43xx_radio_write16(bcm, 0x0005, (bcm43xx_radio_read16(bcm, 0x0081) & ~0x0008) | 0x0008);
	bcm43xx_phy_write(bcm, 0x0063, 0xDDC6);
	bcm43xx_phy_write(bcm, 0x0069, 0x07BE);
	bcm43xx_phy_write(bcm, 0x006A, 0x0000);

	err = bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_A, 0);
	assert(err == 0);
	udelay(1000);
}

/* Get the freq, as it has to be written to the device. */
static inline
u16 channel2freq_bg(u8 channel)
{
	/* Frequencies are given as frequencies_bg[index] + 2.4GHz
	 * Starting with channel 1
	 */
	static const u16 frequencies_bg[14] = {
		12, 17, 22, 27,
		32, 37, 42, 47,
		52, 57, 62, 67,
		72, 84,
	};

	assert(channel >= 1 && channel <= 14);

	return frequencies_bg[channel - 1];
}

/* Get the freq, as it has to be written to the device. */
static inline
u16 channel2freq_a(u8 channel)
{
	assert(channel <= 200);

	return (5000 + 5 * channel);
}

static inline
u16 freq_r3A_value(u16 frequency)
{
	u16 value;

	if (frequency < 5091)
		value = 0x0040;
	else if (frequency < 5321)
		value = 0x0000;
	else if (frequency < 5806)
		value = 0x0080;
	else
		value = 0x0040;

	return value;
}

void bcm43xx_radio_set_tx_iq(struct bcm43xx_private *bcm)
{
	static const u8 data_high[5] = { 0x00, 0x40, 0x80, 0x90, 0xD0 };
	static const u8 data_low[5]  = { 0x00, 0x01, 0x05, 0x06, 0x0A };
	u16 tmp = bcm43xx_radio_read16(bcm, 0x001E);
	int i, j;
	
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			if (tmp == (data_high[i] << 4 | data_low[j])) {
				bcm43xx_phy_write(bcm, 0x0069, (i - j) << 8 | 0x00C0);
				return;
			}
}

int bcm43xx_radio_selectchannel(struct bcm43xx_private *bcm,
				u8 channel,
				int synthetic_pu_workaround)
{
	struct bcm43xx_radioinfo *radio = bcm->current_core->radio;
	u16 r8, tmp;
	u16 freq;

	if ((radio->manufact == 0x17F) &&
	    (radio->version == 0x2060) &&
	    (radio->revision == 1)) {
		if (channel > 200)
			return -EINVAL;
		freq = channel2freq_a(channel);

		r8 = bcm43xx_radio_read16(bcm, 0x0008);
		bcm43xx_write16(bcm, 0x03F0, freq);
		bcm43xx_radio_write16(bcm, 0x0008, r8);

		TODO();//TODO: write max channel TX power? to Radio 0x2D
		tmp = bcm43xx_radio_read16(bcm, 0x002E);
		tmp &= 0x0080;
		TODO();//TODO: OR tmp with the Power out estimation for this channel?
		bcm43xx_radio_write16(bcm, 0x002E, tmp);

		if (freq >= 4920 && freq <= 5500) {
			/* 
			 * r8 = (((freq * 15 * 0xE1FC780F) >> 32) / 29) & 0x0F;
			 *    = (freq * 0.025862069
			 */
			r8 = 3 * freq / 116; /* is equal to r8 = freq * 0.025862 */
		}
		bcm43xx_radio_write16(bcm, 0x0007, (r8 << 4) | r8);
		bcm43xx_radio_write16(bcm, 0x0020, (r8 << 4) | r8);
		bcm43xx_radio_write16(bcm, 0x0021, (r8 << 4) | r8);
		bcm43xx_radio_write16(bcm, 0x0022,
				      (bcm43xx_radio_read16(bcm, 0x0022)
				       & 0x000F) | (r8 << 4));
		bcm43xx_radio_write16(bcm, 0x002A, (r8 << 4));
		bcm43xx_radio_write16(bcm, 0x002B, (r8 << 4));
		bcm43xx_radio_write16(bcm, 0x0008,
				      (bcm43xx_radio_read16(bcm, 0x0008)
				       & 0x00F0) | (r8 << 4));
		bcm43xx_radio_write16(bcm, 0x0029,
				      (bcm43xx_radio_read16(bcm, 0x0029)
				       & 0xFF0F) | 0x00B0);
		bcm43xx_radio_write16(bcm, 0x0035, 0x00AA);
		bcm43xx_radio_write16(bcm, 0x0036, 0x0085);
		bcm43xx_radio_write16(bcm, 0x003A,
				      (bcm43xx_radio_read16(bcm, 0x003A)
				       & 0xFF20) | freq_r3A_value(freq));
		bcm43xx_radio_write16(bcm, 0x003D,
				      bcm43xx_radio_read16(bcm, 0x003D) & 0x00FF);
		bcm43xx_radio_write16(bcm, 0x0081,
				      (bcm43xx_radio_read16(bcm, 0x0081)
				       & 0xFF7F) | 0x0080);
		bcm43xx_radio_write16(bcm, 0x0035,
				      bcm43xx_radio_read16(bcm, 0x0035) & 0xFFEF);
		bcm43xx_radio_write16(bcm, 0x0035,
				      (bcm43xx_radio_read16(bcm, 0x0035)
				       & 0xFFEF) | 0x0010);
		bcm43xx_radio_set_tx_iq(bcm);
		TODO();	//TODO:	TSSI2dbm workaround
		bcm43xx_phy_xmitpower(bcm);//FIXME correct?
	} else {
		if ((channel < 1) || (channel > 14))
			return -EINVAL;

		if (synthetic_pu_workaround) {
			/* Synthetic PU workaround */
			if (radio->version == 0x2050 &&
			    radio->revision < 6) {
				if (channel <= 10)
					bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL,
							channel2freq_bg(channel + 4));
				else
					bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL,
							channel2freq_bg(1));
				udelay(100);
				bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL,
						channel2freq_bg(channel));
			}
		}
		bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL,
				channel2freq_bg(channel));

		if (channel == 14) {
			if (bcm->sprom.locale == BCM43xx_LOCALE_JAPAN) {
				bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED,
						    BCM43xx_UCODEFLAGS_OFFSET,
						    bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED,
								       BCM43xx_UCODEFLAGS_OFFSET)
						    & ~(1 << 7));
			} else {
				bcm43xx_shm_write32(bcm, BCM43xx_SHM_SHARED,
						    BCM43xx_UCODEFLAGS_OFFSET,
						    bcm43xx_shm_read32(bcm, BCM43xx_SHM_SHARED,
								       BCM43xx_UCODEFLAGS_OFFSET)
						    | (1 << 7));
			}
			bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT,
					bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT)
					| (1 << 11));
		} else {
			bcm43xx_write16(bcm, BCM43xx_MMIO_CHANNEL_EXT,
					bcm43xx_read16(bcm, BCM43xx_MMIO_CHANNEL_EXT)
					& 0xF7BF);
		}
	}

	bcm->current_core->radio->channel = channel;
	//XXX: Using the longer of 2 timeouts (8000 vs 2000 usecs). Specs states
	//     that 2000 usecs might suffice.
	udelay(8000);

	return 0;
}

void bcm43xx_radio_set_txantenna(struct bcm43xx_private *bcm, u32 val)
{
	u16 tmp;

	tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x0022) & 0xFCFF;
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0022, tmp | val);
	tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x03a8) & 0xFCFF;
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x03a8, tmp | val);
	tmp = bcm43xx_shm_read16(bcm, BCM43xx_SHM_SHARED, 0x0054) & 0xFFCFF;
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0054, tmp | val);
}

/* http://bcm-specs.sipsolutions.net/TX_Gain_Base_Band */
static u16 bcm43xx_get_txgain_base_band(u16 txpower)
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
static u16 bcm43xx_get_txgain_freq_power_amp(u16 txpower)
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
static u16 bcm43xx_get_txgain_dac(u16 txpower)
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

void bcm43xx_radio_set_txpower_a(struct bcm43xx_private *bcm, u16 txpower)
{
	u16 pamp, base, dac, ilt;

	txpower = limit_value(txpower, 0, 63);

	pamp = bcm43xx_get_txgain_freq_power_amp(txpower);
	pamp <<= 5;
	pamp &= 0x00E0;
	bcm43xx_phy_write(bcm, 0x0019, pamp);

	base = bcm43xx_get_txgain_base_band(txpower);
	base &= 0x000F;
	bcm43xx_phy_write(bcm, 0x0017, base | 0x0020);

	ilt = bcm43xx_ilt_read16(bcm, 0x3001);
	ilt &= 0x0007;

	dac = bcm43xx_get_txgain_dac(txpower);
	dac <<= 3;
	dac |= ilt;

	bcm43xx_ilt_write16(bcm, 0x3001, dac);

	bcm->current_core->radio->txpower[0] = txpower;

	TODO();
	//TODO: FuncPlaceholder (Adjust BB loft cancel)
}

void bcm43xx_radio_set_txpower_bg(struct bcm43xx_private *bcm,
                                 u16 baseband_attenuation, u16 attenuation,
			         u16 txpower)
{
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

	bcm43xx_phy_set_baseband_attenuation(bcm, baseband_attenuation);
	bcm43xx_radio_write16(bcm, 0x0043, attenuation);
	bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0064, attenuation);
	if (bcm->current_core->radio->version == 0x2050) {
		bcm43xx_radio_write16(bcm, 0x0052,
		                      (bcm43xx_radio_read16(bcm, 0x0052) & 0xFF8F)
				       | (txpower << 4));
	}

	if (bcm->current_core->phy->type == BCM43xx_PHYTYPE_G)
		bcm43xx_phy_lo_adjust(bcm, 0);
}


int bcm43xx_radio_turn_on(struct bcm43xx_private *bcm)
{
	if (bcm->current_core->radio->enabled)
		return 0;

	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		bcm43xx_radio_write16(bcm, 0x0004, 0x00C0);
		bcm43xx_radio_write16(bcm, 0x0005, 0x0008);
		bcm43xx_phy_write(bcm, 0x0010, bcm43xx_phy_read(bcm, 0x0010) & 0xFFF7);
		bcm43xx_phy_write(bcm, 0x0011, bcm43xx_phy_read(bcm, 0x0011) & 0xFFF7);
		bcm43xx_radio_init2060(bcm);	
		break;
        case BCM43xx_PHYTYPE_B:
        case BCM43xx_PHYTYPE_G:
		bcm43xx_phy_write(bcm, 0x0015, 0x8000);
		bcm43xx_phy_write(bcm, 0x0015, 0xCC00);
		bcm43xx_phy_write(bcm, 0x0015, ((bcm->current_core->phy->connected) ? 0x00C0 : 0x0000));
		bcm43xx_radio_selectchannel(bcm, BCM43xx_RADIO_DEFAULT_CHANNEL_BG, 1);
		break;
	default:
		assert(0);
	}
	dprintk(KERN_INFO PFX "Radio turned on\n");

	bcm->current_core->radio->enabled = 1;

	return 0;
}
	
int bcm43xx_radio_turn_off(struct bcm43xx_private *bcm)
{
	const u8 phytype = bcm->current_core->phy->type;

	if (phytype == BCM43xx_PHYTYPE_A) {
		bcm43xx_radio_write16(bcm, 0x0004, 0x00FF);
		bcm43xx_radio_write16(bcm, 0x0005, 0x00FB);
		bcm43xx_phy_write(bcm, 0x0010, bcm43xx_phy_read(bcm, 0x0010) | 0x0008);
		bcm43xx_phy_write(bcm, 0x0011, bcm43xx_phy_read(bcm, 0x0011) | 0x0008);
	}
	if (phytype == BCM43xx_PHYTYPE_G &&
	    bcm->current_core->rev >= 5) {
		bcm43xx_phy_write(bcm, 0x0811, bcm43xx_phy_read(bcm, 0x0811) | 0x008C);
		bcm43xx_phy_write(bcm, 0x0812, bcm43xx_phy_read(bcm, 0x0812) & 0xFF73);
	} else
		bcm43xx_phy_write(bcm, 0x0015, 0xAA00);
	dprintk(KERN_INFO PFX "Radio turned off\n");

	bcm->current_core->radio->enabled = 0;

	return 0;
}

void bcm43xx_radio_clear_tssi(struct bcm43xx_private *bcm)
{
	switch (bcm->current_core->phy->type) {
	case BCM43xx_PHYTYPE_A:
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0068, 0x7F7F);
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x006a, 0x7F7F);
		break;
	case BCM43xx_PHYTYPE_B:
	case BCM43xx_PHYTYPE_G:
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0058, 0x7F7F);
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x005a, 0x7F7F);
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0070, 0x7F7F);
		bcm43xx_shm_write16(bcm, BCM43xx_SHM_SHARED, 0x0072, 0x7F7F);
		break;
	}
}