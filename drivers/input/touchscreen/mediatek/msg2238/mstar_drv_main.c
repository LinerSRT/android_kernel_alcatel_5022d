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
 * @file    mstar_drv_main.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.4.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_main.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_ic_fw_porting_layer.h"

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/


/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
extern FirmwareInfo_t g_FirmwareInfo;

extern u8 g_LogModePacket[DEBUG_MODE_PACKET_LENGTH];
extern u16 g_FirmwareMode;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
extern u32 g_GestureWakeupMode[2];

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern u8 g_LogGestureDebug[128];
extern u8 g_GestureDebugFlag;
extern u8 g_GestureDebugMode;

extern struct input_dev *g_InputDevice;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
extern u32 g_LogGestureInfor[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH];
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

extern u8 g_ChipType;

#ifdef CONFIG_ENABLE_ITO_MP_TEST
#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
extern TestScopeInfo_t g_TestScopeInfo;
#endif //CONFIG_ENABLE_CHIP_MSG26XXM
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static u16 _gDebugReg[MAX_DEBUG_REGISTER_NUM] = {0};
static u16 _gDebugRegValue[MAX_DEBUG_REGISTER_NUM] = {0};
static u32 _gDebugRegCount = 0;

static u8 *_gPlatformFwVersion = NULL; // internal use firmware version for MStar

#ifdef CONFIG_ENABLE_ITO_MP_TEST
static ItoTestMode_e _gItoTestMode = 0;
#endif //CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
static u32 _gLogGestureCount = 0;
static u8 _gLogGestureInforType = 0;
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

static u32 _gIsUpdateComplete = 0;

static u8 *_gFwVersion = NULL; // customer firmware version

static struct class *_gFirmwareClass = NULL;
static struct device *_gFirmwareCmdDev = NULL;

#ifndef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
static struct proc_dir_entry *_gMsTouchScreenMsg20xx = NULL;
static struct proc_dir_entry *_gSdCardUpdate = NULL;
#endif

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
struct kset *g_TouchKSet = NULL;
struct kobject *g_TouchKObj = NULL;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
struct kset *g_GestureKSet = NULL;
struct kobject *g_GestureKObj = NULL;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

u8 g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
u32 g_FwDataCount = 0;

/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/


/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

ssize_t DrvMainFirmwareChipTypeShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

    return sprintf(pBuf, "%d", g_ChipType);
}

ssize_t DrvMainFirmwareChipTypeStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

//    g_ChipType = DrvIcFwLyrGetChipType();

    return nSize;
}

static DEVICE_ATTR(chip_type, SYSFS_AUTHORITY, DrvMainFirmwareChipTypeShow, DrvMainFirmwareChipTypeStore);

ssize_t DrvMainFirmwareDriverVersionShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

    return sprintf(pBuf, "%s", DEVICE_DRIVER_RELEASE_VERSION);
}

ssize_t DrvMainFirmwareDriverVersionStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

    return nSize;
}

static DEVICE_ATTR(driver_version, SYSFS_AUTHORITY, DrvMainFirmwareDriverVersionShow, DrvMainFirmwareDriverVersionStore);

/*--------------------------------------------------------------------------*/

ssize_t DrvMainFirmwareUpdateShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    return sprintf(pBuf, "%s\n", _gFwVersion);
}

ssize_t DrvMainFirmwareUpdateStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DrvPlatformLyrDisableFingerTouchReport();

    DBG("*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);

    if (0 != DrvIcFwLyrUpdateFirmware(g_FwData, EMEM_ALL))
    {
        _gIsUpdateComplete = 0;
        DBG("Update FAILED\n");
    }
    else
    {
        _gIsUpdateComplete = 1;
        DBG("Update SUCCESS\n");
    }

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
//    DrvIcFwLyrRestoreFirmwareModeToLogDataMode();    
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    DrvPlatformLyrEnableFingerTouchReport();
  
    return nSize;
}

static DEVICE_ATTR(update, SYSFS_AUTHORITY, DrvMainFirmwareUpdateShow, DrvMainFirmwareUpdateStore);

ssize_t DrvMainFirmwareVersionShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    return sprintf(pBuf, "%s\n", _gFwVersion);
}

ssize_t DrvMainFirmwareVersionStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u16 nMajor = 0, nMinor = 0;
    
    DrvIcFwLyrGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG("*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    return nSize;
}

static DEVICE_ATTR(version, SYSFS_AUTHORITY, DrvMainFirmwareVersionShow, DrvMainFirmwareVersionStore);

ssize_t DrvMainFirmwareDataShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() g_FwDataCount = %d ***\n", __func__, g_FwDataCount);
    
    return g_FwDataCount;
}

ssize_t DrvMainFirmwareDataStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 nCount = nSize / 1024;
    u32 nRemainder = nSize % 1024;
    u32 i;

    DBG("*** %s() ***\n", __func__);

    if (nCount > 0) // nSize >= 1024
   	{
        for (i = 0; i < nCount; i ++)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), 1024);

            g_FwDataCount ++;
        }

        if (nRemainder > 0) // Handle special firmware size like MSG22XX(48.5KB)
        {
            DBG("nRemainder = %d\n", nRemainder);

            memcpy(g_FwData[g_FwDataCount], pBuf+(i*1024), nRemainder);

            g_FwDataCount ++;
        }
    }
    else // nSize < 1024
    {
        if (nSize > 0)
        {
            memcpy(g_FwData[g_FwDataCount], pBuf, nSize);

            g_FwDataCount ++;
        }
    }

    DBG("*** g_FwDataCount = %d ***\n", g_FwDataCount);

    if (pBuf != NULL)
    {
        DBG("*** buf[0] = %c ***\n", pBuf[0]);
    }
   
    return nSize;
}

static DEVICE_ATTR(data, SYSFS_AUTHORITY, DrvMainFirmwareDataShow, DrvMainFirmwareDataStore);

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_ITO_MP_TEST
ssize_t DrvMainFirmwareTestShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);
    DBG("*** ctp mp test status = %d ***\n", DrvIcFwLyrGetMpTestResult());
    
    return sprintf(pBuf, "%d", DrvIcFwLyrGetMpTestResult());
}

ssize_t DrvMainFirmwareTestStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 nMode = 0;

    DBG("*** %s() ***\n", __func__);
    
    if (pBuf != NULL)
    {
        sscanf(pBuf, "%x", &nMode);   

        DBG("Mp Test Mode = 0x%x\n", nMode);

        if (nMode == ITO_TEST_MODE_OPEN_TEST) //open test
        {
            _gItoTestMode = ITO_TEST_MODE_OPEN_TEST;
            DrvIcFwLyrScheduleMpTestWork(ITO_TEST_MODE_OPEN_TEST);
        }
        else if (nMode == ITO_TEST_MODE_SHORT_TEST) //short test
        {
            _gItoTestMode = ITO_TEST_MODE_SHORT_TEST;
            DrvIcFwLyrScheduleMpTestWork(ITO_TEST_MODE_SHORT_TEST);
        }
        else
        {
            DBG("*** Undefined MP Test Mode ***\n");
        }
    }

    return nSize;
}

static DEVICE_ATTR(test, SYSFS_AUTHORITY, DrvMainFirmwareTestShow, DrvMainFirmwareTestStore);

ssize_t DrvMainFirmwareTestLogShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u32 nLength = 0;
    
    DBG("*** %s() ***\n", __func__);
    
    DrvIcFwLyrGetMpTestDataLog(_gItoTestMode, pBuf, &nLength);
    
    return nLength;
}

ssize_t DrvMainFirmwareTestLogStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

    return nSize;
}

static DEVICE_ATTR(test_log, SYSFS_AUTHORITY, DrvMainFirmwareTestLogShow, DrvMainFirmwareTestLogStore);

ssize_t DrvMainFirmwareTestFailChannelShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u32 nCount = 0;

    DBG("*** %s() ***\n", __func__);
    
    DrvIcFwLyrGetMpTestFailChannel(_gItoTestMode, pBuf, &nCount);
    
    return nCount;
}

ssize_t DrvMainFirmwareTestFailChannelStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

    return nSize;
}

static DEVICE_ATTR(test_fail_channel, SYSFS_AUTHORITY, DrvMainFirmwareTestFailChannelShow, DrvMainFirmwareTestFailChannelStore);

ssize_t DrvMainFirmwareTestScopeShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
    DrvIcFwLyrGetMpTestScope(&g_TestScopeInfo);
    
    return sprintf(pBuf, "%d,%d", g_TestScopeInfo.nMx, g_TestScopeInfo.nMy);
#else
    return 0;    
#endif //CONFIG_ENABLE_CHIP_MSG26XXM
}

ssize_t DrvMainFirmwareTestScopeStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

    return nSize;
}

static DEVICE_ATTR(test_scope, SYSFS_AUTHORITY, DrvMainFirmwareTestScopeShow, DrvMainFirmwareTestScopeStore);
#endif //CONFIG_ENABLE_ITO_MP_TEST

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP

ssize_t DrvMainFirmwareGestureWakeupModeShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
    DBG("g_GestureWakeupMode = 0x%x, 0x%x\n", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);

    return sprintf(pBuf, "%x,%x", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);
#else
    DBG("g_GestureWakeupMode = 0x%x\n", g_GestureWakeupMode[0]);

    return sprintf(pBuf, "%x", g_GestureWakeupMode[0]);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
}

ssize_t DrvMainFirmwareGestureWakeupModeStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 nLength;
    u32 nWakeupMode[2] = {0};

    DBG("*** %s() ***\n", __func__);

    if (pBuf != NULL)
    {
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
        u32 i;
        char *pCh;

        i = 0;
        while ((pCh = strsep((char **)&pBuf, " ,")) && (i < 2))
        {
            DBG("pCh = %s\n", pCh);

            nWakeupMode[i] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));

            DBG("nWakeupMode[%d] = 0x%04X\n", i, nWakeupMode[i]);
            i ++;
        }
