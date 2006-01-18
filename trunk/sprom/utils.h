#ifndef BCM43xx_SPROMTOOL_UTILS_H_
#define BCM43xx_SPROMTOOL_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef ATTRIBUTE_PRINTF_1_2
# undef ATTRIBUTE_PRINTF_1_2
#endif
#ifdef __GNUC__
# define ATTRIBUTE_PRINTF_1_2	__attribute__((format(printf, 1, 2)))
#else
# define ATTRIBUTE_PRINTF_1_2	/* nothing */
#endif

int prinfo(const char *fmt, ...) ATTRIBUTE_PRINTF_1_2;
int prerror(const char *fmt, ...) ATTRIBUTE_PRINTF_1_2;
int prdata(const char *fmt, ...) ATTRIBUTE_PRINTF_1_2;

void * malloce(size_t size);
void * realloce(void *ptr, size_t newsize);

/* CRC-8 with polynomial x^8+x^7+x^6+x^4+x^2+1 */
uint8_t crc8(uint8_t crc, uint8_t data);

#endif /* BCM43xx_SPROMTOOL_UTILS_H_ */
