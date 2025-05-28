#!/bin/bash

DEBUG="0"

CURRENT_KERNEL_VERSION=5.10
CFG_PATH_DEFAULT=./res
CFG_TMP_DIR=./.temp

CFG_DEBUG_LIST_FILE=$CFG_PATH_DEFAULT/debug_list.load
CFG_KERNEL_DRIVERS_PATH=../kernel-$CURRENT_KERNEL_VERSION
CFG_SAMPLE_BOOTIMG=./prebuilts/boot-$CURRENT_KERNEL_VERSION.img
CFG_VENDOR_BOOTCONFIG_FILE=$CFG_PATH_DEFAULT/bootconfig

TMP_BOOT_DIR=$CFG_TMP_DIR/boot
TMP_MODULES_PATH=$CFG_TMP_DIR/lib/modules/0.0
TMP_VENDOR_RAMDISK_FILE=out/vendor_ramdisk.cpio.gz
TMP_KERNEL_IMAGE=$TMP_BOOT_DIR/kernel

OUT_VENDOR_BOOT_FILE=out/vendor_boot.img
OUT_BOOT_FILE=out/boot.img
OUT_VENDOR_RAMDISK_DIR=./vendor_ramdisk
OUT_MODULE_DIR=$OUT_VENDOR_RAMDISK_DIR/lib/modules

readonly OBJCOPY_BIN=llvm-objcopy

log() {
    if [[ $DEBUG = "1" ]]; then
        echo $1
    fi
}

success() {
    echo -e "\033[32m$1\033[0m"
}

fail() {
    echo -e "\033[31m$1\033[0m"
}

# $1 origin path
# $2 target path
objcopy() {
    if [ ! -f $1 ]; then
        echo "NOT FOUND!"
        return 1
    fi
    local module_name=`basename -a $1`
    local OBJCOPY_ARGS=""
    if [ $USE_STRIP = "1" ]; then
        echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++=="
        OBJCOPY_ARGS="--strip-debug"
        $OBJCOPY_BIN $OBJCOPY_ARGS $1 $2$module_name
    else
        cp $1 $2$module_name
    fi
}

clean_file() {
    if [ -f $1 ]; then
        echo "cleaning file $1"
        rm -rf $1
    fi
    if [ -d $1 ]; then
        echo "cleaning dir $1"
        rm -rf $1
    fi
}

create_dir() {
    if [ ! -d $1 ]; then
        mkdir -p $1
    fi
}

copy_from_load_file() {
    TMP_LOAD_FILE=$1
    TMP_SOURCE_PATH=$2
    echo -e "\033[33mRead modules list from $TMP_LOAD_FILE\033[0m"
    modules_ramdisk_array=($(cat $TMP_LOAD_FILE))
    for MODULE in "${modules_ramdisk_array[@]}"
    do
        module_file=($(find $TMP_SOURCE_PATH -name $MODULE))

        log "Copying $module_file"
        objcopy $module_file $TMP_MODULES_PATH/
        if [ $? -eq 0 ];then
                echo "objcopy $module_file done."
        else
                echo "objcopy $MODULE failed"
                exit 0
        fi

        if [ -f vendor_boot.img ]; then
            GLOBAL_UPDATE_LIST="$GLOBAL_UPDATE_LIST --ramdisk_add $TMP_MODULES_PATH/$MODULE:lib/modules/$MODULE"
        fi
    done
}

get_param_value() {
    echo "$1" | cut -d '=' -f 2
}

validate_soc() {
    local valid_socs=("rk3562" "rk3588" "rk356x" "rk3399" "rk3326")
    local soc=$1
    for valid_soc in "${valid_socs[@]}"; do
        if [ "$soc" == "$valid_soc" ]; then
            return 0
        fi
    done
    return 1
}

process_param() {
    local param="$1"
    local name=$(echo "$param" | cut -d '=' -f 1)
    local value=$(get_param_value "$param")
    
    case "$name" in
        "SOC")
            if ! validate_soc "$value"; then
                echo "Invalid SOC parameter: $value"
                exit 1
            fi
            echo "Selected SOC: $value"
            ENV_SOC=$value
            ;;
        "DTS")
            echo "Selected DTS: $value"
            ENV_DTB=$value
            ;;
        "DEBUG")
            echo "DEBUG: $value"
            DEBUG=$value
            ;;
        *)
            fail "Unsupported parameter: $name"
            exit 1
            ;;
    esac
}

# Setup PATH
setup_path() {
    export PATH=./bin:$PATH
}

main() {
    for param in "$@"; do
        process_param "$param"
    done
}

###########################################################################################
# Start
###########################################################################################
if [ "$#" -eq 0 ]; then
    fail "Usage: $0 [SOC=rk356x,rk3326,rk3562,rk3399,rk3588] [DTS=rk3588-evb1-lp4-v10]"
    echo "Use export COPY_ALL_KO=1 to copy ko from kernel-$CURRENT_KERNEL_VERSION"
    exit 1
fi

main "$@"

# Setup SOC/DTS
CFG_VENDOR_RAMDISK_LOAD_FILE=$CFG_PATH_DEFAULT/soc/$ENV_SOC/vendor_ramdisk_modules.load
CFG_BOARD_VRAMDISK_LOAD_FILE=$CFG_PATH_DEFAULT/board/$ENV_DTB.load

# Check file exist or not.
if [ -f $CFG_VENDOR_RAMDISK_LOAD_FILE ]; then
  success "Using $CFG_VENDOR_RAMDISK_LOAD_FILE"
else
  fail "Current SoC: $ENV_SOC is not support."
  exit 0
