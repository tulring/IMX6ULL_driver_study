KERNELDIR := /home/book/arm/imx6ull/ebf_6ull_linux
CURRENT_PATH := $(shell pwd)

obj-m :=  miscleds.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean