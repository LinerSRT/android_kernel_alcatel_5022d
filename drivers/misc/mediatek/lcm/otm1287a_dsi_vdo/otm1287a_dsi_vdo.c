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



	//************* Start Initial Sequence **********//
	{0x00,1,{0x00}},
	{0xff,3,{0x12,0x87,0x01}},	//EXTC=1
	{0x00,1,{0x80}},	        //Orise mode enable
	{0xff,2,{0x12,0x87}},
	{0x00,1,{0x92}},
	{0xff,2,{0x20,0x02}},               //MIPI 3 lane
	//-------------------- panel setting --------------------//
	{0x00,1,{0x80}},             //TCON Setting
	{0xc0,9,{0x00,0x64,0x00,0x10,0x10,0x00,0x64,0x10,0x10}},

	{0x00,1,{0x90}},             //Panel Timing Setting
	{0xc0,6,{0x00,0x5c,0x00,0x01,0x00,0x04}},

	{0x00,1,{0xa2}},
	{0xC0,3,{0x01,0x00,0x00}},

	{0x00,1,{0xb3}},             //Interval Scan Frame: 0 frame, column inversion
	{0xc0,3,{0x00,0x55,0x18}},

	{0x00,1,{0x81}},             //frame rate:65Hz
	{0xc1,1,{0x44}},		//0x55

	//-------------------- power setting --------------------//
	{0x00,1,{0xa0}},             //dcdc setting
	{0xc4,14,{0x05,0x10,0x04,0x02,0x05,0x15,0x11,0x05,0x10,0x07,0x02,0x05,0x15,0x11}},

	{0x00,1,{0xb0}},             //clamp voltage setting
	{0xc4,2,{0x00,0x00}},

	{0x00,1,{0x91}},             //VGH=13V, VGL=-12V, pump ratio:VGH=6x, VGL=-5x
	{0xc5,2,{0x29,0x52}},//29

	{0x00,1,{0x00}},             //GVDD=4.396V, NGVDD=-4.396V
	{0xd8,2,{0xbe,0xbe}},     

	{0x00,1,{0x00}}, 		//VCOM=-0.936V
	{0xd9,1,{0x47}}, //6C            // ×î¼ÑvcomÖµ£º#1->4a, #2->42, #3->46£¬aver£º47        

	{0x00,1,{0xb3}},             //VDD_18V=1.7V, LVDSVDD=1.6V
	{0xc5,1,{0x44}},//84

	{0x00,1,{0xbb}},             //LVD voltage level setting
	{0xc5,1,{0x8a}},//8a

	{0x00,1,{0x82}},		//chopper
	{0xC4,1,{0x0a}},

	{0x00,1,{0xc6}},		//debounce
	{0xb0,1,{0x03}},

	//-------------------- control setting --------------------//
	{0x00,1,{0x00}},             //ID1
	{0xd0,1,{0x40}},

	{0x00,1,{0x00}},             //ID2, ID3
	{0xd1,2,{0x00,0x00}},

	//-------------------- power on setting --------------------//
	{0x00,1,{0x80}},             //source blanking frame = black, defacult='30'
	{0xc4,1,{0x00}},
	                        
	{0x00,1,{0x98}},             //vcom discharge=gnd:'10', '00'=disable
	{0xc5,1,{0x10}},
	                           
	{0x00,1,{0x81}},
	{0xf5,1,{0x15}},  // ibias off
	{0x00,1,{0x83}},
	{0xf5,1,{0x15}},  // lvd off
	{0x00,1,{0x85}},
	{0xf5,1,{0x15}},  // gvdd off
	{0x00,1,{0x87}},
	{0xf5,1,{0x15}},  // lvdsvdd off
	{0x00,1,{0x89}},
	{0xf5,1,{0x15}},  // nvdd_18 off
	{0x00,1,{0x8b}},
	{0xf5,1,{0x15}},  // en_vcom off
	                            
	{0x00,1,{0x95}},
	{0xf5,1,{0x15}},  // pump3 off
	{0x00,1,{0x97}},
	{0xf5,1,{0x15}},  // pump4 off
	{0x00,1,{0x99}},
	{0xf5,1,{0x15}},  // pump5 off
	                             
	{0x00,1,{0xa1}},
	{0xf5,1,{0x15}},  // gamma off
	{0x00,1,{0xa3}},
	{0xf5,1,{0x15}},  // sd ibias off
	{0x00,1,{0xa5}},
	{0xf5,1,{0x15}},  // sdpch off
	{0x00,1,{0xa7}},
	{0xf5,1,{0x15}},  // sdpch bias off
	{0x00,1,{0xab}},
	{0xf5,1,{0x18}},  // ddc osc off
	                             
	{0x00,1,{0x94}},             //VCL pump dis
	{0xf5,2,{0x00,0x00}},
	                             
	{0x00,1,{0xd2}},             //VCL reg. en
	{0xf5,2,{0x06,0x15}},
	                           
	{0x00,1,{0xb2}},             //VGLO1
	{0xf5,2,{0x00,0x00}},
	                             
	{0x00,1,{0xb6}},             //VGLO2
	{0xf5,2,{0x00,0x00}},
	                           
	{0x00,1,{0xb4}},             //VGLO1/2 Pull low setting
	{0xc5,1,{0xcc}},		//d[7] vglo1 d[6] vglo2 => 0: pull vss, 1: pull vgl
	                            
	//-------------------- for Po,{wer IC ---------------------------------
	{0x00,1,{0x90}},             //Mode-3
	{0xf5,4,{0x02,0x11,0x02,0x15}},
	                            
	{0x00,1,{0x90}},             //2xVPNL, 1.5*=00, 2*=50, 3*=a0
	{0xc5,1,{0x50}},
	                          
	{0x00,1,{0x94}},             //Frequency
	{0xc5,1,{0x55}},

	//-------------------- panel timing state control --------------------//
	{0x00,1,{0x80}},             //panel timing state control
	{0xcb,11,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0x90}},             //panel timing state control
	{0xcb,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0xff,0x00}},

	{0x00,1,{0xa0}},             //panel timing state control
	{0xcb,15,{0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xb0}},             //panel timing state control
	{0xcb,15,{0x00,0x00,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00}},

	{0x00,1,{0xc0}},             //panel timing state control
	{0xcb,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x00,0x05,0x05,0x05,0x05,0x05}},

	{0x00,1,{0xd0}},             //panel timing state control
	{0xcb,15,{0x05,0x05,0x05,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05}},

	{0x00,1,{0xe0}},             //panel timing state control
	{0xcb,14,{0x05,0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x00,0x00}},

	{0x00,1,{0xf0}},             //panel timing state control
	{0xcb,11,{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}},

	//-------------------- panel pad mapping control --------------------//
	{0x00,1,{0x80}},             //panel pad mapping control
	{0xcc,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x07,0x00,0x11,0x15,0x13,0x17,0x0d}},

	{0x00,1,{0x90}},             //panel pad mapping control
	{0xcc,15,{0x09,0x0f,0x0b,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06}},

	{0x00,1,{0xa0}},             //panel pad mapping control
	{0xcc,14,{0x08,0x00,0x12,0x16,0x14,0x18,0x0e,0x0a,0x10,0x0c,0x02,0x04,0x00,0x00}},

	{0x00,1,{0xb0}},             //panel pad mapping control
	{0xcc,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x02,0x00,0x14,0x18,0x12,0x16,0x0c}},

	{0x00,1,{0xc0}},             //panel pad mapping control
	{0xcc,15,{0x10,0x0a,0x0e,0x08,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03}},

	{0x00,1,{0xd0}},             //panel pad mapping control
	{0xcc,14,{0x01,0x00,0x13,0x17,0x11,0x15,0x0b,0x0f,0x09,0x0d,0x07,0x05,0x00,0x00}},

	//-------------------- panel timing setting --------------------//
	{0x00,1,{0x80}},             //panel VST setting
	{0xce,12,{0x87,0x03,0x28,0x86,0x03,0x28,0x85,0x03,0x28,0x84,0x03,0x28}},

	{0x00,1,{0x90}},             //panel VEND setting
	{0xce,14,{0x34,0xfc,0x28,0x34,0xfd,0x28,0x34,0xfe,0x28,0x34,0xff,0x28,0x00,0x00}},

	{0x00,1,{0xa0}},             //panel CLKA1/2 setting
	{0xce,14,{0x38,0x07,0x05,0x00,0x00,0x28,0x00,0x38,0x06,0x05,0x01,0x00,0x28,0x00}},

	{0x00,1,{0xb0}},             //panel CLKA3/4 setting
	{0xce,14,{0x38,0x05,0x05,0x02,0x00,0x28,0x00,0x38,0x04,0x05,0x03,0x00,0x28,0x00}},

	{0x00,1,{0xc0}},             //panel CLKb1/2 setting
	{0xce,14,{0x38,0x03,0x05,0x04,0x00,0x28,0x00,0x38,0x02,0x05,0x05,0x00,0x28,0x00}},

	{0x00,1,{0xd0}},             //panel CLKb3/4 setting
	{0xce,14,{0x38,0x01,0x05,0x06,0x00,0x28,0x00,0x38,0x00,0x05,0x07,0x00,0x28,0x00}},

	{0x00,1,{0x80}},             //panel CLKc1/2 setting
	{0xcf,14,{0x38,0x07,0x05,0x00,0x00,0x18,0x25,0x38,0x06,0x05,0x01,0x00,0x18,0x25}},

	{0x00,1,{0x90}},             //panel CLKc3/4 setting
	{0xcf,14,{0x38,0x05,0x05,0x02,0x00,0x18,0x25,0x38,0x04,0x05,0x03,0x00,0x18,0x25}},

	{0x00,1,{0xa0}},             //panel CLKd1/2 setting
	{0xcf,14,{0x38,0x03,0x05,0x04,0x00,0x18,0x25,0x38,0x02,0x05,0x05,0x00,0x18,0x25}},

	{0x00,1,{0xb0}},             //panel CLKd3/4 setting
	{0xcf,14,{0x38,0x01,0x05,0x06,0x00,0x18,0x25,0x38,0x00,0x05,0x07,0x00,0x18,0x25}},

	{0x00,1,{0xc0}},             //panel ECLK setting
	{0xcf,11,{0x01,0x01,0x20,0x20,0x00,0x00,0x01,0x81,0x00,0x03,0x08}},

	//-------------------- gamma2.2 --------------------//
	{0x00,1,{0x00}},
	{0xe1, 20,{0x02,0x4a,0x56,0x65,0x72,0x7f,0x7f,0xa8,0x96,0xab,0x5a,0x45,0x5a,0x3b,0x40,0x32,0x25,0x1a,0x11,0x06}},
	{0x00,1,{0x00}},
	{0xe2, 20,{0x02,0x4b,0x56,0x65,0x72,0x7f,0x7f,0xa8,0x96,0xab,0x5a,0x45,0x5a,0x3b,0x40,0x32,0x25,0x1a,0x11,0x06}},


	{0x35,1,{0x01}},

	{0x00,1,{0xa0}},             //
	{0xc1,1,{0x02}},
	
	//{0x00,1,{0xa0}},             //
	//{0xc1,1,{0xe8}},
	//{0x00,1,{0x90}},             //
	//{0xf6,1,{0x15}},

	{0x00,1,{0x80}},             
	{0xc4,1,{0x01}},

	{0x00,1,{0x88}},             
	{0xc4,1,{0x80}},

	{0x00,1,{0xc2}},             
	{0xf5,1,{0xc0}},

	{0x00,1,{0x81}},             
	{0xc4,1,{0x82}},
	
	{0x00,1,{0x00}},             //Orise mode disable
	{0xff,3,{0xff,0xff,0xff}},

	{0x11, 0, {0x00}},
	{REGFLAG_DELAY, 200, {}},

	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 20, {}}
};        //panel VEND setting
	
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	// Sleep Mode On
	{0x10, 0, {}},
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
	params->dsi.vertical_backporch					= 16;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 6;//32
	params->dsi.horizontal_backporch				= 85;//32
	params->dsi.horizontal_frontporch				= 85;//32
	//  params->dsi.horizontal_blanking_pixel				= 0;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.CLK_HS_PRPR = 3;

