CC = cc
CFLAGS = -O2 -fomit-frame-pointer -Wall -pipe
LDFLAGS =
OBJECTS = add_sta.o

all: add_sta

add_sta: $(OBJECTS)
	$(CC) $(CFLAGS) -o add_sta $(OBJECTS) $(LDFLAGS)

clean:
	-rm -f *.o *.orig *.rej add_sta

# dependencies
add_sta.o: hostapd_ioctl.h ieee80211_shared.h
