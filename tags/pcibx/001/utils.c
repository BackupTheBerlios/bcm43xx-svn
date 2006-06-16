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

#include "utils.h"
#include "pcibx.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>


void udelay(unsigned int usecs)
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

void msleep(unsigned int msecs)
{
	int err;
	struct timespec time;

	time.tv_sec = 0;
	while (msecs >= 1000) {
		time.tv_sec++;
		msecs -= 1000;
	}
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
