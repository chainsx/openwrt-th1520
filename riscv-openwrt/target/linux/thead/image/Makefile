# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2013 OpenWrt.org

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

DEVICE_VARS += UBOOT_DEVICE_NAME

define Build/boot-common
	# This creates a new folder copies the dtb (as rockchip.dtb) 
	# and the kernel image (as kernel.img)
	rm -fR $@.boot
	mkdir -p $@.boot

	$(CP) $(DTS_DIR)/$(DEVICE_DTS).dtb $@.boot/light-lpi4a.dtb
	$(CP) $(IMAGE_KERNEL) $@.boot/kernel.img
endef

define Build/boot-script
	# Make an boot script and copy it to the boot partition
	$(CP) config.txt $@.boot/config.txt
	$(CP) light_aon_fpga.bin $@.boot/light_aon_fpga.bin
	$(CP) light_c906_audio.bin $@.boot/light_c906_audio.bin
endef

define Build/lpi4a-img
	# Creates the final SD/eMMC images, 
	# combining boot partition, root partition as well as the u-boot bootloader

	./gen_thead_sdcard_image.sh \
		$@ \
		$(CONFIG_TARGET_KERNEL_PARTSIZE) $@.boot \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) $(IMAGE_ROOTFS) \
		32768

	# Copy the u-boot-with-spl image to the image at sector 0x40 and 0x4000
	#dd if="$(STAGING_DIR_IMAGE)"/u-boot-with-spl.bin of="$@" seek=34 conv=notrunc
endef

### Devices ###
define Device/Default
  PROFILES := Default
  KERNEL := kernel-bin
  IMAGES := sysupgrade.img.gz
  DEVICE_DTS = thead/light-$(lastword $(subst _, ,$(1)))
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
