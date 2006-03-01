#ifndef PCIBX_UTILS_H_
#define PCIBX_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define pcibx_stringify_1(x)	#x
#define pcibx_stringify(x)	pcibx_stringify_1(x)

#ifdef ATTRIBUTE_FORMAT
# undef ATTRIBUTE_FORMAT
#endif
#ifdef __GNUC__
# define ATTRIBUTE_FORMAT(t, a, b)	__attribute__((format(t, a, b)))
#else
# define ATTRIBUTE_FORMAT(t, a, b)	/* nothing */
#endif

int prinfo(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
int prerror(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
int prdata(const char *fmt, ...) ATTRIBUTE_FORMAT(printf, 1, 2);

void internal_error(const char *message);
#define internal_error_on(condition) \
	do {								\
		if (condition)						\
			internal_error(pcibx_stringify(condition));	\
	} while (0)

void * malloce(size_t size);
void * realloce(void *ptr, size_t newsize);

void udelay(unsigned int usecs);
void msleep(unsigned int msecs);

#endif /* PCIBX_UTILS_H_ */
