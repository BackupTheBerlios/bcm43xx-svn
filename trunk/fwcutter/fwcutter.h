#ifndef _FWCUTTER_H_
#define _FWCUTTER_H_

#define FW_FLAG_LE	0x01	/* little endian? convert */
#define FW_FLAG_V4	0x02	/* b43 vs. b43legacy */

#define fwcutter_stringify_1(x)	#x
#define fwcutter_stringify(x)	fwcutter_stringify_1(x)
#define FWCUTTER_VERSION	fwcutter_stringify(FWCUTTER_VERSION_)

#define ARG_MATCH	0
#define ARG_NOMATCH	1
#define ARG_ERROR	-1

struct cmdline_args {
	const char *infile;
	const char *target_dir;
	int identify_only;
};

struct insn {
	uint32_t opcode;
	uint16_t op1, op2, op3;
};

struct iv {
	uint16_t reg, size;
	uint32_t val;
} __attribute__((__packed__));

enum extract_type {
	EXT_UNDEFINED, /* error catcher  */
	EXT_UCODE_1,   /* rev  <= 4 ucode */
	EXT_UCODE_2,   /* rev 5..14 ucode */
	EXT_UCODE_3,   /* rev >= 15 ucode */
	EXT_PCM,       /* "pcm" values   */
	EXT_IV,        /* initial values */
};

struct extract {
	const char *name;
	const uint32_t offset;
	const uint32_t length;
	const enum extract_type type;
};

#define EXTRACT_LIST_END { .name = NULL, }

struct file {
	const char *name;
	const char *ucode_version;
	const char *md5;
	const struct extract *extract;
	const uint32_t flags;
};

#endif /* _FWCUTTER_H_ */
