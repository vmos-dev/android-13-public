#
# Copyright 2014 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# First lunching is S, api_level is 31
PRODUCT_SHIPPING_API_LEVEL := 31
PRODUCT_DTBO_TEMPLATE := $(LOCAL_PATH)/dt-overlay.in

include device/rockchip/common/build/rockchip/DynamicPartitions.mk
include device/rockchip/rk3588/rk3588_docker/BoardConfig.mk
include device/rockchip/common/BoardConfig.mk

$(call inherit-product, device/rockchip/rk3588/device.mk)
$(call inherit-product, device/rockchip/common/device.mk)
$(call inherit-product, frameworks/native/build/tablet-10in-xhdpi-2048-dalvik-heap.mk)

DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/../overlay
PRODUCT_PACKAGE_OVERLAYS += device/rockchip/rk3588/rk3588_docker/overlay

PRODUCT_CHARACTERISTICS := nosdcard

PRODUCT_NAME := coral
PRODUCT_DEVICE := rk3588_docker
PRODUCT_BRAND := google
PRODUCT_MODEL := Pixel 4 XL
PRODUCT_MANUFACTURER := Google
PRODUCT_AAPT_PREF_CONFIG := mdpi
PRODUCT_ANDROID_VERSION := android13
#
## add Rockchip properties
#
PRODUCT_PROPERTY_OVERRIDES += ro.sf.lcd_density=320
PRODUCT_PROPERTY_OVERRIDES += ro.wifi.sleep.power.down=true
PRODUCT_PROPERTY_OVERRIDES += persist.wifi.sleep.delay.ms=0
PRODUCT_PROPERTY_OVERRIDES += persist.bt.power.down=true
PRODUCT_PROPERTY_OVERRIDES += vendor.hwc.device.primary=DSI
PRODUCT_PROPERTY_OVERRIDES += vendor.hwc.device.extend=HDMI-A,eDP
PRODUCT_PROPERTY_OVERRIDES += persist.vendor.framebuffer.main=720x1280@30
PRODUCT_PROPERTY_OVERRIDES += vendor.hwc.enable_display_configs=true


# redorid add
PRODUCT_PACKAGES += \
    binder_alloc \
    ipconfigstore \

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/chmod.sh:$(TARGET_COPY_OUT_VENDOR)/bin/chmod.sh \
    $(LOCAL_PATH)/init.redroid.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.redroid.rc \
    $(LOCAL_PATH)/mediacodec.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/mediacodec.policy \
    $(LOCAL_PATH)/fstab.rk30board:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.rk30board \
    $(LOCAL_PATH)/host_network.sh:$(TARGET_COPY_OUT_VENDOR)/bin/host_network.sh \
    $(LOCAL_PATH)/console:$(TARGET_COPY_OUT_SYSTEM)/bin/console \

# redroid add end

# add for build kernel for debian
PRODUCT_LINUX_KERNEL_CONFIG := rockchip_linux_defconfig rk3588_linux.config rockchip_nft.config docker_android.config
PRODUCT_LINUX_KERNEL_DTS := rk3588-evb1-lp4-v10-linux

