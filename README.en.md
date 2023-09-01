![image1](https://raw.githubusercontent.com/chainsx/openwrt-th1520/main/lpi4a.png)

## Getting started

### Prepare your environment

Build host: Ubuntu 22.04, Debian 12

#### Install depend

```
sudo apt update -y

sudo apt install -y ack binutils bison build-essential \
	ccache cmake device-tree-compiler flex gawk gettext \
	git gperf intltool libelf-dev libglib2.0-dev \
	libgmp3-dev libltdl-dev libncurses5-dev libssl-dev \
	libreadline-dev libtool wget nano patch sudo \
	pkgconf python3 python3-pyelftools xxd zlib1g-dev \
	subversion swig texinfo unzip rsync
```

#### Clone OpenWrt source code

```
git clone https://github.com/chainsx/openwrt-th1520 --depth=1
```

#### Update feeds

```
cd openwrt-th1520/riscv-openwrt
./scripts/feeds update -a
./scripts/feeds install -a
cd ..
```

#### Apply build configure

```
cp lpi4a.config riscv-openwrt/.config
cd riscv-openwrt && make defconfig
```

#### Build openwrt

```
make download V=s -j$(nproc) && make V=s -j$(nproc)
```

### Simply start with the build script

```
bash build_all.sh
```

## How to flash

Reference [here](https://github.com/chainsx/armbian-riscv-build/blob/main/doc/licheepi-4a-install-guide.md).

## How to enable WiFi

```
wifi config
reboot
```

## Thanks

[Rabenda aka revy](https://github.com/Rabenda)

[saeziae](https://github.com/saeziae)
