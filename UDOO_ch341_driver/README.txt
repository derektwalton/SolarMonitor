1/8/25

The radiant controller PCB based on an Arduino nano has been down for a year or two now.  Issue
seems to be that the nano is no longer functioning correctly.  I purchased new nano's (clones, 
I suppose) and find they use a different USB to serial converter.  This creates challenges as
the drivers are immature everywhere and result in spotty connectivity even on latest Ubuntu and
Windows platforms.  These devices are DEFINITELY not supported by the Ubuntu 14.04 based Linux
on our Udoo dual board.

To resolve this issue I needed to do quite a bit of exploration to get a driver.  Here is how
I finally found success.

1. Start with fresh UDOO mini-SD card

Start with the latest Udoo provided image "UDOObuntu 2.2.0 (Ubuntu 14.04 LTS)" from 
https://www.udoo.org/resources-quad-dual/ .  Follow the usual process to burn this image onto
a mini-SD card.  I then usually boot in the image and setup a manual IP address of 
192.168.100.101 so that I can easily log in remotely (rsh, VNC, scp, etc. work out of the
box with the Udoo provided image.

2. Build a patched CH341 driver

Get a system together to re-build the Udoo image.  I setup a VMware virtual machine with 
Ubuntu 14.04 and then followed https://www.udoo.org/docs/Advanced_Topics/Compile_Linux_Kernel.html 
replicated below:

(a) Install the required packages
Some packages are needed to compile the Linux Kernel for UDOO boards. E.g. in Ubuntu 14.04 it is necessary to install the following packages:

sudo apt-get update
sudo apt-get install gawk wget git diffstat unzip texinfo gcc-multilib \
     build-essential chrpath socat libsdl1.2-dev xterm picocom ncurses-dev lzop \
     gcc-arm-linux-gnueabihf


I ran into some conflict between gcc-multilib and gcc-arm-linux-gnuabihf, but I think installing
gcc-arm-linux-gnuabihf on its own afterward resolved things.

(b) Get the kernel sources from GitHub
Create a develop folder

mkdir udoo-dev
cd udoo-dev
then download the sources:

git clone https://github.com/UDOOboard/linux_kernel
cd linux_kernel
The default branch 3.14-1.0.x-udoo is the one where we are working on for the UDOO QUAD/DUAL. It is based on 3.14.56 Freescale community kernel.

(c) Replace the original ch341.c driver with a patched one from https://github.com/juliagoda/CH341SER .

cp ../CH341SER/ch34x.c ./drivers/usb/serial/ch341.c


(d) Load the default kernel configuration
UDOO QUAD/DUAL has a dedicated default kernel configuration that you can import with:

ARCH=arm make udoo_quad_dual_defconfig

(e) Add ch341 driver by personalizing the kernel configuration
Add or remove kernel modules to fit your project:

ARCH=arm make menuconfig

Navigate to Device Drivers -> USB support -> USB Serial Converter support 

and then select <M> next to "USB Winchiphead CH341 Single Port Serial Driver"

and save modified configuration.

(f) Compile sources
To build the kernel image, type:

ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make zImage -j5
You can safely tweak the -jX parameter. For instance, on quad core CPUs with two threads per core, you can use -j8.

The build can take several minutes, approximately from 2 to 15, depending on your PC host or VM configuration.

[...]
  Kernel: arch/arm/boot/Image is ready
  LZO     arch/arm/boot/compressed/piggy.lzo
  CC      arch/arm/boot/compressed/decompress.o
  CC      arch/arm/boot/compressed/string.o
  SHIPPED arch/arm/boot/compressed/hyp-stub.S
  SHIPPED arch/arm/boot/compressed/lib1funcs.S
  SHIPPED arch/arm/boot/compressed/ashldi3.S
  SHIPPED arch/arm/boot/compressed/bswapsdi2.S
  AS      arch/arm/boot/compressed/hyp-stub.o
  AS      arch/arm/boot/compressed/lib1funcs.o
  AS      arch/arm/boot/compressed/ashldi3.o
  AS      arch/arm/boot/compressed/bswapsdi2.o
  AS      arch/arm/boot/compressed/piggy.lzo.o
  LD      arch/arm/boot/compressed/vmlinux
  OBJCOPY arch/arm/boot/zImage
  Kernel: arch/arm/boot/zImage is ready

(g) Compile Device Trees
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make dtbs -j5

(h)
Compile the modules
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make modules -j5

All we really want from this whole build is ./drivers/usb/serial/ch341.ko.  I placed a prebuilt version at
CH341SER/ch341.ko.  Get this file over to the UDOO machine and then we can follow the directions in
CH341SER/Makefile to load the driver:

sudo modprobe usbserial
sudo insmod 
(plug in nano)
dmesg
[    5.697153] ch341 1-1.1:1.0: ch34x converter detected
[    5.701675] usb 1-1.1: ch34x converter now attached to ttyUSB0

To install this driver so it will be auto-loaded on reboot do:

sudo cp ch341.ko /lib/modules/3.14.56-udooqdl-02044-gddaad11/kernel/drivers/usb/serial/
sudo depmod

More useful info on this driver install at https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all.

(i) Alternatively, you can copy the newly built kernel and modules to the SD card
You can overwrite the kernel on a UDOObuntu SD card with the following commands:

BOOT_PARTITION=/path/to/boot-partition
ROOT_PARTITION=/path/to/root-partition

cp arch/arm/boot/zImage $BOOT_PARTITION
cp arch/arm/boot/dts/*.dtb $BOOT_PARTITION/dts
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make firmware_install modules_install INSTALL_MOD_PATH=$ROOT_PARTITION

