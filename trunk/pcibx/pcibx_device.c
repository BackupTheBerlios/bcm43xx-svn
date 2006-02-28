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
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>


static void udelay(unsigned int usecs)
{
	int err;
	struct timeval time, deadline;

	err = gettimeofday(&deadline, NULL);
	if (err)
		goto error;
	deadline.tv_usec += usecs;
	if (deadline.tv_usec >= 1000000) {
		deadline.tv_sec++;
		deadline.tv_usec -= 1000000;
	}

	while (1) {
		err = gettimeofday(&time, NULL);
		if (err)
			goto error;
		if (time.tv_sec < deadline.tv_sec)
			continue;
		if (time.tv_usec >= deadline.tv_usec)
			break;
	}
	return;
error:
	prerror("gettimeofday() failed with: %s\n",
		strerror(errno));
}

static void msleep(unsigned int msecs)
{
	int err;
	struct timespec time;

	time.tv_sec = 0;
	time.tv_nsec = msecs;
	time.tv_nsec *= 1000000;
	do {
		err = nanosleep(&time, &time);
	} while (err && errno == EINTR);
	if (err) {
		prerror("nanosleep() failed with: %s\n",
			strerror(errno));
	}
}

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

void pcibx_cmd_on(struct pcibx_device *dev)
{
	prsendinfo("PCIBX ON");
	pcibx_write(dev, PCIBX_REG_GLOBALPWR, 1);
	pcibx_write(dev, PCIBX_REG_UUTVOLT, 0);
	msleep(200);
}

void pcibx_cmd_off(struct pcibx_device *dev)
{
	prsendinfo("PCIBX OFF");
	pcibx_write(dev, PCIBX_REG_UUTVOLT, 1);
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
		pcibx_write(dev, PCIBX_REG_AUX5, 0);
	} else {
		prsendinfo("Aux 5V OFF");
		pribc_write(dev, PCIBX_REG_AUX5, 1);
	}
}

void pcibx_cmd_aux33(struct pcibx_device *dev, int on)
{
	if (on) {
		prsendinfo("Aux 3.3V ON");
		pcibx_write(dev, PCIBX_REG_AUX33, 0);
	} else {
		prsendinfo("Aux 3.3V OFF");
		pribc_write(dev, PCIBX_REG_AUX33, 1);
	}
}
