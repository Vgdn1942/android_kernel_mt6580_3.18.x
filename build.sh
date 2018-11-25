#!/bin/bash
export KBUILD_BUILD_USER=kurilko
export KBUILD_BUILD_HOST=4pda
export CROSS_COMPILE=/home/patch/kernel/arm-linux-gnueabihf-linaro-6.3.1-mt6580-master/bin/arm-linux-gnueabihf-
export USE_CCACHE=1
export ARCH=arm ARCH_MTK_PLATFORM=mt6580
export TARGET=out
make O=$TARGET ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE P350_defconfig
make O=$TARGET ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE -j4

