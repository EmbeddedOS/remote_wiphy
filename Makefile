obj-m := usb_wifi.o
KERNEL_DIR := /home/conguyen/repos/jetson/Linux_for_Tegra/source/kernel/kernel-jammy-src
CROSS_COMPILE := /home/conguyen/repos/personal/arm64-toolchain/bin/aarch64-none-linux-gnu-
CC := $(CROSS_COMPILE)gcc
ARCH := arm64

all: usr
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean