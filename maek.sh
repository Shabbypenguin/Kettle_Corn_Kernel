#!/bin/bash

    THREADS=$(expr 4 + $(grep processor /proc/cpuinfo | wc -l))
    DEFCONFIG=m2_spr_defconfig
    ARCH="ARCH=arm"
    DIR=/home/shabbypenguin/Public
    RAMDISK=$DIR/Ramdisks/SPH-L710
    CMRAMDISK=$DIR/Ramdisks/SPH-L710-CM
    TOOLS=$DIR/Tools
    KERNEL=$DIR/Kettle_Corn_Kernel
    PACK=$KERNEL/package
    OUT=$KERNEL/arch/arm/boot
    LOG=$TOOLS/log.txt

    # Edit this to change the kernel name
    KBUILD_BUILD_VERSION="Kettle.Corn-1.1"
    export KBUILD_BUILD_VERSION

    MAKE="make -j${THREADS}"
    export USE_CCACHE=1
    export $ARCH

    # This cleans out crud and makes new config
    $MAKE clean
    $MAKE mrproper
    rm -rf $RAMDISK/lib
    rm -rf $PACK/*
    $MAKE $DEFCONFIG

    echo "All clean!" > $LOG
    date >> $LOG
    echo "" >> $LOG

    # Finally making the kernel
    $MAKE zImage
    $MAKE modules

    echo "Compiled" >> $LOG
    date >> $LOG
    echo "" >> $LOG

    # This command will detect if the folder is there and if not will recreate it
    [ -d "$RAMDISK/lib/modules" ] || mkdir -p "$RAMDISK/lib/modules"

    # These move files to easier locations
    find -name '*.ko' -exec cp -av {} $RAMDISK/lib/modules/ \;
    find -name '*.ko' -exec cp -av {} $CMRAMDISK/lib/modules/ \;
    cp $OUT/zImage $PACK

    # This part packs the img up :)
    # In order for this part to work you need the mkbootimg tools

    $TOOLS/mkbootfs $RAMDISK | gzip > $PACK/ramdisk.gz
    $TOOLS/mkbootimg --cmdline "console=null androidboot.hardware=qcom user_debug=31" --kernel $PACK/zImage --ramdisk $PACK/ramdisk.gz --base 0x80200000 --ramdiskaddr 0x81500000 -o $PACK/boot.img

    $TOOLS/mkbootfs $CMRAMDISK | gzip > $PACK/CMramdisk.gz
    $TOOLS/mkbootimg --cmdline "console=null androidboot.hardware=qcom user_debug=31" --kernel $PACK/zImage --ramdisk $PACK/CMramdisk.gz --base 0x80200000 --ramdiskaddr 0x81500000 -o $PACK/CM-boot.img
    echo "packed" >> $LOG
    date >> $LOG
