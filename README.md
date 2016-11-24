# U-boot for ARTIK5 and ARTIK10
## Contents
1. [Introduction](#1-introduction)
2. [Build guide](#2-build-guide)
3. [Update guide](#3-update-guide)

## 1. Introduction
This 'u-boot-artik' repository is u-boot source for artik710. The base version
The u-boot of artik710 is based on 2016-01 version.

---
## 2. Build guide
### 2.1 Install cross compiler
+ For artik710> You'll need an arm64 cross compiler
```
sudo apt-get install gcc-aarch64-linux-gnu
```
If you can't install the above toolchain, you can use linaro toolchain.
```
wget https://releases.linaro.org/components/toolchain/binaries/5.3-2016.05/aarch64-linux-gnu/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu.tar.xz
export PATH=~/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu:$PATH
```
You can the path permernently through adding it into ~/.bashrc

### 2.2 Build the u-boot
+ For artik710>
```
make ARCH=arm artik710_raptor_defconfig
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
```

### 2.3 Generate params.bin
The params.bin contains u-boot environment variables and you can generate the file using below commands:

+ For artik710>
```
cp `find . -name "env_common.o"` copy_env_common.o
aarch64-linux-gnu-objcopy -O binary --only-section=.rodata.default_environment `find . -name "copy_env_common.o"`
tr '\0' '\n' < copy_env_common.o | grep '=' > default_envs.txt
cp default_envs.txt default_envs.txt.orig
tools/mkenvimage -s 16384 -o params.bin default_envs.txt
```

### 2.4 Generate fip/nexell compatable image
The u-boot.bin cannot be loaded by bl1/bl2 bootloaders. You have to generate
a fip image and boot image for nexell bl1.

+ For artik710>
a. generate a fip-nonsecure.bin image using fip_create tool(input: u-boot.bin, output:fip-nonsecure.bin)
```
tools/fip_create/fip_create \
	--dump --bl33 u-boot.bin \
	fip-nonsecure.bin
```
b. generate a fip-nonsecure.img using BOOTBIN_GEN tool(input: fip-nonsecure.bin, output: fip-nonsecure.img)
```
tools/nexell/SECURE_BINGEN \
	-c S5P6818 -t 3rdboot \
	-n tools/nexell/nsih/raptor-64.txt \
	-i fip-nonsecure.bin \
	-o fip-nonsecure.img \
	-l 0x7df00000 -e 0x00000000
```

---
## 3. Update Guide
You can update the u-boot through fastboot or micro sd card(ext4 partition)

### 3.1 Fastboot
For artik710, you should prepare micro USB cable.

Install android-tools-fastboot
```
sudo apt-get install android-tools-fastboot
wget -S -O - http://source.android.com/source/51-android.rules | sed "s/<username>/$USER/" | sudo tee >/dev/null /etc/udev/rules.d/51-android.rules; sudo udevadm control --reload-rules
```

Insert the USB cable(not MicroUSB Cable) into your board.

Enter u-boot shell mode during boot countdown:
```
Net:   No ethernet found.
Hit any key to stop autoboot:  0
ARTIK710 #
ARTIK710 #
ARTIK710 # fastboot 0
```

You'll need to upload the partmap_emmc.txt prior than uploading the binaries.
It can be downloaded from boot-firmwares-artik710.
On your Host PC(Linux), flash the u-boot using below command
```
sudo fastboot flash partmap partmap_emmc.txt
sudo fastboot flash fip-nonsecure fip-nonsecure.img
sudo fastboot flash env params.bin
sudo fastboot reboot
```

### 3.2 microSD Card
Prepare a micro SD card and format to ext4 file system.
```
sudo mkfs.ext4 /dev/sd[X]1
sudo mount /dev/sd[X]1 /mnt
```
Copy compiled binaries(fip-nonsecure.img, params.bin) into a micro sd card.
```
sudo cp fip-nonsecure.img params.bin /mnt
sudo umount /mnt
```

Insert the microSD card into your board and enter u-boot shell during boot countdown
```
sd_recovery mmc 1:1 48000000 partmap_emmc.txt
```
