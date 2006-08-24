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

#include "md5.h"
#include "fwcutter.h"
#include "fwcutter_list.h"


static struct cmdline_args cmdargs;
int big_endian_cpu;

static void write_little_endian(FILE *f, byte *buffer, int len, uint8_t flags) 
{
	byte swapbuf[4];

	while (len > 0) {
		swapbuf[0] = buffer[3]; swapbuf[1] = buffer[2];
		swapbuf[2] = buffer[1]; swapbuf[3] = buffer[0];
		fwrite(swapbuf, 4, 1, f);
		buffer = buffer + 4;
		len  = len - 4;

		if (flags & OLD_VERSION_STYLE_3_8)
			buffer = buffer + 8;
		if (flags & OLD_VERSION_STYLE_3_10)
			buffer = buffer + 4;
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

static void write_fw(const char *outfilename, uint8_t flags, 
		     byte *data, int len)
{
	FILE* fw;
	char outfile[2048];

	snprintf(outfile, sizeof(outfile),
		 "%s/%s", cmdargs.target_dir, outfilename);

	fw = fopen(outfile, "w");
	if (!fw) {
		perror(outfile);
		exit(1);
	}

	if (flags & BYTE_ORDER_LITTLE_ENDIAN)
		write_little_endian(fw, data, len, flags);
	else if (flags & BYTE_ORDER_BIG_ENDIAN)
		write_big_endian(fw, data, len);
	else
		printf("unknown byteorder...\n");

	fflush(fw);
	fclose(fw);
}

static void write_iv(uint8_t flags, uint8_t type, byte *data)
{
	FILE* fw;
	char ivfilename[2048];
	int i=0;
	int j;

	while (ivmap[i].type) {
		if (type == ivmap[i].type)
			break;
		i++;
	}

	if (!type) 
		return;

	for (j = 0; j < ivmap[i].number; j++) {
		
		snprintf(ivfilename, sizeof(ivfilename),
			 "%s/bcm43xx_initval%02d%s.fw",
			 cmdargs.target_dir,
			 ivmap[i].scheme[j],
			 cmdargs.postfix);

		/* don't extract initval 0 */
		if ( ivmap[i].scheme[j] == 0 ) {
			if (flags & OLD_VERSION_STYLE_3_8) {
				while (1) {
					if ((data[0]==0xff) &&
					    (data[1]==0xff) &&
					    (data[2]==0xff) &&
					    (data[3]==0xff) &&
					    (data[4]==0x00) &&
					    (data[5]==0x00) &&
					    (data[6]==0x00) &&
					    (data[7]==0x00))
						break;

					data = data + 12;
				}
			} else {
				while (1) {
					if ((data[2]==0x00) &&
					    (data[3]==0x00) &&
					    (data[0]==0xff) &&
					    (data[1]==0xff))
						break;

					data = data + 8;
				}
			}
		}

		/* skip empty initval files */
		if (flags & OLD_VERSION_STYLE_3_8) {
			if ((data[0]==0xff) && (data[1]==0xff) &&
			    (data[2]==0xff) && (data[3]==0xff) &&
			    (data[4]==0x00) && (data[5]==0x00) &&
			    (data[6]==0x00) && (data[7]==0x00)) {
				data = data + 12;
				continue;
			}
		} else {
			if ((data[2]==0x00) && (data[3]==0x00) &&
			    (data[0]==0xff) && (data[1]==0xff)) {
				data = data + 8;
				continue;
			}
		}

		fw = fopen(ivfilename, "w");
		if (!fw) {
			perror(ivfilename);
			exit(1);
		}

		printf("extracting bcm43xx_initval%02d%s.fw ...\n", 
		       ivmap[i].scheme[j],
		       cmdargs.postfix);

		while (1) {

			if (flags & OLD_VERSION_STYLE_3_8) {

				if ((data[0]==0xff) &&
				    (data[1]==0xff) &&
				    (data[2]==0xff) &&
				    (data[3]==0xff) &&
				    (data[4]==0x00) &&
				    (data[5]==0x00) &&
				    (data[6]==0x00) &&
				    (data[7]==0x00)) {
					data = data + 12;
					break;
				}

				if (flags & BYTE_ORDER_LITTLE_ENDIAN)
					fprintf(fw, "%c%c%c%c%c%c%c%c",
						/* offset */
						data[1], data[0],
						/* size */
						data[5], data[4],
						/* value */
						data[11], data[10], 
						data[9], data[8]);

				data = data + 12;
				continue;
			}

			if ((data[2]==0x00) && (data[3]==0x00)) {
				if ((data[0]==0x00) && 
				    (data[1]==0x00) &&
				    (data[4]==0x00) && 
				    (data[5]==0x00) &&
				    (data[6]==0x00) && 
				    (data[7]==0x00)) {
					data = data + 8;
					continue;
				} else if ((data[0]==0xff) && 
					   (data[1]==0xff)) {
					data = data + 8;
					break;
				}
			}

			if (flags & BYTE_ORDER_LITTLE_ENDIAN)
				fprintf(fw, "%c%c%c%c%c%c%c%c",
					/* offset */
					data[1], data[0],
					/* size */
					data[3], data[2],
					/* value */
					data[7], data[6], data[5], data[4]);
			else if (flags & BYTE_ORDER_BIG_ENDIAN)
				fprintf(fw, "%c%c%c%c%c%c%c%c",
					/* offset */
					data[0], data[1],
					/* size */
					data[2], data[3],
					/* value */
					data[4], data[5], data[6], data[7]);
			else {
				printf("unknown byteorder...\n");
				exit(1);
			}

			data = data + 8;
		};

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

static void extract_fw(uint8_t fwtype, uint8_t flags, 
		       uint32_t pos, uint32_t length)
{
	byte* filedata;
	char outfile[1024];

	switch (fwtype) {
	case FIRMWARE_UCODE_2:
	case FIRMWARE_UCODE_4:
	case FIRMWARE_UCODE_5:
	case FIRMWARE_UCODE_11:
	case FIRMWARE_UCODE_13:
		snprintf(outfile, sizeof(outfile), "bcm43xx_microcode%i%s.fw", 
			 fwtype - FIRMWARE_UCODE_OFFSET, cmdargs.postfix);
		break;
	case FIRMWARE_PCM_4:
	case FIRMWARE_PCM_5:
		snprintf(outfile, sizeof(outfile), "bcm43xx_pcm%i%s.fw", 
			 fwtype, cmdargs.postfix);
		break;
	default:
		snprintf(outfile, sizeof(outfile), "bcm43xx_unknown.fw");
	}

	if (length > 0) {
		printf("extracting %s ...\n", outfile);
		filedata = read_file(cmdargs.infile);
		write_fw(outfile, flags, filedata + pos, length);
		free(filedata);
	} else {
		printf("WARNING! This binary driver doesn't contain \"%s\".\n",
		       outfile);

		switch (fwtype) {
		case FIRMWARE_UCODE_2:
		case FIRMWARE_UCODE_4:
			printf("WARNING! Therefore bcm43xx with 0x812 cores "
			       "rev 0x%x will not work.\n", 
			       fwtype - FIRMWARE_UCODE_OFFSET);
			break;
		case FIRMWARE_UCODE_5:
			printf("WARNING! Therefore bcm43xx with cores 0x812 "
			       "rev 0x5 up to 0xa will not work.\n");
			break;
		case FIRMWARE_UCODE_11:
			printf("WARNING! Therefore bcm43xx with cores 0x812 "
			       "rev 0xb or 0xc will not work.\n");
			break;
		case FIRMWARE_UCODE_13:
			printf("WARNING! Therefore bcm43xx with cores 0x812 "
			       "rev 0xd or greater will not work.\n");
			break;
		case FIRMWARE_PCM_4:
			printf("WARNING! Therefore bcm43xx with cores 0x812 "
			       "rev 0x%x or smaller will not work.\n", 
			       fwtype);
			break;
		case FIRMWARE_PCM_5:
			printf("WARNING! Therefore bcm43xx with cores 0x812 "
			       "rev 0x%x or greater will not work.\n", 
			       fwtype);
			break;
		}
	}
}

static void extract_iv(uint8_t flags, uint32_t pos, uint8_t type)
{
	byte* filedata;

	if (pos > 0) {
		filedata = read_file(cmdargs.infile);
		write_iv(flags, type, filedata + pos);
		free(filedata);
	}
}

static void analyse_ucode(int type, const struct file * f, byte * data)
{
	uint32_t fwdata = 0;

	if (f->flags & BYTE_ORDER_LITTLE_ENDIAN) {
		if ((type==FIRMWARE_UCODE_2) || (type==FIRMWARE_UCODE_4)) {
			if (data[4]==0x0c && data[5]==0xe0 &&
			    data[6]==0x2d && data[7]==0x00) {
				fwdata = data[0] + (data[1]<<8) + 
					(data[2]<<16) + (data[3]<<24);
				if ((fwdata & 0xfff) == 0x000) {
					printf("  revision   :  0x%.4x\n", 
					       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
				if ((fwdata & 0xfff) == 0x001) {
					printf("  patchlevel :  0x%.4x\n", 
				       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
			}
		} else {
			if (data[4]==0x8c && data[5]==0x37 &&
			    data[6]==0x00 && data[7]==0x00) {
				fwdata = data[0] + (data[1]<<8) + 
					(data[2]<<16) + (data[3]<<24);
				if ((fwdata & 0xfff) == 0x000) {
					printf("  revision   :  0x%.4x\n", 
					       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
				if ((fwdata & 0xfff) == 0x001) {
					printf("  patchlevel :  0x%.4x\n", 
				       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
			}
		}
	} else if (f->flags & BYTE_ORDER_BIG_ENDIAN) {
		if ((type==FIRMWARE_UCODE_2) || (type==FIRMWARE_UCODE_4)) {
			if (data[7]==0x0c && data[6]==0xe0 &&
			    data[5]==0x2d && data[4]==0x00) {
				fwdata = data[3] + (data[2]<<8) + 
					(data[1]<<16) + (data[0]<<24);
				if ((fwdata & 0xfff) == 0x000) {
					printf("  revision   :  0x%.4x\n", 
					       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
				if ((fwdata & 0xfff) == 0x001) {
					printf("  patchlevel :  0x%.4x\n", 
				       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
			}
		} else {
			if (data[7]==0x8c && data[6]==0x37 &&
			    data[5]==0x00 && data[4]==0x00) {
				fwdata = data[3] + (data[2]<<8) + 
					(data[1]<<16) + (data[0]<<24);
				if ((fwdata & 0xfff) == 0x000) {
					printf("  revision   :  0x%.4x\n", 
					       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
				if ((fwdata & 0xfff) == 0x001) {
					printf("  patchlevel :  0x%.4x\n", 
				       ((fwdata >> 12) & 0xff) +
					       (((fwdata >> 16) & 0xff00)));
				}
			}
		}
	}
}

static void get_ucode_rev(int type, const struct file * f, int pos, int len)
{
	byte* data;
	int len_count = 0;

	if (len == 0) return;
	if ((f->flags & OLD_VERSION_STYLE_3_8) ||
	    (f->flags & OLD_VERSION_STYLE_3_10)) return;

	printf("  microcode  :  %i\n", type - FIRMWARE_UCODE_OFFSET);

	data = read_file(cmdargs.infile) + pos;
	while (len_count < len) {
		analyse_ucode(type, f, data);
		data = data + 8;
		len_count = len_count + 8;
	}

	printf("\n");
}

static void get_ucode_info(const struct file * f)
{
	get_ucode_rev(FIRMWARE_UCODE_2, f, 
		      f->uc2_pos, 
		      f->uc2_length);
	get_ucode_rev(FIRMWARE_UCODE_4, f, 
		      f->uc4_pos, 
		      f->uc4_length);
	get_ucode_rev(FIRMWARE_UCODE_5, f, 
		      f->uc5_pos, 
		      f->uc5_length);
	get_ucode_rev(FIRMWARE_UCODE_11, f, 
		      f->uc11_pos, 
		      f->uc11_length);
	get_ucode_rev(FIRMWARE_UCODE_13, f, 
		      f->uc13_pos, 
		      f->uc13_length);
}

static void print_banner(void)
{
	printf("bcm43xx-fwcutter version " FWCUTTER_VERSION "\n");
}

static void print_file(const struct file *file)
{
	char filename[30];
	char shortname[30];

	if (strlen(file->name) > 20) {
		strncpy(shortname, file->name, 18);
		snprintf(filename, sizeof(filename), "%s..", shortname);
	} else
		strcpy (filename, file->name);

	printf("%s\t", filename);
	if (strlen(filename) < 8) printf("\t");
	if (strlen(filename) < 16) printf("\t");

	printf("%s\t", file->version);
	if (strlen(file->version) < 8) printf("\t");
	if (strlen(file->version) < 16) printf("\t");

	printf("%s\n", file->md5);
}

static void print_supported_files(void)
{
	int i;

	print_banner();
	printf("\nExtracting firmware is possible "
	       "from these binary driver files:\n\n");
	printf("<filename>\t\t"
	       "<version>\t\t"
	       "<MD5 checksum>\n\n");
	for (i = 0; i < FILES; i++) {
		if (files[i].flags & DRIVER_UNSUPPORTED)
			continue;
		print_file(&files[i]);
	}
	printf("\n\nExtracting firmware is IMPOSSIBLE "
	       "from these binary driver files:\n\n");
	printf("<filename>\t\t"
	       "<version>\t\t"
	       "<MD5 checksum>\n\n");
	for (i = 0; i < FILES; i++) {
		if (!(files[i].flags & DRIVER_UNSUPPORTED))
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
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x"
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		 signature[0], signature[1], signature[2], signature[3],
		 signature[4], signature[5], signature[6], signature[7],
		 signature[8], signature[9], signature[10], signature[11],
		 signature[12], signature[13], signature[14], signature[15]);

	for (i = 0; i < FILES; ++i) {
		if (strcasecmp(md5sig, files[i].md5) == 0) {
			if (files[i].flags & DRIVER_UNSUPPORTED) {
				printf("Extracting firmware from this file "
				       "is IMPOSSIBLE. (e.g. too old/new)\n");
				return 0;
			}
			printf("\n  filename   :  %s\n", files[i].name);
			printf("  version    :  %s\n", files[i].version);
			printf("  MD5        :  %s\n", files[i].md5);
			printf("  microcodes :  ");
			if (files[i].uc2_length > 0) printf("2 ");
			if (files[i].uc4_length > 0) printf("4 ");
			if (files[i].uc5_length > 0) printf("5 ");
			if (files[i].uc11_length > 0) printf("11 ");
			if (files[i].uc13_length > 0) printf("13 ");
			printf("\n  pcms       :  ");
			if (files[i].pcm4_length > 0) printf("4 ");
			if (files[i].pcm5_length > 0) printf("5 ");
			printf("\n\n");
			if (files[i].flags & MISSING_INITVAL_80211_A) {
				printf("WARNING! This firmware doesn't "
				       "include support for 802.11a cards.\n");
				printf("WARNING! Use this firmware only for "
				       "802.11b/g cards.\n\n");
			}
			return &(files[i]);
		}
	}
	printf("Sorry, the input file is either wrong or "
	       "not supported by bcm43xx-fwcutter.\n");
	printf("This file has an unknown MD5sum %s.\n", md5sig);

	return 0;
}

static void get_endianess(void)
{
	const unsigned char x[] = { 0xde, 0xad, 0xbe, 0xef, };
	const uint32_t *p = (uint32_t *)x;

	if (*p == 0xdeadbeef) {
		big_endian_cpu = 1;
	} else if (*p == 0xefbeadde) {
		big_endian_cpu = 0;
	} else {
		printf("Confused: NUXI endian machine??\n");
		exit(-1);
	}
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
	printf("  -p|--postfix \".FOO\"   "
	       "Postfix for firmware filenames (.FOO.fw)\n");
	printf("  -v|--version          "
	       "Print bcm43xx-fwcutter version\n");
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

	get_endianess();

	cmdargs.target_dir = ".";
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

	get_ucode_info(file);

	if (cmdargs.identify_only) {
		err = 0;
		goto out_close;
	}

	if (file->uc2_length > 0)
		extract_fw(FIRMWARE_UCODE_2, 
			   file->flags, 
			   file->uc2_pos, 
			   file->uc2_length);
	if (file->uc4_length > 0)
		extract_fw(FIRMWARE_UCODE_4, 
			   file->flags, 
			   file->uc4_pos, 
			   file->uc4_length);
	if (file->uc5_length > 0)
		extract_fw(FIRMWARE_UCODE_5, 
			   file->flags, 
			   file->uc5_pos, 
			   file->uc5_length);
	if (file->uc11_length > 0)
		extract_fw(FIRMWARE_UCODE_11, 
			   file->flags, 
			   file->uc11_pos, 
			   file->uc11_length);
	if (file->uc13_length > 0)
		extract_fw(FIRMWARE_UCODE_13, 
			   file->flags, 
			   file->uc13_pos, 
			   file->uc13_length);
	if (file->pcm4_length > 0)
		extract_fw(FIRMWARE_PCM_4, 
			   file->flags, 
			   file->pcm4_pos, 
			   file->pcm4_length);
	if (file->pcm5_length > 0)
		extract_fw(FIRMWARE_PCM_5, 
			   file->flags, 
			   file->pcm5_pos, 
			   file->pcm5_length);
	if (file->iv_map > 0)
		extract_iv(file->flags, 
			   file->iv_pos, 
			   file->iv_map);

	err = 0;
out_close:
	fclose(fd);

	return err;
}
