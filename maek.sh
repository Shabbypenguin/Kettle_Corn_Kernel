#!/bin/bash

    THREADS=$(expr 1 + $(grep processor /proc/cpuinfo | wc -l))
    DEFCONFIG=m2_spr_defconfig
    ARCH="ARCH=arm"
    KERNEL=/home/shabbypenguin/Public/SPH-L710
    PACK=$KERNEL/package
    OUT=$KERNEL/arch/arm/boot

    MAKE="make -j${THREADS}"
    export $ARCH

    # This cleans out crud and makes new config
    $MAKE clean
    $MAKE mrproper
    rm -rf ./package/ramdisk_tmp/lib
    rm -f $PACK/zImage
    rm -f $PACK/ramdisk.gz
    rm -f $PACK/*oot.*
    $MAKE $DEFCONFIG

    # Edit this to change the kernel name
    KBUILD_BUILD_VERSION="Kettle.Corn-0.0.3-NO-OC"
    export KBUILD_BUILD_VERSION

    # Finally making the kernel
    $MAKE zImage

    # This command will detect if the folder is there and if not will recreate it
    [ -d "$PACK/ramdisk_tmp/lib/modules" ] || mkdir -p "$PACK/ramdisk_tmp/lib/modules"

    # These move files to easier locations
    find -name '*.ko' -exec cp -av {} $PACK/ramdisk_tmp/lib/modules/ \;
    cp $OUT/zImage $PACK

    # This part packs the img up :)
    # In order for this part to work you need the mkbootimg tools
    $PACK/mkbootfs $PACK/ramdisk_tmp | gzip > $PACK/ramdisk.gz
    $PACK/mkbootimg --cmdline "console=null androidboot.hardware=qcom user_debug=31" --kernel $PACK/zImage --ramdisk $PACK/ramdisk.gz --base 0x80200000 --ramdiskaddr 0x81500000 -o $PACK/boot.img
    tar -H ustar -c $PACK/boot.img > $PACK/Testing.tar
