////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_platform_interface.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.4.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_platform_interface.h"
#include "mstar_drv_main.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_platform_porting_layer.h"

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern u32 g_GestureWakeupMode[2];
extern u8 g_GestureWakeupFlag;

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern u8 g_GestureDebugFlag;
extern u8 g_GestureDebugMode;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

extern struct input_dev *g_InputDevice;

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

void MsDrvInterfaceTouchDeviceSuspend(struct early_suspend *pSuspend)
{
    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    //if (g_GestureWakeupMode[0] != 0x00000000 || g_GestureWakeupMode[1] != 0x00000000)
			if(1)
    {
        DrvIcFwLyrOpenGestureWakeup(&g_GestureWakeupMode[0]);
        return;
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DrvPlatformLyrFingerTouchReleased(0, 0); // Send touch end for clearing point touch
    input_sync(g_InputDevice);

    DrvPlatformLyrDisableFingerTouchReport();
    DrvPlatformLyrTouchDevicePowerOff(); 
}

void MsDrvInterfaceTouchDeviceResume(struct early_suspend *pSuspend)
{
    DBG("*** %s() ***\n", __func__);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureDebugMode == 1)
    {
        DrvIcFwLyrCloseGestureDebugMode();
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

    if (g_GestureWakeupFlag == 1)
    {
        DrvIcFwLyrCloseGestureWakeup();
    }
    else
    {
        DrvPlatformLyrEnableFingerTouchReport(); 
    }
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
    
    DrvPlatformLyrTouchDevicePowerOn();

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
//    DrvIcFwLyrRestoreFirmwareModeToLogDataMode(); // Mark this function call for avoiding device driver may spend longer time to resume from suspend state.
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifndef CONFIG_ENABLE_GESTURE_WAKEUP
    DrvPlatformLyrEnableFingerTouchReport(); 
#endif //CONFIG_ENABLE_GESTURE_WAKEUP
}

extern int DrvFwCtrlHandleTestIIC(void);

/* probe function is used for matching and initializing input device */
s32 /*__devinit*/ MsDrvInterfaceTouchDeviceProbe(struct i2c_client *pClient, const struct i2c_device_id *pDeviceId)
{
	s32 nRetVal = 0;

	DBG("*** %s() ***\n", __func__);

	if(DrvPlatformLyrTouchDeviceRequestGPIO() < 0)
	{
		DBG("%s: request gpio fail!!\n", __func__);
		return -1;
	}
		
#if 0
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
	DrvPlatformLyrTouchDeviceRegulatorPowerOn();
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif

	DrvPlatformLyrTouchDevicePowerOn();

	if(DrvFwCtrlHandleTestIIC() < 0)
	{
		DBG("%s: test iic bus fail!!\n", __func__);
		DrvPlatformLyrTouchDeviceFreeGPIO();
		return -1;
	}

	DrvPlatformLyrInputDeviceInitialize(pClient);

	DrvPlatformLyrTouchDeviceRegisterFingerTouchInterruptHandler();

	nRetVal = DrvMainTouchDeviceInitialize();
	if (nRetVal == -ENODEV)
	{
		DrvPlatformLyrTouchDeviceRemove(pClient);
		return nRetVal;
	}

	DrvPlatformLyrTouchDeviceRegisterEarlySuspend();

	DBG("*** MStar touch driver registered ***\n");

	return nRetVal;
}

/* remove function is triggered when the input device is removed from input sub-system */
s32 /*__devexit*/ MsDrvInterfaceTouchDeviceRemove(struct i2c_client *pClient)
{
    DBG("*** %s() ***\n", __func__);

    return DrvPlatformLyrTouchDeviceRemove(pClient);
}

void MsDrvInterfaceTouchDeviceSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    DBG("*** %s() ***\n", __func__);

    DrvPlatformLyrSetIicDataRate(pClient, nIicDataRate);
}    
