## 开始

### 环境准备

推荐使用 ubuntu-20.04 x86_64 主机进行编译，磁盘空间剩余大于 50 GB。

#### 安装依赖

```
sudo apt update -y
sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
bzip2 ccache cmake cpio curl device-tree-compiler fastjar flex gawk gettext gcc-multilib g++-multilib \
git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libglib2.0-dev libgmp3-dev libltdl-dev \
libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libreadline-dev libssl-dev libtool lrzsz \
mkisofs msmtp nano ninja-build p7zip p7zip-full patch pkgconf python2.7 python3 python3-pyelftools \
libpython3-dev qemu-utils rsync scons squashfs-tools subversion swig texinfo uglifyjs upx-ucl unzip \
vim wget xmlto xxd zlib1g-dev
```

#### 下载源码

```
git clone https://github.com/chainsx/openwrt-th1520 --depth=1
```

#### 更新代码

```
cd openwrt-th1520/riscv-openwrt
./scripts/feeds update -a
./scripts/feeds install -a
cd ..
```

#### 应用配置

```
cp lpi4a.config riscv-openwrt/
cd riscv-openwrt && make defconfig
```

#### 自行配置

```
make menuconfig
```

#### 编译

```
make download V=s -j$(nproc) && make V=s -j$(nproc)
```

### 一次构建脚本

```
bash build_all.sh
```

## 如何使用

### 刷写 u-boot 到 EMMC

1.  [从这里](./u-boot-with-spl.bin)下载 u-boot 文件。

2.  按住板子上的 BOOT 键，然后将板子用数据线与电脑连接。

3.  以下是在 Linux 上刷写 u-boot 的命令示例，Windows 同理。


```
sudo ./fastboot flash ram ./images/u-boot-with-spl.bin
sudo ./fastboot reboot
sleep 1
sudo ./fastboot flash uboot ./images/u-boot-with-spl.bin
```

### 刷写 openwrt 到 SD

自行编译的 openwrt 将会在 `riscv-openwrt/bin/target` 文件夹下生成，将其解压然后刷写到 SD 卡上。

## 感谢

[Estela ad Astra](https://github.com/saeziae)
