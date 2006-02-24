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

#include "bcm43xx_ival.h"

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


struct cmdline_args cmdargs;


#define MAX_DESC	512

static void get_description(char *desc,
			    uint16_t offset,
			    uint16_t size,
			    uint32_t value)
{
	switch (offset) {
	case 0x18:
		strcpy(desc, "Cram (>= rev3)");
		break;
	case 0x20:
		strcpy(desc, "DMA/PIO IRQ Reason 1");
		break;
	case 0x24:
		strcpy(desc, "DMA/PIO IRQ Mask 1");
		break;
	case 0x28:
		strcpy(desc, "DMA/PIO IRQ Reason 2");
		break;
	case 0x2C:
		strcpy(desc, "DMA/PIO IRQ Mask 2");
		break;
	case 0x30:
		strcpy(desc, "DMA/PIO IRQ Reason 3");
		break;
	case 0x34:
		strcpy(desc, "DMA/PIO IRQ Mask 3");
		break;
	case 0x38:
		strcpy(desc, "DMA/PIO IRQ Reason 4");
		break;
	case 0x3C:
		strcpy(desc, "DMA/PIO IRQ Mask 4");
		break;
	case 0x120:
		strcpy(desc, "Status BitField");
		break;
	case 0x124:
		strcpy(desc, "Reg124BitField");
		break;
	case 0x128:
		strcpy(desc, "Generic IRQ Reason");
		break;
	case 0x12C:
		strcpy(desc, "Generic IRQ Mask");
		break;
	case 0x130:
		strcpy(desc, "Template RAM Address");
		break;
	case 0x134:
		strcpy(desc, "Template RAM Data");
		break;
	case 0x140:
		strcpy(desc, "PowerSave status BitField");
		break;
	case 0x144:
		strcpy(desc, "PMQ MAC");
		break;
	case 0x158:
		strcpy(desc, "Radio HW-disabled (corerev >= 3)");
		break;
	case 0x160:
		strcpy(desc, "SHM Control");
		switch (value & 0x0000FFFF) {
		case 0x0001:
			strcat(desc, "  (Shared Memory)");
			break;
		case 0x0002:
			strcat(desc, "  (Unknown 0x0002)");
			break;
		case 0x0003:
			strcat(desc, "  (PCM data)");
			break;
		case 0x0004:
			strcat(desc, "  (MAC Address list)");
			break;
		case 0x0300:
			strcat(desc, "  (Microcode)");
			break;
		case 0x0301:
			strcat(desc, "  (Initial value microcode?)");
			break;
		default:
			strcat(desc, "  (Unknown routing)");
		}
		break;
	case 0x164:
		strcpy(desc, "SHM High");
		break;
	case 0x166:
		strcpy(desc, "SHM Low");
		break;
	case 0x170:
		strcpy(desc, "Xmitstatus 0");
		break;
	case 0x174:
		strcpy(desc, "Xmitstatus 1");
		break;
	case 0x180:
		strcpy(desc, "TSF Low (corerev >= 3)");
		break;
	case 0x184:
		strcpy(desc, "TSF High (corerev >= 3)");
		break;
	case 0x188:
		strcpy(desc, "Timing (corerev >= 3)");
		break;
	case 0x18C:
		strcpy(desc, "Timing (TBTT) (corerev >= 3)");
		break;
	case 0x190:
		strcpy(desc, "ATIM window (corerev >= 3 && Ad-Hoc)");
		break;
	case 0x200:
		strcpy(desc, "DMA controller 1 base");
		break;
	case 0x220:
		strcpy(desc, "DMA controller 2 base");
		break;
	case 0x240:
		strcpy(desc, "DMA controller 3 base");
		break;
	case 0x260:
		strcpy(desc, "DMA controller 4 base");
		break;
	case 0x300:
		strcpy(desc, "PIO queue 1");
		break;
	case 0x310:
		strcpy(desc, "PIO queue 2");
		break;
	case 0x320:
		strcpy(desc, "PIO queue 3");
		break;
	case 0x330:
		strcpy(desc, "PIO queue 4");
		break;
	case 0x3E0:
		strcpy(desc, "PHY Versioning");
		break;
	case 0x3E2:
		strcpy(desc, "A/B PHY Radio BitField (antennadiv)");
		break;
	case 0x3E6:
		strcpy(desc, "Baseband Attenuation for B/G PHYs Revision 0");
		break;
	case 0x3E8:
		strcpy(desc, "Antenna BitField");
		break;
	case 0x3EC:
		strcpy(desc, "PHY init (FIXME)");
		break;
	case 0x3F0:
		strcpy(desc, "Channel");
		break;
	case 0x3F6:
		strcpy(desc, "Radio Control");
		break;
	case 0x3F8:
		strcpy(desc, "Radio Data High");
		break;
	case 0x3FA:
		strcpy(desc, "Radio Data Low");
		break;
	case 0x3FC:
		strcpy(desc, "PHY Control");
		break;
	case 0x3FE:
		strcpy(desc, "PHY Data");
		break;
	case 0x420:
		strcpy(desc, "MacAddressFilter Control");
		break;
	case 0x422:
		strcpy(desc, "MacAddressFilter Data");
		break;
	case 0x43C:
		strcpy(desc, "Security Clear");
		break;
	case 0x49A:
		strcpy(desc, "Radio HW-disabled (corerev < 3)");
		break;
	case 0x49C:
		strcpy(desc, "GPIO Control (Pin ON/OFF)");
		break;
	case 0x49E:
		strcpy(desc, "GPIO Control (Pin Enable)");
		break;
	case 0x604:
		strcpy(desc, "TBTT in usecs for BSS#3 (corerev < 3)");
		break;
	case 0x606:
		strcpy(desc, "Beacon Interval >> 6 for BSS#3 (corerev < 3)");
		break;
	case 0x60C:
		strcpy(desc, "ATIM Window BSS#3 (corerev < 3)");
		break;
	case 0x60E:
		strcpy(desc, "ATIM Window valid? (corerev < 3)");
		break;
	case 0x610:
		strcpy(desc, "Beacon Interval for BSS#3 (corerev < 3)");
		break;
	case 0x632:
		strcpy(desc, "TSF 0 (corerev < 3)");
		break;
	case 0x634:
		strcpy(desc, "TSF 1 (corerev < 3)");
		break;
	case 0x636:
		strcpy(desc, "TSF 2 (corerev < 3)");
		break;
	case 0x638:
		strcpy(desc, "TSF 3 (corerev < 3)");
		break;
	case 0x65A:
		strcpy(desc, "Random Number Generator");
		break;
	case 0x684:
		strcpy(desc, "SlotTime + 510");
		break;
	case 0x6A8:
		strcpy(desc, "Fast powerup delay control");
		break;
	/* TODO: Add the missing. */
	default:
		strcpy(desc, "No description available");
	}
}

