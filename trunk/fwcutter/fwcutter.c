/*
 * firmware cutter for broadcom 43xx wireless driver files
 * 
 * Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
 *               2005 Michael Buesch <mbuesch@freenet.de>
 *		 2005 Alex Beregszaszi
 *		 2007 Johannes Berg <johannes@sipsolutions.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <byteswap.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "md5.h"
#include "fwcutter.h"
#include "fwcutter_list.h"


static struct cmdline_args cmdargs;

/* tiny disassembler */
static void print_ucode_version(struct insn *insn)
{
	int val;

	/*
	 * The instruction we're looking for is a store to memory
	 * offset insn->op3 of the constant formed like `val' below.
	 * 0x2de00 is the opcode for type 1, 0x378 is the opcode
	 * for type 2 and 3.
	 */
	if (insn->opcode != 0x378 && insn->opcode != 0x2de00)
		return;

	val = ((0xFF & insn->op1) << 8) | (0xFF & insn->op2);

	/*
	 * Memory offsets are word-offsets, for the meaning
	 * see http://bcm-v4.sipsolutions.net/802.11/ObjectMemory
	 */
	switch (insn->op3) {
	case 0:
		printf("  ucode version:  %d\n", val);
		break;
	case 1:
		printf("  ucode revision: %d\n", val);
		break;
	case 2:
		printf("  ucode date:     %.4d-%.2d-%.2d\n",
		       2000 + (val >> 12), (val >> 8) & 0xF, val & 0xFF);
		break;
	case 3:
		printf("  ucode time:     %.2d:%.2d:%.2d\n",
		       val >> 11, (val >> 5) & 0x3f, val & 0x1f);
		break;
	}
}

static void disasm_ucode_1(uint64_t in, struct insn *out)
{
	/* xxyyyzzz00oooooX -> ooooo Xxx yyy zzz
	 * if we swap the upper and lower 32-bits first it becomes easier:
	 * 00oooooxxxyyyzzz -> ooooo xxx yyy zzz
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0xFFF;
	out->op2	= (in >> 12) & 0xFFF;
	out->op1	= (in >> 24) & 0xFFF;
	out->opcode	= (in >> 36) & 0xFFFFF;
	/* the rest of the in word should be zero */
}

static void disasm_ucode_2(uint64_t in, struct insn *out)
{
	/* xxyyyzzz0000oooX -> ooo Xxx yyy zzz
	 * if we swap the upper and lower 32-bits first it becomes easier:
	 * 0000oooxxxyyyzzz -> ooo xxx yyy zzz
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0xFFF;
	out->op2	= (in >> 12) & 0xFFF;
	out->op1	= (in >> 24) & 0xFFF;
	out->opcode	= (in >> 36) & 0xFFF;
	/* the rest of the in word should be zero */
}

static void disasm_ucode_3(uint64_t in, struct insn *out)
{
	/*
	 * like 2, but each operand has one bit more; appears
	 * to use the same instruction set slightly extended
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0x1FFF;
	out->op2	= (in >> 13) & 0x1FFF;
	out->op1	= (in >> 26) & 0x1FFF;
	out->opcode	= (in >> 39) & 0xFFF;
	/* the rest of the in word should be zero */
}

