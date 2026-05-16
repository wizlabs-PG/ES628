#ifndef _FPGA_DRAW_H_
#define _FPGA_DRAW_H_

#include <model_data.h>

//#define FPGA_SPI_DEV0 				"/dev/spidev1.0"
//#define FPGA_SPI_DEV1 				"/dev/spidev1.1"
#define FPGA_SPI_DEV0 				"/dev/spidev0.0"
#define FPGA_SPI_DEV1 				"/dev/spidev0.1"

#define DVI_TEMP		// dvi or hdmi converter
#if defined(DVI_TEMP)	// comment
#define FPGA_VBY1_4LANE_DVI_RBF		"ep628_dp2vx1_quad.rbf"
#define FPGA_VBY1_8LANE_DVI_RBF		"ep628_dp2vx1_octa.rbf"
#define FPGA_VBY1_16LANE_DVI_RBF	"ep628_dp2vx1_hexa.rbf"
#define FPGA_VBY1_32LANE_DVI_RBF	"ep628_dp2vx1_32lane.rbf"
#define FPGA_VBY1_64LANE_DVI_RBF	"ep628_dp2vx1_64lane.rbf"
#define FPGA_VBY1_DEF_DVI_RBF		"ep628_dp2vx1_hexa.rbf"

#define FPGA_LVDS_SINGLE_DVI_RBF	"ep628_dp2lvds_single.rbf"
#define FPGA_LVDS_DUAL_DVI_RBF		"ep628_dp2lvds_dual.rbf"
#define FPGA_LVDS_DEF_DVI_RBF		"ep628_dp2lvds_quad.rbf"

#define FPGA_eDP_QUAD_DVI_RBF		"ep628_dp2edp_quad.rbf"
#define FPGA_eDP_OCTA_DVI_RBF		"ep628_dp2edp_octa.rbf"
#define FPGA_eDP_HEX_DVI_RBF		"ep628_dp2edp_hex.rbf"
#define FPGA_eDP_32LANE_DVI_RBF		"ep628_dp2edp_32lane.rbf"
#endif

#define FPGA_LVDS_SINGLE_RBF		"ep628_lvds_sing.rbf"
#define FPGA_LVDS_DUAL_RBF			"ep628_lvds_dual.rbf"
#define FPGA_LVDS_DEF_RBF			"ep628_lvds_quad.rbf"
#define FPGA_LVDS_QUAD_RBF			"ep628_lvds_quad.rbf"
#define FPGA_LVDS_QUAD_5K_RBF		"ep628_lvds_quad_5k.rbf"	// seki 2020.01.08 (requested by kim tae-hee)

#define	FPGA_VBY1_4LANE_RBF			"ep628_vx1_quad.rbf"
#define FPGA_VBY1_8LANE_RBF			"ep628_vx1_octa.rbf"
#define FPGA_VBY1_16LANE_RBF		"ep628_vx1_hexa.rbf"
#define FPGA_VBY1_4LANEx4_RBF		"ep628_vx1_4lane_x4.rbf"
#define FPGA_VBY1_32LANE_RBF		"ep628_vx1_32lane.rbf"
#define FPGA_VBY1_64LANE_RBF		"ep628_vx1_64lane.rbf"
#define FPGA_VBY1_DEF_RBF			"ep628_vx1_hexa.rbf"
#define FPGA_VBY1_DEF_8BIT_RBF		"ep628_vx1_hexa.rbf"

#define FPGA_DP_4LANE_RBF			"ep628_edp_quad.rbf"
#define FPGA_DP_8LANE_RBF			"ep628_edp_octa.rbf"
#define FPGA_DP_16LANE_RBF			"ep628_edp_hexa.rbf"
#define FPGA_DP_32LANE_RBF			"ep628_edp_32lane.rbf"
#define FPGA_DP_DEF_RBF				"ep628_edp_hexa.rbf"


#define BURST_WRITE				4096
#define FPGA_SPI_MODE			0
#define FPGA_SPI_BITS			32
#define FPGA_SPI_SPEED			10000000
#define FPGA_UP_SPEED			100000000
#define FPGA_UP_LIMIT			45000000		// Arria 10 GX900	23M -> 24M	19-11-12
#define FPGA_SPI_DELAY			0
#define RegWrite				0x56
#define RegRead					0x0f
#define EdidWrite				0x3a
#define HgrayWrite				0x74

