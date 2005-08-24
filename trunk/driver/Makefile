
obj-m := bcm430x.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	find . \( -name '*.ko' -o -name '*.o' -o -name '*~' -o -name '.*.cmd' -o -name '*.mod.c' \) \
		-type f -print | xargs rm -f

