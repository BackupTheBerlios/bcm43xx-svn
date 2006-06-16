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
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>


struct cmdline_args cmdargs;
struct timeval starttime;


/* Subtract the `struct timeval' values X and Y,
 * storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 *
 * Taken from the glibc docs, so should be
 * Copyright (c) Free Software Foundation
 */
static int timeval_subtract(struct timeval *result,
			    struct timeval *x,
			    struct timeval *y)
{
	int nsec;

	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait.
	 * tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/* Helper function to plot informational text. */
static void plot_text(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
static void plot_text(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "# ");
	vfprintf(stderr, fmt, va);
	va_end(va);
}

/* Helper function to plot data values. */
static void plot_data(const char *text, float data)
{
	struct timeval tmp, tv;
	int err;

	err = gettimeofday(&tmp, NULL);
	if (err) {
		prdata("gettimeofday() failure!\n");
		return;
	}
	err = timeval_subtract(&tv, &tmp, &starttime);
	if (err) {
		prdata("timeval_subtract() went negative!\n");
		return;
	}
	prdata("%ld.%06ld %f  # ",
	       tv.tv_sec, tv.tv_usec,
	       data);
	prdata(text, data);
}

static int send_commands(struct pcibx_device *dev)
{
	struct pcibx_command *cmd;
	uint8_t v;
	float f;
	int i;

	for (i = 0; i < cmdargs.nr_commands; i++) {
		cmd = &(cmdargs.commands[i]);

		switch (cmd->id) {
		case CMD_GLOB:
			pcibx_cmd_global_pwr(dev, cmd->u.boolean);
			break;
		case CMD_UUT:
			pcibx_cmd_uut_pwr(dev, cmd->u.boolean);
			break;
		case CMD_PRINTBOARDID:
			v = pcibx_cmd_getboardid(dev);
			plot_text("Board ID: 0x%02X\n", v);
			break;
		case CMD_PRINTFIRMREV:
			v = pcibx_cmd_getfirmrev(dev);
			plot_text("Firmware revision: 0x%02X\n", v);
			break;
		case CMD_PRINTSTATUS:
			v = pcibx_cmd_getstatus(dev);
			plot_text("Board status:  %s;  %s;  %s;  %s;  %s\n",
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
		case CMD_MEASUREFREQ:
			f = pcibx_cmd_sysfreq(dev);
			plot_data("Measured system frequency: %f Mhz\n", f);
			break;
		case CMD_MEASUREV25REF:
			f = pcibx_cmd_measure(dev, MEASURE_V25REF);
			plot_data("Measured +2.5V Reference: %f Volt\n", f);
			break;
		case CMD_MEASUREV12UUT:
			f = pcibx_cmd_measure(dev, MEASURE_V12UUT);
			plot_data("Measured +12V UUT: %f Volt\n", f);
			break;
		case CMD_MEASUREV5UUT:
			f = pcibx_cmd_measure(dev, MEASURE_V5UUT);
			plot_data("Measured +5V UUT: %f Volt\n", f);
			break;
		case CMD_MEASUREV33UUT:
			f = pcibx_cmd_measure(dev, MEASURE_V33UUT);
			plot_data("Measured +33V UUT: %f Volt\n", f);
			break;
		case CMD_MEASUREV5AUX:
			f = pcibx_cmd_measure(dev, MEASURE_V5AUX);
			plot_data("Measured +5V AUX: %f Volt\n", f);
			break;
		case CMD_MEASUREA5:
			f = pcibx_cmd_measure(dev, MEASURE_A5);
			plot_data("Measured +5V Current: %f Ampere\n", f);
			break;
		case CMD_MEASUREA12:
			f = pcibx_cmd_measure(dev, MEASURE_A12);
			plot_data("Measured +12V Current: %f Ampere\n", f);
			break;
		case CMD_MEASUREA33:
			f = pcibx_cmd_measure(dev, MEASURE_A33);
			plot_data("Measured +3.3V Current: %f Ampere\n", f);
			break;
		case CMD_FASTRAMP:
			pcibx_cmd_ramp(dev, cmd->u.boolean);
			break;
		case CMD_RST:
			pcibx_cmd_rst(dev, cmd->u.d);
			break;
		case CMD_RSTDEFAULT:
			pcibx_cmd_rstdefault(dev);
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
{
	struct sched_param param;
	int err;

	param.sched_priority = sched_get_priority_max(cmdargs.sched);
	err = sched_setscheduler(0, cmdargs.sched, &param);
	if (err) {
		prerror("Could not set scheduling policy (%s).\n",
			strerror(errno));
	}

	return err;
}

static int request_permissions(void)
{
	int err;

	err = ioperm(cmdargs.port, 1, 1);
	if (err)
		goto error;
	err = ioperm(cmdargs.port + 2, 1, 1);
	if (err)
		goto error;

	return err;
error:
	prerror("Could not aquire I/O permissions for "
		"port 0x%03X.\n", cmdargs.port);
	return err;
}

static void print_banner(int forceprint)
{
	const char *str =
		"Catalyst PCIBX32 PCI Extender control utility version " VERSION "\n"
		"\n"
		"Copyright 2006 Michael Buesch <mbuesch@freenet.de>\n"
		"Licensed under the GNU General Public License v2+\n"
		"\n";
	if (forceprint)
		prdata(str);
	else
		prinfo(str);
}

static void print_usage(int argc, char **argv)
{
	print_banner(1);
	prdata("Usage: %s [OPTION]\n", argv[0]);
	prdata("  -V|--verbose          Be verbose\n");
	prdata("  -v|--version          Print version\n");
	prdata("  -h|--help             Print this help\n");
	prdata("\n");
	prdata("  -p|--port 0x378       Port base address (Default: 0x378)\n");
	prdata("  -P|--pci1 BOOL        If true, PCI_1 (default), otherwise PCI_2. (See JP15)\n");
	prdata("  -s|--sched POLICY     Scheduling policy (normal, fifo, rr)\n");
	prdata("  -c|--cycle DELAY      Execute the commands in a cycle and delay\n");
	prdata("                        DELAY msecs after each cycle\n");
	prdata("  -n|--nrcycle COUNT    Cycle COUNT times. 0 == infinite (default)\n");
	prdata("\n");
	prdata("Device commands\n");
	prdata("  --cmd-glob ON/OFF     Turn Global power ON/OFF (does not turn ON UUT Voltages)\n");
	prdata("  --cmd-uut ON/OFF      Turn UUT Voltages ON/OFF (also turns Global power ON)\n");
	prdata("  --cmd-printboardid    Print the Board ID\n");
	prdata("  --cmd-printfirmrev    Print the Firmware revision\n");
	prdata("  --cmd-printstatus     Print the Board Status Bits\n");
	prdata("  --cmd-clearbitstat    Clear 32/64 bit status\n");
	prdata("  --cmd-aux5 ON/OFF     Turn +5V Aux ON or OFF\n");
	prdata("  --cmd-aux33 ON/OFF    Turn +3.3V Aux ON or OFF\n");
	prdata("  --cmd-measurefreq     Measure system frequency\n");
	prdata("  --cmd-measurev25ref   Measure +2.5V Reference\n");
	prdata("  --cmd-measurev12uut   Measure +12V UUT\n");
	prdata("  --cmd-measurev5uut    Measure +5V UUT\n");
	prdata("  --cmd-measurev33uut   Measure +3.3V UUT\n");
	prdata("  --cmd-measurev5aux    Measure +5V AUX\n");
	prdata("  --cmd-measurea5       Measure +5V Current\n");
	prdata("  --cmd-measurea12      Measure +12V Current\n");
	prdata("  --cmd-measurea33      Measure +3.3V Current\n");
	prdata("  --cmd-fastramp ON/OFF Select slow/fast +5V ramp\n");
	prdata("  --cmd-rst 0.150       Set RST# (reset) delay (in seconds)\n");
	prdata("  --cmd-rstdefault      Set RST# to default (150msec)\n");
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
	if (strcasecmp(str, "true") == 0)
		return 1;
	if (strcasecmp(str, "false") == 0)
		return 0;
	if (strcasecmp(str, "yes") == 0)
		return 1;
	if (strcasecmp(str, "no") == 0)
		return 0;
	if (strcasecmp(str, "on") == 0)
		return 1;
	if (strcasecmp(str, "off") == 0)
		return 0;

	if (param) {
		prerror("%s boolean parsing error. Format: BOOL\n",
			param);
	}

	return -1;
}

static int parse_double(const char *str,
			double *value,
			const char *param)
{
	double v;

	errno = 0;
	v = strtod(str, NULL);
	if (errno)
		goto error;
	*value = v;

	return 0;
error:
	if (param) {
		prerror("%s value parsing error. Format: 0.00123\n",
			param);
	}
	return -1;
}

static int parse_int(const char *str,
		     int *value,
		     const char *param)
{
	int v;

	errno = 0;
	v = strtol(str, NULL, 10);
	if (errno)
		goto error;
	*value = v;

	return 0;
error:
	if (param) {
		prerror("%s value parsing error. Format: 1234\n",
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

static int add_doublecommand(enum command_id cmd,
			     const char *str,
			     const char *param)
{
	int err;
	double value;

	if (cmdargs.nr_commands == MAX_COMMAND) {
		prerror("Maximum number of commands exceed.\n");
		return -1;
	}

	err = parse_double(str, &value, param);
	if (err)
		return err;
	cmdargs.commands[cmdargs.nr_commands].id = cmd;
	cmdargs.commands[cmdargs.nr_commands].u.d = value;
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
	cmdargs.sched = SCHED_OTHER;
	cmdargs.cycle = -1;

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
		} else if (arg_match(argv, &i, "--sched", "-s", &param)) {
			if (strcasecmp(param, "normal") == 0)
				cmdargs.sched = SCHED_OTHER;
			else if (strcasecmp(param, "fifo") == 0)
				cmdargs.sched = SCHED_FIFO;
			else if (strcasecmp(param, "rr") == 0)
				cmdargs.sched = SCHED_RR;
			else {
				prerror("Invalid parameter to --sched\n");
				goto error;
			}
		} else if (arg_match(argv, &i, "--cycle", "-c", &param)) {
			err = parse_int(param, &cmdargs.cycle, "--cycle");
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--nrcycle", "-n", &param)) {
			err = parse_int(param, &cmdargs.nrcycle, "--nrcycle");
			if (err)
				goto error;

		} else if (arg_match(argv, &i, "--cmd-glob", 0, &param)) {
			err = add_boolcommand(CMD_GLOB, param, "--cmd-glob");
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-uut", 0, &param)) {
			err = add_boolcommand(CMD_UUT, param, "--cmd-uut");
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
		} else if (arg_match(argv, &i, "--cmd-measurefreq", 0, 0)) {
			err = add_command(CMD_MEASUREFREQ);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurev25ref", 0, 0)) {
			err = add_command(CMD_MEASUREV25REF);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurev12uut", 0, 0)) {
			err = add_command(CMD_MEASUREV12UUT);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurev5uut", 0, 0)) {
			err = add_command(CMD_MEASUREV5UUT);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurev33uut", 0, 0)) {
			err = add_command(CMD_MEASUREV33UUT);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurev5aux", 0, 0)) {
			err = add_command(CMD_MEASUREV5AUX);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurea5", 0, 0)) {
			err = add_command(CMD_MEASUREA5);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurea12", 0, 0)) {
			err = add_command(CMD_MEASUREA12);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-measurea33", 0, 0)) {
			err = add_command(CMD_MEASUREA33);
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-fastramp", 0, &param)) {
			err = add_boolcommand(CMD_FASTRAMP, param, "--cmd-fastramp");
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-rst", 0, &param)) {
			err = add_doublecommand(CMD_RST, param, "--cmd-rst");
			if (err)
				goto error;
		} else if (arg_match(argv, &i, "--cmd-rstdefault", 0, 0)) {
			err = add_command(CMD_RSTDEFAULT);
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

static void signal_handler(int sig)
{
	prinfo("Signal %d received. Terminating.\n", sig);
	exit(1);
}

static int setup_sighandler(void)
{
	int err;
	struct sigaction sa;

	sa.sa_handler = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	err = sigaction(SIGINT, &sa, NULL);
	err |= sigaction(SIGTERM, &sa, NULL);
	if (err)
		prerror("sigaction setup failed.\n");

	return err;
}

int main(int argc, char **argv)
{
	struct pcibx_device dev;
	int err;
	int nrcycle;

	err = setup_sighandler();
	if (err)
		goto out;
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
	gettimeofday(&starttime, NULL);
	nrcycle = cmdargs.nrcycle;
	if (nrcycle == 0)
		nrcycle = -1;
	while (1) {
		err = send_commands(&dev);
		if (err)
			goto out;
		if (cmdargs.cycle < 0)
			break;
		if (nrcycle > 0)
			nrcycle--;
		if (nrcycle == 0)
			break;
		if (cmdargs.cycle)
			msleep(cmdargs.cycle);
	}

out:
	return err;
}
