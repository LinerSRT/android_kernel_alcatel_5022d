include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "0")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"0\"
endif

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "90")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"90\"
endif

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "270")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"270\"
endif

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "180")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"180\"
endif

#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/focaltech/
#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/touchpanel/focaltech/

# Linux driver folder /tcl50255d_l1/drivers/misc/mediatek/mach/mt6580/tcl5022d/touchpanel/focaltech/tpd_custom_fts.h
#ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/focaltech/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/focaltech


ifeq ($(CONFIG_MTK_PLATFORM),"mt6572")
    ccflags-y += -DMT6572
endif

obj-y	+=  focaltech_driver.o
obj-y	+=  focaltech_ctl.o
obj-y	+=  focaltech_ex_fun.o
