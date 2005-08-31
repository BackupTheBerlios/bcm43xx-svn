# Makefile for bcm430x driver

VERSION		:= 0.0.1
RELEASE_NAME	:= bcm430x-$(VERSION)

# Comment/uncomment to enable/disable debugging
DEBUG = y


ifeq ($(DEBUG),y)
DEBUGFS_OBJ = bcm430x_debugfs.o
CFLAGS += -O2 -g -DBCM430x_DEBUG
else
DEBUGFS_OBJ =
CFLAGS += -O2 -DBCM430x_NDEBUG
endif

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m := bcm430x.o
bcm430x-objs := bcm430x_main.o $(DEBUGFS_OBJ) bcm430x_ucode.o

else

KDIR	?= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

endif

clean:
	find . \( -name '*.ko' -o -name '*.o' -o -name '.tmp_versions' -o -name '*~' -o -name '.*.cmd' -o -name '*.mod.c' \)\
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
