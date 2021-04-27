LOCAL_PATH := $(call my-dir)

ifeq ($(MTK_GPS_SUPPORT), yes)

include $(CLEAR_VARS)
LOCAL_MODULE := gps_drv_stp.ko
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk
LOCAL_INIT_RC := ../init.gps_drv.rc

ifeq ($(MTK_COMBO_SUPPORT),yes)
LOCAL_REQUIRED_MODULES := wmt_drv.ko
else
$(warning MTK_COMBO=$(MTK_COMBO_SUPPORT))
$(warning gps_drv_stp.ko does not claim the requirement for wmt_drv.ko)
endif

include $(MTK_KERNEL_MODULE)

endif
