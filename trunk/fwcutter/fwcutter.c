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

#define BYTE_ORDER_AABBCCDD    0x01
#define BYTE_ORDER_DDCCBBAA    0x02  /* 4 bytes swapped in source (DDCCBBAA instead of AABBCCDD) */
#define SUPPORT_INCOMPLETE     0x04  /* not all fw files can be extracted */
#define SUPPORT_IMPOSSIBLE     0x08  /* no support for this file */
#define INIT_VAL_08_MISSING    0x10  /* initval 8 is missing in older driver files */

#define fwcutter_stringify_1(x)	#x
#define fwcutter_stringify(x)	fwcutter_stringify_1(x)
#define FWCUTTER_VERSION	fwcutter_stringify(FWCUTTER_VERSION_)

#include "md5.h"
#include "fwcutter_list.h"


struct cmdline_args {
	const char *infile;
};

static struct cmdline_args cmdargs;


static void write_ddccbbaa(FILE *f, byte *buffer, int len) 
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

static void write_aabbccdd(FILE *f, byte *buffer, int len) 
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

	fw = fopen(outfilename, "w");
	if (!fw) {
		perror(outfilename);
		exit(1);
	}

	if (flags & BYTE_ORDER_DDCCBBAA)
		write_ddccbbaa(fw, data, len);
	else if (flags & BYTE_ORDER_AABBCCDD)
		write_aabbccdd(fw, data, len);
	else
		printf("unknown byteorder...\n");

	fflush(fw);
	fclose(fw);
}

static void write_iv(const char *infilename, uint8_t flags, byte *data)
{
	FILE* fw;
	char ivfilename[21];
	int i;

	for (i = 1; i <= 10; i++) {

		if ((flags & INIT_VAL_08_MISSING) && (i==8)) {
			printf("WARNING: initval 08 not available in driver file \"%s\". "
			       "Driver file is too old.\n", infilename);
			i++;
		}

		sprintf(ivfilename, "bcm430x_initval%02d.fw", i);
		fw = fopen(ivfilename, "w");

		if (!fw) {
			perror(ivfilename);
			exit(1);
		}

		printf("extracting %s ...\n", ivfilename);

		while (1) {

			if ((data[0]==0xff) && (data[1]==0xff) && (data[2]==0x00) && (data[3]==0x00)) {
				data = data + 8;
				break;
			}

			if (flags & BYTE_ORDER_DDCCBBAA)
				fprintf(fw, "%02x%02x%02x%02x%02x%02x%02x%02x",
					data[1], data[0], data[3], data[2], 
					data[7], data[6], data[5], data[4]);
			else if (flags & BYTE_ORDER_AABBCCDD)
				fprintf(fw, "%02x%02x%02x%02x%02x%02x%02x%02x",
					data[0], data[1], data[2], data[3], 
					data[4], data[5], data[6], data[7]);
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
	data = (byte*)malloc(len);
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

static void print_supported_files(void)
{
	int i;

	printf("fwcutter " FWCUTTER_VERSION "\n\n");
	printf("Extracting firmware is possible from these binary driver files:\n\n");
	printf("   *  fwcutter can't extract all firmware files\n");
	printf("   -  not supported\n\n");
	for (i = 0; i < FILES; ++i) {
		printf("%s\t", files[i].name);
		if (strlen(files[i].name) < 8)
			printf("\t");

		printf("%s\t", files[i].version);
		if (strlen(files[i].version) < 8)
			printf("\t");
		if (strlen(files[i].version) < 16)
			printf("\t");

		printf("md5: %s", files[i].md5);

		if ((files[i].flags & INIT_VAL_08_MISSING) || 
		    (files[i].flags & SUPPORT_INCOMPLETE))
			printf(" *");
		if (files[i].flags & SUPPORT_IMPOSSIBLE)
			printf(" -");

		printf("\n");
	}
}

static void print_banner(void)
{
	printf("fwcutter " FWCUTTER_VERSION "\n");
}

static void print_usage(int argc, char *argv[])
{
	print_banner();
	printf("\nUsage: %s [OPTION] [driver.sys]\n", argv[0]);
	printf("  -l             list supported driver versions\n");
	printf("  -v             print fwcutter version\n");
	printf("  -h|--help      print this help\n");
	printf("\nExample: %s bcmwl5.sys\n"
	       "         to extract the firmware blobs from bcmwl5.sys\n", argv[0]);
}

static int cmp_arg(char *arg, const char *template, char **param)
{
	size_t arg_len, template_len;

	if (param) {
		arg_len = strlen(arg);
		template_len = strlen(template);
		/* maybe we have a merged parameter here. */
		if (arg_len > template_len) {
			if (memcmp(arg, template, template_len) == 0) {
				*param = arg + template_len;
				return 0;
			}
			return -1;
		} else if (arg_len != template_len)
			return -1;
	}
	if (strcmp(arg, template) == 0)
		return 0;

	return -1;
}

static int parse_args(int argc, char *argv[])
{
	int i;
	char *arg;
	char *next_arg;
	char *param;

	if (argc < 2)
		goto out_usage;
	for (i = 1; i < argc; i++) {
		param = 0;
		arg = argv[i];
		if (i + 1 < argc)
			next_arg = argv[i + 1];
		else
			next_arg = 0;
		if (cmp_arg(arg, "-l", 0) == 0) {
			print_supported_files();
			return 1;
		} else if (cmp_arg(arg, "-v", 0) == 0) {
			print_banner();
			return 1;
		} else if (cmp_arg(arg, "-h", 0) == 0 ||
			   cmp_arg(arg, "--help", 0) == 0) {
			goto out_usage;
		} else if (cmp_arg(arg, "-i", &param) == 0) {
			if (param)
				next_arg = param;
			else
				i++;
			if (!next_arg) {
				printf("-i needs a parameter\n");
				return -1;
			}
			cmdargs.infile = next_arg;
		} else {
			cmdargs.infile = arg;
			break;
		}
	}

	if (!cmdargs.infile)
		goto out_usage;
	return 0;

out_usage:
	print_usage(argc, argv);
	return -1;	
}

int main(int argc, char *argv[])
{
	unsigned char buffer[16384], signature[16];
	char md5sig[33];
	FILE *in = stdin;
	int i, count = 0;
	struct MD5Context md5c;

	i = parse_args(argc, argv);
	if (i == 1)
		return 0;
	else if (i != 0)
		return i;

	if ((in = fopen(cmdargs.infile, "rb")) == NULL) {
		fprintf(stderr, "Cannot open input file %s\n", cmdargs.infile);
		return 2;
	}

	MD5Init(&md5c);
	while ((i = (int) fread(buffer, 1, sizeof buffer, in)) > 0)
		MD5Update(&md5c, buffer, (unsigned) i);
	MD5Final(signature, &md5c);

	snprintf(md5sig, sizeof md5sig,
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		 signature[0], signature[1], signature[2], signature[3],
		 signature[4], signature[5], signature[6], signature[7],
		 signature[8], signature[9], signature[10], signature[11],
		 signature[12], signature[13], signature[14], signature[15]);

	for (i = 0; i < FILES; ++i) {
		if (strcasecmp(md5sig, files[i].md5) == 0) {
			printf("Your firmware file is known. It's version %s.\n", files[i].version);
			extract_fw(cmdargs.infile, "bcm430x_microcode2.fw",
				   files[i].flags, files[i].uc2_pos, files[i].uc2_length);
			extract_fw(cmdargs.infile, "bcm430x_microcode4.fw",
				   files[i].flags, files[i].uc4_pos, files[i].uc4_length);
			extract_fw(cmdargs.infile, "bcm430x_microcode5.fw",
				   files[i].flags, files[i].uc5_pos, files[i].uc5_length);
			extract_fw(cmdargs.infile, "bcm430x_pcm4.fw",
				   files[i].flags, files[i].pcm4_pos, files[i].pcm4_length);
			extract_fw(cmdargs.infile, "bcm430x_pcm5.fw",
				   files[i].flags, files[i].pcm5_pos, files[i].pcm5_length);
			extract_iv(cmdargs.infile, files[i].flags, files[i].iv_pos);
			++count;
			break;
		}
	}

	if (count == 0) {
		fputs("driver version unknown or wrong input file.\n", stderr);
		return 1;
	}

	return 0;
}