fi

if [ -f $CFG_BOARD_VRAMDISK_LOAD_FILE ]; then
  success "Using $CFG_BOARD_VRAMDISK_LOAD_FILE"
else
  fail "Current Board: $ENV_DTB is not support."
  exit 0
fi

# Set DTS
DTB_PATH=$CFG_KERNEL_DRIVERS_PATH/arch/arm64/boot/dts/rockchip/$ENV_DTB.dtb

# Copy ko or not
if [ -z $COPY_ALL_KO ]; then
    echo "Skip coping ko from kernel-$CURRENT_KERNEL_VERSION"
    USE_STRIP=0
else
    USE_STRIP=1
fi

setup_path

# Build repack command
if [ -f vendor_boot.img ]; then
GLOBAL_UPDATE_LIST="repack_bootimg --local --dst_bootimg new_vendor_boot.img \
--ramdisk_add $TMP_MODULES_PATH/modules.load:lib/modules/modules.load \
--ramdisk_add $TMP_MODULES_PATH/modules.load:lib/modules/modules.load.recovery"
fi

# Prepare dirs
echo "Preparing $CFG_TMP_DIR dirs and use placeholder 0.0..."
clean_file system
clean_file $CFG_TMP_DIR
clean_file $OUT_VENDOR_BOOT_FILE
clean_file $TMP_VENDOR_RAMDISK_FILE
create_dir system
create_dir out
create_dir $TMP_MODULES_PATH
echo "Prepare $CFG_TMP_DIR dirs done."
echo "==========================================="
echo -e "\033[33mUse DTS as $DTB_PATH\033[0m"

# Copy ko
if [ -z $COPY_ALL_KO ]; then
copy_from_load_file $CFG_VENDOR_RAMDISK_LOAD_FILE $OUT_MODULE_DIR
copy_from_load_file $CFG_BOARD_VRAMDISK_LOAD_FILE $OUT_MODULE_DIR
copy_from_load_file $CFG_DEBUG_LIST_FILE $CFG_KERNEL_DRIVERS_PATH
else
copy_from_load_file $CFG_VENDOR_RAMDISK_LOAD_FILE $CFG_KERNEL_DRIVERS_PATH
copy_from_load_file $CFG_BOARD_VRAMDISK_LOAD_FILE $CFG_KERNEL_DRIVERS_PATH
fi

# Depmod
echo "Generating depmod..."
depmod -b $CFG_TMP_DIR 0.0
echo "Generate depmod done."

#clean_file $OUT_MODULE_DIR
#create_dir $OUT_MODULE_DIR
#mv $TMP_MODULES_PATH/* $OUT_MODULE_DIR/
# Generate load files
cp $CFG_VENDOR_RAMDISK_LOAD_FILE $TMP_MODULES_PATH/modules.load -f
cat $CFG_BOARD_VRAMDISK_LOAD_FILE >> $TMP_MODULES_PATH/modules.load
rm -rf $OUT_MODULE_DIR/modules.*.bin
clean_file $OUT_MODULE_DIR/modules.symbols
clean_file $OUT_MODULE_DIR/modules.devname

echo "==========================================="
# Repack
if [ -f vendor_boot.img ]; then
    clean_file new_vendor_boot.img
    cp vendor_boot.img new_vendor_boot.img
    log "Using repack_bootimg: $GLOBAL_UPDATE_LIST"
    RET=`$GLOBAL_UPDATE_LIST`
    log "$RET"
    # check result file
    file_size=`du -sh new_vendor_boot.img`
    if [[ $file_size = "0" ]]; then
        fail "Failed to repack vendor boot image"
    else
        success "Successfully repack vendor boot image: new_vendor_boot.img"
    fi
    exit 0
else # Unused now
echo "unpacking $CFG_SAMPLE_BOOTIMG..."
unpack_bootimg --boot_img $CFG_SAMPLE_BOOTIMG --out $TMP_BOOT_DIR
if [ $? -eq 0 ];then
        echo "unpack $CFG_SAMPLE_BOOTIMG done."
else
        echo "unpack $CFG_SAMPLE_BOOTIMG failed"
        exit 0
fi
echo "making vendor_ramdisk..."
mkbootfs -d ./system $OUT_VENDOR_RAMDISK_DIR | minigzip > $TMP_VENDOR_RAMDISK_FILE
if [ $? -eq 0 ];then
        echo "make vendor_ramdisk done."
else
        echo "make vendor_ramdisk failed"
        exit 0
fi

echo "==========================================="
echo "making vendor_boot image..."
mkbootimg --dtb $DTB_PATH --vendor_cmdline "console=ttyFIQ0 firmware_class.path=/vendor/etc/firmware init=/init rootwait ro loop.max_part=7 bootconfig buildvariant=userdebug" --header_version 4 --vendor_bootconfig $CFG_VENDOR_BOOTCONFIG_FILE --vendor_ramdisk $TMP_VENDOR_RAMDISK_FILE --vendor_boot $OUT_VENDOR_BOOT_FILE
if [ $? -eq 0 ];then
        echo "make vendor_boot image done."
else
        echo "make vendor_boot image failed"
        exit 0
fi

echo "==========================================="

echo "making boot image..."
mkbootimg --kernel $TMP_KERNEL_IMAGE --ramdisk $TMP_BOOT_DIR/ramdisk --os_version 12 --os_patch_level 2022-09-05 --header_version 4 --output $OUT_BOOT_FILE
if [ $? -eq 0 ];then
        echo "make boot image done."
else
        echo "make boot image failed"
        exit 0
fi

fi
echo "make boot image done."
echo "==========================================="