//ES628 I2C
#define I2c1OnAddrCtrlWrite		0xb3
#define I2c1OnAddrCtrlRead		0xb4
#define I2c1OnDataWrite			0xb7
#define I2c1OnDataRead			0xb8
#define I2c1OffAddrCtrlWrite	0xb5
#define I2c1OffAddrCtrlRead		0xb6
#define I2c1OffDataWrite		0xb9
#define I2c1OffDataRead			0xba

#define I2c2OnAddrCtrlWrite		0xc3
#define I2c2OnAddrCtrlRead		0xc4
#define I2c2OnDataWrite			0xc7
#define I2c2OnDataRead			0xc8
#define I2c2OffAddrCtrlWrite	0xc5
#define I2c2OffAddrCtrlRead		0xc6
#define I2c2OffDataWrite		0xc9
#define I2c2OffDataRead			0xca

#define I2cRdDataRead			0xbb

//ES628 VRR
#define VrrDataWrite			0xbc
#define VrrDataRead				0xbd
#define VrrDataExWrite			0xbe
#define VrrDataExRead			0xbf

//ES628 CTL
#define CtlWrite				0xc1
#define CtlRead					0xc2

//ES628 OCMD 2023.05.25 ksk
#define	OCMD1OnTableDataHWrite	0xcb
#define	OCMD1OnTableDataHRead 	0xcc
#define	OCMD1OnTableDataLWrite	0xcd
#define	OCMD1OnTableDataLRead	0xce
#define	OCMD1OnRegDataHWrite	0xcf
#define	OCMD1OnRegDataHRead		0xd0
#define	OCMD1OnRegDataLWrite	0xd1
#define	OCMD1OnRegDataLRead		0xd2

#define	OCMD1OffTableDataHWrite	0xd3
#define	OCMD1OffTableDataHRead 	0xd4
#define	OCMD1OffTableDataLWrite	0xd5
#define	OCMD1OffTableDataLRead	0xd6
#define	OCMD1OffRegDataHWrite	0xd7
#define	OCMD1OffRegDataHRead	0xd8
#define	OCMD1OffRegDataLWrite	0xd9
#define	OCMD1OffRegDataLRead	0xda

#define	OCMD2OnTableDataHWrite	0xdb
#define	OCMD2OnTableDataHRead 	0xdc
#define	OCMD2OnTableDataLWrite	0xdd
#define	OCMD2OnTableDataLRead	0xde
#define	OCMD2OnRegDataHWrite	0xdf
#define	OCMD2OnRegDataHRead		0xe0
#define	OCMD2OnRegDataLWrite	0xe1
#define	OCMD2OnRegDataLRead		0xe2

#define	OCMD2OffTableDataHWrite	0xe3
#define	OCMD2OffTableDataHRead 	0xe4
#define	OCMD2OffTableDataLWrite	0xe5
#define	OCMD2OffTableDataLRead	0xe6
#define	OCMD2OffRegDataHWrite	0xe7
#define	OCMD2OffRegDataHRead	0xe8
#define	OCMD2OffRegDataLWrite	0xe9
#define	OCMD2OffRegDataLRead	0xea
#define OCMDHDataRead			0xeb
#define OCMDLDataRead			0xec

#define	VrrGrayRWrite			0xed
#define	VrrGrayGWrite			0xee
#define	VrrGrayBWrite			0xef


#define FKEY_MASK				0x3FFF
//#define FPGA_DRAW_CNT			0x3F00
#define FPGA_DRAW_CNT			80000
//#define FPGA_DRAW_CNT			322560

