/*
 * firmware cutter for broadcom 43xx wireless driver files
 * 
 * Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>
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

#define BYTE_ORDER_UNKNOWN   0
#define BYTE_ORDER_DDCCBBAA  1

#include "md5.h"
#include "fwcutter_list.h"


static void write_ddccbbaa(FILE *f, byte *buffer, int len) 
{
	while (len > 0) {
		fprintf(f, "%02x%02x%02x%02x", buffer[3], buffer[2], buffer[1], buffer[0]);
		buffer = buffer + 4;
		len  = len - 4;
	}
}

static void write_fw(const char *infilename, const char *outfilename, int byteorder, byte *data, int len)
{
	FILE* fw;

	fw = fopen(outfilename, "w");
	if (!fw) {
		perror(outfilename);
		exit(1);
	}

        switch(byteorder) {
        case BYTE_ORDER_DDCCBBAA:
		write_ddccbbaa(fw, data, len);
                break;
        case BYTE_ORDER_UNKNOWN:
        default:
                printf("unknown byteorder...\n");
        }

	fflush(fw);
	fclose(fw);
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

static void extract_fw(const char *infile, const char *outfile, int byteorder, uint32 pos, uint32 length)
{
	byte* filedata;

	if (length > 0) {
		printf("extracting %s ...\n", outfile);
		filedata = read_file(infile);
		write_fw(infile, outfile, byteorder, filedata + pos, length);
		free(filedata);
	}
}

int main(int argc, char *argv[])
{
	unsigned char buffer[16384], signature[16];
	char md5sig[33];
	char *cp;
	FILE *in = stdin;
	int i, opt, count = 0;
	struct MD5Context md5c;

	for (i = 1; i < argc; i++) {
		cp = argv[i];
		if (*cp == '-') {
			if (strlen(cp) == 1) {
				i++;
				break;
			}
			opt = *(++cp);
			if (isupper(opt)) {
				opt = tolower(opt);
			}

			switch (opt) {
			case 'i':
				cp++;
				if ((in = fopen(cp, "rb")) == NULL) {
					fprintf(stderr, "Cannot open input file %s\n", cp);
					return 2;
				}

				MD5Init(&md5c);
				while ((i = (int) fread(buffer, 1, sizeof buffer, in)) > 0) {
					MD5Update(&md5c, buffer, (unsigned) i);
				}
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
						extract_fw(cp, "microcode2.fw", files[i].byteorder, files[i].uc2_pos, files[i].uc2_length);
						extract_fw(cp, "microcode4.fw", files[i].byteorder, files[i].uc4_pos, files[i].uc4_length);
						extract_fw(cp, "microcode5.fw", files[i].byteorder, files[i].uc5_pos, files[i].uc5_length);
						extract_fw(cp, "pcm4.fw", files[i].byteorder, files[i].pcm4_pos, files[i].pcm4_length);
						extract_fw(cp, "pcm5.fw", files[i].byteorder, files[i].pcm5_pos, files[i].pcm5_length);
						++count;
					}
				}

				if (count == 0) {
					fputs("driver version unknown or wrong input file.\n", stderr);
					return 1;
				}

				break;
			case 'l':
				printf("\nlist of driver files which are ready for firmware extracting:\n");
				for (i = 0; i < FILES; ++i) {
					printf("\nversion: %s   md5sum: ", files[i].version); 
					printf(files[i].md5);
				}
				printf("\n");
				break;
			case 'v':
				printf("fwcutter 0.0.1\n");
				break;
			}
		}
	}

	return 0;
}