#else
        sscanf(pBuf, "%x", &nWakeupMode[0]);
        DBG("nWakeupMode = 0x%x\n", nWakeupMode[0]);
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

        nLength = nSize;
        DBG("nLength = %d\n", nLength);

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG) == GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG) == GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG) == GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE1_FLAG) == GESTURE_WAKEUP_MODE_RESERVE1_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE1_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE1_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE2_FLAG) == GESTURE_WAKEUP_MODE_RESERVE2_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE2_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE2_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE3_FLAG) == GESTURE_WAKEUP_MODE_RESERVE3_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE3_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE3_FLAG);
        }

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE4_FLAG) == GESTURE_WAKEUP_MODE_RESERVE4_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE4_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE4_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE5_FLAG) == GESTURE_WAKEUP_MODE_RESERVE5_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE5_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE5_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE6_FLAG) == GESTURE_WAKEUP_MODE_RESERVE6_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE6_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE6_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE7_FLAG) == GESTURE_WAKEUP_MODE_RESERVE7_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE7_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE7_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE8_FLAG) == GESTURE_WAKEUP_MODE_RESERVE8_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE8_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE8_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE9_FLAG) == GESTURE_WAKEUP_MODE_RESERVE9_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE9_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE9_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE10_FLAG) == GESTURE_WAKEUP_MODE_RESERVE10_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE10_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE10_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE11_FLAG) == GESTURE_WAKEUP_MODE_RESERVE11_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE11_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE11_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE12_FLAG) == GESTURE_WAKEUP_MODE_RESERVE12_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE12_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE12_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE13_FLAG) == GESTURE_WAKEUP_MODE_RESERVE13_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE13_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE13_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE14_FLAG) == GESTURE_WAKEUP_MODE_RESERVE14_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE14_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE14_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE15_FLAG) == GESTURE_WAKEUP_MODE_RESERVE15_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE15_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE15_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE16_FLAG) == GESTURE_WAKEUP_MODE_RESERVE16_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE16_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE16_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE17_FLAG) == GESTURE_WAKEUP_MODE_RESERVE17_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE17_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE17_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE18_FLAG) == GESTURE_WAKEUP_MODE_RESERVE18_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE18_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE18_FLAG);
        }

        if ((nWakeupMode[0] & GESTURE_WAKEUP_MODE_RESERVE19_FLAG) == GESTURE_WAKEUP_MODE_RESERVE19_FLAG)
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] | GESTURE_WAKEUP_MODE_RESERVE19_FLAG;
        }
        else
        {
            g_GestureWakeupMode[0] = g_GestureWakeupMode[0] & (~GESTURE_WAKEUP_MODE_RESERVE19_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE20_FLAG) == GESTURE_WAKEUP_MODE_RESERVE20_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE20_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE20_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE21_FLAG) == GESTURE_WAKEUP_MODE_RESERVE21_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE21_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE21_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE22_FLAG) == GESTURE_WAKEUP_MODE_RESERVE22_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE22_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE22_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE23_FLAG) == GESTURE_WAKEUP_MODE_RESERVE23_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE23_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE23_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE24_FLAG) == GESTURE_WAKEUP_MODE_RESERVE24_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE24_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE24_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE25_FLAG) == GESTURE_WAKEUP_MODE_RESERVE25_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE25_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE25_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE26_FLAG) == GESTURE_WAKEUP_MODE_RESERVE26_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE26_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE26_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE27_FLAG) == GESTURE_WAKEUP_MODE_RESERVE27_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE27_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE27_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE28_FLAG) == GESTURE_WAKEUP_MODE_RESERVE28_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE28_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE28_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE29_FLAG) == GESTURE_WAKEUP_MODE_RESERVE29_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE29_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE29_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE30_FLAG) == GESTURE_WAKEUP_MODE_RESERVE30_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE30_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE30_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE31_FLAG) == GESTURE_WAKEUP_MODE_RESERVE31_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE31_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE31_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE32_FLAG) == GESTURE_WAKEUP_MODE_RESERVE32_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE32_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE32_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE33_FLAG) == GESTURE_WAKEUP_MODE_RESERVE33_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE33_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE33_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE34_FLAG) == GESTURE_WAKEUP_MODE_RESERVE34_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE34_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE34_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE35_FLAG) == GESTURE_WAKEUP_MODE_RESERVE35_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE35_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE35_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE36_FLAG) == GESTURE_WAKEUP_MODE_RESERVE36_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE36_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE36_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE37_FLAG) == GESTURE_WAKEUP_MODE_RESERVE37_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE37_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE37_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE38_FLAG) == GESTURE_WAKEUP_MODE_RESERVE38_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE38_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE38_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE39_FLAG) == GESTURE_WAKEUP_MODE_RESERVE39_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE39_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE39_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE40_FLAG) == GESTURE_WAKEUP_MODE_RESERVE40_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE40_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE40_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE41_FLAG) == GESTURE_WAKEUP_MODE_RESERVE41_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE41_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE41_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE42_FLAG) == GESTURE_WAKEUP_MODE_RESERVE42_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE42_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE42_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE43_FLAG) == GESTURE_WAKEUP_MODE_RESERVE43_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE43_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE43_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE44_FLAG) == GESTURE_WAKEUP_MODE_RESERVE44_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE44_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE44_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE45_FLAG) == GESTURE_WAKEUP_MODE_RESERVE45_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE45_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE45_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE46_FLAG) == GESTURE_WAKEUP_MODE_RESERVE46_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE46_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE46_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE47_FLAG) == GESTURE_WAKEUP_MODE_RESERVE47_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE47_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE47_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE48_FLAG) == GESTURE_WAKEUP_MODE_RESERVE48_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE48_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE48_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE49_FLAG) == GESTURE_WAKEUP_MODE_RESERVE49_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE49_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE49_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE50_FLAG) == GESTURE_WAKEUP_MODE_RESERVE50_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE50_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE50_FLAG);
        }

        if ((nWakeupMode[1] & GESTURE_WAKEUP_MODE_RESERVE51_FLAG) == GESTURE_WAKEUP_MODE_RESERVE51_FLAG)
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] | GESTURE_WAKEUP_MODE_RESERVE51_FLAG;
        }
        else
        {
            g_GestureWakeupMode[1] = g_GestureWakeupMode[1] & (~GESTURE_WAKEUP_MODE_RESERVE51_FLAG);
        }
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

        DBG("g_GestureWakeupMode = 0x%x,  0x%x\n", g_GestureWakeupMode[0], g_GestureWakeupMode[1]);
    }
        
    return nSize;
}

static DEVICE_ATTR(gesture_wakeup_mode, SYSFS_AUTHORITY, DrvMainFirmwareGestureWakeupModeShow, DrvMainFirmwareGestureWakeupModeStore);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE

ssize_t DrvMainFirmwareGestureDebugShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

    return sprintf(pBuf, "%d", g_GestureDebugMode);
}

ssize_t DrvMainFirmwareGestureDebugStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u8 ucGestureMode[2];
    u8 i;
    char *pCh;

    if (pBuf != NULL)
    {
        i = 0;
        while ((pCh = strsep((char **)&pBuf, " ,")) && (i < 2))
        {
            DBG("pCh = %s\n", pCh);

            ucGestureMode[i] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));

            DBG("ucGestureMode[%d] = 0x%04X\n", i, ucGestureMode[i]);
            i ++;
        }

        g_GestureDebugMode = ucGestureMode[0];
        g_GestureDebugFlag = ucGestureMode[1];

        DBG("Gesture flag = 0x%x\n", g_GestureDebugFlag);

        if (g_GestureDebugMode == 0x01) //open gesture debug mode
        {
            DrvIcFwLyrOpenGestureDebugMode(g_GestureDebugFlag);

//            input_report_key(g_InputDevice, RESERVER42, 1);
            input_report_key(g_InputDevice, KEY_POWER, 1);
            input_sync(g_InputDevice);
//            input_report_key(g_InputDevice, RESERVER42, 0);
            input_report_key(g_InputDevice, KEY_POWER, 0);
            input_sync(g_InputDevice);
        }
        else if (g_GestureDebugMode == 0x00) //close gesture debug mode
        {
            DrvIcFwLyrCloseGestureDebugMode();
        }
        else
        {
            DBG("*** Undefined Gesture Debug Mode ***\n");
        }
    }

    return nSize;
}

