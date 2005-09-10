
static const struct file {
	const char *version;
	const char *md5;
	const int byteorder;
	const uint32 iv_pos;
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
/*		.iv_pos      = 0x2a1d0,                               FIXME: one initvalXX.fw is missing */
		.uc2_pos     = 0x2d228,  .uc2_length  = 0x3d9f,
		.uc4_pos     = 0x30fd8,  .uc4_length  = 0x4467,
		.uc5_pos     = 0x35450,  .uc5_length  = 0x4b9f,
		.pcm4_pos    = 0x39ff8,  .pcm4_length = 0x477,
		.pcm5_pos    = 0,	 .pcm5_length = 0,	       /* not available, driver is too old */
	},
	{ 
		.version     = "3.30.15.0",                            /* bcmwl5.sys  07/17/2003 */ 
		.md5         = "ebf36d658d0da5b1ea667fa403919c26", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
/*		.iv_pos      = 0x2c658,                               FIXME: one initvalXX.fw is missing */
		.uc2_pos     = 0x2f738,  .uc2_length  = 0x3d87,
		.uc4_pos     = 0x334c8,  .uc4_length  = 0x449f,
		.uc5_pos     = 0x37970,  .uc5_length  = 0x4ebf,
		.pcm4_pos    = 0x3c838,  .pcm4_length = 0x477,
		.pcm5_pos    = 0x3ccb8,  .pcm5_length = 0x477,
	},
	{ 
		.version     = "3.40.25.3",                            /* bcmwl5.sys  10/28/2003 */ 
		.md5         = "5e58a3148b98c9f356cde6049435cb21", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x30970,
		.uc2_pos     = 0x34408,  .uc2_length  = 0x3db7,
		.uc4_pos     = 0x381c0,  .uc4_length  = 0x45cf,
		.uc5_pos     = 0x3c798,  .uc5_length  = 0x504f,
		.pcm4_pos    = 0x417f0,  .pcm4_length = 0x477,
		.pcm5_pos    = 0x41c70,  .pcm5_length = 0x477,
	},
	{ 
		.version     = "3.50.x.y",                            /* bcmwl5.sys */
		.md5         = "ae96075a3aed5c40f1ead477ea94acd7", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x33370,
		.uc2_pos     = 0x36e58,	 .uc2_length  = 0x3dff,
		.uc4_pos     = 0x3ac60,	 .uc4_length  = 0x4627,
		.uc5_pos     = 0x3f290,	 .uc5_length  = 0x5547,
		.pcm4_pos    = 0x447e0,	 .pcm4_length = 0x477,
		.pcm5_pos    = 0x44c60,	 .pcm5_length = 0x477,
	},
	{ 
		.version     = "3.70.22.0",                            /* bcmwl5.sys  10/20/2004 */
		.md5         = "185a6dc6d655dc31c0b228cc94fb99ac", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x39a88,
		.uc2_pos     = 0x3d6d0,	 .uc2_length  = 0x3e7f,
		.uc4_pos     = 0x41558,	 .uc4_length  = 0x4ecf,
		.uc5_pos     = 0x46430,	 .uc5_length  = 0x567f,
		.pcm4_pos    = 0x4bab8,	 .pcm4_length = 0x477,
		.pcm5_pos    = 0x4bf38,	 .pcm5_length = 0x477,
	},
	{ 
		.version     = "3.100.46.0",                           /* bcmwl5.sys  12/22/2004 */
		.md5         = "38ca1443660d0f5f06887c6a2e692aeb", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3de80,
		.uc2_pos     = 0x41af0,	 .uc2_length  = 0x3f57,
		.uc4_pos     = 0x45a50,	 .uc4_length  = 0x4df7,
		.uc5_pos     = 0x4a850,	 .uc5_length  = 0x57f7,
		.pcm4_pos    = 0x50050,	 .pcm4_length = 0x51f,
		.pcm5_pos    = 0x50578,	 .pcm5_length = 0x51f,
	},
	{ 
		.version     = "3.100.64.0",                           /* bcmwl5.sys  02/11/2005 */
		.md5         = "e7debb46b9ef1f28932e533be4a3d1a9", 
		.byteorder   = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3e980,
		.uc2_pos     = 0x425f0,	 .uc2_length  = 0x3f57,
		.uc4_pos     = 0x46550,	 .uc4_length  = 0x4dff,
		.uc5_pos     = 0x4b358,	 .uc5_length  = 0x57ff,
		.pcm4_pos    = 0x50b60,	 .pcm4_length = 0x51f,
		.pcm5_pos    = 0x51088,	 .pcm5_length = 0x51f,
	},
};

#define FILES (sizeof(files) / sizeof(files[0]))