static int print_plaintext(struct bcm43xx_initval *vals, size_t nr_vals)
{
	struct bcm43xx_initval *val;
	uint16_t offset, size;
	uint32_t value;
	size_t i;
	char desc[MAX_DESC];

	for (i = 0; i < nr_vals; i++) {
		val = &(vals[i]);
		offset = be16_to_cpu(val->offset);
		size = be16_to_cpu(val->size);
		value = be16_to_cpu(val->value);

		get_description(desc, offset, size, value);
		if (size == 2) {
			prdata("Offset: 0x%03X, Value: 0x%04X    "
			       "  (%s)\n",
			       offset, value, desc);
		} else if (size == 4) {
			prdata("Offset: 0x%03X, Value: 0x%08X"
			       "  (%s)\n",
			       offset, value, desc);
		} else {
			prerror("Invalid size (%u) found\n", size);
//			return 1;
		}
	}

	return 0;
}

static int read_input(struct bcm43xx_initval **vals, size_t *nr_vals)
{
	int err;
	int fd = STDIN_FILENO;
	struct stat s;
	ssize_t r;

	err = fstat(fd, &s);
	if (err) {
		prerror("Could not stat input data.\n");
		return err;
	}
	if (s.st_size == 0) {
		prerror("Input data size == 0\n");
		return 1;
	}
	if (s.st_size % sizeof(struct bcm43xx_initval)) {
		prerror("Corrupt input data\n");
		return 1;
	}
	*nr_vals = s.st_size / sizeof(struct bcm43xx_initval);
	*vals = malloce(s.st_size);
	r = read(fd, *vals, s.st_size);
	if (r != s.st_size) {
		prerror("Could not read input data.\n");
		return -1;
	}

	return 0;
}

static void print_banner(int forceprint)
{
	const char *str = "BCM43xx InitVal file tool version " VERSION "\n";
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
	prdata("  -p|--printplain       Print the initvals in human readable format\n");
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

static int parse_args(int argc, char **argv)
{
	int i;
//	char *param;

	for (i = 1; i < argc; i++) {
		if (arg_match(argv, &i, "--version", "-v", 0)) {
			print_banner(1);
			return 1;
		} else if(arg_match(argv, &i, "--help", "-h", 0)) {
			goto out_usage;
		} else if(arg_match(argv, &i, "--verbose", "-V", 0)) {
			cmdargs.verbose = 1;
		} else if(arg_match(argv, &i, "--printplain", "-p", 0)) {
			cmdargs.printplain = 1;
		} else {
			prerror("Unrecognized argument: %s\n", argv[i]);
			goto out_usage;
		}
	}
	return 0;

out_usage:
	print_usage(argc, argv);
error:
	return -1;
}

int main(int argc, char **argv)
{
	struct bcm43xx_initval *vals;
	size_t nr_vals;
	int err;

	get_endianess();

	err = parse_args(argc, argv);
	if (err == 1)
		return 0;
	else if (err != 0)
		goto out;
	print_banner(0);

	err = read_input(&vals, &nr_vals);
	if (err)
		goto out;

	if (cmdargs.printplain)
		print_plaintext(vals, nr_vals);

	free(vals);
out:
	return err;
}
