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

#define PCIBX_REG_FIRMREV		0x50
#define PCIBX_REG_BOARDID		0x53
#define PCIBX_REG_GLOBALPWR		0x63
#define PCIBX_REG_UUTVOLT		0x64
#define PCIBX_REG_AUX5V			0x65
#define PCIBX_REG_AUX33V		0x66
#define PCIBX_REG_MEASURE_CTL		0x67
#define PCIBX_REG_MEASURE_CONV		0x68
#define PCIBX_REG_MEASURE_STROBE	0x69
#define PCIBX_REG_MEASURE_DATA0		0x6A
#define PCIBX_REG_MEASURE_DATA1		0x6B
#define PCIBX_REG_RST_0			0x71
#define PCIBX_REG_RST_1			0x72
#define PCIBX_REG_RST_2			0x73
#define PCIBX_REG_STATUS		0x76
#define PCIBX_REG_CLEARBITSTAT		0x77
#define PCIBX_REG_FREQMEASURE_CTL	0x7B
#define PCIBX_REG_FREQMEASURE_0		0x78
#define PCIBX_REG_FREQMEASURE_1		0x79
#define PCIBX_REG_FREQMEASURE_2		0x7A
#define PCIBX_REG_RAMP			0x7C

#define PCIBX_REGOFFSET_PCI1	0x00
#define PCIBX_REGOFFSET_PCI2	0x80

#define PCIBX_STATUS_RSTDEASS	(1 << 0)
#define PCIBX_STATUS_64BIT	(1 << 1)
#define PCIBX_STATUS_32BIT	(1 << 2)
#define PCIBX_STATUS_MHZ	(1 << 3)
#define PCIBX_STATUS_DUTASS	(1 << 4)

struct pcibx_device {
	unsigned short port;
	uint8_t regoffset;
};

enum measure_id {
	MEASURE_V25REF	= 0x08,
	MEASURE_V12UUT	= 0x09,
	MEASURE_V5UUT	= 0x0A,
	MEASURE_V33UUT	= 0x0B,
	MEASURE_V5AUX	= 0x0C,
	MEASURE_A5	= 0x0D,
	MEASURE_A12	= 0x0E,
	MEASURE_A33	= 0x0F,
};

void pcibx_cmd_global_pwr(struct pcibx_device *dev, int on);
void pcibx_cmd_uut_pwr(struct pcibx_device *dev, int on);
uint8_t pcibx_cmd_getboardid(struct pcibx_device *dev);
uint8_t pcibx_cmd_getfirmrev(struct pcibx_device *dev);
uint8_t pcibx_cmd_getstatus(struct pcibx_device *dev);
void pcibx_cmd_clearbitstat(struct pcibx_device *dev);
void pcibx_cmd_aux5(struct pcibx_device *dev, int on);
void pcibx_cmd_aux33(struct pcibx_device *dev, int on);
float pcibx_cmd_sysfreq(struct pcibx_device *dev);
float pcibx_cmd_measure(struct pcibx_device *dev, enum measure_id id);
void pcibx_cmd_ramp(struct pcibx_device *dev, int fast);
void pcibx_cmd_rst(struct pcibx_device *dev, double sec);
void pcibx_cmd_rstdefault(struct pcibx_device *dev);

#endif /* PCIBX_DEVICE_H_ */