static void analyse_ucode(int ucode_rev, uint8_t *data, uint32_t len)
{
	uint64_t *insns = (uint64_t*)data;
	struct insn insn;
	uint32_t i;

	for (i=0; i<len/sizeof(*insns); i++) {
		switch (ucode_rev) {
		case 1:
			disasm_ucode_1(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		case 2:
			disasm_ucode_2(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		case 3:
			disasm_ucode_3(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		}
	}
}

static void swap_endianness_ucode(uint8_t *buf, uint32_t len)
{
	uint32_t *buf32 = (uint32_t*)buf;
	uint32_t i;

	for (i=0; i<len/4; i++)
		buf32[i] = bswap_32(buf32[i]);
}

#define swap_endianness_pcm swap_endianness_ucode

static void swap_endianness_iv(uint8_t *buf, uint32_t len)
{
	struct iv *iv = (struct iv*)buf;
	uint32_t i;

	for (i=0; i<len/sizeof(*iv); i++) {
		iv[i].reg = bswap_16(iv[i].reg);
		iv[i].size = bswap_16(iv[i].size);
		iv[i].val = bswap_32(iv[i].val);
	}
}

static void write_file(const char *name, uint8_t *buf, uint32_t len,
		       uint32_t flags)
{
	FILE *f;
	char nbuf[4096];
	const char *dir;
	int r;

	if (flags & FW_FLAG_V4)
		dir = "b43";
	else
		dir = "b43legacy";

	r = snprintf(nbuf, sizeof(nbuf),
		     "%s/%s", cmdargs.target_dir, dir);
	if (r >= sizeof(nbuf)) {
		fprintf(stderr, "name too long");
		exit(2);
	}

	r = mkdir(nbuf, 0770);
	if (r && errno != EEXIST) {
		perror("failed to create output directory");
		exit(2);
	}

	r = snprintf(nbuf, sizeof(nbuf),
		     "%s/%s/%s.fw", cmdargs.target_dir, dir, name);
	if (r >= sizeof(nbuf)) {
		fprintf(stderr, "name too long");
		exit(2);
	}
	f = fopen(nbuf, "w");
	if (!f) {
		perror("failed to open file");
		exit(2);
	}
	fwrite(buf, 1, len, f);
	fclose(f);
}

static void extract_or_identify(FILE *f, const struct extract *extract,
				uint32_t flags)
{
	uint8_t *buf;
	int ucode_rev = 0;

	if (fseek(f, extract->offset, SEEK_SET)) {
		perror("failed to seek on file");
		exit(2);
	}

	buf = malloc(extract->length);
	if (fread(buf, 1, extract->length, f) != extract->length) {
		perror("failed to read complete data");
		exit(3);
	}

	switch (extract->type) {
	case EXT_UCODE_3:
		ucode_rev += 1;
	case EXT_UCODE_2:
		ucode_rev += 1;
	case EXT_UCODE_1:
		ucode_rev += 1;
		if (flags & FW_FLAG_LE)
			swap_endianness_ucode(buf, extract->length);
		analyse_ucode(ucode_rev, buf, extract->length);
		break;
	case EXT_PCM:
		if (flags & FW_FLAG_LE)
			swap_endianness_pcm(buf, extract->length);
		break;
	case EXT_IV:
		if (flags & FW_FLAG_LE)
			swap_endianness_iv(buf, extract->length);
		break;
	default:
		exit(255);
	}

	if (!cmdargs.identify_only)
		write_file(extract->name, buf, extract->length, flags);

	free(buf);
}

static void print_banner(void)
{
	printf("b43-fwcutter version " FWCUTTER_VERSION "\n");
}

static void print_file(const struct file *file)
{
	char filename[30];
	char shortname[30];

	if (file->flags & FW_FLAG_V4)
		printf("b43\t\t");
	else
		printf("b43legacy\t");

	if (strlen(file->name) > 20) {
		strncpy(shortname, file->name, 20);
		shortname[20] = '\0';
		snprintf(filename, sizeof(filename), "%s..", shortname);
	} else
		strcpy (filename, file->name);

	printf("%s\t", filename);
	if (strlen(filename) < 8) printf("\t");
	if (strlen(filename) < 16) printf("\t");

	printf("%s\t", file->ucode_version);
	if (strlen(file->ucode_version) < 8) printf("\t");

	printf("%s\n", file->md5);
}

static void print_supported_files(void)
{
	int i;

	print_banner();
	printf("\nExtracting firmware is possible "
	       "from these binary driver files:\n\n");
	printf("<driver>\t"
	       "<filename>\t\t"
	       "<microcode>\t"
	       "<MD5 checksum>\n\n");
	/* print for legacy driver first */
	for (i = 0; i < FILES; i++)
		if (!(files[i].flags & FW_FLAG_V4))
			print_file(&files[i]);
	for (i = 0; i < FILES; i++)
		if (files[i].flags & FW_FLAG_V4)
			print_file(&files[i]);
	printf("\n");
}

static const struct file *find_file(FILE *fd)
{
	unsigned char buffer[16384], signature[16];
	struct MD5Context md5c;
	char md5sig[33];
	int i;

	MD5Init(&md5c);
	while ((i = (int) fread(buffer, 1, sizeof(buffer), fd)) > 0)
		MD5Update(&md5c, buffer, (unsigned) i);
	MD5Final(signature, &md5c);

	snprintf(md5sig, sizeof(md5sig),
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x"
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		 signature[0], signature[1], signature[2], signature[3],
		 signature[4], signature[5], signature[6], signature[7],
		 signature[8], signature[9], signature[10], signature[11],
		 signature[12], signature[13], signature[14], signature[15]);

	for (i = 0; i < FILES; ++i) {
		if (strcasecmp(md5sig, files[i].md5) == 0) {
			printf("This file is recognised as:\n");
			printf("  filename   :  %s\n", files[i].name);
			printf("  version    :  %s\n", files[i].ucode_version);
			printf("  MD5        :  %s\n", files[i].md5);
			return &files[i];
		}
	}
	printf("Sorry, the input file is either wrong or "
	       "not supported by b43-fwcutter.\n");
	printf("This file has an unknown MD5sum %s.\n", md5sig);

	return NULL;
}

static void print_usage(int argc, char *argv[])
{
	print_banner();
	printf("\nUsage: %s [OPTION] [driver.sys]\n", argv[0]);
	printf("  -l|--list             "
	       "List supported driver versions\n");
	printf("  -i|--identify         "
	       "Only identify the driver file (don't extract)\n");
	printf("  -w|--target-dir DIR   "
	       "Extract and write firmware to DIR\n");
	printf("  -v|--version          "
	       "Print b43-fwcutter version\n");
	printf("  -h|--help             "
	       "Print this help\n");
	printf("\nExample: %s bcmwl5.sys\n"
	       "         to extract the firmware blobs from bcmwl5.sys\n", 
	       argv[0]);
}

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
				printf("%s needs a parameter\n", arg);
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

static int parse_args(int argc, char *argv[])
{
	int i, res;
	char *param;

	if (argc < 2)
		goto out_usage;
	for (i = 1; i < argc; i++) {
		res = cmp_arg(argv, &i, "--list", "-l", 0);
		if (res == ARG_MATCH) {
			print_supported_files();
			return 1;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--version", "-v", 0);
		if (res == ARG_MATCH) {
			print_banner();
			return 1;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--help", "-h", 0);
		if (res == ARG_MATCH)
			goto out_usage;
		else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--identify", "-i", 0);
		if (res == ARG_MATCH) {
			cmdargs.identify_only = 1;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--target-dir", "-w", &param);
		if (res == ARG_MATCH) {
			cmdargs.target_dir = param;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		cmdargs.infile = argv[i];
		break;
	}

	if (!cmdargs.infile)
		goto out_usage;
	return 0;

out_usage:
	print_usage(argc, argv);
out:
	return -1;	
}

int main(int argc, char *argv[])
{
	FILE *fd;
	const struct file *file;
	const struct extract *extract;
	int err;
	const char *dir;

	cmdargs.target_dir = ".";
	err = parse_args(argc, argv);
	if (err == 1)
		return 0;
	else if (err != 0)
		return err;

	fd = fopen(cmdargs.infile, "rb");
	if (!fd) {
		fprintf(stderr, "Cannot open input file %s\n", cmdargs.infile);
		return 2;
	}

	err = -1;
	file = find_file(fd);
	if (!file)
		goto out_close;

	if (file->flags & FW_FLAG_V4)
		dir = "b43";
	else
		dir = "b43legacy";

	extract = file->extract;
	while (extract->name) {
		printf("%s %s/%s.fw\n",
		       cmdargs.identify_only ? "Contains" : "Extracting",
		       dir, extract->name);
		extract_or_identify(fd, extract, file->flags);
		extract++;
	}

	err = 0;
out_close:
	fclose(fd);

	return err;
}
