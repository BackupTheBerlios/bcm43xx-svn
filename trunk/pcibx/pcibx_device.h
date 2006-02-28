/*

  Catalyst PCIBX32 PCI Extender control utility

  Copyright (c) 2006 Michael Buesch <mbuesch@freenet.de>

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

#ifndef PCIBX_DEVICE_H_
#define PCIBX_DEVICE_H_

#include <stdint.h>

#define PCIBX_REG_FIRMREV	0x50
#define PCIBX_REG_BOARDID	0x53
#define PCIBX_REG_GLOBALPWR	0x63
#define PCIBX_REG_UUTVOLT	0x64
#define PCIBX_REG_AUX5V		0x65
#define PCIBX_REG_AUX33V	0x66
#define PCIBX_REG_STATUS	0x76
#define PCIBX_REG_CLEARBITSTAT	0x77
//TODO

#define PCIBX_REGOFFSET_PCI1	0x00
#define PCIBX_REGOFFSET_PCI2	0x80

#define PCIBX_STATUS_RSTDEASS	(1 << 0)
#define PCIBX_STATUS_64BIT	(1 << 1)
#define PCIBX_STATUS_32BIT	(1 << 2)
#define PCIBX_STATUS_MHZ	(1 << 3)
#define PCIBX_STATUS_DUTASS	(1 << 4)

struct pcibx_device {
	unsigned short port;
	unsigned short regoffset;
};

void pcibx_cmd_on(struct pcibx_device *dev);
void pcibx_cmd_off(struct pcibx_device *dev);
uint8_t pcibx_cmd_getboardid(struct pcibx_device *dev);
uint8_t pcibx_cmd_getfirmrev(struct pcibx_device *dev);
uint8_t pcibx_cmd_getstatus(struct pcibx_device *dev);
void pcibx_cmd_clearbitstat(struct pcibx_device *dev);
void pcibx_cmd_aux5(struct pcibx_device *dev, int on);
void pcibx_cmd_aux33(struct pcibx_device *dev, int on);

#endif /* PCIBX_DEVICE_H_ */
