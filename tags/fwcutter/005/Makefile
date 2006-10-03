VERSION = 005

FIRMWARE_INSTALL_DIR ?= /lib/firmware

CC ?= cc
PREFIX ?= /usr/local
CFLAGS ?= -O2 -fomit-frame-pointer
CFLAGS += -std=c99 -Wall -pedantic -D_BSD_SOURCE
LDFLAGS ?=


OBJECTS = fwcutter.o md5.o

CFLAGS += -DFWCUTTER_VERSION_=$(VERSION)

all: fwcutter

fwcutter: $(OBJECTS)
	$(CC) $(CFLAGS) -o bcm43xx-fwcutter $(OBJECTS) $(LDFLAGS)

install: all
	-install -d -o 0 -g 0 -m 755 $(PREFIX)/bin/
	-install -o 0 -g 0 -m 755 bcm43xx-fwcutter $(PREFIX)/bin/
	-install -d -o 0 -g 0 -m 755 $(PREFIX)/man/man1/
	-install -o 0 -g 0 -m 644 bcm43xx-fwcutter.1 $(PREFIX)/man/man1/

clean:
	-rm -f *~ *.o *.orig *.rej *.fw bcm43xx-fwcutter

installfw:
	-if ! [ -d $(FIRMWARE_INSTALL_DIR) ]; then mkdir -p $(FIRMWARE_INSTALL_DIR); fi
	-install -o 0 -g 0 -m 600 bcm43xx_*.fw $(FIRMWARE_INSTALL_DIR)

# dependencies
fwcutter.o: fwcutter.h md5.h fwcutter_list.h
md5.o: md5.h
