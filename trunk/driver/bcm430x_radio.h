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

#define BCM430x_RADIO_DEFAULT_CHANNEL_A		0
#define BCM430x_RADIO_DEFAULT_CHANNEL_BG	6
#define BCM430x_RADIO_DEFAULT_ANTENNA		0x0300

#define BCM430x_RADIO_INTERFMODE_NONE		0
#define BCM430x_RADIO_INTERFMODE_NONWLAN	(1 << 0)
#define BCM430x_RADIO_INTERFMODE_MANUALWLAN	(1 << 1)
#define BCM430x_RADIO_INTERFMODE_AUTOWLAN	(1 << 2)
#define BCM430x_RADIO_INTERFMODE_DISABLE	(1 << 3)

u16 bcm430x_radio_read16(struct bcm430x_private *bcm, u16 offset);
void bcm430x_radio_write16(struct bcm430x_private *bcm, u16 offset, u16 val);

int bcm430x_radio_turn_on(struct bcm430x_private *bcm);
int bcm430x_radio_turn_off(struct bcm430x_private *bcm);

u16 bcm430x_radio_init2050(struct bcm430x_private *bcm);
u16 bcm430x_radio_init2060(struct bcm430x_private *bcm);

int bcm430x_radio_selectchannel(struct bcm430x_private *bcm, u8 channel);
void bcm430x_radio_set_txpower_a(struct bcm430x_private *bcm, u16 txpower);
void bcm430x_radio_set_txpower_b(struct bcm430x_private *bcm,
                               u16 baseband_attenuation, u16 attenuation,
			       u16 txpower);
void bcm430x_radio_set_txantenna(struct bcm430x_private *bcm, u32 val);

void bcm430x_radio_calc_interference(struct bcm430x_private *bcm, u16 mode);
void bcm430x_radio_clear_tssi(struct bcm430x_private *bcm);
