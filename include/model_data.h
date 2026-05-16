/*
 * model_data.h
 *
 *  Created on: Sep 19, 2017
 *      Author: root
 */

#ifndef MODEL_DATA_H_
#define MODEL_DATA_H_

#include <global.h>
#include <gx.h>

// Definitions
#define MAX_MODEL_CNT			256
#define MAX_BMP_PATH			260
#define MAX_INS_CNT				2
#define MAX_SEQ_CNT				4
#define MAX_CYCLETEST_CNT		10
#define MAX_RESERVED			202
										// 2018.02.14 (254 -> 252)
										// 2018.03.19 (252 -> 251)
										// 2019.08.01 (251->207)
										// 2020.04.28 (207->206)
										// 2023.02.24 (206->203)
										// 2023.03.17 (203->202) for vbl timeout
										// 2023.03.22 (202->201) for dp input port select
#define MAX_PORT_CNT			7
#define MAX_HANDO_CNT			40
#define MAX_TEGRA_HACTIVE		3840
#define MAX_TEGRA_VACTIVE		2160

#define FPGA_8K_DIVIDER			4	// 7680 x 4320
#define TEGRA_5K_DIVIDER		2	// 5120 x 1440
#define MAX_MEMORY_PRELOAD_CNT	30
//#define MAX_MEMORY_PRELOAD_CNT	200

#define MAX_FPGA_FRELOAD_CNT	128 -2
#define FPGA_REALTIME_MEMORY0	126
#define FPGA_REALTIME_MEMORY1	127
#define FPGA_MEMORY_BANK_OFFSET 5
//#define MAX_FPGA_FRELOAD_CNT	256 -2
//#define FPGA_REALTIME_MEMORY0	124
//#define FPGA_REALTIME_MEMORY1	125
//#define FPGA_MEMORY_BANK_OFFSET 200

#define GL_GLEXT_PROTOTYPES 	(1)
#define GLX_GLXEXT_PROTOTYPES 	(1)



// Enumerations
typedef enum
{
	MODE_SINGLE,
	MODE_DUAL,
	MODE_QUAD,
	MODE_OCTA,
	MODE_HEXA,
	MODE_32LANE_8x4,
	MODE_64LANE,
	MODE_4LANEx4,
	MODE_32LANE_16x2
} enum_mode_t;

typedef enum
{
	TYPE_6B,
	TYPE_8B,
	TYPE_10B
} enum_type_t;

typedef enum
{
	TWIST_JEIDA,
	TWIST_VESA
} enum_twist_t;

typedef enum
{
	IF_LVDS,
	IF_VBY1,
	IF_HDMI,
	IF_DP,
	IF_eDP
} enum_if_t;

typedef enum
{
	FLIP_NONE=0,
	FLIP_HV,
	FLIP_V,
	FLIP_H
} enum_disp_mode_t;

//#define DRAWCHECK(x)			((x<IF_HDMI) ? FPGA : NV_CPU)
#define DRAWCHECK(x)			((x==IF_HDMI) ? NV_CPU : FPGA)

// Structures
typedef struct
{
	uint8_t			start_idx;						// 0 ~ 255
	uint8_t			end_idx;						// 0 ~ 255
	uint16_t		on_time;						// sec
	uint16_t		off_time;						// sec
	uint16_t		cycle_cnt;						// 1 ~ 999
} __PACKED__ cycle_test_t;

typedef struct
{
	uint16_t		useage;
	char			name[MAX_PATH];
} __PACKED__ hando_list_t;

typedef struct
{
	uint16_t		i2c_clock_sel;		//0:25kHz, 1:100kHz, 2:400kHz, 3:1MHz
	uint16_t		reg_addr_size;		//1:1byte, 2:2byte
	uint16_t		data_size;			//1:1byte, 2:2byte
	uint16_t		dev_addr;
	uint16_t		reg_addr[6];
	uint16_t		data_min[6];
	uint16_t		data_max[6];
} __PACKED__ i2c_test_t;

typedef struct
{
	uint32_t		ver;

	uint16_t		mode;							// Bit[3:0]:0-single,1-dual,2-quad,3-octa,4-hexa,5-32lane(8x4),6-62lane,7-4lane_x_4,8-32lane(16x2)
													// Bit[5:4]:0-6bit,1-8bit,2-10bit
													// Bit[9:8]:0-jeida,1-vesa
													// Bit[13:12]:0-normal,1-2divide,2-4divide
	uint32_t		freq;
	uint32_t		h_total;
	uint32_t		h_active;
	uint32_t		h_bpo;
	uint32_t		h_width;
	uint32_t		v_total;
	uint32_t		v_active;
	uint32_t		v_bpo;
	uint32_t		v_width;
	uint16_t		pol;							// b0:HS, b1:VS, b2:DE

	uint8_t			sync;							// 0~7 : clock delay
	uint8_t			if_type;						// 0:LVDS, 1:Vx1, 2:HDMI
	uint8_t			rgb_set;						// 0:RGB-Hor, 1:RGB-Ver, 2:BGR-Hor, 3:BGR-Ver
	uint8_t			disp_mode;						// Low  - 0:Normal, 1:HV-flip, 2:V-flip, 3:H-flip

	uint16_t		vdd;							// 12.00V -> 1200
	uint16_t		vdd_h;
	uint16_t		vdd_l;
	uint16_t		idd_h;							// 0.6A -> 600
	uint16_t		idd_l;
	uint16_t		vbl;							// 24.00V -> 2400
	uint16_t		vbl_h;
	uint16_t		vbl_l;
	uint16_t		ibl_h;
	uint16_t		ibl_l;

	uint16_t		seq;							// 0:v-s-b, 1:v-b-s, 2:s-v-b, 3:s-b-v, 4:b-v-s, 5:b-s-v
	uint8_t			on_seq[MAX_SEQ_CNT];			// 0:vdd, 1:signal, 2:vbl, 3:invon
	uint8_t			off_seq[MAX_SEQ_CNT];			// 0:vdd, 1:signal, 2:vbl, 3:invon
	uint16_t		on_delay[MAX_SEQ_CNT];
	uint16_t		off_delay[MAX_SEQ_CNT];

	uint16_t		ins_type;						// 0:ccfl, 1:common, 2:nbpc, 3:led
	uint8_t			dim[MAX_INS_CNT];				// 0:analog_vbr, 1:pwm_vbr, 2:ext_vbr
	uint8_t			vbr[MAX_INS_CNT];				// 0 ~ 33(0~3.3V)
	uint16_t		pwm_freq[MAX_INS_CNT];			// 120 ~ 960
	uint8_t			pwm_duty[MAX_INS_CNT];			// 0:0%, ..., 10:100%

	uint8_t			use_cycle;
	cycle_test_t	cycle_test[MAX_CYCLETEST_CNT];

	uint8_t			group_name[MAX_GROUP_NAME];		// UnUse
	uint16_t		port;							// 0:1234, 1:2413, 2:1324, 3:3142, 4:4321, 5:3412, 6:2143
	uint16_t		use_dvi;
	uint32_t		freq_high;						// 2018.02.14(high 32bit freq)
	uint16_t		use_8k_hg2d;					// 8K hg2d model enable
	i2c_test_t		tcon_i2c_test[2];				// 19-08-01	add i2c test
	uint16_t		use_spc;						// 0:Disable,	1:SPC,	2:OTC

	uint16_t		FABIP;
	uint16_t		FABAddress;
	uint16_t		tcon_model;

	uint16_t		VBL_EN_timeout;					//timeout for model which use extra VBL enable signal
//	uint16_t		DP_Port_sel;					//choose the number of DP input port 0: 1port, 1: 2port dp edid

	uint16_t		Corrention_Type;				// 0 : 1chip, 1 : 2chip
	uint16_t		Corrention_Index;
	uint16_t		PSIPCtrl_Index;
	uint16_t		linear_value;
	uint16_t		UPE_bit_flag;					// 0 : 12Bit, 1 : 11Bit, 2 : 10Bit, 3 : 9Bit, 4 : 8Bit
	uint16_t		reserved[197];
} __PACKED__ model_data_t;

typedef struct {
	uint8_t   		Header[8];                        // EDID header "00 FF FF FF FF FF FF 00"
	uint16_t  		ManufactureName;                  // EISA 3-character ID
	uint16_t  		ProductCode;                      // Vendor assigned code
	uint32_t  		SerialNumber;                     // 32-bit serial number
	uint8_t   		WeekOfManufacture;                // Week number
	uint8_t   		YearOfManufacture;                // Year
	uint8_t   		EdidVersion;                      // EDID Structure Version
	uint8_t   		EdidRevision;                     // EDID Structure Revision
	uint8_t   		VideoInputDefinition;
	uint8_t   		MaxHorizontalImageSize;           // cm
	uint8_t   		MaxVerticalImageSize;             // cm
	uint8_t   		DisplayTransferCharacteristic;
	uint8_t   		FeatureSupport;
	uint8_t   		RedGreenLowBits;                  // Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
	uint8_t   		BlueWhiteLowBits;                 // Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
	uint8_t   		RedX;                             // Red-x Bits 9 - 2
	uint8_t   		RedY;                             // Red-y Bits 9 - 2
	uint8_t   		GreenX;                           // Green-x Bits 9 - 2
	uint8_t   		GreenY;                           // Green-y Bits 9 - 2
	uint8_t   		BlueX;                            // Blue-x Bits 9 - 2
	uint8_t   		BlueY;                            // Blue-y Bits 9 - 2
	uint8_t   		WhiteX;                           // White-x Bits 9 - 2
	uint8_t   		WhiteY;                           // White-x Bits 9 - 2
	uint8_t   		EstablishedTimings[3];
	uint8_t   		StandardTimingIdentification[16];
	uint8_t   		DetailedTimingDescriptions[72];
	uint8_t   		ExtensionFlag;                    // Number of (optional) 128-byte EDID extension blocks to follow
	uint8_t   		Checksum;
} edid_data_t;

