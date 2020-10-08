#!/bin/bash
#################################################
#### Cosmic Build Script v1.0 for Exynos7580 ####
################### Dev name ####################
COSMIC_DEV=themagicalmammal
#################### Main Dir ###################
COSMIC_DIR=$(pwd)
############## Define toolchan path #############
COSMIC_TOOl=aarch64-linux-android-4.9
COSMIC_COMPILE_TYPE=linux-android
COSMIC_TC=~/$COSMIC_TOOl/bin/aarch64-$COSMIC_COMPILE_TYPE-
### Define proper arch and dir for dts files ###
COSMIC_DTS=arch/arm64/boot/dts
########### Compiled image location #############
COSMIC_KERNEL=$COSMIC_DIR/arch/arm64/boot/Image
################ Compiled dtb by  ###############
COSMIC_DTB=$COSMIC_DIR/boot.img-dtb
############# Kernel Name & Version #############
COSMIC_VERSION=v1.4
COSMIC_NAME=Cosmic
################## Thread count #################
COSMIC_JOBS=$((`nproc`-1))
########### Target Android version ##############
COSMIC_PLATFORM=7
################## Target ARCH ##################
COSMIC_ARCH=arm64
################# Current Date ##################
COSMIC_DATE=$(date +%Y%m%d)
############## Build Requirements ###############
export ARCH=$COSMIC_ARCH
export CROSS_COMPILE=$COSMIC_TC
export PLATFORM_VERSION=$COSMIC_PLATFORM
############### Naming the kernel ###############
export KBUILD_BUILD_HOST=$COSMIC_NAME
export KBUILD_BUILD_USER=$COSMIC_DEV
##### Device specific Variables [SM-J700F] ######
COSMIC_DTSFILES_J700F="
exynos7580-j7e3g_rev00.dtb
exynos7580-j7e3g_rev05.dtb
exynos7580-j7e3g_rev08.dtb
exynos7580-j7elte_rev00.dtb
exynos7580-j7elte_rev04.dtb
exynos7580-j7elte_rev06.dtb"
COSMIC_CONFG_J700F=cosmic_defconfig
COSMIC_VARIANT_J700F=J700X
############### Script functions ################
read -p "Clean or Dirty ? (c/d) > " yn
if [ "$yn" = "C" -o "$yn" = "c" ]; then
     echo "Clean Build"
     make clean && make mrproper
     rm -r -f $COSMIC_DTB
     rm -rf $COSMIC_DTS/.*.tmp
     rm -rf $COSMIC_DTS/.*.cmd
     rm -rf $COSMIC_DTS/*.dtb
else
     echo "Dirty Build"
     rm -r -f $COSMIC_DTB
     rm -rf $COSMIC_DTS/.*.tmp
     rm -rf $COSMIC_DTS/.*.cmd
     rm -rf $COSMIC_DTS/*.dtb
fi
################### Kernel ######################
BUILD_ZIMAGE()
{
	echo "----------------------------------------------"
	echo " "
	echo "Building zImage for $COSMIC_VARIANT"
	export LOCALVERSION=-$COSMIC_VERSION-$COSMIC_VARIANT-$COSMIC_DATE
	make  $COSMIC_CONFG
	make -j$COSMIC_JOBS
	if [ ! -e ./arch/arm64/boot/Image ]; then
	exit 0;
	echo "Image Failed to Compile"
	echo " Abort "
	fi
	du -k "$COSMIC_KERNEL" | cut -f1 >sizT
	sizT=$(head -n 1 sizT)
	rm -rf sizT
	echo " "
	echo "----------------------------------------------"
}
############## Compiling the DTB ################
BUILD_DTB()
{
	echo "----------------------------------------------"
	echo " "
	echo "Building DTB for $COSMIC_VARIANT"
	make $COSMIC_DTSFILES
	./scripts/dtbTool/dtbTool -o $COSMIC_DTB -d $COSMIC_DTS/ -s 2048
	if [ ! -e $COSMIC_DTB ]; then
	exit 0;
	echo "DTB Failed to Compile"
	echo " Abort "
	fi
	rm -rf $COSMIC_DTS/.*.tmp
	rm -rf $COSMIC_DTS/.*.cmd
	rm -rf $COSMIC_DTS/*.dtb
	du -k "$COSMIC_DTB" | cut -f1 >sizdT
	sizdT=$(head -n 1 sizdT)
	rm -rf sizdT
	echo " "
	echo "----------------------------------------------"
}
################## Main Menu ####################
clear
echo "----------------------------------------------"
echo "$COSMIC_NAME $COSMIC_VERSION Kernel Script"
echo "----------------------------------------------"
PS3='Please select your option (1-2): '
menuvar=("SM-J700X" "Exit")
select menuvar in "${menuvar[@]}"
do
    case $menuvar in
        "SM-J700X")
            clear
            echo "Starting $COSMIC_VARIANT_J700F kernel build..."
            COSMIC_VARIANT=$COSMIC_VARIANT_J700F
            COSMIC_CONFG=$COSMIC_CONFG_J700F
            COSMIC_DTSFILES=$COSMIC_DTSFILES_J700F
            BUILD_ZIMAGE
            BUILD_DTB
            echo " "
            echo "----------------------------------------------"
            echo "$COSMIC_VARIANT kernel build finished."
            echo "Compiled DTB Size = $sizdT Kb"
            echo "Kernel Image Size = $sizT Kb"
            echo "Boot Image   Size = $sizkT Kb"
            echo "Press Any key to end the script"
            echo "----------------------------------------------"
            read -n1 -r key
            break
            ;;
            "Exit")
            break
            ;;
        *) echo Invalid option.;;
    esac
done
#################################################
########## coded by themagicalmammal ############
#################################################
