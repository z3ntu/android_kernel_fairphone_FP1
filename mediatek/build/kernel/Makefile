include $(srctree)/../mediatek/build/Makefile
ifneq ($(KBUILD_SRC),)
$(call codebase-path,kernel,$(srctree)/..)
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

ifeq (,$(M))
clean: mtkclean
mtkclean:
	@echo "clean mediatek kernel source tree"
	@find $(MTK_PATH_PLATFORM) $(MTK_PATH_SOURCE) $(MTK_PATH_CUSTOM) $(RCS_FIND_IGNORE) \
	\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
	-o -name '*.symtypes' -o -name 'modules.order' \
	-o -name 'Module.markers' -o -name '.tmp_*.o.*' \
	-o -name '*.gcno' \) -type f -print | grep -v "/prebuilt/" | xargs rm -f
	find $(MTK_ROOT_GEN_CONFIG) -name kconfig -type f -print | xargs rm -f
endif

generated_files := $(call mtk.config.generate-rules)
.config mediatek-configs: $(MTK_ROOT_GEN_CONFIG)/kconfig $(MTK_ROOT_GEN_CONFIG)
	@echo "[COPY] $< --> $@"
	@cp $< $@
	@echo "[COPY] $< --> .config"
	@cp $< .config
ifeq (mediatek-configs,$(MAKECMDGOALS))
include/config/auto.conf: ;
endif

