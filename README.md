ix4-300d
========

Mainline linux on ix4-300d

The objective was to create a device tree for this device on mainline linux.

Lenovo ix4-300d is a small 4 bay NAS based on armada XP dual core 78230 armv7 cpu.

The device has dual port ethernet, lcd screen, gpio buttons, usb 2.0 (2 rear) & 3.0 (1 front)

what is supported so far:
 - full armada xp cpu feature from mvebu linux mainline
 - i2c (mv64xxx-i2c) bus connected component : rtc pcf8563 module, adt7473 pwm/sensor managment
 - dual gigabit, full duplex mode
 - pcie two ports : one lane connected to the USB3.0 NEC D720200F1 controller, one to the 4 sata Marvell 88SX7042 adapter
 - both USB2.0 & USB3.0 working full & high speed
 - power, reset, select & scroll button support as gpio-key recognized as keyboard entry (working with acpid)
 - 74hc164 8-shift as gpio extender for power, sysfail, sys, hddfail gpio-led (note hdd led is gpio independant)
 - gpio-poweroff setup yet it does reboot instead of halt :( TODO track why and what to set for PIN24/MPP24 to halt instead or reboot the system
 - the integrated LCD is a gpio 8-bit 128x64 monchrome GLCD, in this projet is a small driver that create a /dev/lcm device to right byte graphics or ascii text to it.
 - UART on the 3.3V output on the board see pinout.png in the repository for RX,TX,GND pins

how to compile a kernel:
-----------------------
- just download a mainline kernel 3.15 or 3.16
- add the git dts to the arch/arm/boot/dts folder
- add the dts file as a dtb into the Makefile in the same folder
- apply the git ix4-300d_defconfig as .config in the kernel path
- make zImage (or cross compil it)
- then copy the bb.sh and adapt it to your path, it will generate a uImage file which can be used by the firmware u-boot

booting from USB:
----------------
- format a flash USB with 2 partition one VFAT big enough for a kernel 10-20Mb, one with a bootstrap linux a ext2,3 or 4
- connect UART to the lenovo
- hit a key to stop auto boot
- then on the promt you can boot by typing:
- usb reset;fatload usb 0:1 $loadaddr zImage;setenv bootargs $console $mtdparts root=/dev/sdb2 rw; bootm $loadaddr;
PLEASE NOTE this line is very specific to boting the lenovo with 1 drive connected on sata and USB key pluged in on the rear upper usb port (I was neither able to boot with a USB onnly with no sata drive connected, nor using the second USB or front USB connector)
PLEASE NOTE that the original firmware is a quite complete u-boot version with tft, nand, fat, ubi etc... boot support, you can look into u-boot documentation
PLEASE NOTE lenovo is making the u-boot and kernel source/patch they use available as part of the GPL

Using led:
---------
- since all leds are gpio-led ocnfigured you can use all the standard linux gpio-led interface and triggers (see /sys/class/led)

Controling brightness:
----------------------
 - the whole front panel brightness (includng lcd and all leds) are pwm brightness crontoled through pwm3 on the i2C (see /sys/class/i2c-0/0-0ée/pwm3 where you can simply cat 0..255 > pwm3))

Checking temperature:
--------------------
 - the hmon0 & hmon1 contains sensors and temps value for differents part of the nas.

Controlling fans:
----------------
 - fan is controlled though i2C using pwm1
NOTE: the original fan is quite noisy, compared to some noctua or other quality brand you can found, I've swicth mine to a 92mm PWM notcua one with great gain from a noise perspecitive

lcd screen:
----------
The lcd is a 128x64 pixel monchrome lcd drivers through RS, E, RW, S, and 8 bit gpio. I was able to code a ery light alpha driver for the kernel tha allow graphics and ascii display cat /dev/lcm give you the syntax for wriiting to the device

power-off:
---------
The current DTS include the handling of the poweroff by pulling up GPIO24. This works except the orion watchdog doesn't seems to be disabled and therefore reboots the nas instead of stoping it. One workarounf is not to compil watchdog support in the kernel this way you can shutdown the nas. I was not lucky with watchdog service or systemd disabling teh watchdog by clsing the /dev/watchdog file, even though the CONFIG_WATCHDOG_?OWAYOUT is n...
 
