CC = cc
PREFIX = /usr/local
CFLAGS = -std=c99 -O2 -Wall -pedantic -D_BSD_SOURCE
LDFLAGS =


OBJECTS = fwcutter.o md5.o

CFLAGS += $(shell if [ -x ./.get_endianess ]; then ./.get_endianess; fi)

all: fwcutter

fwcutter: get_endianess $(OBJECTS)
	$(CC) $(CFLAGS) -o fwcutter $(OBJECTS) $(LDFLAGS)

install: all
	-install -o 0 -g 0 -m 755 fwcutter $(PREFIX)/bin/

clean:
	-rm -f *~ *.o *.orig *.rej .get_endianess *.fw fwcutter

get_endianess: get_endianess.o
	$(CC) $(CFLAGS) -o .get_endianess get_endianess.o

installfw:
	-if ! [ -d /lib/firmware ]; then mkdir /lib/firmware; fi
	-install -o 0 -g 0 -m 600 bcm430x_*.fw /lib/firmware/

# dependencies
fwcutter.o: md5.h fwcutter_list.h
md5.o: md5.h