typedef struct {
	edid_data_t 	data;
	unsigned char   ext[128];
} EDID_BLOCK_256;

typedef struct
{
	uint16_t 		use;
	uint16_t 		index;
	uint16_t 		type;
	pattern_head_t 	indirect;
} __PACKED__ PRELOAD_T;

typedef struct
{
	uint32_t		freq;
	uint32_t		h_total;
	uint32_t		h_active;
	uint32_t		h_bpo;
	uint32_t		h_width;
	uint32_t		v_total;
	uint32_t		v_active;
	uint32_t		v_bpo;
	uint32_t		v_width;
} timing_data_t;

typedef struct
{
	uint64_t		freq;
	uint32_t		h_total;
	uint32_t		h_active;
	uint32_t		h_bpo;
	uint32_t		h_width;
	uint32_t		v_total;
	uint32_t		v_active;
	uint32_t		v_bpo;
	uint32_t		v_width;
} fpga_timing_data_t;

typedef struct
{
	uint32_t 		t;
	uint16_t 		ha;
	uint16_t 		va;
	uint16_t 		hblank;
	uint16_t 		vblank;
	uint16_t 		hsync;
	uint16_t 		vsync;
	uint16_t 		hfpo;
	uint16_t 		vfpo;
} edid_timing_t;

// variables
model_data_t		model_data, old_model_data;
enum_mode_t			mode_data;
char				model_name[MAX_MODEL_NAME];			// current model name
char				old_model_name[MAX_MODEL_NAME];
char				model_list[MAX_MODEL_CNT][MAX_MODEL_NAME];
int					model_cnt, model_idx, model_comp_res, model_init_status, model_selection_end, use_hando, quhd_copy_mode, hando_model_idx;
PRELOAD_T			preload_data[MAX_PAT_CNT];
hando_list_t		hando_list[MAX_HANDO_CNT];

/***************************************************/
// QUHD REFERENCE
//--------------------------------------------------|
// model_data	| tegra_time		| fpga_time		|
// dc_buffer	| dc_buffer_quhd	|				|
//--------------------------------------------------|
// 7680			| 1920(H/4)			| 1920(H/4)		|
// 4320			| 2160(V/2)			| 4320			|
/***************************************************/
timing_data_t		tegra_time;
fpga_timing_data_t	fpga_time;
dc_t				*dc_buffer;
dc_t				*dc_output;//3840x2160
dc_t				*dc_buffer_quhd;	// for QUHD(8K)
dc_t				*dc_buffer_preload[MAX_MEMORY_PRELOAD_CNT];	// for HDMI
dc_t         		*dc_ai1_buffer;
dc_t         		*dc_ai2_buffer;

XVisualInfo 		*visInfo;
Display 			*xdisp;
Window 				window;
GLXContext 			glxContext;

// Functions
extern void 		model_var_init(void);
extern void			model_init(void);
extern void 		model_update(void);
extern int 			model_select(char* name, int flag);
extern void 		change_model_set(void);
extern int 			screen_init(int screen_sel);
extern int 			screen_close(void);
extern int 			get_divider(void);
extern void 		set_portmap(int port);
extern void 		set_mode_by_twist(enum_twist_t twist);
extern void 		set_mode_bit_type_qems(int bittype);
extern void 		set_curmodel_idx(char *name);
extern int 			get_model_init_status(void);
extern void 		set_model_init_status(int status);
extern int 			get_hando_list(void);
extern int 			is_5k_lvds_quad(void);

extern int			draw_fail_check(void);
extern int			memory_preload(void);
extern int			quhd_memory_preload(void);

extern void			ext_cnt_vdd_sel(uint16_t sel);
extern void 		dp_port_sel(int flag);

extern int			is_over_4k_less_8k(void);
extern int			is_4k_8k_hdmi_or_not(void);

#endif /* MODEL_DATA_H_ */
