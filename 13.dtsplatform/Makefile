#KERNELDIR := /home/book/arm/imx6ull/ebf_6ull_linux
KERNELDIR := /home/turing/arm/imx6ull-ebf/debian/ebf-buster-linux/output
CURRENT_PATH := $(shell pwd)

obj-m :=  dtsleddriver.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
