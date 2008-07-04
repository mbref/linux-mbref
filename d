#!/bin/bash

#make mrproper && cp config .config && make oldconfig && make
location="arch/microblaze/boot/"
location2="/tftpboot/"
board=`cat .config | awk -F "[_=]" '/PLATFORM/ && /=y/{ print tolower($3)}'`
addr=`cat .config | awk -F "[=]" '/KERNEL_BASE_ADDR/{ print $2}'`

if [ "$addr" == "" ]
then
	error "Missing base address"
fi

echo Board is $board with baseaddr $addr

mkimage  -A microblaze -O linux -T kernel -C none -a $addr -e $addr -n "kernel bez FS" -d arch/microblaze/boot/linux.bin $location/image.ub;
mkimage  -A microblaze -O linux -T kernel -C none -a $addr -e $addr -n "kernel bez FS" -d arch/microblaze/boot/linux.bin $location2/image.ub;
cat System.map | grep __log_buf

dtc -v 2>/dev/null 1>&2
if [ $? == 0 ]
then
	echo nasel 
	dtc -f -O dtb -b 0 -V 16 arch/microblaze/platform/$board/system-full.dts > $location/system.dtb
	dtc -f -O dtb -b 0 -V 16 arch/microblaze/platform/$board/system-full.dts > $location2/system.dtb
fi


if [ -f arch/microblaze/platform/$board/romfs.img ]
then
	mkimage  -A microblaze -O linux -T ramdisk -C none -n "fs v no network" -d arch/microblaze/platform/$board/romfs.img $location/romfs.ub;
	mkimage  -A microblaze -O linux -T ramdisk -C none -n "fs v no network" -d arch/microblaze/platform/$board/romfs.img $location2/romfs.ub;
fi



