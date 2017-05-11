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
 * @file    mstar_drv_mtk.c
 *
 * @brief   This file defines the interface of touch screen
 *
 * @version v2.4.0.0
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/hwmsen_helper.h>
//#include <linux/hw_module_info.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>

#include <cust_eint.h>

//++ yanjun.jiang add for touchpanle version, 2015.05.09
//#include "tpd.h"
//-- yanjun.jiang add for touchpanle version, 2015.05.09

#include "cust_gpio_usage.h"
#include "pmic_drv.h"

//++ yanjun.jiang add for touchpanle version, 2015.05.09
#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_utility_adaption.h"
//-- yanjun.jiang add for touchpanle version, 2015.05.09

#include "mstar_drv_platform_interface.h"
#include "mstar_drv_common.h"

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/

#define MSG_TP_IC_NAME "msg2xxx" //"msg21xxA" or "msg22xx" or "msg26xxM" /* Please define the mstar touch ic name based on the mutual-capacitive ic or self capacitive ic that you are using */
#define I2C_BUS_ID   (1)       // i2c bus id : 0 or 1

#define TPD_OK (0)

//++ yanjun.jiang add for touchpanle version, 2015.05.09
#define TP_READ_VER
#ifdef TP_READ_VER
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#endif
//++ yanjun.jiang add for touchpanle version, 2015.05.09

/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
extern const int g_TpVirtualKey[];

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
extern const int g_TpVirtualKeyDimLocal[][4];
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

extern struct tpd_device *tpd;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT] = TPD_WARP_END;
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8] = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif
*/
struct i2c_client *g_I2cClient = NULL;

//static int boot_mode = 0;

//++ yanjun.jiang add for touchpanle version, 2015.05.09
#ifdef TP_READ_VER
    int data_buf[1];
#endif
//-- yanjun.jiang add for touchpanle version, 2015.05.09

/*=============================================================*/
// FUNCTION DECLARATION
/*=============================================================*/

/*=============================================================*/
// FUNCTION DEFINITION
/*=============================================================*/


#if defined(TPD_PROXIMITY)
u8 MSG_tpd_proximity_detect	= 1;//0-->close ; 1--> far away
static u8 	tpd_proximity_flag_suspend= 0;
static u8	tpd_proximity_flag = 0;

static int tpd_read_ps(void)
{
	MSG_tpd_proximity_detect;
	return 0;    
}

static int tpd_get_ps_value(void)
{
	return MSG_tpd_proximity_detect;
}
extern s32 IicWriteData(u8 nSlaveId, u8* pBuf, u16 nSize);
static int tpd_enable_ps(int enable)
{
	U8 bWriteData[4] =
     {
         0x52, 0x00, 0x4a/*0x24*/, 0xA0
     };

     MSG_tpd_proximity_detect=1;

     if(enable==1)
     {
		tpd_proximity_flag =1;
		bWriteData[3] = 0xA0;
		printk("msg_ps enable ps  tpd_proximity_flag =%d\n", tpd_proximity_flag);

     }
     else
     {       
		tpd_proximity_flag = 0;
		bWriteData[3] = 0xA1;
	printk("msg_ps disable ps  tpd_proximity_flag =%d\n", tpd_proximity_flag);
     }
  
     IicWriteData(0x26, &bWriteData[0], 4);
	 return 0;
}

