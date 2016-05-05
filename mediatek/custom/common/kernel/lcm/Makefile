ccflags-y += -I$(srctree)/mediatek/custom/common/kernel/lcm/inc/

obj-y	+= mt65xx_lcm_list.o

ifneq (,$(filter nt35516_qhd_dsi_cmd,$(subst ",,$(CONFIG_CUSTOM_KERNEL_LCM))))
obj-y	+= nt35516_qhd_dsi_cmd/
endif
