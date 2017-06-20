#!/bin/bash

export ARCH=arm64
#export CROSS_COMPILE=/opt/toolchains/aarch64-gnu-4.9/bin/aarch64-

if [ ! -d $(pwd)/output ];
    then
        mkdir $(pwd)/output;
    fi

make -C $(pwd) O=output ARCH=arm64 a3xelte_00_defconfig
make -j6 -C $(pwd) O=output ARCH=arm64

$(pwd)/dtbtool  -o $(pwd)/output/arch/arm64/boot/dt.img -s 2048 -p $(pwd)/output/scripts/dtc/ $(pwd)/output/arch/arm64/boot/dts/ -v

cp output/arch/arm64/boot/Image  output/arch/arm64/boot/boot.img-kernel

exit