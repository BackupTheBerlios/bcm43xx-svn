#ifndef _FWCUTTER_H_
#define _FWCUTTER_H_

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define DRIVER_UNSUPPORTED       0x01  /* no support for this driver file */
#define BYTE_ORDER_BIG_ENDIAN    0x02  /* ppc, bcm6345/6348 driver files */
#define BYTE_ORDER_LITTLE_ENDIAN 0x04  /* x86, mips driver files */
#define MISSING_INITVAL_80211_A  0x08  /* empty initvals 3,7,9,10 */

#define FIRMWARE_UCODE_OFFSET    100
#define FIRMWARE_UNDEFINED       0
#define FIRMWARE_PCM_4           4
#define FIRMWARE_PCM_5           5
#define FIRMWARE_UCODE_2         (FIRMWARE_UCODE_OFFSET + 2)
#define FIRMWARE_UCODE_4         (FIRMWARE_UCODE_OFFSET + 4)
#define FIRMWARE_UCODE_5         (FIRMWARE_UCODE_OFFSET + 5)
#define FIRMWARE_UCODE_11        (FIRMWARE_UCODE_OFFSET + 11)
#define FIRMWARE_UCODE_13        (FIRMWARE_UCODE_OFFSET + 13)

#define fwcutter_stringify_1(x)	#x
#define fwcutter_stringify(x)	fwcutter_stringify_1(x)
#define FWCUTTER_VERSION	fwcutter_stringify(FWCUTTER_VERSION_)

#define ARG_MATCH	0
#define ARG_NOMATCH	1
#define ARG_ERROR	-1

typedef unsigned char byte;

enum { /* initvals numbering schemes */
	INITVALS_MAP_UNKNOWN = 0,
	INITVALS_MAP_V3_WITHOUT_IV8,
	INITVALS_MAP_V3_DEFAULT,
	INITVALS_MAP_V3_REVERSE_ORDER,
	INITVALS_MAP_V3_UP_TO_REV11,
	INITVALS_MAP_V3_UP_TO_REV11_REVERSE_ORDER,
	INITVALS_MAP_V4_UP_TO_REV11,
	INITVALS_MAP_V4_UP_TO_REV13,
};

struct cmdline_args {
	const char *infile;
	const char *postfix;
	const char *target_dir;
	int identify_only;
};

struct file {
	const char *name;
	const char *version;
	const char *md5;
	const uint8_t flags;
	const uint32_t iv_pos;
	const uint8_t iv_map;
	const uint32_t uc2_pos;
	const uint32_t uc2_length;
	const uint32_t uc4_pos;
	const uint32_t uc4_length;
	const uint32_t uc5_pos;
	const uint32_t uc5_length;
	const uint32_t uc11_pos;
	const uint32_t uc11_length;
	const uint32_t uc13_pos;
	const uint32_t uc13_length;
	const uint32_t pcm4_pos;
	const uint32_t pcm4_length;
	const uint32_t pcm5_pos;
	const uint32_t pcm5_length;
};

struct initval_mapdef {
	const uint8_t type;
	const uint8_t number;
	const uint8_t scheme[30];
	};

static struct initval_mapdef ivmap[] =
{
	/* core rev 0x2, 0x4 initval numbers: 1, 2, 3, 4 */
	/* core rev 0x5 initval numbers: 5, 6, 7, 8, 9, 10 */
	/* core rev 0x9 initval numbers: 11, 12, 13, 14, 15, 16 */
	/* core rev 0xb initval numbers: 17, 18 */
	/* core rev 0xd initval numbers: 19, 20 */

	/* initval number 8 is missing in 3.20 and 3.30 */
	{ INITVALS_MAP_V3_WITHOUT_IV8, 9,
	  { 1, 2, 3, 4, 5, 6, 7, 9, 10 }},

	/* most 3.x versions since 3.40 */
	{ INITVALS_MAP_V3_DEFAULT, 10,
	  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }},

	/* Apple-x86 and Linux-BCM6345/6348 drivers are reverse ordered */ 
	{ INITVALS_MAP_V3_REVERSE_ORDER, 10,
	  { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }},

	/* version 3.130 */
	{ INITVALS_MAP_V3_UP_TO_REV11, 12,
	  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 18 }},

	/* Linux-BCM96348 driver 3.131 is reverse ordered */ 
	{ INITVALS_MAP_V3_UP_TO_REV11_REVERSE_ORDER, 12,
	  { 18, 17, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }},

	/* 4.x versions up to 4.40 */
	{ INITVALS_MAP_V4_UP_TO_REV11, 18,
	  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 }},

	/* version 4.80 and newer */
	{ INITVALS_MAP_V4_UP_TO_REV13, 20,
	  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	    19, 20 }},

	{ 0 },
};

#endif /* _FWCUTTER_H_ */
