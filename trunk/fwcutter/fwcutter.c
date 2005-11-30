/*
 * firmware cutter for broadcom 43xx wireless driver files
 * 
 * Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
 *               2005 Michael Buesch <mbuesch@freenet.de>
 *		 2005 Alex Beregszaszi
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

typedef unsigned char byte;

#define BYTE_ORDER_BIG_ENDIAN    0x01  /* ppc driver files */
#define BYTE_ORDER_LITTLE_ENDIAN 0x02  /* x86, mips driver files */
#define SUPPORT_INCOMPLETE       0x04  /* not all fw files can be extracted */
#define SUPPORT_IMPOSSIBLE       0x08  /* no support for this file */
#define INIT_VAL_08_MISSING      0x10  /* initval 8 is missing in older driver files */

#define fwcutter_stringify_1(x)	#x
#define fwcutter_stringify(x)	fwcutter_stringify_1(x)
#define FWCUTTER_VERSION	fwcutter_stringify(FWCUTTER_VERSION_)

#include "md5.h"
#include "fwcutter_list.h"


struct cmdline_args {
	const char *infile;
	const char *postfix;
};

static struct cmdline_args cmdargs;
int big_endian_cpu;
char* target_dir;


static void write_little_endian(FILE *f, byte *buffer, int len) 
{
	byte swapbuf[4];

	while (len > 0) {
		swapbuf[0] = buffer[3]; swapbuf[1] = buffer[2];
		swapbuf[2] = buffer[1]; swapbuf[3] = buffer[0];
		fwrite(swapbuf, 4, 1, f);
		buffer = buffer + 4;
		len  = len - 4;
	}
}

static void write_big_endian(FILE *f, byte *buffer, int len) 
{
	while (len > 0) {
		fwrite(buffer, 4, 1, f);
		buffer = buffer + 4;
		len  = len - 4;
	}
}

static void write_fw(const char *infilename, const char *outfilename, uint8_t flags, byte *data, int len)
{
	FILE* fw;
	char outfile[40];

	sprintf(outfile, "%s/%s", target_dir, outfilename);

	fw = fopen(outfile, "w");
	if (!fw) {
		perror(outfile);
		exit(1);
	}

	if (flags & BYTE_ORDER_LITTLE_ENDIAN)
		write_little_endian(fw, data, len);
	else if (flags & BYTE_ORDER_BIG_ENDIAN)
		write_big_endian(fw, data, len);
	else
		printf("unknown byteorder...\n");

	fflush(fw);
	fclose(fw);
}

static void write_iv(const char *infilename, uint8_t flags, byte *data)
{
	FILE* fw;
	char ivfilename[2048];
	int i;

	for (i = 1; i <= 10; i++) {

		if ((flags & INIT_VAL_08_MISSING) && (i==8)) {
			printf("WARNING: initval 08 not available in driver file \"%s\". "
			       "Driver file is too old.\n", infilename);
			i++;
		}

		snprintf(ivfilename, sizeof(ivfilename),
			 "%s/bcm430x_initval%02d%s.fw",
			 target_dir, i, cmdargs.postfix);
		fw = fopen(ivfilename, "w");

		if (!fw) {
			perror(ivfilename);
			exit(1);
		}

		printf("extracting bcm430x_initval%02d%s.fw ...\n", i, cmdargs.postfix);

		while (1) {

			if ((data[0]==0xff) && (data[1]==0xff) && (data[2]==0x00) && (data[3]==0x00)) {
				data = data + 8;
				break;
			}

			if (flags & BYTE_ORDER_LITTLE_ENDIAN)
				fprintf(fw, "%c%c%c%c%c%c%c%c",
					data[1], data[0],                       /* offset */
					data[3], data[2],                       /* size */
					data[7], data[6], data[5], data[4]);    /* value */
			else if (flags & BYTE_ORDER_BIG_ENDIAN)
				fprintf(fw, "%c%c%c%c%c%c%c%c",
					data[0], data[1],                       /* offset */
					data[2], data[3],                       /* size */
					data[4], data[5], data[6], data[7]);    /* value */
			else {
				printf("unknown byteorder...\n");
				exit(1);
			}

			data = data + 8;
		}
		fflush(fw);
		fclose(fw);
	}
}

static byte* read_file(const char* filename)
{
	FILE* file;
	long len;
	byte* data;

	file = fopen(filename, "rb");
	if (!file) {
		perror(filename);
		exit(1);
	}
	if (fseek(file, 0, SEEK_END)) {
		perror("cannot seek");
		exit(1);
	}
	len = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = malloc(len);
	if (!data) {
		fputs("out of memory\n", stderr);
		exit(1);
	}
	if (fread(data, 1, len, file) != len) {
		perror("cannot read");
		exit(1);
	}
	fclose(file);
	return data;
}

static void extract_fw(const char *infile, const char *outfile, uint8_t flags, uint32_t pos, uint32_t length)
{
	byte* filedata;

	if (length > 0) {
		printf("extracting %s ...\n", outfile);
		filedata = read_file(infile);
		write_fw(infile, outfile, flags, filedata + pos, length);
		free(filedata);
	} else {
		printf("WARNING: \"%s\" not available in driver file \"%s\". "
		       "Driver file is too old.\n", outfile, infile);
	}
}

