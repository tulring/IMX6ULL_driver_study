#KERNELDIR := /home/turing/arm/imx6ull-ebf/debian/ebf-buster-linux/output
KERNELDIR := /home/turing/arm/imx6ull-ebf/ebf_6ull_linux

CURRENT_PATH := $(shell pwd)

obj-m :=  imx6uirq.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean