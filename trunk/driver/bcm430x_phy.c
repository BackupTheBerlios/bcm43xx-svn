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

static void bcm430x_phy_initg(struct bcm430x_private *bcm);

#define ILLT_ROTOR_SIZE 53
u32 illt_rotor[ILLT_ROTOR_SIZE] = {
	0xFEB93FFD, 0xFEC63FFD, /* 0 */
	0xFED23FFD, 0xFEDF3FFD,
	0xFEEC3FFE, 0xFEF83FFE,
	0xFF053FFE, 0xFF113FFE,
	0xFF1E3FFE, 0xFF2A3FFF, /* 8 */
	0xFF373FFF, 0xFF443FFF,
	0xFF503FFF, 0xFF5D3FFF,
	0xFF693FFF, 0xFF763FFF,
	0xFF824000, 0xFF8F4000, /* 16 */
	0xFF9B4000, 0xFFA84000,
	0xFFB54000, 0xFFC14000,
	0xFFCE4000, 0xFFDA4000,
	0xFFE74000, 0xFFF34000, /* 24 */
	0x00004000, 0x000D4000,
	0x00194000, 0x00264000,
	0x00324000, 0x003F4000,
	0x004B4000, 0x00584000, /* 32 */
	0x00654000, 0x00714000,
	0x007E4000, 0x008A3FFF,
	0x00973FFF, 0x00A33FFF,
	0x00B03FFF, 0x00BC3FFF, /* 40 */
	0x00C93FFF, 0x00D63FFF,
	0x00E23FFE, 0x00EF3FFE,
	0x00FB3FFE, 0x01083FFE,
	0x01143FFE, 0x01213FFD, /* 48 */
	0x012E3FFD, 0x013A3FFD,
	0x01473FFD,
};

#define ILLT_RETARD_SIZE 53
u32 illt_retard[ILLT_RETARD_SIZE] = {
	0xDB93CB87, 0xD666CF64, /* 0 */
	0xD1FDD358, 0xCDA6D826,
	0xCA38DD9F, 0xC729E2B4,
	0xC469E88E, 0xC26AEE2B,
	0xC0DEF46C, 0xC073FA62, /* 8 */
	0xC01D00D5, 0xC0760743,
	0xC1560D1E, 0xC2E51369,
	0xC4ED18FF, 0xC7AC1ED7,
	0xCB2823B2, 0xCEFA28D9, /* 16 */
	0xD2F62D3F, 0xD7BB3197,
	0xDCE53568, 0xE1FE3875,
	0xE7D13B35, 0xED663D35,
	0xF39B3EC4, 0xF98E3FA7, /* 24 */
	0x00004000, 0x06723FA7,
	0x0C653EC4, 0x129A3D35,
	0x182F3B35, 0x1E023875,
	0x231B3568, 0x28453197, /* 32 */
	0x2D0A2D3F, 0x310628D9,
	0x34D823B2, 0x38541ED7,
	0x3B1318FF, 0x3D1B1369,
	0x3EAA0D1E, 0x3F8A0743, /* 40 */
	0x3FE300D5, 0x3F8DFA62,
	0x3F22F46C, 0x3D96EE2B,
	0x3B97E88E, 0x38D7E2B4,
	0x35C8DD9F, 0x325AD826, /* 48 */
	0x2E03D358, 0x299ACF64,
	0x246DCB87,
};

