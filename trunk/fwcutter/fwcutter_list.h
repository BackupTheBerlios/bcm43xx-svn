
static const struct file {
	const char *version;
	const unsigned char *md5;
	const int byteorder;
	const uint32 uc2_pos;
	const uint32 uc2_length;
	const uint32 uc4_pos;
	const uint32 uc4_length;
	const uint32 uc5_pos;
	const uint32 uc5_length;
	const uint32 pcm4_pos;
	const uint32 pcm4_length;
	const uint32 pcm5_pos;
	const uint32 pcm5_length;
} files[] = 
{
	{ 
		.version     = "3.20.23.0",                            /* bcmwl5.sys  06/13/2003 */ 
		.md5         = "1b1cf5e962c15abca83d1ef2b3906e2f",
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.uc2_pos     = 0x2d228,
		.uc2_length  = 0x3d9f,
		.uc4_pos     = 0x30fd8,
		.uc4_length  = 0x4467,
		.uc5_pos     = 0x35450,
		.uc5_length  = 0x4b9f,
		.pcm4_pos    = 0x39ff8,
		.pcm4_length = 0x477,
		.pcm5_pos    = 0,		/* not available, */
		.pcm5_length = 0,		/* driver is too old? */
	},
	{ 
		.version     = "3.30.15.0",                            /* bcmwl5.sys  07/17/2003 */ 
		.md5         = "ebf36d658d0da5b1ea667fa403919c26", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.uc2_pos     = 0x2f738,
		.uc2_length  = 0x3d87,
		.uc4_pos     = 0x334c8,
		.uc4_length  = 0x449f,
		.uc5_pos     = 0x37970, 
		.uc5_length  = 0x4ebf,
		.pcm4_pos    = 0x3c838,
		.pcm4_length = 0x477,
		.pcm5_pos    = 0x3ccb8,
		.pcm5_length = 0x477,
	},
	{ 
		.version     = "3.40.25.3",                            /* bcmwl5.sys  10/28/2003 */ 
		.md5         = "5e58a3148b98c9f356cde6049435cb21", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.uc2_pos     = 0x34408,
		.uc2_length  = 0x3db7,
		.uc4_pos     = 0x381c0, 
		.uc4_length  = 0x45cf,
		.uc5_pos     = 0x3c798, 
		.uc5_length  = 0x504f,
		.pcm4_pos    = 0x417f0,
		.pcm4_length = 0x477,
		.pcm5_pos    = 0x41c70,
		.pcm5_length = 0x477,
	},
	{ 
		.version     = "3.70.22.0",                            /* bcmwl5.sys  10/20/2004 */
		.md5         = "185a6dc6d655dc31c0b228cc94fb99ac", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.uc2_pos     = 0x3d6d0,
		.uc2_length  = 0x3e7f,
		.uc4_pos     = 0x41558, 
		.uc4_length  = 0x4ecf,
		.uc5_pos     = 0x46430, 
		.uc5_length  = 0x567f,
		.pcm4_pos    = 0x4bab8,
		.pcm4_length = 0x477,
		.pcm5_pos    = 0x4bf38,
		.pcm5_length = 0x477,
	},
};

#define FILES (sizeof(files) / sizeof(files[0]))

