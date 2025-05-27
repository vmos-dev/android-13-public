#!/bin/bash

version_num=`get_build_var PLATFORM_VERSION`
if [ $? -ne 0 ]; then
exit 0
fi

echo "Android version $version_num"

for file in `ls $1`
do
	if [ -d $1"/"$file ]; then
		sed -i 's/CONFIG_RTW_ANDROID =.*$/CONFIG_RTW_ANDROID = '$version_num'/' $1/$file/Makefile
		sed -i 's/CONFIG_BCMDHD_ANDROID_VERSION :=.*$/CONFIG_BCMDHD_ANDROID_VERSION := '$version_num'/' $1/$file/Makefile
	fi
done

