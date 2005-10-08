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

#include "bcm430x.h"


#define _bcm430x_declare_plcp_hdr(size) \
	struct bcm430x_plcp_hdr##size {			\
		union {					\
			u32 data;			\
			unsigned char raw[size];	\
		} __attribute__((__packed__));		\
	} __attribute__((__packed__))

/* struct bcm430x_plcp_hdr4 */
_bcm430x_declare_plcp_hdr(4);
/* struct bcm430c_plcp_hdr6 */
_bcm430x_declare_plcp_hdr(6);

#undef _bcm430x_declare_plcp_hdr

/* Device specific TX header. To be prepended to TX frames. */
struct bcm430x_txhdr {
	union {
		struct {
			u16 flags;
			u16 wsec_rate;
			u16 frame_control;
			u16 unknown_zeroed_0;
			u16 control;
			unsigned char wep_iv[10];
			unsigned char unknown_wsec_tkip_data[3]; //FIXME
			unsigned char dest_mac[6];
			u16 unknown_zeroed_1;
			struct bcm430x_plcp_hdr4 rts_cts_fallback_plcp;
			u16 rts_cts_dur_fallback;
			struct bcm430x_plcp_hdr4 fallback_plcp;
			u16 fallback_dur_id;
			u16 cookie;
			u16 unknown_scb_stuff; //FIXME
			struct bcm430x_plcp_hdr6 rts_cts_plcp;
			u16 rts_cts_frame_type;
			u16 rts_cts_dur;
			unsigned char first_mac[6];
			unsigned char second_mac[6];
			struct bcm430x_plcp_hdr6 plcp_header;
		} __attribute__((__packed__));

		unsigned char raw[82];
	} __attribute__((__packed__));
} __attribute__((__packed__));

void FASTCALL(bcm430x_generate_txhdr(struct bcm430x_txhdr *txhdr));


/* write the SHM Control word with a 32bit word offset */
void bcm430x_shm_control_word(struct bcm430x_private *bcm,
			      u16 routing, u16 offset);

/* write the SHM Control word with a byte offset */
static inline
void bcm430x_shm_control_byte(struct bcm430x_private *bcm,
			      u16 routing, u16 offset)
{
	bcm430x_shm_control_word(bcm, routing, offset >> 2);
}

u32 bcm430x_shm_read32(struct bcm430x_private *bcm,
		       u16 routing, u16 offset);
u16 bcm430x_shm_read16(struct bcm430x_private *bcm,
		       u16 routing, u16 offset);
void bcm430x_shm_write32(struct bcm430x_private *bcm,
			 u16 routing, u16 offset,
			 u32 value);
void bcm430x_shm_write16(struct bcm430x_private *bcm,
			 u16 routing, u16 offset,
			 u16 value);

void bcm430x_dummy_transmission(struct bcm430x_private *bcm);

int bcm430x_switch_core(struct bcm430x_private *bcm, struct bcm430x_coreinfo *new_core);

void bcm430x_wireless_core_reset(struct bcm430x_private *bcm, int connect_phy);

int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset, u16 *val);
int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset, u32 *val);
int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset, u16 val);
int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset, u32 val);
