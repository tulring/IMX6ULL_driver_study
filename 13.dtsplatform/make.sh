#!/bin/bash
sudo make -j12  ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
arm-linux-gnueabihf-gcc ledApp.c -o ledApp
sudo rm -r .*.cmd *.mod.* *.o *.symvers *.order
sudo cp *.ko /home/book/arm/imx6ull/eth_file/modules
sudo cp *App /home/book/arm/imx6ull/eth_file/modules