static DEVICE_ATTR(gesture_debug, SYSFS_AUTHORITY, DrvMainFirmwareGestureDebugShow, DrvMainFirmwareGestureDebugStore);

ssize_t DrvMainKObjectGestureDebugShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 i = 0;
    u32 nLength = 0;

    DBG("*** %s() ***\n", __func__);

    if (strcmp(pAttr->attr.name, "gesture_debug") == 0)
    {
        if (g_LogGestureDebug != NULL)
        {
            DBG("g_LogGestureDebug[0]=%x, g_LogGestureDebug[1]=%x\n", g_LogGestureDebug[0], g_LogGestureDebug[1]);
            DBG("g_LogGestureDebug[2]=%x, g_LogGestureDebug[3]=%x\n", g_LogGestureDebug[2], g_LogGestureDebug[3]);
            DBG("g_LogGestureDebug[4]=%x, g_LogGestureDebug[5]=%x\n", g_LogGestureDebug[4], g_LogGestureDebug[5]);

            if (g_LogGestureDebug[0] == 0xA7 && g_LogGestureDebug[3] == 0x51)
            {
                for (i = 0; i < 0x80; i ++)
                {
                    pBuf[i] = g_LogGestureDebug[i];
                }

                nLength = 0x80;
                DBG("nLength = %d\n", nLength);
            }
            else
            {
                DBG("CURRENT MODE IS NOT GESTURE DEBUG MODE/WRONG GESTURE DEBUG MODE HEADER\n");
//            nLength = 0;
            }
        }
        else
        {
            DBG("g_LogGestureDebug is NULL\n");
//            nLength = 0;
        }
    }
    else
    {
        DBG("pAttr->attr.name = %s \n", pAttr->attr.name);
//        nLength = 0;
    }

    return nLength;
}

ssize_t DrvMainKObjectGestureDebugStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount)
{
    DBG("*** %s() ***\n", __func__);
/*
    if (strcmp(pAttr->attr.name, "packet") == 0)
    {

    }
*/
    return nCount;
}

static struct kobj_attribute gesture_attr = __ATTR(gesture_debug, 0666, DrvMainKObjectGestureDebugShow, DrvMainKObjectGestureDebugStore);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *gestureattrs[] = {
    &gesture_attr.attr,
    NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
struct attribute_group gestureattr_group = {
    .attrs = gestureattrs,
};
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
ssize_t DrvMainFirmwareGestureInforShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u8 szOut[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH*5] = {0}, szValue[10] = {0};
    u32 szLogGestureInfo[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH] = {0};
    u32 i = 0;

    DBG("*** %s() ***\n", __func__);

    _gLogGestureCount = 0;
    if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_A) //FIRMWARE_GESTURE_INFORMATION_MODE_A
    {
        for (i = 0; i < 2; i ++)//0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }

        for (i = 2; i < 8; i ++)//2~3 Xst Yst; 4~5 Xend Yend; 6~7 char_width char_height
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }
    }
    else if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_B) //FIRMWARE_GESTURE_INFORMATION_MODE_B
    {
        for (i = 0; i < 2; i ++)//0 EventFlag; 1 RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[4 + i];
            _gLogGestureCount ++;
        }

        for (i = 0; i< g_LogGestureInfor[5]*2 ; i ++)//(X and Y)*RecordNum
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[12 + i];
            _gLogGestureCount ++;
        }
    }
    else if (_gLogGestureInforType == FIRMWARE_GESTURE_INFORMATION_MODE_C) //FIRMWARE_GESTURE_INFORMATION_MODE_C
    {
        for (i = 0; i < 6; i ++)//header
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[i];
            _gLogGestureCount ++;
        }

        for (i = 6; i < 86; i ++)
        {
            szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[i];
            _gLogGestureCount ++;
        }

        szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[86];//dummy
        _gLogGestureCount ++;
        szLogGestureInfo[_gLogGestureCount] = g_LogGestureInfor[87];//checksum
        _gLogGestureCount++;
    }
    else
    {
        DBG("*** Undefined GESTURE INFORMATION MODE ***\n");
    }

    for (i = 0; i < _gLogGestureCount; i ++)
    {
        sprintf(szValue, "%d", szLogGestureInfo[i]);
        strcat(szOut, szValue);
        strcat(szOut, ",");
    }

    return sprintf(pBuf, "%s\n", szOut);
}

ssize_t DrvMainFirmwareGestureInforStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 nMode;

    DBG("*** %s() ***\n", __func__);

    if (pBuf != NULL)
    {
        sscanf(pBuf, "%x", &nMode);
        _gLogGestureInforType = nMode;
    }

    DBG("*** _gLogGestureInforType type = 0x%x ***\n", _gLogGestureInforType);

    return nSize;
}

static DEVICE_ATTR(gesture_infor, SYSFS_AUTHORITY, DrvMainFirmwareGestureInforShow, DrvMainFirmwareGestureInforStore);
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

/*--------------------------------------------------------------------------*/

ssize_t DrvMainFirmwareDebugShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u32 i;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG("*** %s() ***\n", __func__);
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG("reg(0x%X,0x%X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    return sprintf(pBuf, "%s\n", szOut);
}

ssize_t DrvMainFirmwareDebugStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 i;
    char *pCh;

    DBG("*** %s() ***\n", __func__);

    if (pBuf != NULL)
    {
        DBG("*** %s() pBuf[0] = %c ***\n", __func__, pBuf[0]);
        DBG("*** %s() pBuf[1] = %c ***\n", __func__, pBuf[1]);
        DBG("*** %s() pBuf[2] = %c ***\n", __func__, pBuf[2]);
        DBG("*** %s() pBuf[3] = %c ***\n", __func__, pBuf[3]);
        DBG("*** %s() pBuf[4] = %c ***\n", __func__, pBuf[4]);
        DBG("*** %s() pBuf[5] = %c ***\n", __func__, pBuf[5]);

        DBG("nSize = %d\n", nSize);
       
        i = 0;
        while ((pCh = strsep((char **)&pBuf, " ,")) && (i < MAX_DEBUG_REGISTER_NUM))
        {
            DBG("pCh = %s\n", pCh);
            
            _gDebugReg[i] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));

            DBG("_gDebugReg[%d] = 0x%04X\n", i, _gDebugReg[i]);
            i ++;
        }
        _gDebugRegCount = i;
        
        DBG("_gDebugRegCount = %d\n", _gDebugRegCount);
    }

    return nSize;
}

static DEVICE_ATTR(debug, SYSFS_AUTHORITY, DrvMainFirmwareDebugShow, DrvMainFirmwareDebugStore);

ssize_t DrvMainFirmwareSetDebugValueShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u32 i;
    u8 nBank, nAddr;
    u16 szRegData[MAX_DEBUG_REGISTER_NUM] = {0};
    u8 szOut[MAX_DEBUG_REGISTER_NUM*25] = {0}, szValue[10] = {0};

    DBG("*** %s() ***\n", __func__);
    
    DbBusEnterSerialDebugMode();
    DbBusStopMCU();
    DbBusIICUseBus();
    DbBusIICReshape();
    mdelay(100);
    
    for (i = 0; i < _gDebugRegCount; i ++)
    {
        szRegData[i] = RegGet16BitValue(_gDebugReg[i]);
    }

    DbBusIICNotUseBus();
    DbBusNotStopMCU();
    DbBusExitSerialDebugMode();

    for (i = 0; i < _gDebugRegCount; i ++)
    {
        nBank = (_gDebugReg[i] >> 8) & 0xFF;
        nAddr = _gDebugReg[i] & 0xFF;
    	  
        DBG("reg(0x%X,0x%X)=0x%04X\n", nBank, nAddr, szRegData[i]);

        strcat(szOut, "reg(");
        sprintf(szValue, "0x%X", nBank);
        strcat(szOut, szValue);
        strcat(szOut, ",");
        sprintf(szValue, "0x%X", nAddr);
        strcat(szOut, szValue);
        strcat(szOut, ")=");
        sprintf(szValue, "0x%04X", szRegData[i]);
        strcat(szOut, szValue);
        strcat(szOut, "\n");
    }

    return sprintf(pBuf, "%s\n", szOut);
}

ssize_t DrvMainFirmwareSetDebugValueStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 i, j, k;
    char *pCh;

    DBG("*** %s() ***\n", __func__);

    if (pBuf != NULL)
    {
        DBG("*** %s() pBuf[0] = %c ***\n", __func__, pBuf[0]);
        DBG("*** %s() pBuf[1] = %c ***\n", __func__, pBuf[1]);

        DBG("nSize = %d\n", nSize);
       
        i = 0;
        j = 0;
        k = 0;
        
        while ((pCh = strsep((char **)&pBuf, " ,")) && (i < 2))
        {
            DBG("pCh = %s\n", pCh);

            if ((i%2) == 0)
            {
                _gDebugReg[j] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG("_gDebugReg[%d] = 0x%04X\n", j, _gDebugReg[j]);
                j ++;
            }
            else // (i%2) == 1
            {	
                _gDebugRegValue[k] = DrvCommonConvertCharToHexDigit(pCh, strlen(pCh));
                DBG("_gDebugRegValue[%d] = 0x%04X\n", k, _gDebugRegValue[k]);
                k ++;
            }

            i ++;
        }
        _gDebugRegCount = j;
        
        DBG("_gDebugRegCount = %d\n", _gDebugRegCount);

        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        mdelay(100);
    
        for (i = 0; i < _gDebugRegCount; i ++)
        {
            RegSet16BitValue(_gDebugReg[i], _gDebugRegValue[i]);
            DBG("_gDebugReg[%d] = 0x%04X, _gDebugRegValue[%d] = 0x%04X\n", i, _gDebugReg[i], i , _gDebugRegValue[i]); // add for debug
        }

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();
    }

    return nSize;
}

static DEVICE_ATTR(set_debug_value, SYSFS_AUTHORITY, DrvMainFirmwareSetDebugValueShow, DrvMainFirmwareSetDebugValueStore);

/*--------------------------------------------------------------------------*/

ssize_t DrvMainFirmwarePlatformVersionShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    return sprintf(pBuf, "%s\n", _gPlatformFwVersion);
}

ssize_t DrvMainFirmwarePlatformVersionStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DrvIcFwLyrGetPlatformFirmwareVersion(&_gPlatformFwVersion);

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    DrvIcFwLyrRestoreFirmwareModeToLogDataMode();    
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    DBG("*** %s() _gPlatformFwVersion = %s ***\n", __func__, _gPlatformFwVersion);

    return nSize;
}

static DEVICE_ATTR(platform_version, SYSFS_AUTHORITY, DrvMainFirmwarePlatformVersionShow, DrvMainFirmwarePlatformVersionStore);

