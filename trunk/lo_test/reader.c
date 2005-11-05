#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>

static FILE *f = NULL;

/* this is userspace. I won't care about closing the file */
static open_file()
{
	if (f == NULL) {
		f = fopen("log", "r");
	}
}

static char g_direction = '\0';
static int g_count = 0;
static int g_offset = 0;
static int g_value = 0;
static char g_time[100];

/*
 * the log line format I'm using is
 * 1131185442.931569 0x155b45e4 mem r 0x03fe 2     0xdb7c
 * ---- time ------- ---addr--- typ ^ offset bytes value-
 *                                  |
 *                                  \- direction
 */
static parse_log_line()
{
	int dummy[3];
	char type[4];
	char buf[1024];
	open_file();
	fgets(buf, 1024, f);
//	printf("read line %s", buf);
	sscanf(buf, "%s %x %s %c %x %d %x", &g_time, &dummy[2], &type,
	       &g_direction, &g_offset, &g_count, &g_value);
//	printf(" -- direction %c\n", g_direction);
//	printf(" -- offset    0x%.4x\n", g_offset);
//	printf(" -- count     %d\n", g_count);
//	printf(" -- value     0x%x\n", g_value);
}

static void assert(int condition)
{
	if (!condition) {
		printf("read/write failed with timestamp %s\n", g_time);
		exit(2);
	}
}

static int assert_read(int count, int offset)
{
	parse_log_line();
	assert('r' == g_direction);
	assert(count == g_count);
	assert(offset == g_offset);
	return g_value;
}

static void assert_write(int count, int offset, int value)
{
	parse_log_line();
	assert('w' == g_direction);
	assert(count == g_count);
	assert(offset == g_offset);
	assert(value == g_value);
}

u16 bcm430x_read16(struct bcm430x_private *bcm, u16 offset)
{
	return (u16) assert_read(2, offset);
}

void bcm430x_write16(struct bcm430x_private *bcm, u16 offset,
		     u16 value)
{
	assert_write(2, offset, value);
}

u32 bcm430x_read32(struct bcm430x_private *bcm, u16 offset)
{
	return (u32) assert_read(4, offset);
}

void bcm430x_write32(struct bcm430x_private *bcm, u16 offset,
		     u32 value)
{
	assert_write(4, offset, value);
}