// FPGA ADDRESS
typedef enum
{
	FPGA_POSION_R=0,		//0x00
	FPGA_POSION_G,			//0x01
	FPGA_POSION_B,			//0x02
	FPGA_MODE,				//0x03
	FPGA_BLANK1,			//0x04
	FPGA_DP_RX_CTRL,		//0x05
	FPGA_GRAY_SCALE,		//8,16,32,64,128,256,512,1024 	// 0x06
	FPGA_COLOR_SEL, 		//0:OFF 1:ON					// 0x07
	FPGA_H_ACTIVE,			//0x08
	FPGA_V_ACTIVE,			//0x09
	FPGA_VERSION,   		//30->0.3.6 122=>1.1.2			// 0x0a
	FPGA_INVERSION,  		//0:increase, 1:decrease		// 0x0b
	FPGA_H_WIDTH,			//0x0c
	FPGA_H_BPORCH,			//0x0d
	FPGA_H_TOTAL,			//0x0e
	FPGA_V_WIDTH,			//0x0f
	FPGA_V_BPORCH,			//0x10
	FPGA_V_TOTAL,			//0x11
	FPGA_QUAD_2DIV,			//0x12
	FPGA_PAL3_R,			//0x13
	FPGA_PAL3_G,			//0x14
	FPGA_PAL3_B,			//0x15
	FPGA_DCLK_LOW,			//0x16
	FPGA_DCLK_HIGH,			//0x17
	FPGA_SRAM_WR_CTRL,		//0x18
	AI_FRAME_COUNT,			//0x19
	FPGA_X_POSITION,		//0x1a
	FPGA_Y_POSITION,		//0x1b
	FPGA_MEM_WR_BANK,		//0x1c
	FPGA_MEM_RD_BANK,		//0x1d
	FPGA_MEM_WR_CTRL,		//0x1e
	FPGA_MEM_RD_CTRL,		//0x1f
	FPGA_PAL0_R,			//0x20
	FPGA_PAL0_G,			//0x21
	FPGA_PAL0_B,			//0x22
	FPGA_PAL1_R,			//0x23
	FPGA_PAL1_G,			//0x24
	FPGA_PAL1_B,			//0x25
	FPGA_PAL2_R,			//0x26
	FPGA_PAL2_G,			//0x27
	FPGA_PAL2_B,			//0x28
	FPGA_GRAY_LEVEL_R,		//0x29
	FPGA_GRAY_LEVEL_G,		//0x2A
	FPGA_GRAY_LEVEL_B,		//0x2B
	FPGA_VDD_VALUE,			//0x2C
	FPGA_IF_PWR_CTRL,		//0x2D (b0:if 3.3V ON/OFF, 0-on, 1-off) (b1:if 3.3V ON/OFF, 0-on, 1-off) (b2:if 3.3V ON/OFF, 0-on, 1-off)
	FPGA_KEY,				//0x2E [13:2]key, [1:0]jog
	FPGA_VAR_TCLK_LOW,		//0x2F [15:0] low main clock variable		// added by jschoi 2017.11.13
	FPGA_VAR_TCLK_HIGH,		//0x30 [31:16] high main clock variable		// added by jschoi 2017.11.13
	FPGA_VDD_OCP_DETECT,	//0x31 [0] high:normal, low:detect
	FPGA_PORT_MAP,			//0x32
	FPGA_DVI_ENABLE,		//0x33
	FPGA_SCROLL_CTRL,		//0x34 (b0:direction, 0:normal, 1:inverse) (b1:ver_enabel, 0:disable, 1:enable) (b2:hor_enable, 0:disable, 1:enable)
	FPGA_SCROLL_FRAME,		//0x35 [15:0]
	FPGA_SCROLL_PIXEL,		//0x36 [15:0]
	FPGA_REVERSE,			//0x37 (b2:hs, b1:vs, b0:de) 0:-, 1:+
	FPGA_PGID,				//0x38 [b5:port4, b4:port3, b3:port2, b2:port1](1:EP620T , 0:EP620S or X)		[1:0](master:0, slave:1~3)
	FPGA_VX1_SIG_ONOFF,		//0x39 0:off, 1:on
	FPGA_DPRX_LOCK_CHECK,	//0x3a 0:fail, 1:normal
	FPGA_8K_HG2D_ENABLE,		//0x3b 0:normal 8k , 1:hG2D 8k
	FPGA_I2C_MASTER_EN,				//0x3c [0] I2C master enable, [1] I2C select (0: I2C, 1: OCMD)
	FPGA_I2C_MASTER_CLK_SEL,		//0x3d
	FPGA_I2C_MASTER_REG_ADDR_SIZE,	//0x3e
	FPGA_I2C_MASTER_REG_DATA_SIZE,	//0x3f
	FPGA_I2C_MASTER_DEV_ADDR,		//0x40
	FPGA_I2C_MASTER_REG_ADDR,		//0x41
	FPGA_I2C_MASTER_REG_DATA,		//0x42
	FPGA_I2C_MASTER_ACK,			//0x43
	FPGA_I2C_SLAVE_EN,				//0x44
	FPGA_I2C_SLAVE_CLK_SEL,			//0x45
	FPGA_I2C_SLAVE_REG_ADDR_SIZE,	//0x46
	FPGA_I2C_SLAVE_REG_DATA_SIZE,	//0x47
	FPGA_I2C_SLAVE_DEV_ADDR,		//0x48
	FPGA_I2C_SLAVE_REG_ADDR,		//0x49
	FPGA_I2C_SLAVE_REG_DATA,		//0x4a
	FPGA_I2C_SLAVE_ACK,				//0x4b
	FPGA_TCLK_PLL_M_HIGH,			//0x4c
	FPGA_TCLK_PLL_M_LOW,			//0x4d
	FPGA_TCLK_PLL_N_HIGH,			//0x4e
	FPGA_TCLK_PLL_N_LOW,			//0x4f
	FPGA_XCVR_VOD,					//0x50
	FPGA_XCVR_1POST_TAP,			//0x51
	FPGA_XCVR_2POST_TAP,			//0x52
	FPGA_XCVR_1PRE_TAP,				//0x53
	FPGA_XCVR_2PRE_TAP,				//0x54
	FPGA_PWM_TOTAL,					//0x55
	FPGA_PWM_HIGH,					//0x56
	FPGA_PWM_EN,					//0x57
	FPGA_PARAM_LATCH_EN,			//0x58
	FPGA_VDE_READ,					//0x59
	FPGA_MOV_POS_X,					//0x5a
	FPGA_MOV_POS_Y,					//0x5b
	FPGA_CPPA_SRC_BANK,				//0x5c
	FPGA_CPPA_DST_BANK,				//0x5d
	FPGA_BIT_SLIP,					//0x5e
/*	FPGA_VRR_STEP,					//0x5f
	FPGA_VRR_CH0_V_TOTAL,			//0x60
	FPGA_VRR_CH1_V_TOTAL,			//0x61
	FPGA_VRR_CH2_V_TOTAL,			//0x62
	FPGA_VRR_CH3_V_TOTAL,			//0x63
	FPGA_VRR_CH4_V_TOTAL,			//0x64
	FPGA_VRR_CH5_V_TOTAL,			//0x65
	FPGA_VRR_CH6_V_TOTAL,			//0x66
	FPGA_VRR_CH7_V_TOTAL,			//0x67
	FPGA_VRR_CH8_V_TOTAL,			//0x68
	FPGA_VRR_CH9_V_TOTAL,			//0x69
	FPGA_VRR_CH0_FRAME,				//0x6a
	FPGA_VRR_CH1_FRAME,				//0x6b
	FPGA_VRR_CH2_FRAME,				//0x6c
	FPGA_VRR_CH3_FRAME,				//0x6d
	FPGA_VRR_CH4_FRAME,				//0x6e
	FPGA_VRR_CH5_FRAME,				//0x6f
	FPGA_VRR_CH6_FRAME,				//0x70
	FPGA_VRR_CH7_FRAME,				//0x71
	FPGA_VRR_CH8_FRAME,				//0x72
	FPGA_VRR_CH9_FRAME,				//0x73
*/	FPGA_SSC_AMPLITUDE = 0x74,		//0x74
	FPGA_SSC_RATE,					//0x75
	FPGA_IN_START_BAR_DESCENDING,	//0x76
	FPGA_IN_PIXEL_SPEED,			//0x77
	FPGA_IN_BAR_VSIZE,				//0x78
	FPGA_IN_SCROLL_CNT,				//0x79
	FPGA_IN_START_FRAME_PATTERN,	//0x7A
	FPGA_FIRST_LAST_FRAME,			//0x7B
	FPGA_ACL_MODE,					//0x7C
	FPGA_BIT_SHIFT,					//0x7D
	FPGA_CTL_EN,					//0x7E
	FPGA_DDR_CONTROLLER_RESET,		//0x7F
	FPGA_DP_H_ACTIVE,				//0x80
	FPGA_DP_H_WIDTH,				//0x81
	FPGA_DP_H_BPORCH,				//0x82
	FPGA_DP_H_TOTAL,				//0x83
	FPGA_DP_V_ACTIVE,				//0x84
	FPGA_DP_V_WIDTH,				//0x85
	FPGA_DP_V_BPORCH,				//0x86
	FPGA_DP_V_TOTAL,				//0x87 added in case dp 2port input
	FPGA_DP_HZ,						//0x88
	EDP_SIGNAL_I2C_ACK,				//0x89
	EDP_SIGNAL_READ_REQ,			//0x8a
	EDP_TX1_AC_PARAM_READ_WRITE,	//0x8b
	EDP_TX2_AC_PARAM_READ_WRITE,	//0x8c
	EDP_TX3_AC_PARAM_READ_WRITE,	//0x8d
	EDP_TX4_AC_PARAM_READ_WRITE,	//0x8e
	EDP_TX1_EQ_PARAM_READ_WRITE1,	//0x8f
	EDP_TX1_EQ_PARAM_READ_WRITE2,	//0x90
	EDP_TX2_EQ_PARAM_READ_WRITE1,	//0x91
	EDP_TX2_EQ_PARAM_READ_WRITE2,	//0x92
	EDP_TX3_EQ_PARAM_READ_WRITE1,	//0x93
	EDP_TX3_EQ_PARAM_READ_WRITE2,	//0x94
	EDP_TX4_EQ_PARAM_READ_WRITE1,	//0x95
	EDP_TX4_EQ_PARAM_READ_WRITE2,	//0x96

	FPGA_AUX_ADDR,					//0x97
	FPGA_AUX_WR_DATA,				//0x98
	FPGA_AUX_CTRL_REG,				//0x99
	FPGA_AUX_RD_DATA,				//0x9a
	FPGA_AUX_BYTE_NUM,				//0x9b
	FPGA_SEND_ARRAY_DATA,			//0x9c
	FPGA_SEND_ARRAY_SIZE,			//0x9d
	FPGA_SEND_ARRAY_CNT,			//0x9e
	FPGA_SEND_ARRAY_CTRL_REG,		//0x9f
	FPGA_AUX_RD_REG,				//0xa0

	FPGA_END_OF_REG
} enum_fpga_addr_t;

// VRR DATA
typedef enum
{
	FPGA_VRR_STEP=0,					// 0
	FPGA_VRR_V_TOTAL,				// 1 ~ 100
	FPGA_VRR_FRAME=101,				// 101 ~ 200
	FPAG_VRR_GRAY_SEL=201
} enum_fpga_vrr_data_addr_t;

typedef enum
{
	FPGA_VRR_V_TOTAL_EX=0,
	FPGA_VRR_FRAME_EX=100
} enum_fpga_vrr_ex_data_addr_t;

// I2C ON ADDR & CTRL ADDRESS
typedef enum
{
	FPGA_I2C_1_ON_REG_ADDR=0,			//0x00~0xc7
	FPGA_I2C_1_ON_DEV_ADDR=200,			//0xc8
	FPGA_I2C_1_ON_REG_ADDR_SIZE,		//0xc9
	FPGA_I2C_1_ON_REG_DATA_SIZE,		//0xca
	FPGA_I2C_1_ON_NUM,					//0xcb
	FPGA_I2C_1_ON_CLK_SEL,				//0xcc
	FPGA_I2C_1_ON_EN,					//0xcd

	FPGA_END_OF_I2C_1_ON_REG
} enum_fpga_i2c_1_on_addr_t;


typedef enum
{
	FPGA_I2C_2_ON_REG_ADDR=0,			//0x00~0xc7
	FPGA_I2C_2_ON_DEV_ADDR=200,			//0xc8
	FPGA_I2C_2_ON_REG_ADDR_SIZE,		//0xc9
	FPGA_I2C_2_ON_REG_DATA_SIZE,		//0xca
	FPGA_I2C_2_ON_NUM,					//0xcb
	FPGA_I2C_2_ON_CLK_SEL,				//0xcc
	FPGA_I2C_2_ON_EN,					//0xcd

	FPGA_END_OF_I2C_2_ON_REG
} enum_fpga_i2c_2_on_addr_t;

// I2C ON DATA ADDRESS
typedef enum
{
	FPGA_I2C_1_ON_REG_DATA=0,			//0x00~0xc7

	FPGA_END_OF_I2C_1_ON_DATA
} enum_fpga_i2c_1_on_data_t;

typedef enum
{
	FPGA_I2C_2_ON_REG_DATA=0,			//0x00~0xc7

	FPGA_END_OF_I2C_2_ON_DATA
} enum_fpga_i2c_2_on_data_t;

// I2C OFF ADDR & CTRL ADDRESS
typedef enum
{
	FPGA_I2C_1_OFF_REG_ADDR=0,		//0x00~0xc7
	FPGA_I2C_1_OFF_DEV_ADDR=200,	//0xc8
	FPGA_I2C_1_OFF_REG_ADDR_SIZE,	//0xc9
	FPGA_I2C_1_OFF_REG_DATA_SIZE,	//0xca
	FPGA_I2C_1_OFF_NUM,				//0xcb
	FPGA_I2C_1_OFF_CLK_SEL,			//0xcc
	FPGA_I2C_1_OFF_EN,				//0xcd

	FPGA_END_OF_I2C_1_OFF_REG
} enum_fpga_i2c_1_off_addr_t;

typedef enum
{
	FPGA_I2C_2_OFF_REG_ADDR=0,		//0x00~0xc7
	FPGA_I2C_2_OFF_DEV_ADDR=200,	//0xc8
	FPGA_I2C_2_OFF_REG_ADDR_SIZE,	//0xc9
	FPGA_I2C_2_OFF_REG_DATA_SIZE,	//0xca
	FPGA_I2C_2_OFF_NUM,				//0xcb
	FPGA_I2C_2_OFF_CLK_SEL,			//0xcc
	FPGA_I2C_2_OFF_EN,				//0xcd

	FPGA_END_OF_I2C_2_OFF_REG
} enum_fpga_i2c_2_off_addr_t;

