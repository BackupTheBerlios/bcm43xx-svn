VERSION = 0.0.1

CC = cc
PREFIX = /usr/local
CFLAGS = -std=c99 -O2 -Wall -pedantic -D_BSD_SOURCE
LDFLAGS =


OBJECTS = fwcutter.o md5.o

CFLAGS += -DFWCUTTER_VERSION_=$(VERSION)

all: fwcutter

fwcutter: $(OBJECTS)
	$(CC) $(CFLAGS) -o fwcutter $(OBJECTS) $(LDFLAGS)

install: all
	-install -o 0 -g 0 -m 755 fwcutter $(PREFIX)/bin/

clean:
	-rm -f *~ *.o *.orig *.rej *.fw fwcutter

installfw:
	-if ! [ -d /lib/firmware ]; then mkdir /lib/firmware; fi
	-install -o 0 -g 0 -m 600 bcm430x_*.fw /lib/firmware/

# dependencies
fwcutter.o: md5.h fwcutter_list.h
md5.o: md5.h
