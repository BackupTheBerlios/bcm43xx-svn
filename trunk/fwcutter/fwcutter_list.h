

static const struct file {
	const char *name;
	const char *version;
	const char *md5;
	const uint8_t flags;
	const uint32_t iv_pos;
	const uint32_t uc2_pos;
	const uint32_t uc2_length;
	const uint32_t uc4_pos;
	const uint32_t uc4_length;
	const uint32_t uc5_pos;
	const uint32_t uc5_length;
	const uint32_t pcm4_pos;
	const uint32_t pcm4_length;
	const uint32_t pcm5_pos;
	const uint32_t pcm5_length;
} files[] = 
{
	{
		.name        = "AppleAirPort2",
		.version     = "3.30.15.p8 (3.3b1)",			/* 01/19/2004 */
		.md5         = "87c74c55d2501d2e968f8c132e160b6e",
		.flags       = BYTE_ORDER_AABBCCDD,
		.iv_pos      = 269452,
		.uc2_pos     = 278500,   .uc2_length  = 15752,
		.uc4_pos     = 294256,   .uc4_length  = 17586,
		.uc5_pos     = 311828,   .uc5_length  = 20160,
		.pcm4_pos    = 331992,   .pcm4_length = 1144,
		.pcm5_pos    = 333140,   .pcm5_length = 1144,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.50.37.p4 (3.4.3f1)",			/* 09/29/2004 */
		.md5         = "c672b8c218c5dc4a55060bdfa9f58a69",
		.flags       = BYTE_ORDER_AABBCCDD | INIT_VAL_08_MISSING,
		.iv_pos      = 324472,
		.uc2_pos     = 339552,   .uc2_length  = 15664,
		.uc4_pos     = 355220,   .uc4_length  = 17864,
		.uc5_pos     = 373088,   .uc5_length  = 21760,
		.pcm4_pos    = 394852,   .pcm4_length = 1144,
		.pcm5_pos    = 396000,   .pcm5_length = 1144,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.50.37.p6",
		.md5         = "a62e35ee9956b286c46b145d35bd6e0c",
		.flags       = BYTE_ORDER_AABBCCDD,
		.iv_pos      = 0x4f9b8,
		.uc2_pos     = 0x534a0,  .uc2_length  = 0x3d30,
		.uc4_pos     = 0x571d4,  .uc4_length  = 0x45c8,
		.uc5_pos     = 0x5b7a0,  .uc5_length  = 0x5500,
		.pcm4_pos    = 0x60ca4,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x61120,  .pcm5_length = 0x478,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.50.37.p6",
		.md5         = "b6f3d2437c40277c197f0afcf12208e9",
		.flags       = BYTE_ORDER_AABBCCDD,
		.iv_pos      = 0x4f9b8,
		.uc2_pos     = 0x534a0,  .uc2_length  = 0x3d30,
		.uc4_pos     = 0x571d4,  .uc4_length  = 0x45c8,
		.uc5_pos     = 0x5b7a0,  .uc5_length  = 0x5500,
		.pcm4_pos    = 0x60ca4,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x61120,  .pcm5_length = 0x478,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.90.34.0.p11 (400.17)",			/* 09/13/2005 (??) */
		.md5         = "ca0f34df2f0bfb8b5cfd83b5848d2bf5",
		.flags       = BYTE_ORDER_AABBCCDD | INIT_VAL_08_MISSING,
		.iv_pos      = 327468,
		.uc2_pos     = 333852,   .uc2_length  = 16200,
		.uc4_pos     = 350052,   .uc4_length  = 19952,
		.uc5_pos     = 370004,   .uc5_length  = 22496,
		.pcm4_pos    = 392500,   .pcm4_length = 1312,
		.pcm5_pos    = 393812,   .pcm5_length = 1312,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.90.34.0.p11",
		.md5         = "dc3a69aac95c68fe8edc760e39bbb2c9",
		.flags       = BYTE_ORDER_AABBCCDD | SUPPORT_INCOMPLETE,
		.iv_pos      = 0x50efc,                                /* A-PHY init vals empty */
		.uc2_pos     = 0x527ec,   .uc2_length  = 0x3f48,
		.uc4_pos     = 0x56734,   .uc4_length  = 0x4df0,
		.uc5_pos     = 0x5b524,   .uc5_length  = 0x57e0,
		.pcm4_pos    = 0x60d04,   .pcm4_length = 0x520,
		.pcm5_pos    = 0x61224,   .pcm5_length = 0x520,
	},
	{
		.name        = "AppleAirPort2",
		.version     = "3.90.34.0.p13 (401.2)",                /* 07/10/2005 */
		.md5         = "6ecf38e5ab6997c7ec483c0d637f5c68",
		.flags       = BYTE_ORDER_AABBCCDD | INIT_VAL_08_MISSING,
		.iv_pos      = 0x50fcc,
		.uc2_pos     = 0x528bc,  .uc2_length  = 0x3f48,
		.uc4_pos     = 0x56804,  .uc4_length  = 0x4df0,
		.uc5_pos     = 0x5b5f4,  .uc5_length  = 0x57e0,
		.pcm4_pos    = 0x60dd4,  .pcm4_length = 0x520,
		.pcm5_pos    = 0x612f4,  .pcm5_length = 0x520,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.10.8.0",                             /* 10/04/2002 */ 
		.md5         = "288923b401e87ef76b7ae2652601ee47",
		.flags       = SUPPORT_IMPOSSIBLE,                     /* file differs from later ones */
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.10.53.6",                            /* 04/28/2003 */ 
		.md5         = "b43c593fd7c2a47cdc40580fe341f674",
		.flags       = SUPPORT_IMPOSSIBLE,                     /* file differs from later ones */
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.20.23.0",                            /* 06/13/2003 */ 
		.md5         = "1b1cf5e962c15abca83d1ef2b3906e2f",     /* pcm5 not available, driver is too old */
		.flags       = BYTE_ORDER_DDCCBBAA | INIT_VAL_08_MISSING 
		               | SUPPORT_INCOMPLETE,
		.iv_pos      = 0x2a1d0,
		.uc2_pos     = 0x2d228,  .uc2_length  = 0x3d9f,
		.uc4_pos     = 0x30fd8,  .uc4_length  = 0x4467,
		.uc5_pos     = 0x35450,  .uc5_length  = 0x4b9f,
		.pcm4_pos    = 0x39ff8,  .pcm4_length = 0x477,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.30.15.0",                            /* 07/17/2003 */ 
		.md5         = "ebf36d658d0da5b1ea667fa403919c26", 
		.flags       = BYTE_ORDER_DDCCBBAA | INIT_VAL_08_MISSING,
		.iv_pos      = 0x2c658,
		.uc2_pos     = 0x2f738,  .uc2_length  = 0x3d87,
		.uc4_pos     = 0x334c8,  .uc4_length  = 0x449f,
		.uc5_pos     = 0x37970,  .uc5_length  = 0x4ebf,
		.pcm4_pos    = 0x3c838,  .pcm4_length = 0x477,
		.pcm5_pos    = 0x3ccb8,  .pcm5_length = 0x477,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.40.20.0",                            /* 09/24/2003 */ 
		.md5         = "0c3fc803184f6f85e665dd012611225b", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x302f0,
		.uc2_pos     = 0x33d88,  .uc2_length  = 0x3db8,
		.uc4_pos     = 0x37b48,  .uc4_length  = 0x45d8,
		.uc5_pos     = 0x3c128,  .uc5_length  = 0x5050,
		.pcm4_pos    = 0x41180,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x41600,  .pcm5_length = 0x478,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.40.25.3",                            /* 10/28/2003 */ 
		.md5         = "5e58a3148b98c9f356cde6049435cb21", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x30970,
		.uc2_pos     = 0x34408,  .uc2_length  = 0x3db7,
		.uc4_pos     = 0x381c0,  .uc4_length  = 0x45cf,
		.uc5_pos     = 0x3c798,  .uc5_length  = 0x504f,
		.pcm4_pos    = 0x417f0,  .pcm4_length = 0x477,
		.pcm5_pos    = 0x41c70,  .pcm5_length = 0x477,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.40.73.0",                            /* 06/25/2004 */ 
		.md5         = "52d67c5465c01913b03b7daca0cc4077", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x37398,
		.uc2_pos     = 0x3ae30,  .uc2_length  = 0x3ff0,
		.uc4_pos     = 0x3ee28,  .uc4_length  = 0x47f0,
		.uc5_pos     = 0x43620,  .uc5_length  = 0x5260,
		.pcm4_pos    = 0x48888,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x48d08,  .pcm5_length = 0x478,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.50.21.11",                           /* 02/19/2004 */
		.md5         = "ae96075a3aed5c40f1ead477ea94acd7", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x33370,
		.uc2_pos     = 0x36e58,	 .uc2_length  = 0x3dff,
		.uc4_pos     = 0x3ac60,	 .uc4_length  = 0x4627,
		.uc5_pos     = 0x3f290,	 .uc5_length  = 0x5547,
		.pcm4_pos    = 0x447e0,	 .pcm4_length = 0x477,
		.pcm5_pos    = 0x44c60,	 .pcm5_length = 0x477,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.60.7.0",                             /* 03/22/2004 */
		.md5         = "c5616736df4e83930780dca5795387ca", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3b988,
		.uc2_pos     = 0x3f580,	 .uc2_length  = 0x3e08,
		.uc4_pos     = 0x43390,	 .uc4_length  = 0x4e58,
		.uc5_pos     = 0x481f0,	 .uc5_length  = 0x5608,
		.pcm4_pos    = 0x4d800,	 .pcm4_length = 0x478,
		.pcm5_pos    = 0x4dc80,	 .pcm5_length = 0x478,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.70.22.0",                            /* 10/20/2004 */
		.md5         = "185a6dc6d655dc31c0b228cc94fb99ac", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x39a88,
		.uc2_pos     = 0x3d6d0,	 .uc2_length  = 0x3e7f,
		.uc4_pos     = 0x41558,	 .uc4_length  = 0x4ecf,
		.uc5_pos     = 0x46430,	 .uc5_length  = 0x567f,
		.pcm4_pos    = 0x4bab8,	 .pcm4_length = 0x477,
		.pcm5_pos    = 0x4bf38,	 .pcm5_length = 0x477,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.100.46.0",                           /* 12/22/2004 */
		.md5         = "38ca1443660d0f5f06887c6a2e692aeb", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3de80,
		.uc2_pos     = 0x41af0,	 .uc2_length  = 0x3f57,
		.uc4_pos     = 0x45a50,	 .uc4_length  = 0x4df7,
		.uc5_pos     = 0x4a850,	 .uc5_length  = 0x57f7,
		.pcm4_pos    = 0x50050,	 .pcm4_length = 0x51f,
		.pcm5_pos    = 0x50578,	 .pcm5_length = 0x51f,
	},
	{ 
		.name        = "bcmwl5.sys",
		.version     = "3.100.64.0",                           /* 02/11/2005 */
		.md5         = "e7debb46b9ef1f28932e533be4a3d1a9", 
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3e980,
		.uc2_pos     = 0x425f0,	 .uc2_length  = 0x3f57,
		.uc4_pos     = 0x46550,	 .uc4_length  = 0x4dff,
		.uc5_pos     = 0x4b358,	 .uc5_length  = 0x57ff,
		.pcm4_pos    = 0x50b60,	 .pcm4_length = 0x51f,
		.pcm5_pos    = 0x51088,	 .pcm5_length = 0x51f,
	},
	{
		.name        = "bcmwl5.sys",
		.version     = "3.100.65.1",                           /* 04/21/2005 */
		.md5         = "d5f1ab1aab8b81bca6f19da9554a267a",
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3e980,
		.uc2_pos     = 0x425f0,	 .uc2_length  = 0x3f57,
		.uc4_pos     = 0x46550,	 .uc4_length  = 0x4dff,
		.uc5_pos     = 0x4b358,	 .uc5_length  = 0x57ff,
		.pcm4_pos    = 0x50b60,	 .pcm4_length = 0x51f,
		.pcm5_pos    = 0x51088,	 .pcm5_length = 0x51f,
	},
	{
		.name        = "bcmwl564.sys",
		.version     = "3.70.17.5",                            /* 09/21/2004 */
		.md5         = "f5590c8784b91dfd9ee092d3040b6e40",     /* for 64bit machines   */
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x566f0,
		.uc2_pos     = 0x5a360,  .uc2_length  = 0x3e80,
		.uc4_pos     = 0x5e1f0,  .uc4_length  = 0x4ed0,
		.uc5_pos     = 0x630d0,  .uc5_length  = 0x5680,
		.pcm4_pos    = 0x68760,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x68be0,  .pcm5_length = 0x478,
	},
	{
		.name        = "bcmwl5a.sys",
		.version     = "3.90.16.0",                            /* 12/06/2004 */
		.md5         = "e6d927deea6c75bddf84080e6c3837b7",
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3b4c8,
		.uc2_pos     = 0x3f138,  .uc2_length  = 0x3f48,
		.uc4_pos     = 0x43088,  .uc4_length  = 0x4de8,
		.uc5_pos     = 0x47e78,  .uc5_length  = 0x57d8,
		.pcm4_pos    = 0x4d658,  .pcm4_length = 0x520,
		.pcm5_pos    = 0x4db80,  .pcm5_length = 0x520,
	},
	{
		.name        = "wl.o",
		.version     = "3.50.21.0",                            /* 05/11/2003 */
		.md5         = "f71be0e1d14f68c98d916465a300d835",
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x38990,
		.uc2_pos     = 0x3c428,  .uc2_length  = 0x3db8,
		.uc4_pos     = 0x401e4,  .uc4_length  = 0x45d8,
		.uc5_pos     = 0x447c0,  .uc5_length  = 0x5050,
		.pcm4_pos    = 0x49814,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x49c90,  .pcm5_length = 0x478,
	},
	{
		.name        = "wl.o",
		.version     = "3.50.21.10",                           /* 01/21/2004 */
		.md5         = "191029d5e7097ed7db92cbd6e6131f85",
		.flags       = BYTE_ORDER_DDCCBBAA,
		.iv_pos      = 0x3a5d0,
		.uc2_pos     = 0x3e0b8,  .uc2_length  = 0x3e00,
		.uc4_pos     = 0x41ebc,  .uc4_length  = 0x4628,
		.uc5_pos     = 0x464e8,  .uc5_length  = 0x5548,
		.pcm4_pos    = 0x4ba34,  .pcm4_length = 0x478,
		.pcm5_pos    = 0x4beb0,  .pcm5_length = 0x478,
	},
};

#define FILES (sizeof(files) / sizeof(files[0]))