#define ILLT_FINEFREQA_SIZE 256
u16 illt_finefreqa[ILLT_FINEFREQA_SIZE] = {
	0x0082, 0x0082, 0x0102, 0x0182, /* 0 */
 	0x0202, 0x0282, 0x0302, 0x0382,
 	0x0402, 0x0482, 0x0502, 0x0582,
 	0x05E2, 0x0662, 0x06E2, 0x0762,
 	0x07E2, 0x0842, 0x08C2, 0x0942, /* 16 */
 	0x09C2, 0x0A22, 0x0AA2, 0x0B02,
 	0x0B82, 0x0BE2, 0x0C62, 0x0CC2,
 	0x0D42, 0x0DA2, 0x0E02, 0x0E62,
 	0x0EE2, 0x0F42, 0x0FA2, 0x1002, /* 32 */
 	0x1062, 0x10C2, 0x1122, 0x1182,
 	0x11E2, 0x1242, 0x12A2, 0x12E2,
 	0x1342, 0x13A2, 0x1402, 0x1442,
 	0x14A2, 0x14E2, 0x1542, 0x1582, /* 48 */
 	0x15E2, 0x1622, 0x1662, 0x16C1,
 	0x1701, 0x1741, 0x1781, 0x17E1,
 	0x1821, 0x1861, 0x18A1, 0x18E1,
 	0x1921, 0x1961, 0x19A1, 0x19E1, /* 64 */
 	0x1A21, 0x1A61, 0x1AA1, 0x1AC1,
 	0x1B01, 0x1B41, 0x1B81, 0x1BA1,
 	0x1BE1, 0x1C21, 0x1C41, 0x1C81,
 	0x1CA1, 0x1CE1, 0x1D01, 0x1D41, /* 80 */
 	0x1D61, 0x1DA1, 0x1DC1, 0x1E01,
 	0x1E21, 0x1E61, 0x1E81, 0x1EA1,
 	0x1EE1, 0x1F01, 0x1F21, 0x1F41,
 	0x1F81, 0x1FA1, 0x1FC1, 0x1FE1, /* 96 */
 	0x2001, 0x2041, 0x2061, 0x2081,
 	0x20A1, 0x20C1, 0x20E1, 0x2101,
 	0x2121, 0x2141, 0x2161, 0x2181,
 	0x21A1, 0x21C1, 0x21E1, 0x2201, /* 112 */
 	0x2221, 0x2241, 0x2261, 0x2281,
 	0x22A1, 0x22C1, 0x22C1, 0x22E1,
 	0x2301, 0x2321, 0x2341, 0x2361,
 	0x2361, 0x2381, 0x23A1, 0x23C1, /* 128 */
 	0x23E1, 0x23E1, 0x2401, 0x2421,
 	0x2441, 0x2441, 0x2461, 0x2481,
 	0x2481, 0x24A1, 0x24C1, 0x24C1,
 	0x24E1, 0x2501, 0x2501, 0x2521, /* 144 */
 	0x2541, 0x2541, 0x2561, 0x2561,
 	0x2581, 0x25A1, 0x25A1, 0x25C1,
 	0x25C1, 0x25E1, 0x2601, 0x2601,
 	0x2621, 0x2621, 0x2641, 0x2641, /* 160 */
 	0x2661, 0x2661, 0x2681, 0x2681,
 	0x26A1, 0x26A1, 0x26C1, 0x26C1,
 	0x26E1, 0x26E1, 0x2701, 0x2701,
 	0x2721, 0x2721, 0x2740, 0x2740, /* 176 */
 	0x2760, 0x2760, 0x2780, 0x2780,
 	0x2780, 0x27A0, 0x27A0, 0x27C0,
 	0x27C0, 0x27E0, 0x27E0, 0x27E0,
 	0x2800, 0x2800, 0x2820, 0x2820, /* 192 */
 	0x2820, 0x2840, 0x2840, 0x2840,
 	0x2860, 0x2860, 0x2880, 0x2880,
 	0x2880, 0x28A0, 0x28A0, 0x28A0,
 	0x28C0, 0x28C0, 0x28C0, 0x28E0, /* 208 */
 	0x28E0, 0x28E0, 0x2900, 0x2900,
 	0x2900, 0x2920, 0x2920, 0x2920,
 	0x2940, 0x2940, 0x2940, 0x2960,
 	0x2960, 0x2960, 0x2960, 0x2980, /* 224 */
 	0x2980, 0x2980, 0x29A0, 0x29A0,
 	0x29A0, 0x29A0, 0x29C0, 0x29C0,
 	0x29C0, 0x29E0, 0x29E0, 0x29E0,
 	0x29E0, 0x2A00, 0x2A00, 0x2A00, /* 240 */
 	0x2A00, 0x2A20, 0x2A20, 0x2A20,
 	0x2A20, 0x2A40, 0x2A40, 0x2A40,
 	0x2A40, 0x2A60, 0x2A60, 0x2A60,
};

