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

#include "pcibx.h"
#include "pcibx_device.h"

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/io.h>


struct cmdline_args cmdargs;


static int send_commands(struct pcibx_device *dev)
{
	struct pcibx_command *cmd;
	uint8_t v;
	int i;

	for (i = 0; i < cmdargs.nr_commands; i++) {
		cmd = &(cmdargs.commands[i]);

		switch (cmd->id) {
		case CMD_ON:
			pcibx_cmd_on(dev);
			break;
		case CMD_OFF:
			pcibx_cmd_off(dev);
			break;
		case CMD_PRINTBOARDID:
			v = pcibx_cmd_getboardid(dev);
			prdata("Board ID: 0x%02X\n", v);
			break;
		case CMD_PRINTFIRMREV:
			v = pcibx_cmd_getfirmrev(dev);
			prdata("Firmware revision: 0x%02X\n", v);
			break;
		case CMD_PRINTSTATUS:
			v = pcibx_cmd_getstatus(dev);
			prdata("Board status:  %s;  %s;  %s;  %s;  %s\n",
			       (v & PCIBX_STATUS_RSTDEASS) ? "RST# de-asserted"
							   : "RST# asserted",
			       (v & PCIBX_STATUS_64BIT) ? "64-bit operation established"
							: "No 64-bit handshake detected",
			       (v & PCIBX_STATUS_32BIT) ? "32-bit operation established"
							: "No 32-bit handshake detected",
			       (v & PCIBX_STATUS_MHZ) ? "66 Mhz enabled slot"
						      : "33 Mhz enabled slot",
			       (v & PCIBX_STATUS_DUTASS) ? "DUT asserted"
							 : "DUT not fully asserted");
			break;
		case CMD_CLEARBITSTAT:
			pcibx_cmd_clearbitstat(dev);
			break;
		case CMD_AUX5:
			pcibx_cmd_aux5(dev, cmd->u.boolean);
			break;
		case CMD_AUX33:
			pcibx_cmd_aux33(dev, cmd->u.boolean);
			break;
		default:
			internal_error("invalid command");
			return -1;
		}
	}
	prinfo("All commands sent.\n");

	return 0;
}

static int init_device(struct pcibx_device *dev)
{
	memset(dev, 0, sizeof(*dev));
	dev->port = cmdargs.port;
	if (cmdargs.is_PCI_1)
		dev->regoffset = PCIBX_REGOFFSET_PCI1;
	else
		dev->regoffset = PCIBX_REGOFFSET_PCI2;

	return 0;
}

static int request_priority(void)
{//TODO
	return 0;
}

static int request_permissions(void)
{
	int err;

	err = ioperm(cmdargs.port, 3, 1);
	if (err) {
		printf("Could not aquire I/O permissions for "
		       "port 0x%03X.\n", cmdargs.port);
	}

	return err;
}

static void print_banner(int forceprint)
{
	const char *str = "Catalyst PCIBX32 PCI Extender control utility version " VERSION "\n";
	if (forceprint)
		prdata(str);
	else
		prinfo(str);
}

static void print_usage(int argc, char **argv)
{
	print_banner(1);
	prdata("\nUsage: %s [OPTION]\n", argv[0]);
	prdata("  -V|--verbose          Be verbose\n");
	prdata("  -v|--version          Print version\n");
	prdata("  -h|--help             Print this help\n");
	prdata("\n");
	prdata("  -p|--port 0x378       Port base address (Default: 0x378)\n");
	prdata("  -P|--pci1 BOOL        If true, PCI_1 (default), otherwise PCI_2. (See JP15)\n");
	prdata("\nDevice commands\n");
	prdata("  --cmd-on              Turn the device ON\n");
	prdata("  --cmd-off             Turn the device OFF\n");
	prdata("  --cmd-printboardid    Print the Board ID\n");
	prdata("  --cmd-printfirmrev    Print the Firmware revision\n");
	prdata("  --cmd-printstatus     Print the Board Status Bits\n");
	prdata("  --cmd-clearbitstat    Clear 32/64 bit status\n");
	prdata("  --cmd-aux5 ON/OFF     Turn +5V Aux ON or OFF\n");
	prdata("  --cmd-aux33 ON/OFF    Turn +3.3V Aux ON or OFF\n");
}

#define ARG_MATCH		0
#define ARG_NOMATCH		1
#define ARG_ERROR		-1

static int do_cmp_arg(char **argv, int *pos,
		      const char *template,
		      int allow_merged,
		      char **param)
{
	char *arg;
	char *next_arg;
	size_t arg_len, template_len;

	arg = argv[*pos];
	next_arg = argv[*pos + 1];
	arg_len = strlen(arg);
	template_len = strlen(template);

	if (param) {
		/* Maybe we have a merged parameter here.
		 * A merged parameter is "-pfoobar" for example.
		 */
		if (allow_merged && arg_len > template_len) {
			if (memcmp(arg, template, template_len) == 0) {
				*param = arg + template_len;
				return ARG_MATCH;
			}
			return ARG_NOMATCH;
		} else if (arg_len != template_len)
			return ARG_NOMATCH;
		*param = next_arg;
	}
	if (strcmp(arg, template) == 0) {
		if (param) {
			/* Skip the parameter on the next iteration. */
			(*pos)++;
			if (*param == 0) {
				prerror("%s needs a parameter\n", arg);
				return ARG_ERROR;
			}
		}
		return ARG_MATCH;
	}

	return ARG_NOMATCH;
}

