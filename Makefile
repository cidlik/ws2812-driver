KERNEL_VERSION = $(shell uname -r)
KERNEL_DIR = /lib/modules/$(KERNEL_VERSION)/build
PWD = $(shell pwd)

obj-m := ws2812.o

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -rf build

# https://gist.github.com/strezh/01a1849128109e53a65df696d1b2c113
install:
	sudo cp ws2812.ko /lib/modules/$$(uname -r)/kernel/drivers/char
	bash -c "grep ws2812 /etc/modules || echo ws2812 | sudo tee -a /etc/modules"
	sudo depmod
