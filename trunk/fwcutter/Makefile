CC = cc
PREFIX = /usr
CFLAGS = -O2 -Wall
LDFLAGS =


OBJECTS = fwcutter.o md5.o

all: fwcutter

fwcutter: $(OBJECTS)
	$(CC) $(CFLAGS) -o fwcutter $(OBJECTS) $(LDFLAGS)

install: all
	-cp fwcutter $(PREFIX)/bin

clean:
	-rm -f *~ *.o *.orig *.rej

distclean: clean
	-rm -f fwcutter

# dependencies
fwcutter.o: md5.h fwcutter_list.h
md5.o: md5.h