/*--------------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
ssize_t DrvMainFirmwareModeShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
    DrvPlatformLyrDisableFingerTouchReport();

    g_FirmwareMode = DrvIcFwLyrGetFirmwareMode();
    
    DrvPlatformLyrEnableFingerTouchReport();

    DBG("%s() firmware mode = 0x%x\n", __func__, g_FirmwareMode);

    return sprintf(pBuf, "%x", g_FirmwareMode);
#elif defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
    DrvPlatformLyrDisableFingerTouchReport();

    DrvIcFwLyrGetFirmwareInfo(&g_FirmwareInfo);
    g_FirmwareMode = g_FirmwareInfo.nFirmwareMode;

    DrvPlatformLyrEnableFingerTouchReport();

    DBG("%s() firmware mode = 0x%x, can change firmware mode = %d\n", __func__, g_FirmwareInfo.nFirmwareMode, g_FirmwareInfo.nIsCanChangeFirmwareMode);

    return sprintf(pBuf, "%x,%d", g_FirmwareInfo.nFirmwareMode, g_FirmwareInfo.nIsCanChangeFirmwareMode);
#endif
}

ssize_t DrvMainFirmwareModeStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    u32 nMode;
    
    DBG("*** %s() ***\n", __func__);
    
    if (pBuf != NULL)
    {
        sscanf(pBuf, "%x", &nMode);   
        DBG("firmware mode = 0x%x\n", nMode);

        DrvPlatformLyrDisableFingerTouchReport(); 

        if (nMode == FIRMWARE_MODE_DEMO_MODE) //demo mode
        {
            g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_DEMO_MODE);
        }
        else if (nMode == FIRMWARE_MODE_DEBUG_MODE) //debug mode
        {
            g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_DEBUG_MODE);
        }
#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
        else if (nMode == FIRMWARE_MODE_RAW_DATA_MODE) //raw data mode
        {
            g_FirmwareMode = DrvIcFwLyrChangeFirmwareMode(FIRMWARE_MODE_RAW_DATA_MODE);
        }
#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX
        else
        {
            DBG("*** Undefined Firmware Mode ***\n");
        }

        DrvPlatformLyrEnableFingerTouchReport(); 
    }

    DBG("*** g_FirmwareMode = 0x%x ***\n", g_FirmwareMode);

    return nSize;
}

static DEVICE_ATTR(mode, SYSFS_AUTHORITY, DrvMainFirmwareModeShow, DrvMainFirmwareModeStore);
/*
ssize_t DrvMainFirmwarePacketShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    u32 i = 0;
    u32 nLength = 0;
    
    DBG("*** %s() ***\n", __func__);
    
    if (g_LogModePacket != NULL)
    {
        DBG("g_FirmwareMode=%x, g_LogModePacket[0]=%x, g_LogModePacket[1]=%x\n", g_FirmwareMode, g_LogModePacket[0], g_LogModePacket[1]);
        DBG("g_LogModePacket[2]=%x, g_LogModePacket[3]=%x\n", g_LogModePacket[2], g_LogModePacket[3]);
        DBG("g_LogModePacket[4]=%x, g_LogModePacket[5]=%x\n", g_LogModePacket[4], g_LogModePacket[5]);

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
        if ((g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) && (g_LogModePacket[0] == 0xA5 || g_LogModePacket[0] == 0xAB || g_LogModePacket[0] == 0xA7))
#elif defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
        if ((g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE) && (g_LogModePacket[0] == 0x62))
#endif
        {
            for (i = 0; i < g_FirmwareInfo.nLogModePacketLength; i ++)
            {
                pBuf[i] = g_LogModePacket[i];
            }

            nLength = g_FirmwareInfo.nLogModePacketLength;
            DBG("nLength = %d\n", nLength);
        }
        else
        {
            DBG("CURRENT MODE IS NOT DEBUG MODE/WRONG DEBUG MODE HEADER\n");
//        nLength = 0;
        }
    }
    else
    {
        DBG("g_LogModePacket is NULL\n");
//        nLength = 0;
    }

    return nLength;
}

ssize_t DrvMainFirmwarePacketStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);

    return nSize;
}

static DEVICE_ATTR(packet, SYSFS_AUTHORITY, DrvMainFirmwarePacketShow, DrvMainFirmwarePacketStore);
*/
ssize_t DrvMainFirmwareSensorShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
    if (g_FirmwareInfo.nLogModePacketHeader == 0xA5 || g_FirmwareInfo.nLogModePacketHeader == 0xAB)
    {
        return sprintf(pBuf, "%d,%d", g_FirmwareInfo.nMx, g_FirmwareInfo.nMy);
    }
    else if (g_FirmwareInfo.nLogModePacketHeader == 0xA7)
    {
        return sprintf(pBuf, "%d,%d,%d,%d", g_FirmwareInfo.nMx, g_FirmwareInfo.nMy, g_FirmwareInfo.nSs, g_FirmwareInfo.nSd);
    }
    else
    {
        DBG("Undefined debug mode packet format : 0x%x\n", g_FirmwareInfo.nLogModePacketHeader);
        return 0;
    }
#elif defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
    return sprintf(pBuf, "%d", g_FirmwareInfo.nLogModePacketLength);
#endif
}

ssize_t DrvMainFirmwareSensorStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);
/*
    DrvIcFwLyrGetFirmwareInfo(&g_FirmwareInfo);
#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
    g_FirmwareMode = g_FirmwareInfo.nFirmwareMode;
#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX
*/    
    return nSize;
}

static DEVICE_ATTR(sensor, SYSFS_AUTHORITY, DrvMainFirmwareSensorShow, DrvMainFirmwareSensorStore);

ssize_t DrvMainFirmwareHeaderShow(struct device *pDevice, struct device_attribute *pAttr, char *pBuf)
{
    DBG("*** %s() ***\n", __func__);

    return sprintf(pBuf, "%d", g_FirmwareInfo.nLogModePacketHeader);
}

ssize_t DrvMainFirmwareHeaderStore(struct device *pDevice, struct device_attribute *pAttr, const char *pBuf, size_t nSize)
{
    DBG("*** %s() ***\n", __func__);
/*
    DrvIcFwLyrGetFirmwareInfo(&g_FirmwareInfo);
#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
    g_FirmwareMode = g_FirmwareInfo.nFirmwareMode;
#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX
*/    
    return nSize;
}

static DEVICE_ATTR(header, SYSFS_AUTHORITY, DrvMainFirmwareHeaderShow, DrvMainFirmwareHeaderStore);

//------------------------------------------------------------------------------//