#define ILLT_FINEFREQG_SIZE 256
u16 illt_finefreqg[ILLT_FINEFREQG_SIZE] = {
	0x0089, 0x02E9, 0x0409, 0x04E9, /* 0 */
	0x05A9, 0x0669, 0x0709, 0x0789,
	0x0829, 0x08A9, 0x0929, 0x0989,
	0x0A09, 0x0A69, 0x0AC9, 0x0B29,
	0x0BA9, 0x0BE9, 0x0C49, 0x0CA9, /* 16 */
	0x0D09, 0x0D69, 0x0DA9, 0x0E09,
	0x0E69, 0x0EA9, 0x0F09, 0x0F49,
	0x0FA9, 0x0FE9, 0x1029, 0x1089,
	0x10C9, 0x1109, 0x1169, 0x11A9, /* 32 */
	0x11E9, 0x1229, 0x1289, 0x12C9,
	0x1309, 0x1349, 0x1389, 0x13C9,
	0x1409, 0x1449, 0x14A9, 0x14E9,
	0x1529, 0x1569, 0x15A9, 0x15E9, /* 48 */
	0x1629, 0x1669, 0x16A9, 0x16E8,
	0x1728, 0x1768, 0x17A8, 0x17E8,
	0x1828, 0x1868, 0x18A8, 0x18E8,
	0x1928, 0x1968, 0x19A8, 0x19E8, /* 64 */
	0x1A28, 0x1A68, 0x1AA8, 0x1AE8,
	0x1B28, 0x1B68, 0x1BA8, 0x1BE8,
	0x1C28, 0x1C68, 0x1CA8, 0x1CE8,
	0x1D28, 0x1D68, 0x1DC8, 0x1E08, /* 80 */
	0x1E48, 0x1E88, 0x1EC8, 0x1F08,
	0x1F48, 0x1F88, 0x1FE8, 0x2028,
	0x2068, 0x20A8, 0x2108, 0x2148,
	0x2188, 0x21C8, 0x2228, 0x2268, /* 96 */
	0x22C8, 0x2308, 0x2348, 0x23A8,
	0x23E8, 0x2448, 0x24A8, 0x24E8,
	0x2548, 0x25A8, 0x2608, 0x2668,
	0x26C8, 0x2728, 0x2787, 0x27E7, /* 112 */
	0x2847, 0x28C7, 0x2947, 0x29A7,
	0x2A27, 0x2AC7, 0x2B47, 0x2BE7,
	0x2CA7, 0x2D67, 0x2E47, 0x2F67,
	0x3247, 0x3526, 0x3646, 0x3726, /* 128 */
	0x3806, 0x38A6, 0x3946, 0x39E6,
	0x3A66, 0x3AE6, 0x3B66, 0x3BC6,
	0x3C45, 0x3CA5, 0x3D05, 0x3D85,
	0x3DE5, 0x3E45, 0x3EA5, 0x3EE5, /* 144 */
	0x3F45, 0x3FA5, 0x4005, 0x4045,
	0x40A5, 0x40E5, 0x4145, 0x4185,
	0x41E5, 0x4225, 0x4265, 0x42C5,
	0x4305, 0x4345, 0x43A5, 0x43E5, /* 160 */
	0x4424, 0x4464, 0x44C4, 0x4504,
	0x4544, 0x4584, 0x45C4, 0x4604,
	0x4644, 0x46A4, 0x46E4, 0x4724,
	0x4764, 0x47A4, 0x47E4, 0x4824, /* 176 */
	0x4864, 0x48A4, 0x48E4, 0x4924,
	0x4964, 0x49A4, 0x49E4, 0x4A24,
	0x4A64, 0x4AA4, 0x4AE4, 0x4B23,
	0x4B63, 0x4BA3, 0x4BE3, 0x4C23, /* 192 */
	0x4C63, 0x4CA3, 0x4CE3, 0x4D23,
	0x4D63, 0x4DA3, 0x4DE3, 0x4E23,
	0x4E63, 0x4EA3, 0x4EE3, 0x4F23,
	0x4F63, 0x4FC3, 0x5003, 0x5043, /* 208 */
	0x5083, 0x50C3, 0x5103, 0x5143,
	0x5183, 0x51E2, 0x5222, 0x5262,
	0x52A2, 0x52E2, 0x5342, 0x5382,
	0x53C2, 0x5402, 0x5462, 0x54A2, /* 224 */
	0x5502, 0x5542, 0x55A2, 0x55E2,
	0x5642, 0x5682, 0x56E2, 0x5722,
	0x5782, 0x57E1, 0x5841, 0x58A1,
	0x5901, 0x5961, 0x59C1, 0x5A21, /* 240 */
	0x5AA1, 0x5B01, 0x5B81, 0x5BE1,
	0x5C61, 0x5D01, 0x5D80, 0x5E20,
	0x5EE0, 0x5FA0, 0x6080, 0x61C0,
};

