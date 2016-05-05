obj-y				+= common/

obj-$(CONFIG_ARCH_MT6589)	+= mt6589/

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
obj-y				+= $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))/
endif
