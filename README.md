# U-boot for ARTIK530 and ARTIK710
## Contents
1. [Introduction](#1-introduction)
2. [Build guide](#2-build-guide)
3. [Update guide](#3-update-guide)

## 1. Introduction
This 'u-boot-artik' repository is u-boot source for artik710 and artik530.
The base version of the u-boot of artik710/artik530 is based on 2016-01 version.

---
## 2. Build guide
### 2.1 Install cross compiler
#### ARTIK710
- You'll need an arm64 cross compiler
```
sudo apt-get install gcc-aarch64-linux-gnu device-tree-compiler
```
If you can't install the above toolchain, you can use linaro toolchain.
```
wget https://releases.linaro.org/components/toolchain/binaries/5.4-2017.05/aarch64-linux-gnu/gcc-linaro-5.4.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-5.4.1-2017.05-x86_64_aarch64-linux-gnu.tar.xz
export PATH=~/gcc-linaro-5.4.1-2017.05-x86_64_aarch64-linux-gnu/bin:$PATH
```

#### ARTIK530
```
sudo apt-get install gcc-arm-linux-gnueabihf
```
If you can't install the above toolchain, you can use linaro toolchain.
```
wget https://releases.linaro.org/components/toolchain/binaries/5.4-2017.05/arm-linux-gnueabihf/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf.tar.xz
tar xf gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf.tar.xz
export PATH=~/gcc-linaro-5.4.1-2017.05-x86_64_arm-linux-gnueabihf/bin:$PATH
```
You can the path permernently through adding it into ~/.bashrc

### 2.2 Build the u-boot with helper script of build-artik
#### ARTIK710
```
cd build-artik
./build_uboot.sh -b artik710
```
You can find the u-boot binaries from the build-artik/output/images/artik710/

#### ARTIK530
```
cd build-artik
./build_uboot.sh -b artik530
```
You can find the u-boot binaries from the build-artik/output/images/artik530/

### 2.3 Build the u-boot with manual way
#### ARTIK710

```
make ARCH=arm artik710_raptor_defconfig
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
```

#### ARTIK530
```
make ARCH=arm artik530_raptor_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

### 2.4 Generate params.bin
The params.bin contains u-boot environment variables and you can generate the file using below commands:

#### ARTIK710
```
cp `find . -name "env_common.o"` copy_env_common.o
aarch64-linux-gnu-objcopy -O binary --only-section=.rodata.default_environment `find . -name "copy_env_common.o"`
tr '\0' '\n' < copy_env_common.o | grep '=' > default_envs.txt
cp default_envs.txt default_envs.txt.orig
tools/mkenvimage -s 16384 -o params.bin default_envs.txt
```

#### ARTIK530
```
cp `find . -name "env_common.o"` copy_env_common.o
arm-linux-gnueabihf-objcopy -O binary --only-section=.rodata.default_environment `find . -name "copy_env_common.o"`
tr '\0' '\n' < copy_env_common.o | grep '=' > default_envs.txt
cp default_envs.txt default_envs.txt.orig
tools/mkenvimage -s 16384 -o params.bin default_envs.txt
```

### 2.5 Generate fip/nexell compatable image
The u-boot.bin cannot be loaded by bl1/bl2 bootloaders. You have to generate
a fip image and boot image for nexell bl1.

#### ARTIK710
- generate a fip-nonsecure.bin image using fip_create tool(input: u-boot.bin, output:fip-nonsecure.bin)
```
tools/fip_create/fip_create \
	--dump --bl33 u-boot.bin \
	fip-nonsecure.bin
```
- generate a fip-nonsecure.img using SECURE_BINGEN tool(input: fip-nonsecure.bin, output: fip-nonsecure.img)
```
tools/nexell/SECURE_BINGEN \
	-c S5P6818 -t 3rdboot \
	-n tools/nexell/nsih/raptor-64.txt \
	-i fip-nonsecure.bin \
	-o fip-nonsecure.img \
	-l 0x7df00000 -e 0x00000000
```

#### ARTIK530
- generate a bootloader.img using BOOT_BINGEN tool
```
tools/nexell/SECURE_BINGEN \
	-c S5P4418 -t 3rdboot \
	-n tools/nexell/nsih/raptor-sd.txt \
	-i u-boot.bin \
	-o bootloader.img \
	-l 0x94c00000 -e 0x94c00000
```

---
## 3. Update Guide
You can update the u-boot through fastboot or micro sd card(ext4 partition)

### 3.1 Fastboot
You should prepare a micro USB cable to connect between the board and host PC

- Install android-tools-fastboot
```
sudo apt-get install android-tools-fastboot
wget -S -O - http://source.android.com/source/51-android.rules | sed "s/<username>/$USER/" | sudo tee >/dev/null /etc/udev/rules.d/51-android.rules; sudo udevadm control --reload-rules
```

- Insert the USB cable(not MicroUSB Cable) into your board. Enter u-boot shell mode during boot countdown:
```
Net:   No ethernet found.
Hit any key to stop autoboot:  0
ARTIK710 #
ARTIK710 #
ARTIK710 # fastboot 0
```

#### ARTIK710
- You'll need to upload the partmap_emmc.txt prior than uploading the binaries. It can be downloaded from boot-firmwares-artik710. On your Host PC(Linux), flash the u-boot using below command
```
sudo fastboot flash partmap partmap_emmc.txt
sudo fastboot flash fip-nonsecure fip-nonsecure.img
sudo fastboot flash env params.bin
sudo fastboot reboot
```

#### ARTIK530
- You'll need to upload the partmap_emmc.txt prior than uploading the binaries. It can be downloaded from boot-firmwares-artik530. On your Host PC(Linux), flash the u-boot using below command
```
sudo fastboot flash partmap partmap_emmc.txt
sudo fastboot flash bootloader bootloader.img
sudo fastboot flash env params.bin
sudo fastboot reboot
```

### 3.2 microSD Card
Prepare a micro SD card and format to ext4 file system.
```
sudo mkfs.ext4 /dev/sd[X]1
sudo mount /dev/sd[X]1 /mnt
```

#### ARTIK710
- Copy compiled binaries(fip-nonsecure.img, params.bin) into a micro sd card.
```
sudo cp fip-nonsecure.img params.bin partmap_emmc.txt /mnt
sudo umount /mnt
```
- Insert the microSD card into your board and enter u-boot shell during boot countdown
```
sd_recovery mmc 1:1 48000000 partmap_emmc.txt
```

#### ARTIK530
- Copy compiled binaries(bootloader.img, params.bin) into a micro sd card.
```
sudo cp bootloader.img params.bin partmap_emmc.txt /mnt
sudo umount /mnt
```
- Insert the microSD card into your board and enter u-boot shell during boot countdown
```
sd_recovery mmc 1:1 99000000 partmap_emmc.txt
```