#define ILLT_NOISEA2_SIZE 8
u16 illt_noisea2[ILLT_NOISEA2_SIZE] = {
	0x0001, 0x0001, 0x0001, 0xFFFE,
	0xFFFE, 0x3FFF, 0x1000, 0x0393,
};

#define ILLT_NOISEA3_SIZE 8
u16 illt_noisea3[ILLT_NOISEA3_SIZE] = {
	0x4C4C, 0x4C4C, 0x4C4C, 0x2D36,
	0x4C4C, 0x4C4C, 0x4C4C, 0x2D36,
};

#define ILLT_NOISEG1_SIZE 8
u16 illt_noiseg1[ILLT_NOISEG1_SIZE] = {
	0x013C, 0x01F5, 0x031A, 0x0631,
	0x0001, 0x0001, 0x0001, 0x0001,
};

#define ILLT_NOISEG2_SIZE 8
u16 illt_noiseg2[ILLT_NOISEG2_SIZE] = {
	0x5484, 0x3C40, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
};

#define ILLT_NOISESCALEG_SIZE 27
u16 illt_noisescaleg[ILLT_NOISESCALEG_SIZE] = {
	0x6C77, 0x5162, 0x3B40, 0x3335, /* 0 */
	0x2F2D, 0x2A2A, 0x2527, 0x1F21,
	0x1A1D, 0x1719, 0x1616, 0x1414,
	0x1414, 0x1400, 0x1414, 0x1614,
	0x1716, 0x1A19, 0x1F1D, 0x2521, /* 16 */
	0x2A27, 0x2F2A, 0x332D, 0x3B35,
	0x5140, 0x6C62, 0x0077,
};

#define ILLT_SIGMASQR_SIZE 53
u16 illt_sigmasqr[ILLT_SIGMASQR_SIZE] = {
	0x007A, 0x0075, 0x0071, 0x006C, /* 0 */
	0x0067, 0x0063, 0x005E, 0x0059,
	0x0054, 0x0050, 0x004B, 0x0046,
	0x0042, 0x003D, 0x003D, 0x003D,
	0x003D, 0x003D, 0x003D, 0x003D, /* 16 */
	0x003D, 0x003D, 0x003D, 0x003D,
	0x003D, 0x003D, 0x0000, 0x003D,
	0x003D, 0x003D, 0x003D, 0x003D,
	0x003D, 0x003D, 0x003D, 0x003D, /* 32 */
	0x003D, 0x003D, 0x003D, 0x003D,
	0x0042, 0x0046, 0x004B, 0x0050,
	0x0054, 0x0059, 0x005E, 0x0063,
	0x0067, 0x006C, 0x0071, 0x0075, /* 48 */
	0x007A,
};

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