/* Simple and lean command line argument parsing. */
static int cmp_arg(char **argv, int *pos,
		   const char *long_template,
		   const char *short_template,
		   char **param)
{
	int err;

	if (long_template) {
		err = do_cmp_arg(argv, pos, long_template, 0, param);
		if (err == ARG_MATCH || err == ARG_ERROR)
			return err;
	}
	err = ARG_NOMATCH;
	if (short_template)
		err = do_cmp_arg(argv, pos, short_template, 1, param);
	return err;
}
#define arg_match(argv, i, tlong, tshort, param) \
	({						\
		int res = cmp_arg((argv), (i), (tlong),	\
				  (tshort), (param));	\
		if ((res) == ARG_ERROR)			\
	 		goto error;			\
		((res) == ARG_MATCH);			\
	})

static int parse_hexval(const char *str,
			uint32_t *value,
			const char *param)
{
	uint32_t v;

	if (strncmp(str, "0x", 2) != 0)
		goto error;
	str += 2;
	errno = 0;
	v = strtoul(str, NULL, 16);
	if (errno)
		goto error;
	*value = v;

	return 0;
error:
	if (param) {
		prerror("%s value parsing error. Format: 0xFFFFFFFF\n",
			param);
	}
	return -1;
}

static int parse_bool(const char *str,
		      const char *param)
{
	if (strcmp(str, "1") == 0)
		return 1;
	if (strcmp(str, "0") == 0)
		return 0;
	if (strcmp(str, "true") == 0)
		return 1;
	if (strcmp(str, "false") == 0)
		return 0;
	if (strcmp(str, "yes") == 0)
		return 1;
	if (strcmp(str, "no") == 0)
		return 0;
	if (strcmp(str, "on") == 0)
		return 1;
	if (strcmp(str, "off") == 0)
		return 0;

	if (param) {
		prerror("%s boolean parsing error. Format: BOOL\n",
			param);
	}

	return -1;
}

static int add_command(enum command_id cmd)
{
	if (cmdargs.nr_commands == MAX_COMMAND) {
		prerror("Maximum number of commands exceed.\n");
		return -1;
	}
	cmdargs.commands[cmdargs.nr_commands++].id = cmd;

	return 0;
}

static int add_boolcommand(enum command_id cmd,
			   const char *str,
			   const char *param)
{
	int boolean;

	if (cmdargs.nr_commands == MAX_COMMAND) {
		prerror("Maximum number of commands exceed.\n");
		return -1;
	}

	boolean = parse_bool(str, param);
	if (boolean < 0)
		return -1;
	cmdargs.commands[cmdargs.nr_commands].id = cmd;
	cmdargs.commands[cmdargs.nr_commands].u.boolean = !!boolean;
	cmdargs.nr_commands++;

	return 0;
}

static int parse_args(int argc, char **argv)
{
	int i, err;
	char *param;
	uint32_t value;

	cmdargs.port = 0x378;
	cmdargs.is_PCI_1 = 1;

	for (i = 1; i < argc; i++) {
		if (arg_match(argv, &i, "--version", "-v", 0)) {
			print_banner(1);
			return 1;
		} else if (arg_match(argv, &i, "--help", "-h", 0)) {
			goto out_usage;
		} else if (arg_match(argv, &i, "--verbose", "-V", 0)) {
			cmdargs.verbose = 1;
		} else if (arg_match(argv, &i, "--port", "-p", &param)) {
			err = parse_hexval(param, &value, "--port");
			if (err < 0)
				goto error;
			cmdargs.port = value;
		} else if (arg_match(argv, &i, "--pci1", "-P", &param)) {
			err = parse_bool(param, "--pci1");
			if (err < 0)
				goto error;
			cmdargs.is_PCI_1 = !!err;

		} else if (arg_match(argv, &i, "--cmd-on", 0, 0)) {
			err = add_command(CMD_ON);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-off", 0, 0)) {
			err = add_command(CMD_OFF);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-printboardid", 0, 0)) {
			err = add_command(CMD_PRINTBOARDID);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-printfirmrev", 0, 0)) {
			err = add_command(CMD_PRINTFIRMREV);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-printstatus", 0, 0)) {
			err = add_command(CMD_PRINTSTATUS);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-clearbitstat", 0, 0)) {
			err = add_command(CMD_CLEARBITSTAT);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-aux5", 0, &param)) {
			err = add_boolcommand(CMD_AUX5, param, "--cmd-aux5");
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-aux33", 0, &param)) {
			err = add_boolcommand(CMD_AUX33, param, "--cmd-aux33");
			if (err)
				goto error;
		} else {
			prerror("Unrecognized argument: %s\n", argv[i]);
			goto out_usage;
		}
	}
	if (cmdargs.nr_commands == 0) {
		prerror("No device commands specified.\n");
		goto error;
	}
	return 0;

out_usage:
	print_usage(argc, argv);
error:
	return -1;
}

int main(int argc, char **argv)
{
	struct pcibx_device dev;
	int err;

	err = parse_args(argc, argv);
	if (err == 1)
		return 0;
	else if (err != 0)
		goto out;
	print_banner(0);

	err = request_permissions();
	if (err)
		goto out;
	err = request_priority();
	if (err)
		goto out;

	err = init_device(&dev);
	if (err)
		goto out;
	err = send_commands(&dev);
	if (err)
		goto out;

out:
	return err;
}