#ifndef CONFIG_FPGA_EARLY_PORTING
#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 250; //this value must be in MTK suggested table
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
	params->dsi.lcm_esd_check_table[0].cmd          = 0xac;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x00;
	/*
	params->dsi.lcm_esd_check_table[1].cmd          = 0x0E;
	params->dsi.lcm_esd_check_table[1].count        = 1;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x80;
	
	params->dsi.lcm_esd_check_table[2].cmd          = 0xAC;
	params->dsi.lcm_esd_check_table[2].count        = 1;
	params->dsi.lcm_esd_check_table[2].para_list[2] = 0x00;
	*/
	//0E->80 AC ->00
	// Non-continuous clock
	//params->dsi.noncont_clock = 1;
	//params->dsi.noncont_clock_period = 2;	// Unit : frame
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
	printk("[KERNEL]------otm1287a lcm_init---------\n");
}


static void lcm_suspend(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);

	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
	//unsigned char buffer[10];
	//unsigned int array[16];
	lcm_init();
	#if 0
	array[0]= 0x04002300;
	dsi_set_cmdq(array, 1, 1);
	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xF8, &buffer[0], 1);
	printk("otm1287a lcm_resume  READ 0xF8= 0x%x\n", buffer[0]);
	#endif
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

	array[0] = 0x00043700;// return byte number
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xa1, &buffer[0], 4);
	if((0x12==buffer[2]) && (0x87==buffer[3]))
	{
		id = 0x1287;
	}

#ifdef BUILD_LK
//	printf("[LK]------otm1287a read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#else
	printk("[KERNEL]------otm1287a read id = 0x%x, 0x%x, 0x%x---------\n", buffer[0], buffer[1], buffer[2]);
#endif

	return ((id==0x1287)? 1 : 0);
}


LCM_DRIVER otm1287a_dsi_vdo_lcm_drv = 
{
	.name			= "otm1287a_dsi_vdo",
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
