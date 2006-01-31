VERSION = 0.1

CC = cc
PREFIX = /usr/local
CFLAGS = -std=c99 -O2 -fomit-frame-pointer -Wall
LDFLAGS =


OBJECTS = bcm43xx_sprom.o utils.o

CFLAGS += -DVERSION_=$(VERSION)

all: bcm43xx_sprom

bcm43xx_sprom: $(OBJECTS)
	$(CC) $(CFLAGS) -o bcm43xx-sprom $(OBJECTS) $(LDFLAGS)

install: all
	-install -o 0 -g 0 -m 755 bcm43xx-sprom $(PREFIX)/bin/

clean:
	-rm -f *~ *.o *.orig *.rej bcm43xx-sprom

# dependencies
bcm43xx_sprom.o: bcm43xx_sprom.h utils.h
utils.o: utils.h bcm43xx_sprom.h
