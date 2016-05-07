# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2010. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.


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
mediatekconfig: $(MTK_ROOT_GEN_CONFIG)/kconfig $(MTK_ROOT_GEN_CONFIG)
	@echo "[COPY] $< --> $(KCONFIG_CONFIG)"
	@cp $< $(KCONFIG_CONFIG)
