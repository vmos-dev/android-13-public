#!/bin/bash
# if [ "$(id -u)" -ne 0 ]; then
#     echo "You are not root. Please run as root."
#     exit 1
# fi

DATE=$(date  +%Y%m%d)
source build/envsetup.sh >/dev/null
lunch vcloud-user
export PATH=$ANDROID_BUILD_TOP/prebuilts/clang/host/linux-x86/clang-r416183b/bin:$PATH
export TARGET_PRODUCT=`get_build_var TARGET_PRODUCT`
export BUILD_VARIANT=`get_build_var TARGET_BUILD_VARIANT`
export ANDROID_VERSION=`get_build_var PRODUCT_ANDROID_VERSION`


# check TARGET_PRODUCT is null, if null, exit
if [ -z "$TARGET_PRODUCT" ]; then
	echo "TARGET_PRODUCT is null, please run lunch first"
	exit 1
fi


export PROJECT_TOP=`gettop`
#lunch $TARGET_PRODUCT-$BUILD_VARIANT

FORHOSTDATE=$(date  +%Y%m%d)
IMAGE_STUB_PATH=Image/"$TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT"
IMAGE_STUB_PATH="$(echo $IMAGE_STUB_PATH | tr '[:lower:]' '[:upper:]')"
IMAGE_FULL_PATH=$PROJECT_TOP/$IMAGE_STUB_PATH/IMAGES/container/$TARGET_PRODUCT-"$ANDROID_VERSION"-$BUILD_VARIANT.tgz

# check image file path exist, if not exist,  exit
if [ ! -f $IMAGE_FULL_PATH ]; then
	echo "image file not exist, please build android first"
	exit 1
fi

DOCKER_TAR_PATH=$IMAGE_STUB_PATH/DOCKERTARS
mkdir -p $DOCKER_TAR_PATH

DOCKER_TAR_FULL_PATH=$PROJECT_TOP/$DOCKER_TAR_PATH/$TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT"_"$DATE".tar"

DOCKER_TMP_IMG_NAME=$TARGET_PRODUCT"_"$ANDROID_VERSION"_"$BUILD_VARIANT"_tmp"

# check docker command, if not exist, install docker
if ! command -v docker &> /dev/null
then
	echo "docker command not found, please install docker"
	exit 1
fi


echo "===================== start ======================="
mkdir -p $DOCKER_TAR_PATH
mkdir -p $DOCKER_TAR_PATH/super_img

cd $DOCKER_TAR_PATH

rm -rf super_img/*

echo "extract android image"
tar -xvf $IMAGE_FULL_PATH


# check super_img/system.img exist, if not exist, exit
if [ ! -f super_img/system.img ]; then
	echo "android image not correct, system.img not exist"
	exit 1
fi
if [ ! -f super_img/vendor.img ]; then
	echo "android image not correct, vendor.img not exist"
	exit 1
fi

rm -rf super_img/build

cp -rf $PROJECT_TOP/device/rockchip/rk3588/vcloud/container_v1/build/* super_img/

# check init_wrapper exist, if not exist, exit
if [ ! -f super_img/init_wrapper ]; then
	echo "android image not correct, init_wrapper not exist"
	exit 1
fi

cd super_img

# check build.sh exist, if not exist, exit
if [ ! -f build.sh ]; then
	echo "android image not correct, build.sh not exist"
	exit 1
fi
echo "start build docker image now..."

# check docker image exists, if exist, remove it
if [ ! -z "$(docker images -q $DOCKER_TMP_IMG_NAME)" ]; then
	docker rmi $DOCKER_TMP_IMG_NAME
fi

./build.sh $DOCKER_TMP_IMG_NAME

# check docker image exist, if not exist, exit
if [ -z "$(docker images -q $DOCKER_TMP_IMG_NAME)" ]; then
	echo "docker image not exist, build failed"
	exit 1
fi

# save docker image to tar file
echo "save docker image to tar file $DOCKER_TAR_FULL_PATH"
docker save -o $DOCKER_TAR_FULL_PATH $DOCKER_TMP_IMG_NAME

cd ..
rm -rf super_img

if [ ! -z "$(docker images -q $DOCKER_TMP_IMG_NAME)" ]; then
	docker rmi $DOCKER_TMP_IMG_NAME
fi

echo "success build docker image: $DOCKER_TAR_FULL_PATH"
chmod 777 $DOCKER_TAR_FULL_PATH
echo "===================== end ======================="


# if [ "$BUILD_PATCH" = true ] ; then
# 	#Generate patches
# 	mkdir -p $STUB_PATCH_PATH
# 	.repo/repo/repo forall  -c "$PROJECT_TOP/device/rockchip/common/gen_patches_body.sh"
# 	.repo/repo/repo manifest -r -o out/commit_id.xml
# 	#Copy stubs
# 	cp out/commit_id.xml $STUB_PATH/manifest_${DATE}.xml
# fi
