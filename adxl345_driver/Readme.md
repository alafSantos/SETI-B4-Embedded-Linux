# Useful commands
## Lauching QEMU
./qemu-system-arm -nographic -machine vexpress-a9 -kernel linux-5.15.6/build/arch/arm/boot/zImage -dtb linux-5.15.6/build/arch/arm/boot/dts/vexpress-v2p-ca9.dtb -initrd initramfs_busybox/initramfs.gz -fsdev local,path=pilote_i2c,security_model=mapped,id=mnt -device virtio-9p-device,fsdev=mnt,mount_tag=mnt

## Mounting mnt
mount -t 9p -o trans=virtio mnt /mnt -oversion=9p2000.L,msize=10240

## Loading the module
insmod /mnt/adxl345.ko

## Removing the module
rmmod adxl345

## Compiling the driver
make CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm KDIR=../linux-5.15.6/build/

## Compiling the test application
arm-linux-gnueabihf-gcc -static -Wall -o main main.c
