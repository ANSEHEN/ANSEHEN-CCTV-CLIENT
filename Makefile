#KDIR = /lib/modules/3.18.7-v7/build
KDIR = /lib/modules/4.9.31-v7+/build

obj-m := gpio_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

