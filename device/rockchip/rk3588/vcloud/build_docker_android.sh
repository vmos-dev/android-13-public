#!/bin/bash

DATE=$(date  +%Y%m%d)
source build/envsetup.sh >/dev/null
lunch vcloud-user
export PATH=$ANDROID_BUILD_TOP/prebuilts/clang/host/linux-x86/clang-r416183b/bin:$PATH
export TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
export BUILD_VARIANT=`get_build_var TARGET_BUILD_VARIANT`
export ANDROID_VERSION=`get_build_var PRODUCT_ANDROID_VERSION`
export BUILD_JOBS=32

export PROJECT_TOP=`gettop`
#lunch $TARGET_PRODUCT-$BUILD_VARIANT


FORHOSTDATE=$(date  +%Y%m%d)
STUB_PATH=Image/"$TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT"
STUB_PATH="$(echo $STUB_PATH | tr '[:lower:]' '[:upper:]')"

export STUB_PATH=$PROJECT_TOP/$STUB_PATH

if [ -n "$1" ]
then
    while getopts "AD" arg
    do
         case $arg in
	     D)
	         echo "will gen docker tar"
	         BUILD_DOCKER_IMG=true
	         ;;
	     A)
	         echo "will build android"
	         BUILD_ANDROID=true
	         ;;
         ?)
	         echo "will build android AND gen docker tar"
	         BUILD_DOCKER_IMG=true
	         BUILD_ANDROID=true
	         ;;
         esac
    done
else
    echo "will build android AND gen docker tar"
	BUILD_DOCKER_IMG=true
	BUILD_ANDROID=true
fi

if [ "$BUILD_ANDROID" = true ] ; then
	# 判断是否存在lpunpack
	type lpunpack
	if [ $? -eq 0 ]; then
		echo "lpunpack is exit"
	else
		make lpunpack
	fi

	echo "start build android"
	# make installclean
	make
	# check the result of Makefile
	if [ $? -eq 0 ]; then
		echo "Build android ok!"
	else
		echo "Build android failed!"
		exit 1
	fi
	mkdir -p $STUB_PATH
	mkdir -p $STUB_PATH/IMAGES/

	cp $PROJECT_TOP/out/target/product/$TARGET_PRODUCT/super.img $STUB_PATH/IMAGES/
	cp -rf $PROJECT_TOP/device/rockchip/rk3588/vcloud/container $STUB_PATH/IMAGES/
	#ANDROID_VERSION= `get_build_var PRODUCT_ANDROID_VERSION`

	echo "make tgz for android images: $TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT..."

	cd $STUB_PATH/IMAGES/
	mkdir super_img
	cp -rf $PROJECT_TOP/device/rockchip/rk3588/vcloud/container/* super_img/

	# simg2img super.img super.img.ext4
	lpunpack super.img super_img/

	# no need to pack *_dlkm.img
	rm super_img/*_dlkm.img

	tar -cvpf ./container/$TARGET_PRODUCT-"$ANDROID_VERSION"-$BUILD_VARIANT.tgz super_img/

	rm -rf super_img
	rm super.img
	# rm super.img.ext4

	cd $PROJECT_TOP
fi
