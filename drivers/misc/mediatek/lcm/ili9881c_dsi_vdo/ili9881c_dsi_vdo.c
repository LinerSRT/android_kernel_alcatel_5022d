#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/wait.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             							0xFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
	Note :

	Data ID will depends on the following rule.

		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag

	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/


	{0xFF,3,{0x98,0x81,0x03}},

	{0x01,1,{0x00}},

	{0x02,1,{0x00}},

	{0x03,1,{0x53}},

	{0x04,1,{0x13}},

	{0x05,1,{0x13}},

	{0x06,1,{0x06}},

	{0x07,1,{0x00}},

	{0x08,1,{0x04}},

	{0x09,1,{0x00}},

	{0x0a,1,{0x00}},

	{0x0b,1,{0x00}},

	{0x0c,1,{0x00}},

	{0x0d,1,{0x00}},

	{0x0e,1,{0x00}},

	{0x0f,1,{0x00}},

	{0x10,1,{0x00}},

	{0x11,1,{0x00}},

	{0x12,1,{0x00}},

	{0x13,1,{0x00}},

	{0x14,1,{0x00}},

	{0x15,1,{0x00}},

	{0x16,1,{0x00}},

	{0x17,1,{0x00}},

	{0x18,1,{0x00}},

	{0x19,1,{0x00}},

	{0x1a,1,{0x00}},

	{0x1b,1,{0x00}},

	{0x1c,1,{0x00}},

	{0x1d,1,{0x00}},

	{0x1e,1,{0xC0}},

	{0x1f,1,{0x80}},

	{0x20,1,{0x04}},

	{0x21,1,{0x0B}},

	{0x22,1,{0x00}},

	{0x23,1,{0x00}},

	{0x24,1,{0x00}},

	{0x25,1,{0x00}},

	{0x26,1,{0x00}},

	{0x27,1,{0x00}},

	{0x28,1,{0x55}},

	{0x29,1,{0x03}},

	{0x2A,1,{0x00}},

	{0x2B,1,{0x00}},

	{0x2C,1,{0x00}},

	{0x2D,1,{0x00}},

	{0x2E,1,{0x00}},

	{0x2F,1,{0x00}},

	{0x30,1,{0x00}},

	{0x31,1,{0x00}},

	{0x32,1,{0x00}},

	{0x33,1,{0x00}},

	{0x34,1,{0x04}},

	{0x35,1,{0x05}},      

	{0x36,1,{0x05}},

	{0x37,1,{0x00}},

	{0x38,1,{0x3C}},      

	{0x39,1,{0x00}},

	{0x3a,1,{0x40}},

	{0x3b,1,{0x40}},

	{0x3c,1,{0x00}},

	{0x3d,1,{0x00}},

	{0x3e,1,{0x00}},

	{0x3f,1,{0x00}},

	{0x40,1,{0x00}},

	{0x41,1,{0x00}},

	{0x42,1,{0x00}},

	{0x43,1,{0x00}},

	{0x44,1,{0x00}},

	{0x50,1,{0x01}},

	{0x51,1,{0x23}},

	{0x52,1,{0x45}},

	{0x53,1,{0x67}},

	{0x54,1,{0x89}},

	{0x55,1,{0xAB}},

	{0x56,1,{0x01}},

	{0x57,1,{0x23}},

	{0x58,1,{0x45}},

	{0x59,1,{0x67}},

	{0x5a,1,{0x89}},

	{0x5b,1,{0xAB}},

	{0x5c,1,{0xCD}},

	{0x5d,1,{0xEF}},

	{0x5e,1,{0x01}},

	{0x5f,1,{0x14}},

	{0x60,1,{0x15}},

	{0x61,1,{0x0C}},

	{0x62,1,{0x0D}},

	{0x63,1,{0x0E}},

	{0x64,1,{0x0F}},

	{0x65,1,{0x10}},
	
	{0x66,1,{0x11}},

	{0x67,1,{0x08}},

	{0x68,1,{0x02}},

	{0x69,1,{0x0A}},

	{0x6a,1,{0x02}},

	{0x6b,1,{0x02}},

	{0x6c,1,{0x02}},

	{0x6d,1,{0x02}},

	{0x6e,1,{0x02}},

	{0x6f,1,{0x02}},

	{0x70,1,{0x02}},

	{0x71,1,{0x02}},

	{0x72,1,{0x06}},

	{0x73,1,{0x02}},

	{0x74,1,{0x02}},

	{0x75,1,{0x14}},

	{0x76,1,{0x15}},

	{0x77,1,{0x11}},

	{0x78,1,{0x10}},

	{0x79,1,{0x0F}},

	{0x7a,1,{0x0E}},

	{0x7b,1,{0x0D}},

	{0x7c,1,{0x0C}},

	{0x7d,1,{0x06}},

	{0x7e,1,{0x02}},

	{0x7f,1,{0x0A}},      

	{0x80,1,{0x02}},

	{0x81,1,{0x02}},

	{0x82,1,{0x02}},

	{0x83,1,{0x02}},

	{0x84,1,{0x02}},

	{0x85,1,{0x02}},

	{0x86,1,{0x02}},

	{0x87,1,{0x02}},

	{0x88,1,{0x08}},

	{0x89,1,{0x02}},

	{0x8a,1,{0x02}},

	{0xFF,3,{0x98,0x81,0x04}},

	{0x00,1,{0x00}},

	{0x6C,1,{0x15}},

	{0x6E,1,{0x3B}},
	
	{0x65,1,{0x06}},
	
	{0x31,1,{0x75}},

	{0x6F,1,{0x57}}, 

	{0x3A,1,{0xA4}},

	{0x8D,1,{0x15}},

	{0x87,1,{0xBA}},

	{0x26,1,{0x76}},

	{0XB2,1,{0XD1}},

	{0X88,1,{0X0B}},

    {0X17,1,{0X0c}},

	{0xFF,3,{0x98,0x81,0x01}},

	{0x22,1,{0x09}},

	{0x31,1,{0x00}},

	{0x53,1,{0x8A}},

	{0x55,1,{0x88}},

	{0x50,1,{0xA0}},

	{0x51,1,{0xA0}},

	{0x60,1,{0x14}},

	{0xA0,1,{0x08}},

	{0xA1,1,{0x21}},

	{0xA2,1,{0x30}},

	{0xA3,1,{0x0F}},

	{0xA4,1,{0x11}},

	{0xA5,1,{0x27}},

	{0xA6,1,{0x1C}},

	{0xA7,1,{0x1E}},

	{0xA8,1,{0x8C}},

	{0xA9,1,{0x1B}},

	{0xAA,1,{0x28}},

	{0xAB,1,{0x74}},

	{0xAC,1,{0x1A}},

	{0xAD,1,{0x19}},

	{0xAE,1,{0x4D}},

	{0xAF,1,{0x21}},

	{0xB0,1,{0x28}},

	{0xB1,1,{0x4A}},

	{0xB2,1,{0x5B}},

	{0xB3,1,{0x2C}},

	{0xC0,1,{0x08}},

	{0xC1,1,{0x21}},

	{0xC2,1,{0x30}},

	{0xC3,1,{0x0F}},

	{0xC4,1,{0x11}},

	{0xC5,1,{0x27}},

	{0xC6,1,{0x1C}},

	{0xC7,1,{0x1E}},

	{0xC8,1,{0x8C}},

	{0xC9,1,{0x1B}},

	{0xCA,1,{0x28}},

	{0xCB,1,{0x74}},

	{0xCC,1,{0x1A}},

	{0xCD,1,{0x19}},

	{0xCE,1,{0x4D}},

	{0xCF,1,{0x21}},

	{0xD0,1,{0x28}},

	{0xD1,1,{0x4A}},

	{0xD2,1,{0x59}},

	{0xD3,1,{0x2C}},

	{0xFF,3,{0x98,0x81,0x00}},

	{0x35,1,{0x00}},

	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
        {REGFLAG_END_OF_TABLE, 0x00, {}}

};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {}},
	{REGFLAG_DELAY, 20, {}},
	// Sleep Mode On
	{0x10, 0, {}},
        {REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd)
		{
			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;

			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

	params->physical_width = 62;
	params->physical_height = 116;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif

	params->dsi.switch_mode_enable = 0;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 8;
	params->dsi.vertical_backporch					= 20;
	params->dsi.vertical_frontporch					= 10;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 60;//32
	params->dsi.horizontal_backporch				= 60;//32
	params->dsi.horizontal_frontporch				= 60;//32
	//  params->dsi.horizontal_blanking_pixel				= 0;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.CLK_HS_PRPR = 3;
        params->dsi.HS_TRAIL=18;

#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 200; //this value must be in MTK suggested table
#else
	params->dsi.PLL_CLOCK = 250; //this value must be in MTK suggested table
#endif
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.ssc_disable = 1; // disable control (1: disable, 0: enable, default: 0) 

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	params->dsi.cont_clock=1;
}


static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);

	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	printk("[KERNEL]------ili9881c lcm_init--------\n");
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
	lcm_init();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(&data_array, 7, 0);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[10];
	unsigned int array[16];
	int adc_value;

	
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(120);
	array[0] = 0x00043902;
	array[1] = 0x018198FF;
	dsi_set_cmdq(array, 2, 1);
	
	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0x00, &buffer[0], 1);
	
	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0x01, &buffer[1], 1);
	
	if((0x98==buffer[0]) && (0x81==buffer[1]))
	{
		id = 0x9881;
	}

#ifdef BUILD_LK
//	printf("[LK]------otm1287a read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#else
//	printk("[KERNEL]------otm1287a read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#endif


	return ((id==0x9881)? 1 : 0);
}


LCM_DRIVER ili9881c_dsi_vdo_lcm_drv = 
{
	.name			= "ili9881c_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.set_backlight	= lcm_setbacklight,
	//.set_pwm        = lcm_setpwm,
	//.get_pwm        = lcm_getpwm,
	.update         = lcm_update,
#endif
	.compare_id = lcm_compare_id
};
