VERSION = 001

CC = cc
PREFIX = /usr/local
CFLAGS = -std=c99 -O2 -fomit-frame-pointer -Wall -D_BSD_SOURCE
LDFLAGS =


OBJECTS = bcm43xx_ival.o utils.o

CFLAGS += -DVERSION_=$(VERSION)

all: bcm43xx_ival

bcm43xx_ival: $(OBJECTS)
	$(CC) $(CFLAGS) -o bcm43xx-ival $(OBJECTS) $(LDFLAGS)

install: all
	-install -o 0 -g 0 -m 755 bcm43xx-ival $(PREFIX)/bin/

clean:
	-rm -f *~ *.o *.orig *.rej bcm43xx-ival

# dependencies
bcm43xx_ival.o: bcm43xx_ival.h utils.h
utils.o: utils.h