void bcm430x_illt_write16(struct bcm430x_private *bcm, u16 offset, u16 val)
{
	if ( bcm->current_core->phy->type == BCM430x_PHYTYPE_A ) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_CTRL, offset);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, val);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_CTRL, offset);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_DATA1, val);
	}
}
u16 bcm430x_illt_read16(struct bcm430x_private *bcm, u16 offset)
{
	if ( bcm->current_core->phy->type == BCM430x_PHYTYPE_A ) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_CTRL, offset);
		return bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_A_DATA1);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_CTRL, offset);
		return bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_G_DATA1);
	}
}
void bcm430x_illt_write32(struct bcm430x_private *bcm, u16 offset, u32 val)
{
	if ( bcm->current_core->phy->type == BCM430x_PHYTYPE_A ) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_CTRL, offset);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA2, val >> 16);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, val & 0x0000FFFF);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_CTRL, offset);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_DATA2, val >> 16);
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_DATA1, val & 0x0000FFFF);
	}
}
u32 bcm430x_illt_read32(struct bcm430x_private *bcm, u16 offset)
{
	u32 rval;

	if ( bcm->current_core->phy->type == BCM430x_PHYTYPE_A ) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_CTRL, offset);
		rval = bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_A_DATA2);
		rval = (rval << 16) + bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_A_DATA1);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_G_CTRL, offset);
		rval = bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_G_DATA2);
		rval = (rval << 16) + bcm430x_phy_read(bcm, BCM430x_PHY_ILLT_G_DATA1);
	}
	return rval;
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

	bcm430x_illt_write16(bcm, offset, 0x00FE);
	bcm430x_illt_write16(bcm, offset+1, 0x000D);
	bcm430x_illt_write16(bcm, offset+2, 0x0013);
	bcm430x_illt_write16(bcm, offset+3, 0x0019);

	if ( bcm->current_core->phy->rev == 1) {
		bcm430x_illt_write16(bcm, 0x1800, 0x2710);
		bcm430x_illt_write16(bcm, 0x1801, 0x9B83);
		bcm430x_illt_write16(bcm, 0x1802, 0x9B83);
		bcm430x_illt_write16(bcm, 0x1803, 0x0F8D);
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

	bcm430x_illt_write16(bcm, offset + 0x0800, 0x0000);
	bcm430x_illt_write16(bcm, offset + 0x0801, 0x0007);
	bcm430x_illt_write16(bcm, offset + 0x0802, 0x0016);
	bcm430x_illt_write16(bcm, offset + 0x0803, 0x0028);
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

		for ( i = 0; i < ILLT_FINEFREQG_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x5800+i, illt_finefreqg[i]);
		}
		for ( i = 0; i < ILLT_NOISEG1_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x1800+i, illt_noiseg1[i]);
		}
		for ( i = 0; i < ILLT_ROTOR_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x2000+i, illt_rotor[i]);
		}
		for ( i = 0; i < ILLT_NOISESCALEG_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x1400+i, illt_noisescaleg[i]);
		}
		for ( i = 0; i < ILLT_RETARD_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x2400+i, illt_retard[i]);
		}
		for ( i = 0; i < 4; i++ ) {
			bcm430x_illt_write16(bcm, 0x5404+i, 0x0020);
			bcm430x_illt_write16(bcm, 0x5408+i, 0x0020);
			bcm430x_illt_write16(bcm, 0x540C+i, 0x0020);
			bcm430x_illt_write16(bcm, 0x5410+i, 0x0020);
		}
		bcm430x_phy_agcsetup(bcm);
		break;
	case 2:
		bcm430x_phy_write(bcm, BCM430x_PHY_NRSSILT_CTRL, 0xBA98);
		bcm430x_phy_write(bcm, BCM430x_PHY_NRSSILT_DATA, 0x7654);

		bcm430x_phy_write(bcm, 0x04C0, 0x1861);
		bcm430x_phy_write(bcm, 0x04C1, 0x0271);

		for (i = 0; i < 64; i++) {
			bcm430x_illt_write16(bcm, 0x4000+i, i);
		}
		for (i = 0; i < ILLT_NOISEG2_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x1800+i, illt_noiseg2[i]);
		}
		for (i = 0; i < ILLT_NOISESCALEG_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x1400+i, illt_noisescaleg[i]);
		}
		for (i=0; i < ILLT_SIGMASQR_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x5000+i, illt_sigmasqr[i]);
		}
		for (i=0; i <= 0x2F; i++) {
			bcm430x_illt_write16(bcm, 0x1000+i, 0x0820);
		}
		bcm430x_phy_agcsetup(bcm);
		bcm430x_phy_write(bcm, 0x0403, 0x1000);
		bcm430x_illt_write16(bcm, 0x3C02, 0x000F);
		bcm430x_illt_write16(bcm, 0x3C03, 0x0014);

		if ((bcm->board_vendor != PCI_VENDOR_ID_BROADCOM)
			|| (bcm->board_type != 0x0416)
			|| (bcm->board_revision != 0x0017)) {
			if (bcm->current_core->phy->rev < 2) {
				bcm430x_illt_write16(bcm, 0x5001, 0x0002);
				bcm430x_illt_write16(bcm, 0x5002, 0x0001);
			} else {
				bcm430x_illt_write16(bcm, 0x0401, 0x0002);
				bcm430x_illt_write16(bcm, 0x0402, 0x0001);
			}
		}
		break;
	}
}

