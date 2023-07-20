#!/bin/bash

install_depend(){
  sudo apt update -y
  sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
  bzip2 ccache cmake cpio curl device-tree-compiler fastjar flex gawk gettext gcc-multilib g++-multilib \
  git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libglib2.0-dev libgmp3-dev libltdl-dev \
  libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libreadline-dev libssl-dev libtool lrzsz \
  mkisofs msmtp nano ninja-build p7zip p7zip-full patch pkgconf python2.7 python3 python3-pyelftools \
  libpython3-dev qemu-utils rsync scons squashfs-tools subversion swig texinfo uglifyjs upx-ucl unzip \
  vim wget xmlto xxd zlib1g-dev
}

download_toolchain(){
  wget https://github.com/chainsx/armbian-riscv-build/releases/download/toolchain/Xuantie-900-gcc-linux-5.10.4-glibc-x86_64-V2.6.1-20220906.tar.gz
  tar -zxf Xuantie-900-gcc-linux-5.10.4-glibc-x86_64-V2.6.1-20220906.tar.gz
  rm Xuantie*tar.gz && mv Xuantie* riscv64-gcc
}

compile_u-boot(){
  git clone --depth=1 https://github.com/chainsx/thead-u-boot.git -b extlinux
  cd thead-u-boot
  make ARCH=riscv CROSS_COMPILE=../riscv64-gcc/bin/riscv64-unknown-linux-gnu- light_lpi4a_defconfig
  make ARCH=riscv CROSS_COMPILE=../riscv64-gcc/bin/riscv64-unknown-linux-gnu- -j$(nproc)
  cp u-boot-with-spl.bin ..
  cd ..
}

clone_kernel(){
  git clone https://github.com/revyos/thead-kernel.git -b lpi4a --depth=1
  cp patches/*.patch thead-kernel
  cd thead-kernel
  patch -p1 < 001-fix-build.patch
  cd ..
  git config --global user.name "openwrt"
  git config --global user.email "openwrt@openwrt.lan"
  git add . && git commit -m "user_patch"
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

install_depend
download_toolchain
compile_u-boot
clone_kernel
update_feeds
apply_feeds
make_op
