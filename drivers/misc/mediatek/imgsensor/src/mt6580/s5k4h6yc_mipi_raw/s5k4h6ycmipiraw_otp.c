/*************************************************************************************************
S5K4H6YC_otp.c
---------------------------------------------------------
OTP Application file From SEASONS for S5K4H6YC_
2013.01.14
---------------------------------------------------------
NOTE:
The modification is appended to initialization of image sensor. 
After sensor initialization, use the function , and get the id value.
bool otp_wb_update(BYTE zone)
and
bool otp_lenc_update(), 
then the calibration of AWB and LSC will be applied. 
After finishing the OTP written, we will provide you the golden_rg and golden_bg settings.
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
#include "s5k4h6ycmipiraw_Sensor.h"
#include <mach/mt_typedefs.h>
//#include "s5k4h5yc_mipiraw_Camera_Sensor_para.h"
//#include "s5k4h5yc_mipiraw_CameraCustomized.h"

#define S5K4H6YC_MIPI_WRITE_ID 0x30

#define S5K4H6YC_DEBUG
#ifdef S5K4H6YC_DEBUG
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

BYTE S5K4H6YC_byteread_cmos_sensor(kal_uint32 addr)
{
#if 1
	BYTE get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,S5K4H6YC_MIPI_WRITE_ID);
	return get_byte;
#else
   kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,S5K4H6YC_MIPI_WRITE_ID);
    return get_byte;
#endif
}
/*
inline kal_uint16 S5K4H6YC_read_otp_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
	char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,1,S5K4H6YC_MIPI_WRITE_ID);
	return get_byte;
}
*/
static void S5K4H6YC_wordwrite_cmos_sensor(u16 addr, u32 para)
{
#if 1
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,  (char)(para >> 8),	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 4,S5K4H6YC_MIPI_WRITE_ID);
#endif
	//iWriteReg((u16) addr , (u32) para , 2,S5K4H6YC_MIPI_WRITE_ID);
	
}
static void S5K4H6YC_bytewrite_cmos_sensor(u16 addr, u32 para)
{
#if 1
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF)  ,	(char)(para & 0xFF) };
	iWriteRegI2C(puSendCmd , 3,S5K4H6YC_MIPI_WRITE_ID);
#else
	 iWriteReg((u16) addr , (u32) para , 1,S5K4H6YC_MIPI_WRITE_ID);//add by hhl
#endif
}

//extern  void S5K4H6YC_wordwrite_cmos_sensor(u16 addr, u32 para);
//extern  void S5K4H6YC_bytewrite_cmos_sensor(u16 addr, u32 para);


//#define SEASONS_ID           0x02
#define SEASONS_ID           0x10


//#define VALID_OTP          0x00
#define INVALID_OTP        0x01

#define GAIN_DEFAULT       0x0100
#define GAIN_GREEN1_ADDR   0x020E
#define GAIN_BLUE_ADDR     0x0212
#define GAIN_RED_ADDR      0x0210
#define GAIN_GREEN2_ADDR   0x0214

/*
#if 1
#define GRG_TYPICAL   0x31D        //0x3A0  0x2a1    //673,need check from module house,20140514
#define BG_TYPICAL    0x2DD       //0x2D8 0x23f    //575,need check from module house,20140514
#else

#define GRG_TYPICAL 0x001    //673,need check from module house,20140514
#define BG_TYPICAL 0x002    //575,need check from module house,20140514
#endif

kal_uint32 tRG_Ratio_typical = GRG_TYPICAL;
kal_uint32 tBG_Ratio_typical = BG_TYPICAL;
*/

USHORT golden_r;
USHORT golden_g;
USHORT golden_b;

USHORT current_r;
USHORT current_g;
USHORT current_b;

kal_uint32 r_ratio = 0;
kal_uint32 b_ratio = 0;


