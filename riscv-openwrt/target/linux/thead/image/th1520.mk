define Device/sipeed_lpi4a
  DEVICE_VENDOR := Sipeed
  DEVICE_MODEL := LicheePi4A
  UBOOT_DEVICE_NAME := light-lpi4a
  IMAGE/sysupgrade.img.gz := boot-common | boot-script | lpi4a-img | gzip | append-metadata
endef
TARGET_DEVICES += sipeed_lpi4a
