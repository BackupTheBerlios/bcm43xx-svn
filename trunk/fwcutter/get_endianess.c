#include <stdint.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
	uint32_t x;

	((unsigned char *)&x)[0] = 0xde;
	((unsigned char *)&x)[1] = 0xad;
	((unsigned char *)&x)[2] = 0xbe;
	((unsigned char *)&x)[3] = 0xef;

	if (x == 0xdeadbeef)
		printf("-DBIG_ENDIAN_CPU");
	else
		printf("-DLITTLE_ENDIAN_CPU");
}
