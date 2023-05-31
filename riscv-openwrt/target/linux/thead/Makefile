# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013-2014 OpenWrt.org

include $(TOPDIR)/rules.mk

ARCH:=riscv64
BOARD:=thead
BOARDNAME:=T-Head TH1520
FEATURES:=audio display fpu gpio pcie rtc usb usbgadget squashfs targz nand ubifs boot-part rootfs-part
SUBTARGETS:=th1520

KERNEL_PATCHVER:=5.10

include $(INCLUDE_DIR)/target.mk

KERNELNAME:= Image dtbs

DEFAULT_PACKAGES += mkf2fs e2fsprogs blkid

$(eval $(call BuildTarget))