static int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data *sensor_data;
	
	DBG("[msg ps]command = 0x%02X\n", command);		
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				DBG("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				DBG("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{	
				value = *(int *)buff_in;
				if(value)
				{		
					if((tpd_enable_ps(1) != 0))
					{
						DBG("enable ps fail: %d\n", err); 
						return -1;
					}
				}
				else
				{
					if((tpd_enable_ps(0) != 0))
					{
						DBG("disable ps fail: %d\n", err); 
						return -1;
					}
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				DBG("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				
				sensor_data = (hwm_sensor_data *)buff_out;				
				
				if((err = tpd_read_ps()))
				{
					err = -1;;
				}
				else
				{
					sensor_data->values[0] = tpd_get_ps_value();
					DBG("sensor_data->values[0] = %d\n", sensor_data->values[0]);
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}	
				
			}
			break;
		default:
			DBG("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;	
}
#endif

//++ yanjun.jiang add for touchpanle version, 2015.05.09
#ifdef TP_READ_VER
 static ssize_t tp_version_read(struct file *f, char __user *buf, size_t count, loff_t *pos)
 {
 	u16 nRegData1, nRegData2,nMajor,nMinor;

        DrvPlatformLyrTouchDeviceResetHw();
    
        DbBusEnterSerialDebugMode();
        DbBusStopMCU();
        DbBusIICUseBus();
        DbBusIICReshape();
        mdelay(100);
        
        // Stop mcu
        RegSetLByteValue(0x0FE6, 0x01); 

        // Stop watchdog
        RegSet16BitValue(0x3C60, 0xAA55);

        // RIU password
        RegSet16BitValue(0x161A, 0xABBA); 

        // Clear pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x80));

        RegSet16BitValue(0x1600, 0xBFF4); // Set start address for customer firmware version on main block
    
        // Enable burst mode
        // RegSet16BitValue(0x160C, (RegGet16BitValue(0x160C) | 0x01));

        // Set pce
        RegSet16BitValue(0x1618, (RegGet16BitValue(0x1618) | 0x40)); 
    
        RegSetLByteValue(0x160E, 0x01); 

        nRegData1 = RegGet16BitValue(0x1604);
        nRegData2 = RegGet16BitValue(0x1606);

        data_buf[0] = (((((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF)) <<16 )|((((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF));
        // Clear burst mode
        // RegSet16BitValue(0x160C, RegGet16BitValue(0x160C) & (~0x01));      

        RegSet16BitValue(0x1600, 0x0000); 

        // Clear RIU password
        RegSet16BitValue(0x161A, 0x0000); 

        DbBusIICNotUseBus();
        DbBusNotStopMCU();
        DbBusExitSerialDebugMode();

        DrvPlatformLyrTouchDeviceResetHw();
        mdelay(100);
		DBG("*** major = %d ***\n", nMajor);
		DBG("*** minor = %d ***\n", nMinor);
		
		if(copy_to_user(buf, (char *)data_buf, sizeof(data_buf)))
		{
		 return -EFAULT;
	  }
		
	 return count;
	 
         /*
	 u8 data;
	 long err = 0;
	 
 
	 unsigned char dbbus_tx_data[3];
	 unsigned char dbbus_rx_data[4] ;
	 unsigned short major=0, minor=0;
 
	 //Get_Chip_Version();
	 dbbus_tx_data[0] = 0x53;
	 dbbus_tx_data[1] = 0x00;
	 dbbus_tx_data[2] = 0x2A;
	 HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
	 HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);
 
	 major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
	 minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];
 
	 if(copy_to_user(buf, &minor, sizeof(minor)))
	 {
		 return -EFAULT;
	 }
 
	 return count;
	 */
 }
 static int tp_version_open(struct inode *inode, struct file *f)
 {
	 return 0;
 }
 static int tp_version_release(struct inode *inode, struct file *f)
 {
	 return 0;
 }
 static const struct file_operations tp_version_fops = {
	 .owner = THIS_MODULE,
	 .open = tp_version_open,
	 .release = tp_version_release,
	 .read = tp_version_read,
 };
 static struct miscdevice tp_version_struct = {
	 .name = "tp_ver",
	 .fops = &tp_version_fops,
	 .minor = MISC_DYNAMIC_MINOR,
 };
#endif
//-- yanjun.jiang add for touchpanle version, 2015.05.09

/* probe function is used for matching and initializing input device */
static int mstar_tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
#ifdef TPD_PROXIMITY
	int error;
	struct hwmsen_object obj_ps;
#endif

#ifdef CONFIG_ARCH_MT6580
	int ret = 0;
#endif

	//++ yanjun.jiang add for touchpanle version, 2015.05.09
	int err = 0;
	//-- yanjun.jiang add for touchpanle version, 2015.05.09
  
	TPD_DMESG("TPD probe\n");
	if (client == NULL)
	{
		TPD_DMESG("i2c client is NULL\n");
		return -1;
	}
	g_I2cClient = client;

	MsDrvInterfaceTouchDeviceSetIicDataRate(g_I2cClient, 100000); // 100 KHZ

#ifdef CONFIG_ARCH_MT6580
	tpd->reg = regulator_get(tpd->tpd_dev, PMIC_APP_CAP_TOUCH_VDD); // get pointer to regulator structure
	if (IS_ERR(tpd->reg))
	{
		printk("msg2238 tpd_probe regulator_get() failed!!!\n");
	}

	ret = regulator_set_voltage(tpd->reg, 2800000, 2800000);	// set 2.8v
	if(ret)
	{
		printk("msg2238 tpd_probe regulator_set_voltage() failed!\n");
	}
#endif

	if(MsDrvInterfaceTouchDeviceProbe(g_I2cClient, id) < 0)
	{
#ifdef CONFIG_ARCH_MT6580
		regulator_disable(tpd->reg); //disable regulator
		regulator_put(tpd->reg);
#endif
		return -1;
	}

#ifdef TPD_PROXIMITY
	obj_ps.polling = 0;//interrupt mode
	obj_ps.sensor_operate = tpd_ps_operate;
	printk("proxi_fts attach\n");
	if((error = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
	{
		DBG("proxi_fts attach fail = %d\n", error);
		printk("proxi_fts attach fail = %d\n", error);
	}
	else
	{
		//	g_alsps_name = tp_proximity; //ghq add 20140312
		DBG("proxi_fts attach ok = %d\n", error);
		printk("proxi_fts attach ok = %d\n", error);
	}		
#endif

//++ yanjun.jiang add for touchpanle version, 2015.05.09
#ifdef TP_READ_VER
    	if (err = misc_register(&tp_version_struct))
	{
		printk("msg2133 tp_version_struct register failed\n");
	}
#endif
//-- yanjun.jiang add for touchpanle version, 2015.05.09
	
	tpd_load_status = 1;

	TPD_DMESG("TPD probe done\n");

	return TPD_OK;
}

static int mstar_tpd_detect(struct i2c_client *client, struct i2c_board_info *info) 
{
	strcpy(info->type, TPD_DEVICE);    
//	strcpy(info->type, MSG_TP_IC_NAME);

	return TPD_OK;
}

static int mstar_tpd_remove(struct i2c_client *client)
{
	TPD_DEBUG("TPD removed\n");

	MsDrvInterfaceTouchDeviceRemove(client);

#ifdef CONFIG_ARCH_MT6580
	regulator_disable(tpd->reg); //disable regulator
	regulator_put(tpd->reg);
#endif

	return TPD_OK;
}

static struct i2c_board_info __initdata i2c_tpd = {I2C_BOARD_INFO(MSG_TP_IC_NAME, (0x4C>>1))};

/* The I2C device list is used for matching I2C device and I2C device driver. */
static const struct i2c_device_id mstar_tpd_device_id[] =
{
	{MSG_TP_IC_NAME, 0},
	{}, /* should not omitted */
};

MODULE_DEVICE_TABLE(i2c, tpd_device_id);

static struct i2c_driver tpd_i2c_driver = {
	.driver = {
		.name = MSG_TP_IC_NAME,
	},
	.probe = mstar_tpd_probe,
	.remove = mstar_tpd_remove,
	.id_table = mstar_tpd_device_id,
	.detect = mstar_tpd_detect,
};

static int tpd_local_init(void)
{  
	TPD_DMESG("TPD init device driver (Built %s @ %s)\n", __DATE__, __TIME__);
	/*
	// Software reset mode will be treated as normal boot
	boot_mode = get_boot_mode();
	if (boot_mode == 3)
	{
	boot_mode = NORMAL_BOOT;
	}
	*/
	if (i2c_add_driver(&tpd_i2c_driver) != 0)
	{
		TPD_DMESG("unable to add i2c driver.\n");

		return -1;
	}

	if (tpd_load_status == 0) 
	{
		TPD_DMESG("add error touch panel driver.\n");

		i2c_del_driver(&tpd_i2c_driver);
		return -1;
	}

#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
	// initialize tpd button data
	tpd_button_setting(4, g_TpVirtualKey, g_TpVirtualKeyDimLocal); //MAX_KEY_NUM
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE  
#endif //CONFIG_TP_HAVE_KEY  

	/*
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);    
#endif
	*/
	TPD_DMESG("TPD init done %s, %d\n", __FUNCTION__, __LINE__);  

	return TPD_OK;
}

static void tpd_resume(struct early_suspend *h)
{
	TPD_DMESG("TPD wake up\n");
#if defined(TPD_PROXIMITY)
	DBG("msg_ps resume tpd_proximity_flag =%d\n", tpd_proximity_flag);

	if(tpd_proximity_flag_suspend ==1)
	{
		return ;
	}
#endif

	MsDrvInterfaceTouchDeviceResume(h);
#if defined(TPD_PROXIMITY)
	if(tpd_proximity_flag)   /////reopen ps
	{
		tpd_enable_ps(1);
	}
#endif

	TPD_DMESG("TPD wake up done\n");
}

static void tpd_suspend(struct early_suspend *h)
{
	TPD_DMESG("TPD enter sleep\n");
#if defined(TPD_PROXIMITY)
	DBG("msg_ps suspend tpd_proximity_flag =%d\n", tpd_proximity_flag);

	tpd_proximity_flag_suspend = tpd_proximity_flag;
	if(tpd_proximity_flag_suspend ==1)
	{
		return;
	}
#endif

	MsDrvInterfaceTouchDeviceSuspend(h);

	TPD_DMESG("TPD enter sleep done\n");
} 

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = MSG_TP_IC_NAME,
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
#ifdef CONFIG_TP_HAVE_KEY
#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE        
#endif //CONFIG_TP_HAVE_KEY
};

static int __init tpd_driver_init(void) 
{
	TPD_DMESG("touch panel driver init\n");

	i2c_register_board_info(I2C_BUS_ID, &i2c_tpd, 1);
	if (tpd_driver_add(&tpd_device_driver) < 0)
	{
		TPD_DMESG("TPD add driver failed\n");
	}

	return 0;
}
 
static void __exit tpd_driver_exit(void) 
{
	TPD_DMESG("touch panel driver exit\n");

	tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
MODULE_LICENSE("GPL");
