/*

  Broadcom BCM43xx InitVal file tool

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

#include "utils.h"
#include "bcm43xx_ival.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


static int big_endian_cpu;


void get_endianess(void)
{
	const unsigned char x[] = { 0xde, 0xad, 0xbe, 0xef, };
	const uint32_t *p = (uint32_t *)x;

	if (*p == 0xdeadbeef) {
		big_endian_cpu = 1;
	} else if (*p == 0xefbeadde) {
		big_endian_cpu = 0;
	} else {
		prerror("Confused: NUXI endian machine??\n");
		exit(-1);
	}
}

uint16_t be16_to_cpu(uint16_t v)
{
	uint16_t ret = v;

	if (!big_endian_cpu) {
		ret = (v & 0x00FF) << 8;
		ret |= (v & 0xFF00) >> 8;
	}

	return ret;
}

uint32_t be32_to_cpu(uint32_t v)
{
	uint16_t ret = v;

	if (!big_endian_cpu) {
		ret = (v & 0x000000FF) << 24;
		ret |= (v & 0x0000FF00) << 8;
		ret |= (v & 0x00FF0000) >> 8;
		ret |= (v & 0xFF000000) >> 24;
	}

	return ret;
}

int prinfo(const char *fmt, ...)
{
	int ret;
	va_list va;

	if (!cmdargs.verbose)
		return 0;

	va_start(va, fmt);
	ret = vfprintf(stderr, fmt, va);
	va_end(va);

	return ret;
}

int prerror(const char *fmt, ...)
{
	int ret;
	va_list va;

	va_start(va, fmt);
	ret = vfprintf(stderr, fmt, va);
	va_end(va);

	return ret;
}

int prdata(const char *fmt, ...)
{
	int ret;
	va_list va;

	va_start(va, fmt);
	ret = vfprintf(stderr, fmt, va);
	va_end(va);

	return ret;
}

void internal_error(const char *message)
{
	prerror("Internal programming error: %s\n", message);
	exit(1);
}

static void oom(void)
{
	prerror("ERROR: Out of memory!\n"
		"Virtual memory exhausted. "
		"Please close some applications, "
		"add more RAM or SWAP space.\n");
	exit(1);
}

void * malloce(size_t size)
{
	void *ret;

	ret = malloc(size);
	if (!ret)
		oom();

	return ret;
}

void * realloce(void *ptr, size_t newsize)
{
	void *ret;

	ret = realloc(ptr, newsize);
	if (!ret)
		oom();

	return ret;
}
