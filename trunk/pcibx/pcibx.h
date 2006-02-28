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

#ifndef PCIBX_H_
#define PCIBX_H_

#include "utils.h"

#define VERSION		pcibx_stringify(VERSION_)

enum command_id {
	CMD_ON,
	CMD_OFF,
	CMD_PRINTBOARDID,
	CMD_PRINTFIRMREV,
	CMD_PRINTSTATUS,
	CMD_CLEARBITSTAT,
	CMD_AUX5,
	CMD_AUX33,
	//TODO
};

struct pcibx_command {
	enum command_id id;
	union {
		int boolean;
	} u;
};

struct cmdline_args {
	int verbose;

	unsigned short port;
	int is_PCI_1;

#define MAX_COMMAND	512
	struct pcibx_command commands[MAX_COMMAND];
	int nr_commands;
};
extern struct cmdline_args cmdargs;

#endif /* PCIBX_H_ */
