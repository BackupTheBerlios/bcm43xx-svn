# Makefile for bcm430x driver

VERSION		:= 0.0.1
RELEASE_NAME	:= bcm430x-$(VERSION)

KVER		:= $(shell uname -r)
KDIR		?= /lib/modules/$(KVER)/build
PWD		:= $(shell pwd)
MODPATH		:= $(DESTDIR)/lib/modules/$(KVER)/kernel/drivers/net/bcm430

# Comment/uncomment to enable/disable debugging
DEBUG = y


ifeq ($(DEBUG),y)
DEBUGFS_OBJ = bcm430x_debugfs.o
CFLAGS += -O2 -DBCM430x_DEBUG
else
DEBUGFS_OBJ =
CFLAGS += -O2 -DBCM430x_NDEBUG
endif

CFLAGS += -DBCM430x_VERSION=$(VERSION) -I/lib/modules/$(KVER)/include

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m := bcm430x.o
bcm430x-objs := bcm430x_main.o bcm430x_dma.o $(DEBUGFS_OBJ) \
		bcm430x_radio.o bcm430x_phy.o \
		bcm430x_power.o bcm430x_wx.o \
		bcm430x_pio.o bcm430x_ilt.o

else

default: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: bcm430x.ko
	install -d $(MODPATH)
	install -m 644 -c bcm430x.ko $(MODPATH)
	/sbin/depmod -a

uninstall:
	rm -rf $(MODPATH)
	/sbin/depmod -a

endif

clean:
	find . \( -name '*.ko' -o -name '*.o' -o -name '.tmp_versions' -o -name '*~' -o -name '.*.cmd' \
		-o -name '*.mod.c' -o -name '*.tar.bz2' -o -name '*.rej' -o -name '*.orig' \)\
		-print | xargs rm -Rf

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

DISTFILES =  $(shell find . \( -not -name '.' \) -print | grep -v "\.tar\.bz2" | grep -v "\/\." )
DISTDIR = $(RELEASE_NAME)

release: clean
	@rm -rf $(DISTDIR)
	@mkdir $(DISTDIR)
	@chmod 777 $(DISTDIR)
	@for file in $(DISTFILES); do                   \
                if test -d $$file; then                 \
                        mkdir $(DISTDIR)/$$file;        \
                else                                    \
                        cp -p $$file $(DISTDIR)/$$file; \
                fi;                                     \
        done
	@tar -c $(DISTDIR) | bzip2 -9 > $(RELEASE_NAME).tar.bz2
	@rm -rf $(DISTDIR)
