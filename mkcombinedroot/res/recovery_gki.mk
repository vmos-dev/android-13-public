# Rockchip 2022 makefile
# Generate from vendor/rockchip/gki/modular_kernel/configs/recovery_modules.load

# SoC modules
BOARD_VENDOR_RAMDISK_SOC_LIST := $(strip $(shell cat $(KERNEL_GKI_DIR)/res/soc/$(TARGET_BOARD_PLATFORM)/vendor_ramdisk_modules.load))
ifndef BOARD_VENDOR_RAMDISK_SOC_LIST
$(error SoC load file not found, GKI is not support for $(TARGET_BOARD_PLATFORM) now)
endif

# Board modules, refs to DTS
BOARD_VENDOR_RAMDISK_BOARD_LIST := $(strip $(shell cat $(KERNEL_GKI_DIR)/res/board/$(PRODUCT_KERNEL_DTS).load))
ifndef BOARD_VENDOR_RAMDISK_BOARD_LIST
$(error $(PRODUCT_KERNEL_DTS).load not found, please add your proprietary load file.)
endif

BOARD_RECOVERY_KERNEL_MODULES := $(addprefix $(KERNEL_DRIVERS_PATH)/, $(notdir $(BOARD_VENDOR_RAMDISK_SOC_LIST)))
BOARD_RECOVERY_KERNEL_MODULES += $(addprefix $(KERNEL_DRIVERS_PATH)/, $(notdir $(BOARD_VENDOR_RAMDISK_BOARD_LIST)))