// I2C OFF DATA ADDRESS
typedef enum
{
	FPGA_I2C_1_OFF_REG_DATA=0,			//0x00~0xc7

	FPGA_END_OF_I2C_1_OFF_DATA
} enum_fpga_i2c_1_off_data_t;

typedef enum
{
	FPGA_I2C_2_OFF_REG_DATA=0,			//0x00~0xc7

	FPGA_END_OF_I2C_2_OFF_DATA
} enum_fpga_i2c_2_off_data_t;


//	CTL
typedef enum
{
	FPGA_CTL_REG_ADDR=0,			//0x00~0xFF
	FPGA_CTL_ENABLE,				//0x104

	FPGA_END_OF_CTL_REG
} enum_fpga_ctl_data_t;

typedef enum
{
	FPGA_CTL_ON_REG_DATA=0,			//0x00~0xff
	FPGA_END_OF_CTL_ON_DATA
} enum_fpga_ctl_on_data_t;

typedef enum
{
	FKEY_BWD,
	FKEY_FWD,
	FKEY_ONOFF,
	FKEY_AUTOMANU,
	FKEY_GRAY,
	FKEY_POS,
	FKEY_FILE,
	FKEY_UP,
	FKEY_ESC,
	FKEY_MODEL,
	FKEY_FREQ,
	FKEY_FUNC,
	FKEY_OK,
	FKEY_DOWN
} enum_fpga_key_t;

typedef enum
{
	FPGA_LSHIFT_BIT_AUX_WR_REQ		= 0,
	FPGA_LSHIFT_BIT_AUX_VALUE_RST	= 1,
	FPGA_LSHIFT_BIT_AUX_RD_REQ		= 2,
	FPGA_LSHIFT_BIT_DATA_SIZE		= 3,
	FPGA_LSHIFT_BIT_AUX_TX_IDX		= 4
} enum_fpga_bit_aux_ctrl_reg_t;

typedef enum
{
	FPGA_LSHIFT_BIT_AUX_BYTE_NUM	= 0,
	FPGA_LSHIFT_BIT_AUX_CNT			= 8
} enum_fpga_bit_aux_byte_num_t;

typedef enum
{
	FPGA_LSHIFT_BIT_PWR_SEQ_AUX_REQ		= 0,
	FPGA_LSHIFT_BIT_PWR_SEQ_AUX_RST		= 1
} enum_fpga_bit_send_array_ctrl_reg_t;

unsigned int	scroll_frame_num;

int 			FPGA_Open(unsigned char mode,unsigned char bits,unsigned int speed,unsigned short delay);
unsigned short  read_fpga_c_register(unsigned char reg_addr);
int 			FPGA_SpiWrite(char *data);
int 			FPGA_SpiRead(char *data);
void 			FPGA_Write(unsigned char reg_addr, unsigned short data);
unsigned short 	FPGA_Read(unsigned char reg_addr);
void 			write_edid(unsigned char *edid);

