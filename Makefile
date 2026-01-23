obj-m := usb_wifi.o
KERNEL_DIR := /home/conguyen/repos/jetson/Linux_for_Tegra/source/kernel/kernel-jammy-src
CROSS_COMPILE := /home/conguyen/repos/jetson/aarch64--glibc--stable-2022.08-1/bin/aarch64-buildroot-linux-gnu-
CC := $(CROSS_COMPILE)gcc
ARCH := arm64

all:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean