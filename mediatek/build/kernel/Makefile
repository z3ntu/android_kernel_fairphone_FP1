include $(srctree)/../mediatek/build/Makefile
ifneq ($(KBUILD_SRC),)
$(call codebase-path,kernel,$(srctree)/..,$(call lc,$(MTK_PLATFORM)),$(FULL_PROJECT),$(objtree))
else
$(call codebase-path,kernel)
endif
#TO-DO 
#$(call relative-include,common.mk)
include $(srctree)/../mediatek/build/kernel/common.mk
MODULE_PREFIX_KERNEL := CUSTOM_KERNEL_
MODULE_PREFIX_HAL    := CUSTOM_HAL_
machine-y       := $(call lc,$(MTK_PLATFORM))
ifeq ($(strip $(KBUILD_OUTPUT_SUPPORT)),yes)
MACHINE          := mediatek/platform/$(call lc,$(MTK_PLATFORM))/kernel/core/
machdirs         := mediatek/platform/$(call lc,$(MTK_PLATFORM))/kernel/core/
else
MACHINE         := $(MTK_PATH_PLATFORM)/core/
machdirs        := $(MTK_PATH_PLATFORM)/core/
endif
platdirs        :=

ifeq ($(strip $(KBUILD_OUTPUT_SUPPORT)),yes)
drivers-y       += mediatek/kernel/
drivers-y       += mediatek/custom/
drivers-y       += mediatek/platform/$(call lc,$(MTK_PLATFORM))/kernel/drivers/
else
drivers-y       += $(MTK_PATH_PLATFORM)/drivers/
drivers-y       += $(MTK_PATH_SOURCE)/
drivers-y       += $(MTK_PATH_CUSTOM)/
endif

LINUXINCLUDE    += -I$(MTK_PATH_PLATFORM)/core/include/
LINUXINCLUDE    += -I$(MTK_PATH_SOURCE)/include/
KBUILD_CFLAGS   += $(call mtk.custom.generate-macros)

ifneq (,$(MTK_MODEM_SUPPORT))
KBUILD_CPPFLAGS += -D$(call uc,$(MTK_MODEM_SUPPORT))
endif

ifeq ($(TARGET_BUILD_VARIANT),user)
 KBUILD_CFLAGS += -DUSER_BUILD_KERNEL
endif

$(MTK_ROOT_GEN_CONFIG):
	@mkdir -p $@

generated_files := $(call .mtk.config.generate-auto-rules,kconfig)
mediatekconfig: $(MTK_ROOT_GEN_CONFIG)/kconfig
	@echo "[COPY] $< --> $(KCONFIG_CONFIG)"
	@cp $< $(KCONFIG_CONFIG)

MRPROPER_FILES += $(generated_files)