ssize_t DrvMainKObjectPacketShow(struct kobject *pKObj, struct kobj_attribute *pAttr, char *pBuf)
{
    u32 i = 0;
    u32 nLength = 0;

    DBG("*** %s() ***\n", __func__);

    if (strcmp(pAttr->attr.name, "packet") == 0)
    {
        if (g_LogModePacket != NULL)
        {
            DBG("g_FirmwareMode=%x, g_LogModePacket[0]=%x, g_LogModePacket[1]=%x\n", g_FirmwareMode, g_LogModePacket[0], g_LogModePacket[1]);
            DBG("g_LogModePacket[2]=%x, g_LogModePacket[3]=%x\n", g_LogModePacket[2], g_LogModePacket[3]);
            DBG("g_LogModePacket[4]=%x, g_LogModePacket[5]=%x\n", g_LogModePacket[4], g_LogModePacket[5]);

#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
            if ((g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE) && (g_LogModePacket[0] == 0xA5 || g_LogModePacket[0] == 0xAB || g_LogModePacket[0] == 0xA7))
#elif defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
            if ((g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE || g_FirmwareMode == FIRMWARE_MODE_RAW_DATA_MODE) && (g_LogModePacket[0] == 0x62))
#endif
            {
                for (i = 0; i < g_FirmwareInfo.nLogModePacketLength; i ++)
                {
                    pBuf[i] = g_LogModePacket[i];
                }

                nLength = g_FirmwareInfo.nLogModePacketLength;
                DBG("nLength = %d\n", nLength);
            }
            else
            {
                DBG("CURRENT MODE IS NOT DEBUG MODE/WRONG DEBUG MODE HEADER\n");
//            nLength = 0;
            }
        }
        else
        {
            DBG("g_LogModePacket is NULL\n");
//            nLength = 0;
        }
    }
    else
    {
        DBG("pAttr->attr.name = %s \n", pAttr->attr.name);
//        nLength = 0;
    }

    return nLength;
}

ssize_t DrvMainKObjectPacketStore(struct kobject *pKObj, struct kobj_attribute *pAttr, const char *pBuf, size_t nCount)
{
    DBG("*** %s() ***\n", __func__);
/*
    if (strcmp(pAttr->attr.name, "packet") == 0)
    {

    }
*/    	
    return nCount;
}

static struct kobj_attribute packet_attr = __ATTR(packet, 0666, DrvMainKObjectPacketShow, DrvMainKObjectPacketStore);

/* Create a group of attributes so that we can create and destroy them all at once. */
static struct attribute *attrs[] = {
    &packet_attr.attr,
    NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory. If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
struct attribute_group attr_group = {
    .attrs = attrs,
};
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

s32 DrvMainProcfsFirmwareSdCardUpdateShow(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    s32 nCount = 0;
    u16 nMajor = 0, nMinor = 0;
    
    DrvIcFwLyrGetCustomerFirmwareVersion(&nMajor, &nMinor, &_gFwVersion);

    DBG("*** %s() _gFwVersion = %s ***\n", __func__, _gFwVersion);

    nCount = sprintf(page, "%s\n", _gFwVersion);
    
    *eof = 1;

    return nCount;
}

s32 DrvMainProcfsFirmwareSdCardUpdateStore(struct file *file, const char *buffer, unsigned long count, void *data)
{    
    char *pValid = NULL;
    char *pTmpFilePath = NULL;
    char szFilePath[100] = {0};
    
    DBG("*** %s() ***\n", __func__);
    DBG("buffer = %s\n", buffer);

    if (buffer != NULL)
    {
        pValid = strstr(buffer, ".bin");
        
        if (pValid)
        {
            pTmpFilePath = strsep((char **)&buffer, ".");
            
            DBG("pTmpFilePath = %s\n", pTmpFilePath);

            strcat(szFilePath, pTmpFilePath);
            strcat(szFilePath, ".bin");

            DBG("szFilePath = %s\n", szFilePath);
            
            if (0 != DrvFwCtrlUpdateFirmwareBySdCard(szFilePath))
            {
                DBG("Update FAILED\n");
            }
            else
            {
                DBG("Update SUCCESS\n");
            }
        }
        else
        {
            DBG("The file type of the update firmware bin file is not a .bin file.\n");
        }
    }
    else
    {
        DBG("The file path of the update firmware bin file is NULL.\n");
    }

    return count;
}

//------------------------------------------------------------------------------//

s32 DrvMainTouchDeviceInitialize(void)
{
    s32 nRetVal = 0;
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    u8 *pDevicePath = NULL;
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    u8 *pGesturePath = NULL;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DBG("*** %s() ***\n", __func__);

    /* set sysfs for firmware */
    _gFirmwareClass = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
    if (IS_ERR(_gFirmwareClass))
        DBG("Failed to create class(firmware)!\n");

    _gFirmwareCmdDev = device_create(_gFirmwareClass, NULL, 0, NULL, "device");
    if (IS_ERR(_gFirmwareCmdDev))
        DBG("Failed to create device(_gFirmwareCmdDev)!\n");

    // version
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_version) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
    // update
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_update) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
    // data
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_data) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_data.attr.name);
#ifdef CONFIG_ENABLE_ITO_MP_TEST
    // test
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_test) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_test.attr.name);
    // test_log
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_test_log) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_test_log.attr.name);
    // test_fail_channel
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_test_fail_channel) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_test_fail_channel.attr.name);
    // test_scope
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_test_scope) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_test_scope.attr.name);
#endif //CONFIG_ENABLE_ITO_MP_TEST

#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    // mode
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_mode) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_mode.attr.name);
    // packet