//kal_uint32	golden_r = 0, golden_gr = 0, golden_gb = 0, golden_b = 0;
//kal_uint32	current_r = 0, current_gr = 0, current_gb = 0, current_b = 0;
/*************************************************************************************************
* Function    :  start_read_otp
* Description :  before read otp , set the reading block setting  
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  0, reading block setting err
                 1, reading block setting ok 
**************************************************************************************************/
bool start_read_otp(BYTE zone)
{
	BYTE val = 0;
	int i;
    //S5K4H6YC_bytewrite_cmos_sensor(0x3A00, 0x04);//make initial state
	S5K4H6YC_bytewrite_cmos_sensor(0x3A02, zone);   //Select the page to write by writing to 0xD0000A02 0x00~0x0F
	S5K4H6YC_bytewrite_cmos_sensor(0x3A00, 0x01);   //Enter read mode by writing 01h to 0xD0000A00
	
    Sleep(5);//wait time > 47us

	return 1;
}

/*************************************************************************************************
* Function    :  stop_read_otp
* Description :  after read otp , stop and reset otp block setting  
**************************************************************************************************/
void stop_read_otp()
{
    //S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0x04);//make initial state
	S5K4H6YC_bytewrite_cmos_sensor(0x3A00, 0x00);//Disable NVM controller
}


/*************************************************************************************************
* Function    :  get_otp_AWB_flag
* Description :  get otp AWB_WRITTEN_FLAG  
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE], if 0x00 , this type has valid or empty otp data, otherwise, invalid otp data
**************************************************************************************************/
BYTE get_otp_AWB_flag(BYTE zone)
{
    BYTE flag_AWB = 0x00;
    BYTE Flag1=0,Flag0=0;
    
    if(zone != 0) 
    {
    	printk("S5K4H6YC zcw++: get_otp_AWB_flag :page error \n");
    	return flag_AWB;
    }
    start_read_otp(zone);
    
    
    Flag0=S5K4H6YC_byteread_cmos_sensor(0x3A04);
    //Flag1=S5K4H6YC_byteread_cmos_sensor(0x0A0C);
    
    flag_AWB =  Flag0 & 0x80;
    //flag_AWB = Flag0|Flag1;
    
    stop_read_otp();

    printk("S5K4H6YC zcw++: flag_AWB[page %d] := 0x%02x \n", zone, flag_AWB );
	return flag_AWB;
}

/*************************************************************************************************
* Function    :  get_otp_LSC_flag
* Description :  get otp LSC_WRITTEN_FLAG  
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE], if 0x00 , this type has valid or empty otp data, otherwise, invalid otp data
**************************************************************************************************/
BYTE get_otp_LSC_flag(BYTE zone)
{
    BYTE flag_LSC = 0x00;
    start_read_otp(zone);
    flag_LSC = S5K4H6YC_byteread_cmos_sensor(0x3A43);
    stop_read_otp();

    printk("S5K4H6YC zcw++: page[%d] flag_LSC := 0x%02x \n", zone,flag_LSC );
	return flag_LSC;
}

/*************************************************************************************************
* Function    :  get_otp_module_id
* Description :  get otp MID value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : module ID data , SEASONS ID is 0x0001            
**************************************************************************************************/
	BYTE get_otp_module_id(BYTE zone)
	{
		BYTE module_id = 0;
	    kal_uint16 Temp = 0;	
		if(!start_read_otp(zone))
		{
			printk("S5K4H6YC Start read Page %d Fail!", zone);
			return 0;
		}	
		Temp = S5K4H6YC_byteread_cmos_sensor(0x3A04);	
		stop_read_otp();	
		module_id =(Temp & 0xff);
    	printk("S5K4H6YC zcw++: Module ID = 0x%02x.\n",module_id);//module_id == 0x10		
		return module_id;		
	}


/*BYTE get_otp_module_id(BYTE zone)
{
	BYTE module_id = 0;
	kal_uint16 Temp=0;
 	kal_uint16 PageCount = 2;  //PageCount
	kal_uint16 GroupFlag=0,GroupFlag0=0,GroupFlag1=0;
	
	for(PageCount = 3; PageCount>=2; PageCount--)
	{
		SENSORDB("[S5K4H6YC_OFILM] [get_otp_module_id] PageCount=%d\n", PageCount);	
		
		if(PageCount>2)
			{//group 4,3
				//otp start control set 
				S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0X04); 
				S5K4H6YC_bytewrite_cmos_sensor(0x0A02, PageCount); // page 3
				S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0X01);
				mdelay(5);//delay 5ms
				
				GroupFlag1 =0;
				GroupFlag0 =0;
				GroupFlag0 =S5K4H6YC_read_otp_sensor(0x0a23);
				GroupFlag1 =S5K4H6YC_read_otp_sensor(0x0a43);
		
				if((GroupFlag0 ==0x01) || (GroupFlag1 ==0x01))
					{
						if(GroupFlag1 ==0x01)
							{	//group 4 module ID
								Temp=S5K4H6YC_read_otp_sensor(0x0a24);

								GroupFlag =4;//page 3 ->group 4,3
							}
							else
								{
									//group 3 module ID
									Temp=S5K4H6YC_read_otp_sensor(0x0a04);
										
									GroupFlag =3;//page 3 ->group 4,3
								}
						break; //
					}
			}
			else
				{//group 2,1
					//otp start control set 
					S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0X04); 
				    S5K4H6YC_bytewrite_cmos_sensor(0x0A02, PageCount); //page 2
					S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0X01);
					mdelay(5);//delay 5ms
					
					GroupFlag1 =0;
					GroupFlag0 =0;
					GroupFlag1 =S5K4H6YC_read_otp_sensor(0x0a0c);  // 0x0a43
					GroupFlag0 =S5K4H6YC_read_otp_sensor(0x0a04);  //0x0a23
					
					if((GroupFlag0 ==0x01) || (GroupFlag1 ==0x01))
						{
							if(GroupFlag1 ==0x01)
							{	//group 2 module ID
								Temp=S5K4H6YC_read_otp_sensor(0x0a0c);

								GroupFlag =2;//page 2 ->group 2,1
							}
							else
								{
									//group 1 module ID
									Temp=S5K4H6YC_read_otp_sensor(0x0a04);
	
									GroupFlag =1;//page 2 ->group 2,1
								}
									
							break;
						}	
				}
		//otp end control set 
		S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0X04); 
		S5K4H6YC_bytewrite_cmos_sensor(0x0A00, 0x00); 
	}
	

}*/


/*************************************************************************************************
* Function    :  get_otp_date
* Description :  get otp date value    
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f    
**************************************************************************************************/
/*
bool get_otp_date(BYTE zone) 
{
	BYTE year  = 0;
	BYTE month = 0;
	BYTE day   = 0;

	start_read_otp(zone);

	year  = S5K4H6YC_byteread_cmos_sensor(0x0A05);
	month = S5K4H6YC_byteread_cmos_sensor(0x0A06);
	day   = S5K4H6YC_byteread_cmos_sensor(0x0A07);

	stop_read_otp();

    printk("S5K4H5 zcw++: date=%02d.%02d.%02d \n", year,month,day);

	return 1;
}
*/


/*************************************************************************************************
* Function    :  get_otp_lens_id
* Description :  get otp LENS_ID value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : LENS ID data             
**************************************************************************************************/
BYTE get_otp_lens_id(BYTE zone)
{
	BYTE lens_id = 0;

	start_read_otp(zone);
	
	lens_id = S5K4H6YC_byteread_cmos_sensor(0x3A08);
	stop_read_otp();

	printk("S5K4H6YC zcw++: Lens ID = 0x%02x.\n",lens_id);

	return lens_id;
}


/*************************************************************************************************
* Function    :  get_otp_vcm_id
* Description :  get otp VCM_ID value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : VCM ID data             
**************************************************************************************************/
BYTE get_otp_vcm_id(BYTE zone)
{
	BYTE vcm_id = 0;

	start_read_otp(zone);
	
	vcm_id = S5K4H6YC_byteread_cmos_sensor(0x3A09);

	stop_read_otp();

	printk("S5K4H6YC zcw++: VCM ID = 0x%02x.\n",vcm_id);

	return vcm_id;	
}


/*************************************************************************************************
* Function    :  get_otp_driverIC_id
* Description :  get otp driverIC id value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE] 0 : OTP data fail 
                 other value : driver ID data             
**************************************************************************************************/
BYTE get_otp_driverIC_id(BYTE zone)
{
	BYTE driverIC_id = 0;

	start_read_otp(zone);
	
	driverIC_id = S5K4H6YC_byteread_cmos_sensor(0x3A0A);

	stop_read_otp();

	printk("S5K4H6YC zcw++: Driver ID = 0x%02x.\n",driverIC_id);

	return driverIC_id;
}

/*************************************************************************************************
* Function    :  get_light_id
* Description :  get otp environment light temperature value 
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f
* Return      :  [BYTE] 0 : OTP data fail 
                        other value : driver ID data     
			            BIT0:D65(6500K) EN
						BIT1:D50(5100K) EN
						BIT2:CWF(4000K) EN
						BIT3:A Light(2800K) EN
**************************************************************************************************/
BYTE get_light_id(BYTE zone)
{
	BYTE light_id = 0;

	start_read_otp(zone);	

	light_id = S5K4H6YC_byteread_cmos_sensor(0x3A0B);

	stop_read_otp();

	printk("S5K4H6YC zcw++: Light ID: 0x%02x.\n",light_id);

	return light_id;
}

   
/*************************************************************************************************
* Function    :  wb_gain_set
* Description :  Set WB ratio to register gain setting  512x
* Parameters  :  [int] r_ratio : R ratio data compared with golden module R
                       b_ratio : B ratio data compared with golden module B
* Return      :  [bool] 0 : set wb fail 
                        1 : WB set success            
**************************************************************************************************/

bool wb_gain_set()
{
    USHORT R_GAIN;
    USHORT B_GAIN;
    USHORT Gr_GAIN;
    USHORT Gb_GAIN;
    USHORT G_GAIN;

    printk("S5K4H6YC zcw++: OTP WB Gain Set Start: \n"); 
    if(!r_ratio || !b_ratio)
    {
        printk("S5K4H6YC zcw++: OTP WB ratio Data Err!\n");
        return 0;
    }
//S5K4H6YC_wordwrite_cmos_sensor(GAIN_GREEN1_ADDR, GAIN_DEFAULT); //Green 1 default gain 1x
//S5K4H6YC_wordwrite_cmos_sensor(GAIN_GREEN2_ADDR, GAIN_DEFAULT); //Green 2 default gain 1x
    if(r_ratio >= 512 )
    {
        if(b_ratio>=512) 
        {
            R_GAIN = (USHORT)(GAIN_DEFAULT * r_ratio / 512);						
            G_GAIN = GAIN_DEFAULT;
            B_GAIN = (USHORT)(GAIN_DEFAULT * b_ratio / 512);
        }
        else
        {
            R_GAIN =  (USHORT)(GAIN_DEFAULT*r_ratio / b_ratio );
            G_GAIN = (USHORT)(GAIN_DEFAULT*512 / b_ratio );
            B_GAIN = GAIN_DEFAULT;    
        }
    }
    else                      
    {		
        if(b_ratio >= 512)
        {
            R_GAIN = GAIN_DEFAULT;
            G_GAIN = (USHORT)(GAIN_DEFAULT*512 /r_ratio);		
            B_GAIN =  (USHORT)(GAIN_DEFAULT*b_ratio / r_ratio );
        } 
        else 
        {
            Gr_GAIN = (USHORT)(GAIN_DEFAULT*512/ r_ratio );						
            Gb_GAIN = (USHORT)(GAIN_DEFAULT*512/b_ratio );						
            if(Gr_GAIN >= Gb_GAIN)						
            {						
                R_GAIN = GAIN_DEFAULT;						
                G_GAIN = (USHORT)(GAIN_DEFAULT *512/ r_ratio );						
                B_GAIN =  (USHORT)(GAIN_DEFAULT*b_ratio / r_ratio );						
            } 
            else
            {						
                R_GAIN =  (USHORT)(GAIN_DEFAULT*r_ratio  / b_ratio);						
                G_GAIN = (USHORT)(GAIN_DEFAULT*512 / b_ratio );						
                B_GAIN = GAIN_DEFAULT;
            }
        }        
    }

    printk("S5K4H6YC zcw++: WB Gain Set [R_GAIN=%d],[G_GAIN=%d],[B_GAIN=%d] \n",R_GAIN, G_GAIN, B_GAIN);
    S5K4H6YC_wordwrite_cmos_sensor(GAIN_RED_ADDR, R_GAIN);		
    S5K4H6YC_wordwrite_cmos_sensor(GAIN_BLUE_ADDR, B_GAIN);     
    S5K4H6YC_wordwrite_cmos_sensor(GAIN_GREEN1_ADDR, G_GAIN); //Green 1 default gain 1x		
    S5K4H6YC_wordwrite_cmos_sensor(GAIN_GREEN2_ADDR, G_GAIN); //Green 2 default gain 1x

    printk("S5K4H6YC zcw++: OTP WB Gain Set Finished! \n");    
	return 1;
}



/*************************************************************************************************
* Function    :  get_otp_wb
* Description :  Get WB data    
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0f      
**************************************************************************************************/
bool get_otp_wb(BYTE zone)
{
	BYTE temph = 0;
	BYTE templ = 0;
	golden_r = 0, golden_g = 0, golden_b = 0;
	current_r = 0, current_g = 0,current_b = 0;

    start_read_otp(zone);

	current_r = S5K4H6YC_byteread_cmos_sensor(0x3A06);  
	current_g = S5K4H6YC_byteread_cmos_sensor(0x3A07); 
	current_b = S5K4H6YC_byteread_cmos_sensor(0x3A08);
	
	golden_r  = S5K4H6YC_byteread_cmos_sensor(0x3A09);
	golden_g  = S5K4H6YC_byteread_cmos_sensor(0x3A0a);
	golden_b  = S5K4H6YC_byteread_cmos_sensor(0x3A0b);	

	stop_read_otp();

	printk("S5K4H6YC zcw++: get_otp_wb[page %d]\n",zone);
    printk("S5K4H6YC zcw++: golden_r=%d,golden_g=%d,golden_b=%d \n",golden_r,golden_g,golden_b);
    printk("S5K4H6YC zcw++: current_r=%d,current_g=%d,current_b=%d \n",current_r,current_g,current_b);    
	return 1;
}


/*************************************************************************************************
* Function    :  otp_wb_update
* Description :  Update WB correction 
* Return      :  [bool] 0 : OTP data fail 
                        1 : otp_WB update success            
**************************************************************************************************/
bool otp_wb_update(BYTE zone)
{
	//USHORT golden_g, current_g;


	if(!get_otp_wb(zone))  // get wb data from otp
		return 0;

	//golden_g = (golden_gr + golden_gb) / 2;
	//current_g = (current_gr + current_gb) / 2;

	if(!golden_g || !current_g || !golden_r || !golden_b || !current_r || !current_b)
	{
		printk("S5K4H6YC zcw++: WB update Err !%d--%d--%d--%d--%d--%d--\n",golden_g,current_g,golden_r,golden_b,current_r,current_b);
		//return 0;
	}

	r_ratio = 512 * golden_r * current_g /( golden_g * current_r );
	b_ratio = 512 * golden_b * current_g /( golden_g * current_b );
    printk("S5K4H6YC zcw++: r_ratio=%d, b_ratio=%d \n",r_ratio, b_ratio);
	wb_gain_set();

	printk("S5K4H6YC zcw++: WB update finished! \n");

	return 1;
}

/*************************************************************************************************
* Function    :  otp_update()
* Description :  update otp data from otp , it otp data is valid, 
                 it include get ID and WB update function  
* Return      :  [bool] 0 : update fail
                        1 : update success
**************************************************************************************************/
bool otp_update()
{
	BYTE zone = 0x00;//start from Page 2
	BYTE FLG  = 0x00;
	BYTE MID  = 0x00,LENS_ID= 0x00,VCM_ID= 0x00;
	int i;
	
	for(i=0;i<3;i++)
	{
		FLG = get_otp_AWB_flag(zone);
		if(FLG == INVALID_OTP)
			zone ++;
		else
			break;
	}
	if(i==3)
	{
		printk("S5K4H6YC zcw++: No OTP Data or OTP data is invalid!!\n");
		return 0;
	}
    MID = get_otp_module_id(zone);
//    get_otp_date(zone);
//    LENS_ID=	get_otp_lens_id(zone);
//    VCM_ID=	get_otp_vcm_id(zone);
//    get_otp_driverIC_id(zone);
//    get_light_id(zone); 
	if(MID != SEASONS_ID)
	{
		printk("S5K4H6YC zcw++: Not SEASONS Modul!!!!\n");
		return 0;
	}
	otp_wb_update(zone);
	return 1;
}