static void bcm430x_phy_init_noisescaletbl(struct bcm430x_private *bcm)
{
	u16 i = 0x00;

	bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_CTRL, 0x1400);
	for (i=0; i < 12; i++) {
		if (bcm->current_core->phy->rev == 2) {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x6767);
		} else {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x2323);
		}
	}
	if (bcm->current_core->phy->rev == 2) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x6700);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x2300);
	}
	for (i=0; i < 11; i++) {
		if (bcm->current_core->phy->rev == 2) {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x6767);
		} else {
			bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x2323);
		}
	}
	if (bcm->current_core->phy->rev == 2) {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x0067);
	} else {
		bcm430x_phy_write(bcm, BCM430x_PHY_ILLT_A_DATA1, 0x0023);
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

		bcm430x_illt_write16(bcm, 0x3807, 0x0051);

		bcm430x_phy_write(bcm, 0x001C, 0x0FF9);
		bcm430x_phy_write(bcm, 0x0020, bcm430x_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm430x_illt_write16(bcm, 0x3C0C, 0x07BF);
		bcm430x_radio_write16(bcm, 0x0002, 0x07BF);

		bcm430x_phy_write(bcm, 0x0024, 0x4680);
		bcm430x_phy_write(bcm, 0x0020, 0x0003);
		bcm430x_phy_write(bcm, 0x001D, 0x0F40);
		bcm430x_phy_write(bcm, 0x001F, 0x1C00);

		bcm430x_phy_write(bcm, 0x002A, (bcm430x_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);
		bcm430x_phy_write(bcm, 0x002B, bcm430x_phy_read(bcm, 0x002B) & 0xFBFF);
		bcm430x_phy_write(bcm, 0x008E, 0x58C1);

		bcm430x_illt_write16(bcm, 0x0803, 0x000F);
		bcm430x_illt_write16(bcm, 0x0804, 0x001F);
		bcm430x_illt_write16(bcm, 0x0805, 0x002A);
		bcm430x_illt_write16(bcm, 0x0805, 0x0030);
		bcm430x_illt_write16(bcm, 0x0807, 0x003A);

		bcm430x_illt_write16(bcm, 0x0000, 0x0013);
		bcm430x_illt_write16(bcm, 0x0001, 0x0013);
		bcm430x_illt_write16(bcm, 0x0002, 0x0013);
		bcm430x_illt_write16(bcm, 0x0003, 0x0013);
		bcm430x_illt_write16(bcm, 0x0004, 0x0015);
		bcm430x_illt_write16(bcm, 0x0005, 0x0015);
		bcm430x_illt_write16(bcm, 0x0006, 0x0019);

		bcm430x_illt_write16(bcm, 0x0404, 0x0003);
		bcm430x_illt_write16(bcm, 0x0405, 0x0003);
		bcm430x_illt_write16(bcm, 0x0406, 0x0007);

		for (i = 0; i < 16; i++) {
			bcm430x_illt_write16(bcm, 0x4000+i, (i+8)%16);
		}
		
		bcm430x_illt_write16(bcm, 0x3003, 0x1044);
		bcm430x_illt_write16(bcm, 0x3004, 0x7201);
		bcm430x_illt_write16(bcm, 0x3006, 0x0040);
		bcm430x_illt_write16(bcm, 0x3001, (bcm430x_illt_read16(bcm, 0x3001) & 0x0010) | 0x0008);

		for (i = 0; i < ILLT_FINEFREQA_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x5800+i, illt_finefreqa[i]);
		}
		for ( i = 0; i < ILLT_NOISEA2_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x1800+i, illt_noisea2[i]);
		}
		for (i = 0; i < ILLT_ROTOR_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x2000+i, illt_rotor[i]);
		}
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i = 0; i < ILLT_RETARD_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x2400+i, illt_retard[i]);
		}
		break;
	case 3:
		for (i = 0; i < 64; i++) {
			bcm430x_illt_write16(bcm, 0x4000+i, i);
		}

		bcm430x_illt_write16(bcm, 0x3807, 0x0051);

		bcm430x_phy_write(bcm, 0x001C, 0x0FF9);
		bcm430x_phy_write(bcm, 0x0020, bcm430x_phy_read(bcm, 0x0020) & 0xFF0F);
		bcm430x_radio_write16(bcm, 0x0002, 0x07BF);

		bcm430x_phy_write(bcm, 0x0024, 0x4680);
		bcm430x_phy_write(bcm, 0x0020, 0x0003);
		bcm430x_phy_write(bcm, 0x001D, 0x0F40);
		bcm430x_phy_write(bcm, 0x001F, 0x1C00);
		bcm430x_phy_write(bcm, 0x002A, (bcm430x_phy_read(bcm, 0x002A) & 0x00FF) | 0x0400);

		bcm430x_illt_write16(bcm, 0x3001, (bcm430x_illt_read16(bcm, 0x3001) & 0x0010) | 0x0008);
		for ( i = 0; i < ILLT_NOISEA3_SIZE; i++ ) {
			bcm430x_illt_write16(bcm, 0x1800+i, illt_noisea3[i]);
		}
		bcm430x_phy_init_noisescaletbl(bcm);
		for (i=0; i < ILLT_SIGMASQR_SIZE; i++) {
			bcm430x_illt_write16(bcm, 0x5000+i, illt_sigmasqr[i]);
		}
		
		bcm430x_phy_write(bcm, 0x0003, 0x1808);

		bcm430x_illt_write16(bcm, 0x0803, 0x000F);
		bcm430x_illt_write16(bcm, 0x0804, 0x001F);
		bcm430x_illt_write16(bcm, 0x0805, 0x002A);
		bcm430x_illt_write16(bcm, 0x0805, 0x0030);
		bcm430x_illt_write16(bcm, 0x0807, 0x003A);

		bcm430x_illt_write16(bcm, 0x0000, 0x0013);
		bcm430x_illt_write16(bcm, 0x0001, 0x0013);
		bcm430x_illt_write16(bcm, 0x0002, 0x0013);
		bcm430x_illt_write16(bcm, 0x0003, 0x0013);
		bcm430x_illt_write16(bcm, 0x0004, 0x0015);
		bcm430x_illt_write16(bcm, 0x0005, 0x0015);
		bcm430x_illt_write16(bcm, 0x0006, 0x0019);

		bcm430x_illt_write16(bcm, 0x0404, 0x0003);
		bcm430x_illt_write16(bcm, 0x0405, 0x0003);
		bcm430x_illt_write16(bcm, 0x0406, 0x0007);

		bcm430x_illt_write16(bcm, 0x3C02, 0x000F);
		bcm430x_illt_write16(bcm, 0x3C03, 0x0014);
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

		tval = bcm430x_illt_read16(bcm, 0x3001);
		if (bcm->current_core->phy->rev == 1) {
			bcm430x_illt_write16(bcm, 0x3001, (bcm430x_illt_read16(bcm, 0x3001) & 0xFF87) | 0x0058);
		} else {
			bcm430x_illt_write16(bcm, 0x3001, (bcm430x_illt_read16(bcm, 0x3001) & 0xFFC3) | 0x002C);
		}

		bcm430x_dummy_transmission(bcm);
		bcm->current_core->phy->savedpctlreg = bcm430x_phy_read(bcm, BCM430x_PHY_A_PCTL);

		bcm430x_illt_write16(bcm, 0x3001, tval);
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