//    if (device_create_file(_gFirmwareCmdDev, &dev_attr_packet) < 0)
//        DBG("Failed to create device file(%s)!\n", dev_attr_packet.attr.name);
    // sensor
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_sensor) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_sensor.attr.name);
    // header
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_header) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_header.attr.name);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    // debug
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_debug) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_debug.attr.name);
    // set debug value
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_set_debug_value) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_set_debug_value.attr.name);
    // platform version
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_platform_version) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_platform_version.attr.name);
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    // gesture wakeup mode
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_gesture_wakeup_mode) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_gesture_wakeup_mode.attr.name);

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    // gesture debug mode
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_gesture_debug) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_gesture_debug.attr.name);

    /* create a kset with the name of "kset_gesture" which is located under /sys/kernel/ */
    g_GestureKSet = kset_create_and_add("kset_gesture", NULL, kernel_kobj);
    if (!g_GestureKSet)
    {
        DBG("*** kset_create_and_add() failed, nRetVal = %d ***\n", nRetVal);
        nRetVal = -ENOMEM;
    }

    g_GestureKObj = kobject_create();
    if (!g_GestureKObj)
    {
        DBG("*** kobject_create() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    g_GestureKObj->kset = g_GestureKSet;

    nRetVal = kobject_add(g_GestureKObj, NULL, "%s", "kobject_gesture");
    if (nRetVal != 0)
    {
        DBG("*** kobject_add() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    /* create the files associated with this g_GestureKObj */
    nRetVal = sysfs_create_group(g_GestureKObj, &gestureattr_group);
    if (nRetVal != 0)
    {
        DBG("*** sysfs_create_file() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }

    pGesturePath = kobject_get_path(g_GestureKObj, GFP_KERNEL);
    DBG("DEVPATH = %s\n", pGesturePath);
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE

#ifdef CONFIG_ENABLE_GESTURE_INFORMATION_MODE
	// gesture information mode
if (device_create_file(_gFirmwareCmdDev, &dev_attr_gesture_infor) < 0)
	DBG("Failed to create device file(%s)!\n", dev_attr_gesture_infor.attr.name);
#endif //CONFIG_ENABLE_GESTURE_INFORMATION_MODE

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    // chip type
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_chip_type) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_chip_type.attr.name);
    // driver version
    if (device_create_file(_gFirmwareCmdDev, &dev_attr_driver_version) < 0)
        DBG("Failed to create device file(%s)!\n", dev_attr_driver_version.attr.name);

    dev_set_drvdata(_gFirmwareCmdDev, NULL);

#ifndef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
    // Create procfs file node
    _gMsTouchScreenMsg20xx = proc_mkdir(PROC_NODE_MS_TOUCHSCREEN_MSG20XX, NULL);
    _gSdCardUpdate = create_proc_entry(PROC_NODE_SDCARD_UPDATE, 0777, _gMsTouchScreenMsg20xx);

    if (NULL == _gSdCardUpdate) 
    {
        DBG("Failed to create procfs file node(%s)!\n", PROC_NODE_SDCARD_UPDATE);
    } 
    else 
    {
        _gSdCardUpdate->read_proc = DrvMainProcfsFirmwareSdCardUpdateShow;
        _gSdCardUpdate->write_proc = DrvMainProcfsFirmwareSdCardUpdateStore;
        DBG("Create procfs file node(%s) OK!\n", PROC_NODE_SDCARD_UPDATE);
    }
#endif

#ifdef CONFIG_ENABLE_ITO_MP_TEST
    DrvIcFwLyrCreateMpTestWorkQueue();
#endif //CONFIG_ENABLE_ITO_MP_TEST
    
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
    /* create a kset with the name of "kset_example" which is located under /sys/kernel/ */
    g_TouchKSet = kset_create_and_add("kset_example", NULL, kernel_kobj);
    if (!g_TouchKSet)
    {
        DBG("*** kset_create_and_add() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
    }

    g_TouchKObj = kobject_create();
    if (!g_TouchKObj)
    {
        DBG("*** kobject_create() failed, nRetVal = %d ***\n", nRetVal);

        nRetVal = -ENOMEM;
		    kset_unregister(g_TouchKSet);
		    g_TouchKSet = NULL;
    }

    g_TouchKObj->kset = g_TouchKSet;

    nRetVal = kobject_add(g_TouchKObj, NULL, "%s", "kobject_example");
    if (nRetVal != 0)
    {
        DBG("*** kobject_add() failed, nRetVal = %d ***\n", nRetVal);

		    kobject_put(g_TouchKObj);
		    g_TouchKObj = NULL;
		    kset_unregister(g_TouchKSet);
		    g_TouchKSet = NULL;
    }
    
    /* create the files associated with this kobject */
    nRetVal = sysfs_create_group(g_TouchKObj, &attr_group);
    if (nRetVal != 0)
    {
        DBG("*** sysfs_create_file() failed, nRetVal = %d ***\n", nRetVal);

        kobject_put(g_TouchKObj);
		    g_TouchKObj = NULL;
		    kset_unregister(g_TouchKSet);
		    g_TouchKSet = NULL;
    }
    
    pDevicePath = kobject_get_path(g_TouchKObj, GFP_KERNEL);
    DBG("DEVPATH = %s\n", pDevicePath);
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

    g_ChipType = DrvIcFwLyrGetChipType();

    if (g_ChipType != 0) // To make sure TP is attached on cell phone.
    {
#ifdef CONFIG_ENABLE_FIRMWARE_DATA_LOG
#if defined(CONFIG_ENABLE_CHIP_MSG26XXM)
        // get firmware mode for parsing packet judgement.
        g_FirmwareMode = DrvIcFwLyrGetFirmwareMode();
#endif //CONFIG_ENABLE_CHIP_MSG26XXM

        memset(&g_FirmwareInfo, 0x0, sizeof(FirmwareInfo_t));

        DrvIcFwLyrGetFirmwareInfo(&g_FirmwareInfo);

#if defined(CONFIG_ENABLE_CHIP_MSG21XXA) || defined(CONFIG_ENABLE_CHIP_MSG22XX)
        g_FirmwareMode = g_FirmwareInfo.nFirmwareMode;
#endif //CONFIG_ENABLE_CHIP_MSG21XXA || CONFIG_ENABLE_CHIP_MSG22XX
#endif //CONFIG_ENABLE_FIRMWARE_DATA_LOG

#ifdef CONFIG_UPDATE_FIRMWARE_BY_SW_ID
        DrvIcFwLyrCheckFirmwareUpdateBySwId();
#endif //CONFIG_UPDATE_FIRMWARE_BY_SW_ID
    }
    else
    {
    		nRetVal = -ENODEV;
    }

    return nRetVal;
}