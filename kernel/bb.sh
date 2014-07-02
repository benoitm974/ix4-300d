#!/bin/bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j7 dtbs
cat arch/arm/boot/zImage arch/arm/boot/dts/armada-xp-lenovo-ix4300d.dtb > zud
mkimage -A arm -O linux -T kernel -C none -a 0x00008000 -e 0x00008000 -n lenovo-ix4-300d-02 -d zud /tftpboot/uImage

