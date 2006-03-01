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

#include "pcibx_device.h"
#include "pcibx.h"
#include "utils.h"

#include <string.h>
#include <sys/io.h>
#include <errno.h>
#include <unistd.h>


static void pcibx_set_address(struct pcibx_device *dev,
			      uint8_t address)
{
	outb(0xDE, dev->port + 2);
	outb(address + dev->regoffset, dev->port);
	outb(0xD6, dev->port + 2);
	udelay(100);
	outb(0xDE, dev->port + 2);
}

static void pcibx_write_data(struct pcibx_device *dev,
			     uint8_t data)
{
	outb(data, dev->port);
	outb(0xDC, dev->port + 2);
	udelay(100);
	outb(0xDE, dev->port + 2);
}

static void pcibx_write_data_ext(struct pcibx_device *dev,
				 uint8_t data)
{
	outb(0xDE, dev->port + 2);
	outb(data, dev->port);
	outb(0xDC, dev->port + 2);
	msleep(2);
	outb(0xDE, dev->port + 2);
}

static uint8_t pcibx_read_data(struct pcibx_device *dev)
{
	uint8_t v;

	outb(0xFF, dev->port + 2);
	v = inb(dev->port);
	outb(0xDE, dev->port + 2);

	return v;
}

static void pcibx_write(struct pcibx_device *dev,
			uint8_t reg,
			uint8_t value)
{
	pcibx_set_address(dev, reg);
	pcibx_write_data(dev, value);
}

static void pcibx_write_ext(struct pcibx_device *dev,
			    uint8_t reg,
			    uint8_t value)
{
	pcibx_set_address(dev, reg);
	pcibx_write_data_ext(dev, value);
}

static uint8_t pcibx_read(struct pcibx_device *dev,
			       uint8_t reg)
{
	pcibx_set_address(dev, reg);
	return pcibx_read_data(dev);
}

static void prsendinfo(const char *command)
{
	if (cmdargs.verbose)
		prinfo("Sending command: %s\n", command);
}

void pcibx_cmd_global_pwr(struct pcibx_device *dev, int on)
{
	if (on) {
		prsendinfo("Global Power ON");
		pcibx_write(dev, PCIBX_REG_GLOBALPWR, 1);
	} else {
		prsendinfo("Global Power OFF");
		pcibx_write(dev, PCIBX_REG_GLOBALPWR, 0);
	}
}

void pcibx_cmd_uut_pwr(struct pcibx_device *dev, int on)
{
	if (on) {
		pcibx_cmd_global_pwr(dev, 1);
		prsendinfo("UUT Voltages ON");
		pcibx_write(dev, PCIBX_REG_UUTVOLT, 0);
		/* Wait for the RST# to become de-asserted. */
		do {
			msleep(200);
		} while (!(pcibx_read(dev, PCIBX_REG_STATUS) & PCIBX_STATUS_RSTDEASS));
	} else {
		prsendinfo("UUT Voltages OFF");
		pcibx_write(dev, PCIBX_REG_UUTVOLT, 1);
	}
}

uint8_t pcibx_cmd_getboardid(struct pcibx_device *dev)
{
	prsendinfo("Get board ID");
	return pcibx_read(dev, PCIBX_REG_BOARDID);
}

uint8_t pcibx_cmd_getfirmrev(struct pcibx_device *dev)
{
	prsendinfo("Get firmware rev");
	return pcibx_read(dev, PCIBX_REG_FIRMREV);
}

uint8_t pcibx_cmd_getstatus(struct pcibx_device *dev)
{
	prsendinfo("Get status bits");
	return pcibx_read(dev, PCIBX_REG_STATUS);
}

void pcibx_cmd_clearbitstat(struct pcibx_device *dev)
{
	prsendinfo("Clear 32/64 bit status");
	pcibx_write(dev, PCIBX_REG_CLEARBITSTAT, 0);
}

void pcibx_cmd_aux5(struct pcibx_device *dev, int on)
{
	if (on) {
		prsendinfo("Aux 5V ON");
		pcibx_write(dev, PCIBX_REG_AUX5V, 0);
	} else {
		prsendinfo("Aux 5V OFF");
		pcibx_write(dev, PCIBX_REG_AUX5V, 1);
	}
}

void pcibx_cmd_aux33(struct pcibx_device *dev, int on)
{
	if (on) {
		prsendinfo("Aux 3.3V ON");
		pcibx_write(dev, PCIBX_REG_AUX33V, 0);
	} else {
		prsendinfo("Aux 3.3V OFF");
		pcibx_write(dev, PCIBX_REG_AUX33V, 1);
	}
}

float pcibx_cmd_sysfreq(struct pcibx_device *dev)
{
	float mhz;
	uint32_t tmp;
	uint32_t v;

	prsendinfo("Measure system frequency");
	pcibx_write(dev, PCIBX_REG_FREQMEASURE_CTL, 1);
	msleep(15);
	v = pcibx_read(dev, PCIBX_REG_FREQMEASURE_0);
	tmp = v;
	v = pcibx_read(dev, PCIBX_REG_FREQMEASURE_1);
	tmp |= (v << 8);
	v = pcibx_read(dev, PCIBX_REG_FREQMEASURE_2);
	tmp |= (v << 16);

	mhz = tmp * 100 / 1048575;

	return mhz;
}

float pcibx_cmd_measure(struct pcibx_device *dev, enum measure_id id)
{
	float ret;
	int i;
	uint16_t d0, d1;
	uint16_t tmp;

	prsendinfo("Measuring V/A");
	pcibx_write(dev, PCIBX_REG_MEASURE_CTL, id);
	msleep(10);
	pcibx_write_ext(dev, PCIBX_REG_MEASURE_CONV, 0);
	msleep(2);
	pcibx_set_address(dev, PCIBX_REG_MEASURE_STROBE);
	for (i = 0; i < 13; i++)
		pcibx_write_data(dev, 0);
	d0 = pcibx_read(dev, PCIBX_REG_MEASURE_DATA0);
	d1 = pcibx_read(dev, PCIBX_REG_MEASURE_DATA1);
	tmp = d0;
	tmp |= (d1 << 8);

	if (id == MEASURE_V12UUT)
		ret = tmp * 5.75 * 2.5 / 4096;
	else
		ret = tmp * 2.26 * 2.5 / 4096;

	return ret;
}

void pcibx_cmd_ramp(struct pcibx_device *dev, int fast)
{
	prsendinfo("+5V RAMP");
	pcibx_write(dev, PCIBX_REG_RAMP, fast ? 1 : 0);
}

void pcibx_cmd_rst(struct pcibx_device *dev, double sec)
{
	uint32_t tmp;

	prsendinfo("RST#");
	sec /= 2.56;
	sec *= 1000000;
	tmp = sec;
	tmp &= 0x1FFFFF;
	tmp |= (1 << 23);
	pcibx_write(dev, PCIBX_REG_RST_0, (tmp & 0x000000FF));
	pcibx_write(dev, PCIBX_REG_RST_1, (tmp & 0x0000FF00) >> 8);
	pcibx_write(dev, PCIBX_REG_RST_2, (tmp & 0x00FF0000) >> 16);
}

void pcibx_cmd_rstdefault(struct pcibx_device *dev)
{
	prsendinfo("default RST#");
	pcibx_write(dev, PCIBX_REG_RST_0, 0);
	pcibx_write(dev, PCIBX_REG_RST_1, 0);
	pcibx_write(dev, PCIBX_REG_RST_2, 0);
}
