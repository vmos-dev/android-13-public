# Rockchip 2022 makefile
# Generate from vendor/rockchip/gki/modular_kernel/configs/vendor_modules.load

BOARD_VENDOR_KERNEL_MODULES_LOAD := $(strip $(shell cat $(KERNEL_GKI_DIR)/res/vendor_modules.load))
ifdef BOARD_VENDOR_KERNEL_MODULES_LOAD
BOARD_VENDOR_KERNEL_MODULES += $(addprefix $(KERNEL_DRIVERS_PATH)/, $(notdir $(BOARD_VENDOR_KERNEL_MODULES_LOAD)))
endif