void 			FPGA_OR_SET(unsigned char reg_addr, unsigned short data);
void 			FPGA_AND_SET(unsigned char reg_addr, unsigned short data);
void 			FPGA_ANDOR_SET(unsigned char reg_addr, unsigned short data1, unsigned short data2);
void 			FPGA_Close(void);

int 			FPGA_SendDraw(char * data, int len);
void 			FPGA_SendEnd(void);

int 			add_transfer(char * data, int len);
void 			transfer(void);
void 			FPGA_print(void);

extern int 		FPGA_update(int mode, int if_type);
extern void 	FPGA_key_scan(void);
extern int 		FPGA_ocp_detect(void);	// NACK:NORMAL, ACK:OCP
extern void		FPGA_scroll_ctrl(uint16_t dir, uint16_t model_mode, uint16_t pixel);

extern void 	FPGA_pre_emphasis_default(void);

void 			FPGA_Write_signed(unsigned char reg_addr, signed short data);

//es628
void 			FPGA_VRR_DATA_Write(unsigned char reg_addr, unsigned short data);
unsigned short 	FPGA_VRR_DATA_Read(unsigned char reg_addr);
void 			FPGA_VRR_DATA_Write_EX(unsigned char reg_addr, unsigned short data);
unsigned short 	FPGA_VRR_DATA_Read_EX(unsigned char reg_addr);
void 			FPGA_VRR_GRAY_R_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_VRR_GRAY_G_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_VRR_GRAY_B_Write(unsigned char reg_addr, unsigned short data);
void			FPGA_I2C_1_ON_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_1_ON_ADDR_CTRL_Read(unsigned char reg_addr);
void			FPGA_I2C_1_ON_DATA_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_1_ON_DATA_Read(unsigned char reg_addr);
void			FPGA_I2C_2_ON_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_2_ON_ADDR_CTRL_Read(unsigned char reg_addr);
void			FPGA_I2C_2_ON_DATA_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_2_ON_DATA_Read(unsigned char reg_addr);

//es628
void			FPGA_I2C_1_OFF_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_1_OFF_ADDR_CTRL_Read(unsigned char reg_addr);
void			FPGA_I2C_1_OFF_DATA_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_1_OFF_DATA_Read(unsigned char reg_addr);
void			FPGA_I2C_2_OFF_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_2_OFF_ADDR_CTRL_Read(unsigned char reg_addr);
void			FPGA_I2C_2_OFF_DATA_Write(unsigned char reg_addr, unsigned short data);
unsigned short	FPGA_I2C_2_OFF_DATA_Read(unsigned char reg_addr);
unsigned short	FPGA_I2C_RD_DATA_Read(unsigned char reg_addr);

//ocmd
void 			FPGA_OCMD_1_ON_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_ON_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_ON_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_ON_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_OFF_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_OFF_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_OFF_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_1_OFF_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data);

void 			FPGA_OCMD_2_ON_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_ON_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_ON_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_ON_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_OFF_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_OFF_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_OFF_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data);
void 			FPGA_OCMD_2_OFF_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data);

unsigned short  FPGA_OCMD_1_ON_TABLE_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_ON_TABLE_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_ON_REG_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_ON_REG_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_OFF_TABLE_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_OFF_TABLE_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_OFF_REG_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_1_OFF_REG_DATA_L_Read(unsigned char reg_addr);

unsigned short  FPGA_OCMD_2_ON_TABLE_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_ON_TABLE_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_ON_REG_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_ON_REG_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_OFF_TABLE_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_OFF_TABLE_DATA_L_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_OFF_REG_DATA_H_Read(unsigned char reg_addr);
unsigned short  FPGA_OCMD_2_OFF_REG_DATA_L_Read(unsigned char reg_addr);
unsigned short	FPGA_OCMD_H_DATA_Read(unsigned char reg_addr);
unsigned short	FPGA_OCMD_L_DATA_Read(unsigned char reg_addr);

void			FPGA_CTL_DATA_WRITE(unsigned char reg_addr, unsigned short data);
unsigned short 	FPGA_CTL_DATA_READ(unsigned char reg_addr);

void 			DP_EDID_change(void);


#endif // _FPGA_DRAW_H_
