#!/bin/bash

DATE=$(date  +%Y%m%d)
source build/envsetup.sh >/dev/null
export PATH=$ANDROID_BUILD_TOP/prebuilts/clang/host/linux-x86/clang-r416183b/bin:$PATH
export TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
export BUILD_VARIANT=`get_build_var TARGET_BUILD_VARIANT`
export ANDROID_VERSION=`get_build_var PRODUCT_ANDROID_VERSION`
export BUILD_JOBS=32

export PROJECT_TOP=`gettop`
#lunch $TARGET_PRODUCT-$BUILD_VARIANT
lunch vcloud-user

FORHOSTDATE=$(date  +%Y%m%d)
HOSTIMAGE="3588_HOST_IMAGE"_"$FORHOSTDATE"
HOSTSTUB_PATH=Image/$HOSTIMAGE
HOSTSTUB_PATH="$(echo $HOSTSTUB_PATH | tr '[:lower:]' '[:upper:]')"
STUB_PATH=Image/"$TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT"_"$DATE"
STUB_PATH="$(echo $STUB_PATH | tr '[:lower:]' '[:upper:]')"

export STUB_PATH=$PROJECT_TOP/$STUB_PATH
export STUB_PATCH_PATH=$STUB_PATH/PATCHES

if [ -n "$1" ]
then
    while getopts "KAP" arg
    do
         case $arg in
	     K)
	         echo "will build linux kernel with Clang"
	         BUILD_KERNEL=true
	         ;;
	     A)
	         echo "will build android"
	         BUILD_ANDROID=true
	         ;;
	     P)
		 echo "will generate patch"
		 BUILD_PATCH=true
		 ;;
             ?)
	         echo "will build kernel AND android"
	         BUILD_KERNEL=true
	         BUILD_ANDROID=true
		 BUILD_PATCH=true
	         ;;
         esac
    done
else
    echo "will build kernel AND android"
    BUILD_KERNEL=true
    BUILD_ANDROID=true
    BUILD_PATCH=true
fi

if [ "$BUILD_KERNEL" = true ] ; then
	# build kernel
	echo "Start build kernel"
#	export PATH=$PROJECT_TOP/prebuilts/clang/host/linux-x86/clang-r416183b/bin:$PATH
	export KERNEL_VERSION=`get_build_var PRODUCT_KERNEL_VERSION`
	if [ "$ANDROID_VERSION"x == "android10"x ]; then
		export LOCAL_KERNEL_PATH=kernel
	else
		export LOCAL_KERNEL_PATH=kernel-$KERNEL_VERSION
	fi
	echo "ANDROID_VERSION is: $ANDROID_VERSION"
	export ADDON_ARGS="CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 LLVM_IAS=1"
	#export KERNEL_DTS=`get_build_var PRODUCT_LINUX_KERNEL_DTS`
	export KERNEL_DTS="rk3588-evb1-lp4-v10-linux"
	export KERNEL_ARCH=`get_build_var PRODUCT_KERNEL_ARCH`
	#export KERNEL_DEFCONFIG=`get_build_var PRODUCT_LINUX_KERNEL_CONFIG`
	export KERNEL_DEFCONFIG="rockchip_linux_defconfig docker_android.config rockchip_nft.config rk3588_linux.config"
	export KERNEL_DEFCONFIG="rockchip_defconfig android-13.config"
	echo "KERNEL_DEFCONFIG: $KERNEL_DEFCONFIG; dts: $KERNEL_DTS"
	cd $LOCAL_KERNEL_PATH &&  make $ADDON_ARGS ARCH=$KERNEL_ARCH $KERNEL_DEFCONFIG && make $ADDON_ARGS -j 32 ARCH=$KERNEL_ARCH $KERNEL_DTS.img && cd -
	#cd $LOCAL_KERNEL_PATH && make clean && make $ADDON _ARGS ARCH=$KERNEL_ARCH $KERNEL_DTS.img -j$BUILD_JOBS && cd -
	if [ $? -eq 0 ]; then
		mv $LOCAL_KERNEL_PATH/zboot.img $LOCAL_KERNEL_PATH/zboot-linux-$KERNEL_DTS.img
		if [ -d $STUB_PATH ];then
			cp $LOCAL_KERNEL_PATH/zboot-linux-$KERNEL_DTS.img $STUB_PATH/IMAGES/
		fi
		echo "Build kernel ok!"

		mkdir -p $HOSTSTUB_PATH		
		if [ -d $HOSTSTUB_PATH ];then
			echo "pack host images: $HOSTSTUB_PATH..."
			cp $PROJECT_TOP/device/rockchip/rk3588/vcloud/rk3588_flash.sh $HOSTSTUB_PATH/flash.sh
			cp $LOCAL_KERNEL_PATH/zboot-linux-$KERNEL_DTS.img $HOSTSTUB_PATH/boot.img
			cd $HOSTSTUB_PATH/../
			tar -cvzf $HOSTIMAGE.zip $HOSTIMAGE
			echo "output: $PROJECT_TOP/IMAGES/$HOSTIMAGE.zip"
		fi
	else
		echo "Build kernel failed!"
		exit 1
	fi

	cd $PROJECT_TOP
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

	echo "pack docker android images: $TARGET_PRODUCT_$ANDROID_VERSION_$BUILD_VARIANT..."

	cd $STUB_PATH/IMAGES/
	mkdir super_img
	cp -rf $PROJECT_TOP/device/rockchip/rk3588/vcloud/container/* super_img/

	simg2img super.img super.img.ext4
	lpunpack super.img.ext4 super_img/

	# no need to pack *_dlkm.img
	rm super_img/*_dlkm.img

	tar -cvpf ./container/$TARGET_PRODUCT-"$ANDROID_VERSION"-$BUILD_VARIANT.tgz super_img/

	rm -rf super_img
	rm super.img
	rm super.img.ext4

	cd $PROJECT_TOP
fi


# if [ "$BUILD_PATCH" = true ] ; then
# 	#Generate patches
# 	mkdir -p $STUB_PATCH_PATH
# 	.repo/repo/repo forall  -c "$PROJECT_TOP/device/rockchip/common/gen_patches_body.sh"
# 	.repo/repo/repo manifest -r -o out/commit_id.xml
# 	#Copy stubs
# 	cp out/commit_id.xml $STUB_PATH/manifest_${DATE}.xml
# fi