static void extract_iv(const char *infile, uint8_t flags, uint32_t pos)
{
	byte* filedata;

	if (pos > 0) {
		filedata = read_file(infile);
		write_iv(infile, flags, filedata + pos);
		free(filedata);
	}
}

static void print_banner(void)
{
	printf("fwcutter " FWCUTTER_VERSION "\n");
}

static void print_file(const struct file *file)
{
	printf("%s\t", file->name);
	if (strlen(file->name) < 8)
		printf("\t");

	printf("%s\t", file->version);
	if (strlen(file->version) < 8)
		printf("\t");
	if (strlen(file->version) < 16)
		printf("\t");

	printf("md5: %s", file->md5);

	if ((file->flags & INIT_VAL_08_MISSING) || 
	    (file->flags & SUPPORT_INCOMPLETE))
		printf(" *");
	printf("\n");
}

static void print_supported_files(void)
{
	int i;

	print_banner();
	printf("Extracting firmware is possible from these binary driver files:\n\n");
	for (i = 0; i < FILES; i++) {
		if (files[i].flags & SUPPORT_IMPOSSIBLE)
			continue;
		print_file(&files[i]);
	}
	printf("   *  fwcutter can't extract all firmware files\n");
	printf("\nExtracting firmware is IMPOSSIBLE from these binary driver files:\n\n");
	for (i = 0; i < FILES; i++) {
		if (!(files[i].flags & SUPPORT_IMPOSSIBLE))
			continue;
		print_file(&files[i]);
	}
}

static const struct file * find_file(FILE *fd)
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
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		 signature[0], signature[1], signature[2], signature[3],
		 signature[4], signature[5], signature[6], signature[7],
		 signature[8], signature[9], signature[10], signature[11],
		 signature[12], signature[13], signature[14], signature[15]);

	for (i = 0; i < FILES; ++i) {
		if (strcasecmp(md5sig, files[i].md5) == 0) {
			if (files[i].flags & SUPPORT_IMPOSSIBLE) {
				printf("Extracting firmware from this file is IMPOSSIBLE. (too old)\n");
				return 0;
			}
			printf("fwcutter knows your file:\n");
			printf("  filename :  %s\n", files[i].name);
			printf("  version  :  %s\n", files[i].version);
			printf("  MD5      :  %s\n", files[i].md5);
			if (files[i].flags & SUPPORT_INCOMPLETE)
				printf("WARNING: your driver file is not fully supported\n");
			return &(files[i]);
		}
	}
	fputs("Driver version unknown or wrong input file.\n", stderr);

	return 0;
}

static void identify_file(const char *file)
{
	FILE *fd;

	fd = fopen(file, "rb");
	if (!fd) {
		fprintf(stderr, "Cannot open input file %s\n", file);
		return;
	}
	find_file(fd);
	fclose(fd);
}

static void get_endianess(void)
{
	const unsigned char x[] = { 0xde, 0xad, 0xbe, 0xef, };
	const uint32_t *p = (uint32_t *)x;

	if (*p == 0xdeadbeef)
		big_endian_cpu = 1;
}

static void print_usage(int argc, char *argv[])
{
	print_banner();
	printf("\nUsage: %s [OPTION] [driver.sys]\n", argv[0]);
	printf("  -l             List supported driver versions\n");
	printf("  -i DRIVER.SYS  Identify a driver file (don't extract)\n");
	printf("  -w DRIVER.SYS  Extract and write firmware to /lib/firmware\n");
	printf("  -p \".FOO\"      Postfix for firmware filenames (.FOO.fw)\n");
	printf("  -v             Print fwcutter version\n");
	printf("  -h|--help      Print this help\n");
	printf("\nExample: %s bcmwl5.sys\n"
	       "         to extract the firmware blobs from bcmwl5.sys\n", argv[0]);
}

#define ARG_MATCH	0
#define ARG_NOMATCH	1
#define ARG_ERROR	-1

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

		res = cmp_arg(argv, &i, "--identify", "-i", &param);
		if (res == ARG_MATCH) {
			identify_file(param);
			return 1;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, 0, "-w", &param);
		if (res == ARG_MATCH) {
			target_dir = "/lib/firmware";
			cmdargs.infile = param;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--postfix", "-p", &param);
		if (res == ARG_MATCH) {
			cmdargs.postfix = param;
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
	int err;
	char fwname[1024];

	get_endianess();

	target_dir = ".";
	cmdargs.postfix = "";
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

	snprintf(fwname, sizeof(fwname), "bcm430x_microcode2%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->uc2_pos, file->uc2_length);
	snprintf(fwname, sizeof(fwname), "bcm430x_microcode4%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->uc4_pos, file->uc4_length);
	snprintf(fwname, sizeof(fwname), "bcm430x_microcode5%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->uc5_pos, file->uc5_length);
	snprintf(fwname, sizeof(fwname), "bcm430x_microcode11%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->uc11_pos, file->uc11_length);
	snprintf(fwname, sizeof(fwname), "bcm430x_pcm4%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->pcm4_pos, file->pcm4_length);
	snprintf(fwname, sizeof(fwname), "bcm430x_pcm5%s.fw", cmdargs.postfix);
	extract_fw(cmdargs.infile, fwname,
		   file->flags, file->pcm5_pos, file->pcm5_length);
	extract_iv(cmdargs.infile, file->flags, file->iv_pos);

	err = 0;
out_close:
	fclose(fd);

	return err;
}
