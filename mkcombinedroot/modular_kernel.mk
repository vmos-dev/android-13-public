KERNEL_GKI_DIR=mkcombinedroot
KERNEL_DRIVERS_PATH=$(KERNEL_GKI_DIR)/vendor_ramdisk/lib/modules

$(call inherit-product, $(KERNEL_GKI_DIR)/res/recovery_gki.mk)
$(call inherit-product, $(KERNEL_GKI_DIR)/res/vendor_gki.mk)
$(call inherit-product, $(KERNEL_GKI_DIR)/res/vendor_ramdisk_gki.mk)

# Old way to find all of the KOs
#KERNEL_KO_FILES := $(shell find $(TOPDIR)kernel -name "*.ko" -type f)
#BOARD_VENDOR_RAMDISK_KERNEL_MODULES += \
#	$(foreach file, $(KERNEL_KO_FILES), $(file))

# Uses a prebuilt boot.img
BOARD_PREBUILT_BOOTIMAGE := \
   $(KERNEL_GKI_DIR)/prebuilts/boot-$(PRODUCT_KERNEL_VERSION).img
