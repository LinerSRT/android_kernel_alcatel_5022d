/*************************************************************************************************
S5K4H5YC_otp.c
---------------------------------------------------------
OTP Application file From SEASONS for S5K4H5YC_
2013.01.14
---------------------------------------------------------
NOTE:
The modification is appended to initialization of image sensor. 
After sensor initialization, use the function , and get the id value.
bool otp_wb_update(BYTE zone)
and
bool otp_lenc_update(), 
then the calibration of AWB and LSC will be applied. 
After finishing the OTP written, we will provide you the golden_r1g and golden_b1g settings.
**************************************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>  
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>

#include <linux/xlog.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "s5k4h5ycmipiraw_Sensor.h"
#include <mach/mt_typedefs.h>
//#include "s5k4h5yc_mipiraw_Camera_Sensor_para.h"
//#include "s5k4h5yc_mipiraw_CameraCustomized.h"

#define S5K4H5YC_MIPI_WRITE_ID 0x20

#define S5K4H5YC_DEBUG
#ifdef S5K4H5YC_DEBUG
#define LOG_TAG (__FUNCTION__)
#define SENSORDB(fmt,arg...) xlog_printk(ANDROID_LOG_DEBUG , LOG_TAG, fmt, ##arg)  							//printk(LOG_TAG "%s: " fmt "\n", __FUNCTION__ ,##arg)
#else
#define SENSORDB(fmt,arg...)  
#endif

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);//add by hhl
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);//add by hhl
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);

#define USHORT unsigned short
#define BYTE unsigned char
#define Sleep(ms) mdelay(ms)
#define SK4H5_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, S5K4H5YC_MIPI_WRITE_ID)//add by hhl
static kal_uint16 SK4H5_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,S5K4H5YC_MIPI_WRITE_ID);
    return get_byte;
}


struct Darling_SK4H5_otp_struct {
int flag; 
int MID;
int LID;
int RGr_ratio;
int BGr_ratio;
int GbGr_ratio;
int VCM_start;
int VCM_end;
} Darling_SK4H5_OTP;

#define RGr_ratio_Typical    423
#define BGr_ratio_Typical    314
#define GbGr_ratio_Typical  512


//extern  void SK4H5_write_cmos_sensor(u16 addr, u32 para);
//extern  unsigned char SK4H5_read_cmos_sensor(u32 addr);

void Darling_SK4H5_read_OTP(struct Darling_SK4H5_otp_struct *SK4H5_OTP)
{
     unsigned char val=0;
     SK4H5_write_cmos_sensor(0x0100,0x01);
     SK4H5_write_cmos_sensor(0x3A02,0x00);
     SK4H5_write_cmos_sensor(0x3A00,0x01);
     mdelay(20);

     val=SK4H5_read_cmos_sensor(0x3A04);//flag of info and awb
     printk("Darling_SK4H5_read_OTP 0x3A04_val = %x",val);
	 if(val==0x40)
	 	{
	 	SK4H5_OTP->flag = 0x01;
		SK4H5_OTP->MID = SK4H5_read_cmos_sensor(0x3A05);
		SK4H5_OTP->LID = SK4H5_read_cmos_sensor(0x3A06);
		val = SK4H5_read_cmos_sensor(0x3A0A);
		SK4H5_OTP->RGr_ratio = (SK4H5_read_cmos_sensor(0x3A07)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->BGr_ratio = (SK4H5_read_cmos_sensor(0x3A08)<<2)+((val&0x30)>>4);
		SK4H5_OTP->GbGr_ratio = (SK4H5_read_cmos_sensor(0x3A09)<<2)+((val&0x0c)>>2);
	 	}
	 else if(val==0xd0)
	 	{
	 	SK4H5_OTP->flag=0x01;
		SK4H5_OTP->MID = SK4H5_read_cmos_sensor(0x3A0B);
		SK4H5_OTP->LID = SK4H5_read_cmos_sensor(0x3A0C);
		val = SK4H5_read_cmos_sensor(0x3A10);
		SK4H5_OTP->RGr_ratio = (SK4H5_read_cmos_sensor(0x3A0D)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->BGr_ratio = (SK4H5_read_cmos_sensor(0x3A0E)<<2)+((val&0x30)>>4);
		SK4H5_OTP->GbGr_ratio = (SK4H5_read_cmos_sensor(0x3A0F)<<2)+((val&0x0c)>>2);
	 	}
	 else
	 	{
	 	SK4H5_OTP->flag=0x00;
		SK4H5_OTP->MID =0x00;
		SK4H5_OTP->LID = 0x00;
		SK4H5_OTP->RGr_ratio = 0x00;
		SK4H5_OTP->BGr_ratio = 0x00;
		SK4H5_OTP->GbGr_ratio = 0x00;
	 	}

	 val=SK4H5_read_cmos_sensor(0x3A11);//falg of VCM
	 printk("	Darling_SK4H5_read_OTP 0x3A11_val = %x",val);
       if(val==0x40)
	 	{
	 	SK4H5_OTP->flag += 0x04;
		val = SK4H5_read_cmos_sensor(0x3A14);
		SK4H5_OTP->VCM_start= (SK4H5_read_cmos_sensor(0x3A12)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->VCM_end = (SK4H5_read_cmos_sensor(0x3A13)<<2)+((val&0x30)>>4);

	 	}
	 else if(val==0xd0)
	 	{
	 	SK4H5_OTP->flag+=0x04;
		val = SK4H5_read_cmos_sensor(0x3A17);
		SK4H5_OTP->VCM_start= (SK4H5_read_cmos_sensor(0x3A15)<<2)+((val&0xc0)>>6);
		SK4H5_OTP->VCM_end = (SK4H5_read_cmos_sensor(0x3A16)<<2)+((val&0x30)>>4);
	 	}
	 else
	 	{
	 	SK4H5_OTP->flag+=0x00;
		SK4H5_OTP->VCM_start= 0x00;
		SK4H5_OTP->VCM_end = 0x00;
	 	}
	 
	 SK4H5_write_cmos_sensor(0x3A00,0x00);
}

bool Darling_SK4H5_apply_OTP(struct Darling_SK4H5_otp_struct *SK4H5_OTP)
{

	int R_gain,B_gain,Gb_gain,Gr_gain,Base_gain;
	
      if(((SK4H5_OTP->flag)&0x03)!=0x01) 	return 0;
   
   	
	R_gain = (RGr_ratio_Typical*1000)/SK4H5_OTP->RGr_ratio;
	B_gain = (BGr_ratio_Typical*1000)/SK4H5_OTP->BGr_ratio;
	Gb_gain = (GbGr_ratio_Typical*1000)/SK4H5_OTP->GbGr_ratio;
	Gr_gain = 1000;
       Base_gain = R_gain;
	if(Base_gain>B_gain) Base_gain=B_gain;
	if(Base_gain>Gb_gain) Base_gain=Gb_gain;
	if(Base_gain>Gr_gain) Base_gain=Gr_gain;
	R_gain = 0x100 * R_gain /Base_gain;
	B_gain = 0x100 * B_gain /Base_gain;
	Gb_gain = 0x100 * Gb_gain /Base_gain;
	Gr_gain = 0x100 * Gr_gain /Base_gain;
	if(Gr_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x020e,Gr_gain>>8);
                   SK4H5_write_cmos_sensor(0x020f,Gr_gain&0xff);
		}
	if(R_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0210,R_gain>>8);
                   SK4H5_write_cmos_sensor(0x0211,R_gain&0xff);
		}
	if(B_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0212,B_gain>>8);
                   SK4H5_write_cmos_sensor(0x0213,B_gain&0xff);
		}
	if(Gb_gain>0x100)
		{
		     SK4H5_write_cmos_sensor(0x0214,Gb_gain>>8);
                   SK4H5_write_cmos_sensor(0x0215,Gb_gain&0xff);
		}

     return 1;
}

bool Daling_SK4H5_apply_OTP()
{
	return Darling_SK4H5_apply_OTP(&Darling_SK4H5_OTP);
}
void Daling_SK4H5_read_OTP()
{
	Darling_SK4H5_read_OTP(&Darling_SK4H5_OTP);
}