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

update_feeds(){
  cd riscv-openwrt
  ./scripts/feeds update -a
  ./scripts/feeds install -a
  cd ..
}

apply_feeds(){
  cp lpi4a.config riscv-openwrt/
  cd riscv-openwrt && make defconfig
}

make_op(){
  make download V=s -j$(nproc) && make V=s -j$(nproc)
}

install_depend
update_feeds
apply_feeds
make_op
