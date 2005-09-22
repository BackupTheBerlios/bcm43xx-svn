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


struct bcm430x_private;


u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset);
void bcm430x_write16(struct bcm430x_private *bcm, u16 offset, u16 val);

u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset);
void bcm430x_write32(struct bcm430x_private *bcm, u16 offset, u32 val);

void bcm430x_shm_control(struct bcm430x_private *bcm, u32 control);
u16 bcm430x_shm_read16(struct bcm430x_private *bcm);
void bcm430x_shm_write16(struct bcm430x_private *bcm, u16 val);
u32 bcm430x_shm_read32(struct bcm430x_private *bcm);
void bcm430x_shm_write32(struct bcm430x_private *bcm, u32 val);

int bcm430x_dummy_transmission(struct bcm430x_private *bcm);

int bcm430x_switch_core(struct bcm430x_private *bcm, struct bcm430x_coreinfo *new_core);

void bcm430x_wireless_core_reset(struct bcm430x_private *bcm, int connect_phy);

int bcm430x_pci_read_config_8(struct pci_dev *pdev, u16 offset, u8 * val);
int bcm430x_pci_read_config_16(struct pci_dev *pdev, u16 offset, u16 * val);
int bcm430x_pci_read_config_32(struct pci_dev *pdev, u16 offset, u32 * val);

int bcm430x_pci_write_config_8(struct pci_dev *pdev, int offset, u8 val);
int bcm430x_pci_write_config_16(struct pci_dev *pdev, int offset, u16 val);
int bcm430x_pci_write_config_32(struct pci_dev *pdev, int offset, u32 val);
