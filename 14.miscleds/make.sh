#!/bin/bash
sudo make -j12  ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
arm-linux-gnueabihf-gcc miscledApp.c -o miscledApp
sudo rm -r .*.cmd *.mod.* *.o *.symvers *.order
sudo cp *.ko /home/turing/arm/nfs/modules
sudo cp *App /home/turing/arm/nfs/modules
