VERSION = 001

CC = cc
PREFIX = /usr/local
CFLAGS = -std=c99 -O2 -fomit-frame-pointer -Wall -D_BSD_SOURCE -D_GNU_SOURCE
LDFLAGS =


OBJECTS = pcibx.o pcibx_device.o utils.o

CFLAGS += -DVERSION_=$(VERSION)

all: pcibx

pcibx: $(OBJECTS)
	$(CC) $(CFLAGS) -o pcibx $(OBJECTS) $(LDFLAGS)

install: all
	-install -o 0 -g 0 -m 755 pcibx $(PREFIX)/bin/

clean:
	-rm -f *~ *.o *.orig *.rej pcibx

# dependencies
pcibx.o: pcibx.h utils.h
pcibx_device.o: pcibx_device.h
utils.o: utils.h
