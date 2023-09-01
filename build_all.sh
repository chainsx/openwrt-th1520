#!/bin/bash

install_depend(){
  sudo apt update -y
  sudo apt install -y ack binutils bison build-essential \
  ccache cmake device-tree-compiler flex gawk gettext \
  git gperf intltool libelf-dev libglib2.0-dev \
  libgmp3-dev libltdl-dev libncurses5-dev libssl-dev \
  libreadline-dev libtool wget nano patch sudo \
  pkgconf python3 python3-pyelftools xxd zlib1g-dev \
  subversion swig texinfo unzip rsync gcc-riscv64-linux-gnu
}

compile_u-boot(){
  git clone --depth=1 https://github.com/chainsx/thead-u-boot.git -b extlinux
  cd thead-u-boot

  make ARCH=riscv CROSS_COMPILE=riscv64-ulinux-gnu- light_lpi4a_defconfig
  make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)
  cp u-boot-with-spl.bin ../lpi4a-8g-u-boot-with-spl.bin

  make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- clean

  make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- light_lpi4a_16g_defconfig
  make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)
  cp u-boot-with-spl.bin ../lpi4a-16g-u-boot-with-spl.bin
  cd ..
}

update_feeds(){
  cd riscv-openwrt
  ./scripts/feeds update -a
  ./scripts/feeds install -a
  cd ..
}

apply_feeds(){
  cp lpi4a.config riscv-openwrt/.config
  cd riscv-openwrt && make defconfig
}

make_op(){
  make download V=s -j$(nproc) && make V=s -j$(nproc)
}

work_dir=$(pwd)
install_depend
compile_u-boot
update_feeds
apply_feeds
make_op
