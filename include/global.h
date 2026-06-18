/*
 * global.h
 *
 *  Created on: Sep 13, 2017
 *      Author: root
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <termios.h>
#include <omp.h>
#include <inttypes.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
#include "nvgstplayer.h"

//shared memory
#include <sys/shm.h>



#ifdef DEBUG
	#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt,__FILE__, __LINE__, __func__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

#define __PACKED__ 			__attribute__((packed))

#define ERRCHECK(x)	\
		if(x<0)		\
		{			\
			perror("ERROR CHECK\n");\
		}

#define Aprintf	if(APRF) printf

// Usage
//#define USE_I2C_LCD
#define DRAW_FAIL_CHECK
#define USE_SDC 				//define:SDC, undefine:SSM
#define USE_PRE_EMPHASIS
#define USE_SHARED_MEM
//#define USE_SUB_BOARD_ID

#define USE_8K_LOGIC_PATTERN		//test code
#define USE_NEW_WR_BANK				//test code


//#define USE_RCB485

// Shared memory define
#define SHM_KEY 	3111
#define NONE_DATA	-1
#define UPDATE_CUR_MODEL	1
#define UPDATE_CUR_GROUP	1
#define UPDATE_CUR_PATTERN	1
#define UPDATE_FW_VERSION	1




// System Define
//#define FW_VERSION				38		// 3.8 2023.06.
//#define FW_VERSION				39		// 3.9 2023.08.07 test
#define FW_VERSION				73
#define PG_NAME					"ES628"

// Directory Define
#define DIR_ROOT				"/root/hdd"
#define DIR_BOOT				"/boot"
#define DIR_CONFIG				"/config"
#define DIR_MODEL				"/model"
#define DIR_GROUP				"/group"
#define DIR_PATTERN				"/pattern"
#define DIR_BMP					"/bmp"
#define DIR_MOV					"/mov"
#define DIR_FW					"/bin"
#define DIR_FPGA				"/fpga"
#define DIR_FONT				"/font"
#define DIR_ETC					"/etc"
#define DIR_SCHEDULE			"/schedule"
#define DIR_HANDO				"/hando"
#define DIR_PNG					"/png"
#define DIR_JPG					"/jpg"

// File Define
#define	DEV_MEM					"/dev/mem"
#define NET_INTERFACE_FILE		"/etc/network/interfaces"
#define WPA_CONF_TEMP_FILE		"/root/wpa_supplicant_temp.conf"
#define WPA_CONF_FILE			"/root/wpa_supplicant.conf"
#define CURRENT_MODEL_NAME 		"curmodel.txt"
#define CURRENT_GROUP_NAME 		"curgroup.txt"
#define SERVER_IP_FILE			"serverip.txt"
#define QEMS_INFO_FILE			"qemsinfo.txt"
#define QEMS_8K_INFO_FILE		"qems8kinfo.txt"
#define WIFI_INFO_FILE			"wifiinfo.txt"
#define GROUP_ID_FILE			"groupid.txt"
#define PWR_SEL_FILE			"pwrsel.txt"
#define FONT_FILE_NAME			"font.otf"
#define KERNEL_IMAGE_NAME		"Image"
#define KERNEL_ZIMAGE_NAME		"zImage"
#define PWR_FW_FILE				"ep620p.bin"
#define DEF_SERVER_IP			"12.92.205.50"
#define DEF_PREFIX_IP			"192.167."
#define DEF_SUBNET_MASK			"255.255.0.0"
#define DEF_GATEWAY				"192.167.0.1"
#define DEF_WIFI_IP				"12.92.162.100"
#define DEF_WIFI_NETMASK		"255.255.254.0"
#define DEF_WIFI_GATEWAY		"12.92.162.1"
#define DEF_WIFI_SSID			"TJWL_RLT"
#define DEF_WIFI_IDENTITY		"12408280"
#define DEF_WIFI_PASSWORD		"sy092312!@"
#define TX1_DTB_NAME 			"/boot/tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb"//"/root/test.dtb" //tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb
#define ERR_STRING				"ERROR"
#define STR_HANDO0				"Hando_"
#define STR_HANDO1				"hando_"
#define STR_HANDO2				"HANDO_"
#define DEF_IP_CLASS0			192
#define DEF_IP_CLASS1			167
#define DEF_IP_CLASS2			1
#define DEF_IP_CLASS3			10
#define DEF_GROUP_ID			0
#define DEF_PWR_SEL				0
#define DEF_WIFI_SEL			0
#define DEF_QEMS_EQPID			"E_PG000"
#define DEF_VIDEO_PLAYER		"gst-launch-1.0"

//ES628
#define	DEF_SCPI_PWR1_IP		"169.254.1.1"
#define	DEF_SCPI_PWR2_IP		"169.254.1.2"
#define	DEF_SCPI_PWR_NETMASK	"255.255.0.0"
#define POWER_IP_FILE			"powerip.txt"



// Extention Define
#define EXT_MODEL				".amf"
#define EXT_MODEL2				".Amf"
#define EXT_MODEL3				".aMf"
#define EXT_MODEL4				".amF"
#define EXT_MODEL5				".AMf"
#define EXT_MODEL6				".aMF"
#define EXT_MODEL7				".AmF"
#define EXT_MODEL8				".AMF"
#define EXT_GROUP				".grp"
#define EXT_GROUP2				".Grp"
#define EXT_GROUP3				".gRp"
#define EXT_GROUP4				".grP"
#define EXT_GROUP5				".GRp"
#define EXT_GROUP6				".gRP"
#define EXT_GROUP7				".GrP"
#define EXT_GROUP8				".GRP"
#define EXT_PATTERN				".pat"
#define EXT_PATTERN2			".Pat"
#define EXT_PATTERN3			".pAt"
#define EXT_PATTERN4			".paT"
#define EXT_PATTERN5			".PAt"
#define EXT_PATTERN6			".pAT"
#define EXT_PATTERN7			".PaT"
#define EXT_PATTERN8			".PAT"
#define EXT_BMP					".bmp"
#define EXT_BMP2				".Bmp"
#define EXT_BMP3				".bMp"
#define EXT_BMP4				".bmP"
#define EXT_BMP5				".BMp"
#define EXT_BMP6				".bMP"
#define EXT_BMP7				".BmP"
#define EXT_BMP8				".BMP"
#define EXT_PNG					".png"
#define EXT_PNG2				".Png"
#define EXT_PNG3				".pNg"
#define EXT_PNG4				".pnG"
#define EXT_PNG5				".PNg"
#define EXT_PNG6				".pNG"
#define EXT_PNG7				".PnG"
#define EXT_PNG8				".PNG"
#define EXT_JPG					".jpg"
#define EXT_JPG2				".Jpg"
#define EXT_JPG3				".jPg"
#define EXT_JPG4				".jpG"
#define EXT_JPG5				".JPg"
#define EXT_JPG6				".jPG"
#define EXT_JPG7				".JpG"
#define EXT_JPG8				".JPG"
#define EXT_FW					".bin"	// unused(F/W file does not have a extension)
#define EXT_FW2					".Bin"
#define EXT_FW3					".bIn"
#define EXT_FW4					".biN"
#define EXT_FW5					".BIn"
#define EXT_FW6					".bIN"
#define EXT_FW7					".BiN"
#define EXT_FW8					".BIN"
#define EXT_FPGA				".rbf"
#define EXT_FPGA2				".Rbf"
#define EXT_FPGA3				".rBf"
#define EXT_FPGA4				".rbF"
#define EXT_FPGA5				".RBf"
#define EXT_FPGA6				".rBF"
#define EXT_FPGA7				".RbF"
#define EXT_FPGA8				".RBF"
#define EXT_SCHEDULE			".sch"
#define EXT_SCHEDULE2			".Sch"
#define EXT_SCHEDULE3			".sCh"
#define EXT_SCHEDULE4			".scH"
#define EXT_SCHEDULE5			".SCh"
#define EXT_SCHEDULE6			".sCH"
#define EXT_SCHEDULE7			".ScH"
#define EXT_SCHEDULE8			".SCH"
#define EXT_HANDO				".hnd"
#define EXT_HANDO2				".Hnd"
#define EXT_HANDO3				".hNd"
#define EXT_HANDO4				".hnD"
#define EXT_HANDO5				".HNd"
#define EXT_HANDO6				".hND"
#define EXT_HANDO7				".HnD"
#define EXT_HANDO8				".HND"
#define EXT_BIG_PATTERN			".PAT"
#define EXT_BIG_BMP				".BMP"
#define EXT_BIG_PNG				".PNG"
#define EXT_BIG_JPG				".JPG"
#define EXT_BIG_HANDO			".HND"


// Size Define
#define	MAX_MODEL_NAME			128
#define	MAX_CYCLE_NAME			128
#define	MAX_GROUP_NAME			128
#define MAX_PAT_NAME			128
#define MAX_FW_NAME				64
#define MAX_PRODUCT_NAME		32
#define MAX_PAT_CNT				256
#define MAX_BMP_CNT				1024
#define MAX_CMD_LEN				260
#define MAX_PATH				260
#define MAX_EXT					16
#define MAX_TASK_CNT 			20
#define	MAX_PROC_CNT			10
#define MAX_DEV_NAME			20
#define TIME_MULTI_VALUE		100
#define MAX_IP_ADDR				20
#define MAX_QEMS_EQPID			16
#define MAX_WIFI_SSID			16
#define MAX_WIFI_PASSWORD		16
#define MAX_WIFI_IDENTITY		16
#define ENSIS_PWR_CH			4
#define MAX_PSEQ_CNT			7
#define MAX_PG_NAME				32

// Serial Define
#define RS_NONE_PARITY      	0
#define RS_ODD_PARITY       	(PARODD|PARENB)
#define RS_EVEN_PARITY      	PARENB
#define RS_1_STOP_BIT       	0										// 1 stop bit
#define RS_2_STOP_BIT       	CSTOPB									// 2 stop bit

#define	STORAGE_LIMIT			95

/*
#define DEF_TIME_OUT		1000
#define	MAX_PROC_CNT		10
#define	MAX_HEADER_SIZE		4
#define	MAX_MODEL_NAME		128
#define	MAX_PATTERN_NAME	128
#define	MAX_GROUP_NAME		128
*/

#define RES_ID(x)				(x+0x1000)

#define I2C_TEST				1

// Enumerations
typedef enum {
	// Request Command --------------
	SID_CONN_REQ			= 0x1001,
	SID_VER_REQ,
	SID_MODEL_SEL_REQ,
	SID_GROUP_SEL_REQ,
	SID_LCD_ONOFF_REQ,
	SID_PAT_DISP_REQ,
	SID_STATUS_REQ,
	SID_FREQ_SET_REQ,
	SID_VSYNC_SET_REQ,
	SID_POS_SET_REQ,
	SID_FILE_DOWN_REQ,
	SID_COLOR_SET_REQ,
	SID_RUN_TYPE_REQ,
	SID_REBOOT_REQ,
	SID_PWR_FWDN_REQ,
	SID_VDD_SET_REQ,
	SID_ADIM_SET_REQ,
	SID_PDIM_SET_REQ,
	SID_BMP_DELETE_REQ,
	SID_FILE_LIST_REQ,
	SID_FILE_DELETE_REQ,
	SID_STORAGE_SIZE_GET_REQ,
	SID_COMMAND_REQ,
	SID_PWR_OFFSET_REQ,
	SID_QEMS_REQ,
	SID_CYCLE_REQ,
	SID_CHANNEL_SHIFT_SET_REQ,
	SID_BIT_TYPE_SET_REQ,
	SID_VBL_SET_REQ,
	SID_BPORCH_SET_REQ,					//0x101e
	SID_FPORCH_SET_REQ,					//0x101f
	SID_WIDTH_SET_REQ,					//0x1020
	SID_INST_IND_PAT_ONOFF_REQ,			//0x1021
	SID_INST_IND_PAT_COLOR_SET_REQ,		//0x1022

	//es628: Standard set
	SID_VRR_TEST_REQ,					//0x1023
	SID_STD_IP_PWR_ONOFF_REQ,			//0x1024		Keysight MPS
	SID_STD_IP_PWR_ADDR_SET_REQ,		//0x1025		Keysight MPS
	SID_STD_VBY1_CHARA_SET_REQ,			//0x1026
	SID_STD_SSCG_SET_REQ,				//0x1027
	SID_STD_PWR_SEQ_SET_REQ,			//0x1028
	SID_STD_ROLLING_SET_REQ,			//0x1029
	SID_STD_I2C_WR_TEST_REQ,			//0x102a
	SID_STD_ACDET_SET_REQ,				//0x102b
	SID_STD_PWR_SEQ_ONOFF_REQ,			//0x102c
	SID_STD_I2C_RD_TEST_REQ,			//0x102d
	SID_EXCO_CMD_LIST_SET_REQ,			//0x102e
	SID_EXCO_CMD_LIST_EXEC_REQ,			//0x102f
	SID_EXCO_CMD_FILE_CREATE_REQ,		//0x1030

	SID_STD_ACTIVE_SET_REQ = 0x1031,	//0x1031
	SID_EXCO_CMD_WR_REQ,				//0x1032
	SID_AMT_INFO_REQ,					//0x1033
	SID_SCHEDULE_SEL_REQ,				//0x1034
	SID_CYCLE_CHANGE_REQ,				//0x1035
	SID_EXCO_SENDING_REQ,				//0x1036
	SID_EXCO_CYC_INFO_REQ,				//0x1037
	SID_FILE_UPLOAD_REQ,				//0x1038
	SID_EXCO_LOG_REQ,					//0x1039
	SID_CTL_WRITE_REQ = 0x103a,			//0x103A
	SID_CTL_READ_REQ,					//0x103B
	SID_CTL_ENABLE_REQ,					//0x103C
	SID_QEMS_ID_REQ,					//0x103D
	SID_PC_TIME_SYNC_REQ,				//0x103E
	SID_REALTIME_OVERLAY_REQ,			//0x103F
	SID_OVERLAY_CLEAR_REQ,				//0x1040
	SID_DISPLAY_RECTANGLE_REQ,			//0x1041
	SID_OVERLAY_BACKGROUND_REQ,			//0x1042
	SID_CHANGE_OVERLAY_MODE_REQ,		//0x1043
	SID_OPERATE_CA410_REQ,				//0x1044
	SID_DP_TIMING_REQ,					//0x1045
	SID_DEFECT_VIEW_NAME_PC_REQ,		//0x1046
	SID_DEFECT_VIEW_MODE_PC_REQ,		//0x1047
	SID_STD_OCMD_WR_TEST_REQ,			//0x1048
	SID_STD_OCMD_RD_TEST_REQ,			//0x1049
	SID_STD_OCMD_PWR_SEQ_SET_REQ,		//0X104a
	SID_STD_OCMD_PWR_SEQ_ONOFF_REQ,		//0x104b
	SID_CYCLE_CANCEL_REQ,				//0x104c
	SID_PWR_SEQ_STATUS_REQ,				//0x104d
	SID_PCC_LOG_DOWN_REQ,				//0x104e
	SID_STD_AUX_I2C_PWR_SEQ_TEST_REQ,	//0x104f
	SID_STD_AUX_OCMD_PWR_SEQ_TEST_REQ,	//0x1050
	SID_STD_AUX_TEST_WR_REQ,			//0x1051
	SID_STD_AUX_TEST_RD_REQ,			//0x1052
	SID_STD_EDP_CHARA_WR_REQ,			//0x1053
	SID_STD_EDP_CHARA_RD_REQ,			//0x1054

	//DMPG
	SID_CNT_I2C_SEL_REQ		= 0x1100,	//0x1100
	SID_CNT_VCC_CTRL_REQ,				//0x1101
	SID_IND_COLOR_SET_REQ,				//0x1102
	SID_VRR_REQ,						//0x1103
	SID_PRE_EMPHASIS_REQ,				//0x1104
	SID_COLOR_CURSOR_REQ,				//0x1105
	SID_RGB_DISABLE_REQ,				//0x1106
	SID_GRAY_SCALE_SET_REQ,				//0x1107
	SID_GRAY_SCALE_GET_REQ,				//0x1108
	SID_SCROLL_SET_REQ,					//0x1109
	SID_GAMMA_SET_REQ,					//0x110a



	//Ethernet Power Controller
	SID_PWR_CTRL_IP_SET_REQ	= 0x6201,
	SID_PWR_CTRL_ONOFF_REQ,
	SID_PWR_CTRL_VOL_CUR_SET_REQ,
	SID_PWR_CTRL_SLEW_REQ,
	SID_PWR_CTRL_STATUS_REQ

	// Response Command --------------
	// RES_ID(x)
} enum_sigid_t;

typedef enum
{
	NACK              		= 0,
	ACK						= 1,
	DISCONNECT
} enum_resp_t;

typedef enum
{
	ERR_MODEL_NOT_FOUND		= 2,
	ERR_CURMODEL_FAIL,
	ERR_FPGA_FAIL,
	ERR_POWER_FAIL,
	ERR_LIMIT_SET_FAIL,
	ERR_XRANDR_FAIL,
	ERR_DRAW_FAIL,
	ERR_FILE_ERROR
} enum_init_err_t;

// FPGA DRAW CHECK
typedef enum
{
	DRAW_PATTERN_CHECK		= 0x0040,
	DRAW_BMP_CHECK			= 0x0080
} enum_draw_check_t;

typedef enum
{
	INIT=0,
	MENU,
	SERVER_IP,
	GROUP_ID,
	PWR_SELECT,
	MODEL_CHANGE,
	READY,
	READY_FUNC,
	AUTO_RUN,
	MANU_RUN,
	FREQUENCY,
	CURSOR,
	GRAY_CHANGE,
	SCROLL,
	MODEL_SELECT,
	GRAY_SCALE,
	FUNC_MENU,
	HTIME_CHANGE,
	VTIME_CHANGE,
	CH_SHIFT,
	BIT_CHANGE,
	DIMM_CHANGE,
	I2C_CTRL,
	VBY1_EQUAL,
	GROUP_CHANGE,
	VDD_CHANGE,
	PRE_EMPHASIS,
	PWM_CHANGE,
	MODEL_DELETE,
	GROUP_DELETE,
	PATTERN_DELETE,
	BITMAP_DELETE,
	USB_DETECTED,
	NETWORK_INFO,
	QEMS_EQPID,
	WIFI_SELECT,
	WIFI_IP,
	WIFI_NETMASK,
	WIFI_GATEWAY,
	WIFI_SSID,
	WIFI_IDENTITY,
	WIFI_PASSWORD,
	SPC_SET,
	PWR_FW_UPLOAD,
	QEMS_MASTER_EQPID,
	VBL_CHANGE,
	STORAGE_INFO,
	CURSOR_COLOR,

	// ELVDD Alarm
	VBL_CHECK_WARN,
	VBL_CHECK_INFO

} enum_opmode_t;

typedef enum{
	RGB_HORZ = 0,
	RGB_VERT,
	BGR_HORZ,
	BGR_VERT,
} enum_rgb_t;

typedef enum
{
	NV_CPU=0,
	FPGA,
} enum_draw_select_t;

typedef enum
{
	ON_CYCLE = 0,
	OFF_CYCLE
} enum_cycle_t;

// Enumerations
typedef enum
{
	ID_PWR1			= 0,
	ID_PWR2
} enum_pwrid_t;

//typedef enum
//{
//	ID_PWR1			= 0,
//	ID_PWR2
//} enum_pwrid_t;

// Structures
// Request Structure ----------------------------------------------------------------
typedef struct
{
	uint16_t				cmd_id;
	uint16_t				data_len;
} __PACKED__ req_head_t;

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_ver_t;

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_qems_t;

typedef struct
{
	req_head_t				hdr;
	char					model_name[MAX_MODEL_NAME];
	uint16_t				use_hando;	// 0:none, 1:use
} __PACKED__ req_model_sel_t;

typedef struct
{
	req_head_t				hdr;
	char					cycle_name[MAX_CYCLE_NAME];
} __PACKED__ req_cycle_sel_t;

typedef struct
{
	req_head_t				hdr;
	char					group_name[MAX_GROUP_NAME];
} __PACKED__ req_group_sel_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				on_off;
	uint16_t				pat_num;
	uint16_t				type;	// 0:auto, 1:manual
} __PACKED__ req_lcd_onoff_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;
	uint16_t				pat_num;
	uint16_t				vdd;			//12.0V -> 1200
	uint16_t				vsync;			//60Hz -> 60
	uint16_t				scroll_dir;		//0:none, 2:top, 3:bottom, 4:left, 5:right
	uint16_t				scroll_speed;	//1~99
	uint32_t				amt_index[4];// ???
} __PACKED__ req_pat_disp_t;

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_status_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				channel_shift;	// 0:1234, 1:2413, 2:1324, 3:3142, 4:4321, 5:3412, 6:2143
} __PACKED__ req_channel_shift_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;	// 0:diable, 1:enable
	uint64_t				freq;
} __PACKED__ req_freq_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;	// 0:diable, 1:enable
	uint16_t				vsync;	// 60Hz -> 60
} __PACKED__ req_vsync_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				vdd;	// 12.0V -> 1200
} __PACKED__ req_vdd_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				vbl;	// 24.0V -> 2400
} __PACKED__ req_vbl_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				adim;	// 3.3V -> 33
} __PACKED__ req_adim_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				duty;	// 0~100%
} __PACKED__ req_pdim_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;	// 0:diable, 1:enable
	uint16_t				red;
	uint16_t				green;
	uint16_t				blue;
} __PACKED__ req_color_set_t;

typedef struct
{
	req_head_t				hdr;
	uint8_t					type;	// 0:diable, 1:enable
	uint8_t					dir;
	uint16_t				x;		//0~(max-1)
	uint16_t				y;
	uint16_t				red;	//0~4095
	uint16_t				green;	//0~4095
	uint16_t				blue;	//0~4095
} __PACKED__ req_pos_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;	// 0:auto, 1:manual
} __PACKED__ req_run_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;	//bit type:normal 8bit(8),normal 10bit(10),jeida 8bit(12),jeida 10bit(14),Vesa 8bit(16),vesa 10bit(18)
} __PACKED__ req_bit_type_set_t;

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_pwr_fw_t;


typedef struct
{
	req_head_t				hdr;
	uint16_t				channel;	//0:ch0, 1:ch1, 2:ch2, 3:ch3
} __PACKED__ req_ext_i2c_sel_t;			//External i2c channel select

typedef struct
{
	req_head_t				hdr;
	uint16_t				on_off;		//0:off, 1:on
	uint16_t				type;		//bit0:vdd, bit1:vbl, bit2:sig, bit3:bl_on
} __PACKED__ req_onoff_ctrl_t;			//Direct power sequence control

typedef struct
{
	req_head_t				hdr;
	uint16_t				pal;	//0~2
	uint16_t				red;	//0~4095
	uint16_t				green;	//0~4095
	uint16_t				blue;	//0~4095
} __PACKED__ req_ind_color_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				v_total;
} __PACKED__ req_vrr_t;		//For variable refresh rate test

typedef struct
{
	req_head_t				hdr;
	int8_t					vod;		//1~31
	int8_t					post_1st;	//-25~25
	int8_t					post_2nd;	//-12~12
	int8_t					pre_1st;	//-16~16
	int8_t					pre_2nd;	//-7~7
} __PACKED__ req_pre_emphasis_t;

typedef struct
{
	req_head_t				hdr;
	uint8_t					type;	// 0:disable, 1:enable
	uint8_t					dir;
	uint16_t				x;
	uint16_t				y;
	uint16_t				red;
	uint16_t				green;
	uint16_t				blue;
} __PACKED__ req_color_cursor_t;

typedef struct
{
	req_head_t				hdr;
	uint32_t				disable;	//0x01:red, 0x02:green, 0x04:blue
} __PACKED__ req_rgb_disable_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				level;	//8,16,32,64,128,256,512,1024
} __PACKED__ req_gray_scale_set_t;

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_gray_scale_get_t;


typedef struct
{
	req_head_t				hdr;
	uint16_t				type;
	uint16_t				direction;		//0:none, 2:top, 3:bottom, 4:left, 5:right
	uint16_t				speed;			//1~99
} __PACKED__ req_pattern_scroll_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;		//0:H, 1:V
	uint16_t				bpo;		//0~(65535-width-active-fporch)
} __PACKED__ req_bpo_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;		//0:H, 1:V
	uint16_t				fpo;		//0~(65535-width-bporch-active)
} __PACKED__ req_fpo_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;		//0:H, 1:V
	uint16_t				width;		//0~(65535-bporch-active-fporch)
} __PACKED__ req_width_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;		//0:H, 1:V
	uint16_t				active;		//1~(total-bporch-fporch-width > active)
} __PACKED__ req_active_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				on_off;		//0:off, 1:on
	uint16_t				start_h;
	uint16_t				start_v;
	uint16_t				width;
	uint16_t				height;
} __PACKED__ req_inst_ind_pat_onoff_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				pal;		//0:bg, 1:fg
	uint16_t				red;		//0~4095
	uint16_t				green;		//0~4095
	uint16_t				blue;		//0~4095
} __PACKED__ req_inst_ind_pat_color_t;

typedef struct
{
	req_head_t				hdr;
	int16_t					red;
	int16_t					green;
	int16_t					blue;
} __PACKED__ req_gamma_set_t;


//es628
typedef struct
{
	req_head_t				hdr;
	uint16_t				onoff;			// 0:off, 1:on
	uint16_t				step;			// 2~10
	uint16_t				vtotal[10];		// (v_active + v_width + v_bporch + 1) ~ 65535
	uint16_t				frame_num[10];
	uint16_t				vtotal_ex[190];		// (v_active + v_width + v_bporch + 1) ~ 65535
	uint16_t				frame_num_ex[190];
	uint16_t				red[200];
	uint16_t				green[200];
	uint16_t				blue[200];
} __PACKED__ req_vrr_test_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				onoff;			// 0:off, 1:on
	uint16_t				pwr_id;			// 0:power-1, 1:power-2
	uint16_t				pwr_ch;			// 0~3:ch1~ch4, 4:all
	uint16_t				voltage;		// 12.00V -> 1200
	uint16_t				current;		// 10.00A -> 1000
} __PACKED__ req_ip_pwr_onoff_t;

typedef struct
{
	req_head_t				hdr;
	int8_t					pwr1_addr[16];	// ex) 169.254.67.0
	int8_t					pwr2_addr[16];
} __PACKED__ req_ip_pwr_addr_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				ch;				// 0~63:ch1~ch64,64:all
	uint16_t				type;			// 0:vod, 1:pre-amp, 2:skew
	uint32_t				value;			// 807.34 -> 80734
} __PACKED__ req_std_vby1_chara_set_t;

typedef struct
{
	uint16_t				ch;
	uint16_t				type[10];		//0:vod, 1:pre-emphasis, 2:eq-mode, 3:rx-gain, 4:eq dc-gain, 5:eq-setting, 6:tx-gain
	uint32_t				value[10];
} __PACKED__ edp_chara_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				res;
	edp_chara_set_t			edp_sig_chara[16];

} __PACKED__ req_std_edp_chara_set_read_t;

typedef struct
{
	req_head_t				hdr;
	edp_chara_set_t			edp_sig_chara[16];

} __PACKED__ req_std_edp_chara_set_write_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				ratio;			//0.01%~2.50% -> 1~250
	uint16_t				mf;				//KHz, 30.310 -> 30310
} __PACKED__ req_std_sscg_test_set_t;

typedef struct
{
	uint32_t				t1;			//Power1 Rising time 10% to 90%
	uint32_t				t2;			//Vby1 signal on delay after T1
	uint32_t				t5;			//Power2 On delay after T2
	uint32_t				tb1;		//Power2 Rising time 10% to 90%
	uint32_t				ton;		//Power2 On Time after Tb1
	uint32_t				tb2;		//Power2 Falling time 90% to 10%
	uint32_t				t6;			//Vby1 signal off delay after Tb2
	uint32_t				t3;			//Power1 Off delay after T6
	uint32_t				tf;			//Power1 Falling time 90% to 10%
	uint32_t				t4;			//Power1 Off time after Tf
	uint32_t				tlds;		//Vby1 signal on time
	uint32_t				tldo;		//Vby1 signal off time( signal on - off - on - off ...)
//	uint32_t				t_i2c_on;	//I2C on set delay after T1
//	uint32_t				t_i2c_off;	//I2C off set delay before Tf

	/* 2022.04.19 ksk add I2C2 func */
	uint32_t				t_i2c_on_1;	//I2C1 on set delay after T1
	uint32_t				t_i2c_off_1;//I2C1 off set delay before I2C2 off
	uint32_t				t_i2c_on_2;	//I2C2 on set delay after I2C1 on
	uint32_t				t_i2c_off_2;//I2C2 off set delay before Tf
	/*		*/

	uint32_t				acdet_on;	//I2C Enable signal(3.3V) on after T1
	uint32_t				acdet_off;	//I2C Enable signal(3.3V) off before Tf
	uint32_t				repeat;		//repeat count
} __PACKED__ std_pwr_seq_timing;


typedef struct
{
	uint32_t				t1;			//Power1 Rising time 10% to 90%
	uint32_t				t2;			//Vby1 signal on delay after T1
	uint32_t				t5;			//Power2 On delay after T2
	uint32_t				tb1;		//Power2 Rising time 10% to 90%
	uint32_t				ton;		//Power2 On Time after Tb1
	uint32_t				tb2;		//Power2 Falling time 90% to 10%
	uint32_t				t6;			//Vby1 signal off delay after Tb2
	uint32_t				t3;			//Power1 Off delay after T6
	uint32_t				tf;			//Power1 Falling time 90% to 10%
	uint32_t				t4;			//Power1 Off time after Tf
	uint32_t				tlds;		//Vby1 signal on time
	uint32_t				tldo;		//Vby1 signal off time( signal on - off - on - off ...)
	uint32_t				t_i2c_on_1;	//I2C on set delay after T1
	uint32_t				t_i2c_off_1;//I2C off set delay before I2C off
	uint32_t				t_i2c_on_2;	//I2C on set delay after I2C1 on
	uint32_t				t_i2c_off_2;//I2C off set delay before Tf
	uint32_t				acdet_on;	//I2C Enable signal(3.3V) on after T1
	uint32_t				acdet_off;	//I2C Enable signal(3.3V) off before Tf
	uint32_t				repeat;		//repeat count

	uint32_t				pwr1_off_time;
	uint32_t				sequence_time;
	uint32_t				signal_on_time;
	uint32_t				signal_off_time;
	uint32_t				pwr2_on_time;
	uint32_t				pwr2_off_time;
	uint32_t				acdet_on_time;
	uint32_t				acdet_off_time;
	uint32_t				i2c_on_time_1;
	uint32_t				i2c_off_time_1;
	uint32_t				i2c_on_time_2;
	uint32_t				i2c_off_time_2;

	unsigned int t3_1;			// 1234.5ms -> 12345
	unsigned int t3_2;			// 1234.5ms -> 12345
	unsigned int t3_3;			// 1234.5ms -> 12345
	unsigned int t3_4;			// 1234.5ms -> 12345
} std_pwr_seq_timing_buf_t;


typedef struct
{
	uint16_t				i2c_clock_sel;	//0:25KHz, 1:100KHz, 2:400KHz, 3:1MHz
	uint16_t				reg_addr_size;	//1:1byte, 2:2bytes, 4:4bytes
	uint16_t				data_size;		//1:1byte, 2:2bytes, 4:4bytes
	uint16_t				dev_addr;		// LSB 1byte
	uint16_t				byte_num;		//0~10
//	uint16_t				enable;			//0: off 1: on
	uint16_t				reg_addr[10];
	uint16_t				reg_data[10];
} __PACKED__ std_pwr_seq_i2c_t;

typedef struct
{
	uint16_t				i2c_clock_sel;	//0:25KHz, 1:100KHz, 2:400KHz, 3:1MHz
	uint16_t				reg_addr_size;	//1:1byte, 2:2byte 4:4byte
	uint16_t				data_size;		//1:1byte, 2:2byte 4:4byte
	uint16_t				dev_addr;		// LSB 1byte
	uint16_t				byte_num;		//0~200
//	uint16_t				enable;			//0: off 1: on
	uint16_t				reg_addr[200];
	uint16_t				reg_data[200];
} __PACKED__ std_i2c_test_wr;

typedef struct //ksk ctl rw
{
	uint16_t				reg_addr[256];
	uint16_t				reg_data[256];
} __PACKED__ std_ctl_rw;

#pragma pack(2) // HAVE TO BE PACKED WITH 2 IF THE CONSTRUCTURE IS NOT DIVIDED WITH 4
typedef struct
{
	req_head_t				hdr;
	uint32_t				sel;			//b0:ch1, b1:ch2, b2:ch3, b3:ch4, b4:ch5, b5:ch6, b6:ch7
	std_pwr_seq_timing		timing_info[MAX_PSEQ_CNT];
	std_pwr_seq_i2c_t			i2c_info[4];	// [0]:on_i2c1, [1]:off_i2c1, [2]:on_i2c2, [3]:off_i2c2
	uint16_t 				onoff; //[0]:i2c_1_on [1]:i2c_1_off [2]:i2c_2_on [3]:i2c_2_off
	uint32_t				scenario;
	uint32_t				rpcnt;
} __PACKED__ req_std_pwr_seq_test_t;

typedef struct //2023.05.25 ksk OCMD
{
	uint16_t				ocmd_clock_sel;	//0:Not using, 1:100KHz, 2:400KHz, 3:700KHz
	uint16_t				data_size;		//1:1byte, 2:2bytes, 4:4bytes
	uint16_t				dev_addr;		// LSB 1byte
	uint16_t				byte_num;		//0~200
//	uint16_t				enable;			//0: off 1: on
	uint16_t				sub_addr[10];	// fixed with 2bytes
	uint16_t				reg_data_h[10];// length depends on the data_size
	uint16_t				reg_data_l[10];
	uint16_t				table_data_h[10];//fixed with 4bytes
	uint16_t				table_data_l[10];
}__PACKED__ std_pwr_seq_ocmd_t;

typedef struct
{
	req_head_t				hdr;
	uint32_t				sel;			//b0:ch1, b1:ch2, b2:ch3, b3:ch4, b4:ch5, b5:ch6, b6:ch7
	std_pwr_seq_timing		timing_info[MAX_PSEQ_CNT];
	std_pwr_seq_ocmd_t		ocmd_info[4];	// [0]:on_i2c1, [1]:off_i2c1, [2]:on_i2c2, [3]:off_i2c2
	uint16_t 				onoff; //[0]:i2c_1_on [1]:i2c_1_off [2]:i2c_2_on [3]:i2c_2_off
	uint32_t				scenario;	//b0:ch1, b1:ch2, b2:ch3, b3:ch4, b4:ch5, b5:ch6, b6:ch7
	uint32_t				rpcnt;
} __PACKED__ req_std_ocmd_pwr_seq_test_t;
#pragma pack()

typedef struct
{
	req_head_t				hdr;
	uint16_t				type;			// 0:pixel_clock, 1:h_freq, 2:h_fporch, 3:h_width, 4:h_bporch, 5:v_freq, 6:v_fporch, 7:v_width, 8:v_bporch
	uint32_t				value;			// 123.45 -> 12345
} __PACKED__ req_std_rolling_set_set_t;



typedef struct //2023.05.25 ksk OCMD
{
	uint16_t				ocmd_clock_sel;	//0:Not using, 1:100KHz, 2:400KHz, 3:700KHz
//	uint16_t				reg_addr_size;	//1:1byte, 2:2bytes, 4:4bytes
	uint16_t				data_size;		//1:1byte, 2:2bytes, 4:4bytes
	uint16_t				dev_addr;		// LSB 1byte
	uint16_t				byte_num;		//0~200
//	uint16_t				enable;			//0: off 1: on
	uint16_t				sub_addr[200];	// fixed with 2bytes
	uint16_t				reg_data_h[200];// length depends on the data_size
	uint16_t				reg_data_l[200];
	uint16_t				table_data_h[200];//fixed with 4bytes
	uint16_t				table_data_l[200];
}__PACKED__ std_ocmd_wr;

typedef struct
{
	req_head_t				hdr;
	std_i2c_test_wr			i2c_info;
} __PACKED__ req_std_i2c_test_wr_t;

typedef struct
{
	req_head_t				hdr;
	std_ocmd_wr				ocmd_info;
} __PACKED__ req_std_ocmd_test_wr_t;

typedef struct //ksk ctl wr t
{
	req_head_t				hdr;
	std_ctl_rw				ctl_info;
} __PACKED__ req_std_ctl_wr_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				onoff;		//0:off, 1:on
} __PACKED__ req_std_acdet_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				onoff;		//0:off, 1:on
} __PACKED__ req_std_pseq_onoff_t;

typedef struct //ksk CTL
{
	req_head_t				hdr;
	uint16_t				onoff;		//0:off, 1:on
} __PACKED__ req_std_ctl_enable_t;

typedef struct //ksk DP Timing
{
	req_head_t				hdr;
	uint32_t				h_total;
	uint32_t				h_active;
	uint32_t				h_bpo;
	uint32_t				h_width;
	uint32_t				v_total;
	uint32_t				v_active;
	uint32_t				v_bpo;
	uint32_t				v_width;
	uint32_t				freq;
} __PACKED__ req_std_dp_timing_t;

typedef struct
{
	unsigned int t1;			// 1234.5ms -> 12345
	unsigned int t2;			// 1234.5ms -> 12345
	unsigned int t3;			// 1234.5ms -> 12345
	unsigned int t3_1;			// 1234.5ms -> 12345
	unsigned int t3_2;			// 1234.5ms -> 12345
	unsigned int t3_3;			// 1234.5ms -> 12345
	unsigned int t3_4;			// 1234.5ms -> 12345
	unsigned int t4;			// 1234.5ms -> 12345
	unsigned int t5;			// 1234.5ms -> 12345
	unsigned int ton;			// 1234.5ms -> 12345
	unsigned int acdet_on;		// 1234.5ms -> 12345
	unsigned int acdet_off;		// 1234.5ms -> 12345
	unsigned int t_i2c_on_1;	// 1234.5ms -> 12345
	unsigned int t_i2c_off_1;	// 1234.5ms -> 12345
	unsigned int t_i2c_on_2;	// 1234.5ms -> 12345
	unsigned int t_i2c_off_2;	// 1234.5ms -> 12345
	unsigned int repeat;		// 0~
} __PACKED__ std_aux_pwr_seq_timing_t;

typedef struct
{
	unsigned short reg_addr[4];	// [0]:T3-1, [1]:T3-2, [2]:T3-3. [3]:T3-4
	unsigned short reg_data[4];	// [0]:T3-1, [1]:T3-2, [2]:T3-3. [3]:T3-4
} __PACKED__ std_aux_pwr_seq_cmd_t;

typedef struct
{
	req_head_t					hdr;
	unsigned int				sel;				// b0:ch1, b1:ch2, b2:ch3, b3:ch4, b4:ch5, b5:ch6, b6:ch7
	std_aux_pwr_seq_timing_t	pwr_seq_timing[7];
	std_pwr_seq_i2c_t			pwr_seq_i2c[4];		// [0]:on_i2c1, [1]:off_i2c1, [2]:on_i2c2, [3]:off_i2c2
	unsigned short				onoff;				// b0:i2c_1_on, b1:i2c_1_off, b2:i2c_2_on, b3:i2c_2_off
	unsigned int				scenario;
	unsigned int				rpcnt;
	std_aux_pwr_seq_cmd_t		aux_pwr_seq_cmd;
} __PACKED__ req_std_aux_i2c_pwr_seq_test_t;

typedef struct
{
	req_head_t					hdr;
	unsigned int				sel;				// b0:ch1, b1:ch2, b2:ch3, b3:ch4, b4:ch5, b5:ch6, b6:ch7
	std_aux_pwr_seq_timing_t	pwr_seq_timing[7];
	std_pwr_seq_ocmd_t			pwr_seq_ocmd[4];	// [0]:on_ocmd1, [1]:off_ocmd1, [2]:on_ocmd2, [3]:off_ocmd2
	unsigned short				onoff;				// b0:i2c_1_on, b1:i2c_1_off, b2:i2c_2_on, b3:i2c_2_off
	unsigned int				scenario;
	unsigned int				rpcnt;
	std_aux_pwr_seq_cmd_t		aux_pwr_seq_cmd;
} __PACKED__ req_std_aux_ocmd_pwr_seq_test_t;

typedef struct
{
	std_aux_pwr_seq_cmd_t		aux_pwr_seq_cmd;

	unsigned int t1;			// 1234.5ms -> 12345
	unsigned int t2;			// 1234.5ms -> 12345
	unsigned int t3;			// 1234.5ms -> 12345
	unsigned int t3_1;			// 1234.5ms -> 12345
	unsigned int t3_2;			// 1234.5ms -> 12345
	unsigned int t3_3;			// 1234.5ms -> 12345
	unsigned int t3_4;			// 1234.5ms -> 12345
} __PACKED__ std_aux_timing_t;

typedef struct
{
	unsigned short aux_clock_sel;	// 0:25kHz, 1:100kHz, 2:400kHz, 3:1MHz
	unsigned short reg_addr_size;	// 1:1byte, 2:2byte
	unsigned short data_size;		// 1:1byte, 2:2byte
	unsigned short dev_addr;		// Use lsb 1byte.
	unsigned short byte_num;		// 0~200
	unsigned short reg_addr[200];	// if reg_addr is 1, using lsb 1byte.
	unsigned short reg_data[200];	// if reg_data size is 1, using lsb 1byte.
} __PACKED__ std_aux_test_t;

typedef struct
{
	req_head_t		hdr;
	std_aux_test_t	aux_test;
} __PACKED__ req_std_aux_test_wr_t;

typedef struct
{
	req_head_t		hdr;
	std_aux_test_t	aux_test;
} __PACKED__ req_std_aux_test_rd_t;

// Response Structure ----------------------------------------------------------------
typedef struct
{
	uint16_t				cmd_id;
	uint16_t				data_len;
} __PACKED__ res_head_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
	uint16_t				ver;		// P/G F/W version
	uint16_t				pwr_ver[ENSIS_PWR_CH];	// EP616P F/W version
} __PACKED__ res_ver_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				patternid;	//0~128
	char					model_name[MAX_MODEL_NAME];
	uint16_t				alarm;
	uint16_t				state;//run:0 stop:1 hold:2
	uint16_t				vdd_set_value;//file data
	uint16_t				vbl_set_value;//file data
	uint16_t				vdd_sensing_value;//sensing data
	uint16_t				vbl_sensing_value;//sensing data
	uint16_t				idd_sensing_value;//sensing data
	uint16_t				ibl_sensing_value;//sensing data
	uint64_t				time;//run time(min)
} __PACKED__ res_qems_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_model_sel_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_cycle_sel_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_group_sel_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_lcd_onoff_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_pat_disp_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
	uint16_t				state;				// 0:none, 1:ready, 2:auto, 3:manu, 4:booting
	int8_t					cur_model[MAX_MODEL_NAME];
	int8_t					cur_pattern[MAX_PAT_NAME];
	uint16_t				vdd;				// ex) 5.00V -> 500
	uint16_t				idd;				// ex) 50mA -> 50
	uint16_t				vbl;				// ex) 5.00V -> 500
	uint16_t				ibl;				// ex) 50mA -> 50
	uint8_t					limit				;// enum_tcp_error_t
	uint8_t					ocp;
//	int8_t					current_group[MAX_GROUP_NAME];
//	int8_t					fw_version[MAX_FW_NAME];		//ES628 F/W version
//	int8_t					comu_version[MAX_FW_NAME];		//COMMUNICATOR version - shared memory only
//	uint16_t				progress;						//initialization progress check
//	uint16_t				pwr_conn;						//ES620P connection check
//	int8_t					production_name[MAX_PG_NAME];	//PG NAME
} __PACKED__ res_status_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_freq_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_vsync_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_pos_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_color_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_vdd_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_run_set_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_pwr_fw_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
} __PACKED__ res_offset_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
	uint16_t				reg_data[200];
} __PACKED__ res_std_i2c_test_rd_t;

typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
	uint16_t				reg_data_h[200];
	uint16_t				reg_data_l[200];
} __PACKED__ res_std_ocmd_test_rd_t;
typedef struct
{
	res_head_t				hdr;
	uint16_t				res;
	uint16_t				reg_data[256];
} __PACKED__ req_std_ctl_rd_t;


typedef struct
{
	res_head_t		hdr;
	uint32_t		scenario;			// shows which scenario were you in, reset when sequence was stopped / save the param when sequence was paused
	uint32_t		repeat;				// shows when was it paused, reset when sequence was stopped / save the param when sequence was paused
	unsigned short	read_aux_data[4];	// shows when was it paused, reset when sequence was stopped / save the param when sequence was paused
} __PACKED__ res_pwr_seq_t;

typedef struct
{
	res_head_t	hdr;
	uint16_t	res;
} __PACKED__ res_std_aux_i2c_pwr_seq_test_t;

typedef struct
{
	res_head_t	hdr;
	uint16_t	res;
} __PACKED__ res_std_aux_ocmd_pwr_seq_test_t;

typedef struct
{
	res_head_t	hdr;
	uint16_t	res;
} __PACKED__ res_std_aux_test_wr_t;

typedef struct
{
	res_head_t		hdr;
	uint16_t		res;
	unsigned short	read_data[200];
} __PACKED__ res_std_aux_test_rd_t;

typedef struct
{
	req_head_t	hdr;
	uint16_t	res;
} __PACKED__ res_std_edp_chara_set_write_t;

//====================================================

typedef struct
{
	req_head_t				hdr;
} __PACKED__ req_pwrc_ip_addr_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				id;
	uint16_t				ch;			//0~3=ch1~ch4, 4=all
	uint32_t				slew;
} __PACKED__ req_pwrc_slew_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				pwr_id;			// 0:power-1, 1:power-2
	uint16_t				pwr_ch;			// 0~3:ch1~ch4, 4:all
	uint16_t				voltage;		// 12.00V -> 1200
	uint16_t				current;		// 10.00A -> 1000
} __PACKED__ req_pwrc_vol_cur_set_t;

typedef struct
{
	req_head_t				hdr;
	uint16_t				onoff;			// 0:off, 1:on
	uint16_t				pwr_id;			// 0:power-1, 1:power-2
	uint16_t				pwr_ch;			// 0~3:ch1~ch4, 4:all
} __PACKED__ req_pwrc_onoff_t;

//====================================================







typedef struct
{
	uint32_t 				x;
	uint32_t 				y;
} pos_t;

typedef struct {
	uint8_t 				type;
	uint8_t 				way;
	int16_t					x;
	int16_t					y;
	uint16_t				red;
	uint16_t				green;
	uint16_t				blue;
} pos_data_t;

typedef struct
{
//	float					module_hz;
//	uint32_t				gx_ai_enable;
//	uint32_t				indirect_check;
//	uint32_t				odha; 				// overscreen_divide_h active
//	uint32_t				odva; 				// overscreen_divide_v active
//	uint16_t				adim;
//	uint16_t				port;				// 0:1234, 1:2413, 2:1324, 3:3142, 4:4321, 5:3412, 6:2143
//	uint64_t				freq;
//	uint16_t				vsync;
//	uint16_t				bittype;			//bit type:normal 8bit(8),normal 10bit(10),jeida 8bit(12),jeida 10bit(14),Vesa 8bit(16),vesa 10bit(18)
//	uint16_t				network_connect;	//0:disconnect 1:reconnecting 2:connect

	float					module_hz;
	uint32_t				gx_ai_enable;
	uint32_t				indirect_check;
	uint32_t				odha; 				// overscreen_divide_h active
	uint32_t				odva; 				// overscreen_divide_v active
	uint16_t				adim;
	uint16_t				port;				// 0:1234, 1:2413, 2:1324, 3:3142, 4:4321, 5:3412, 6:2143
	uint64_t				freq;
	uint16_t				vsync;
	uint16_t				bittype;			//bit type:normal 8bit(8),normal 10bit(10),jeida 8bit(12),jeida 10bit(14),Vesa 8bit(16),vesa 10bit(18)
	uint16_t				network_connect;	//0:disconnect 1:reconnecting 2:connect
	uint16_t				model_change;		//0:change end 1:changing..
	int8_t					wifi_mac[6];			//mac address
	uint16_t				inst_ind_pat_enable;
	uint8_t					sub_board_id[4];	// 0:ep620s, 1:ep620t		[0]:port1, [1]:port2, [2]:port3, [3]:port4
} global_data_t;



typedef struct {
	unsigned int 			dhcp;				// 0:dhcp, 1:static
	unsigned char 			myip[4];
	unsigned char 			serverip[4];
	unsigned char 			gateway[4];
	unsigned char 			netmask[4];
	unsigned char 			mac[6];
} __PACKED__ net_config_t;

typedef struct {
	unsigned char 			name[32];
	unsigned char 			myip[4];
	unsigned char 			serverip[4];
	unsigned char 			gateway[4];
	unsigned char 			netmask[4];
	unsigned char 			mac[6];
} __PACKED__ net_info_t;


typedef struct
{
	uint16_t	rw;

	uint16_t	res;
	uint16_t	state;		// 0:none, 1:ready, 2:auto_run, 3:manual_run, 4:booting

	int8_t		cur_model_name[MAX_MODEL_NAME];
	int8_t		cur_pat_name[MAX_PAT_NAME];
	int8_t		cur_group_name[MAX_GROUP_NAME];

	uint16_t	vdd;		// ex) 5.00V -> 500
	uint16_t	idd;		// ex) 50mA -> 50
	uint16_t	vbl;		// ex) 5.00V -> 500
	uint16_t	ibl;		// ex) 50mA -> 50

	uint8_t		limit;
	uint8_t		ocp_pid;
	uint16_t	progress;
	int8_t		fw_version[MAX_FW_NAME];
	int8_t		com_version[MAX_FW_NAME];
	int16_t		pwr_conn;
	int8_t		product_name[MAX_PRODUCT_NAME];			//~282

	uint16_t	reserved[742];
} shared_status_t;

typedef struct
{
	uint16_t	rw;

	uint16_t	res;
	uint16_t	state;		// 0:none, 1:ready, 2:auto_run, 3:manual_run

	int8_t		cur_model_name[MAX_MODEL_NAME];
	int8_t		cur_pat_name[MAX_PAT_NAME];
	int8_t		cur_group_name[MAX_GROUP_NAME];

	uint16_t	vdd;		// ex) 5.00V -> 500
	uint16_t	idd;		// ex) 50mA -> 50
	uint16_t	vbl;		// ex) 5.00V -> 500
	uint16_t	ibl;		// ex) 50mA -> 50

	uint8_t		limit;
	uint8_t		ocp_pid;
	uint16_t	progress;
	int8_t		fw_version[MAX_FW_NAME];
	int8_t		com_version[MAX_FW_NAME];
	int16_t		pwr_conn;
	int8_t		product_name[MAX_PRODUCT_NAME];			//~282

	uint16_t	reserve[742];
} buffer_status_t;





// Variables
int							exit_flag, exit_load, reboot_flag;
unsigned char 				task_cnt;
pthread_t 					task_id[MAX_TASK_CNT];
global_data_t 				gp;
int							quhd_enable;
int							op_mode;
int							pwr_vendor;			// 0:ensis, 1:osung
int							wifi_select;		// 0:OFF, 1:static 2:dhcp
int							re_driver_eqdc, re_driver_eqacl, re_driver_eqacu;
struct timeval				aging_start_time, pattern_start_time, sensing_start_time, onoff_start_time, detect_start_time, w_time, inc_start_time;
uint64_t					elapse_qems_aging_time;
uint64_t					elapse_pause_time, old_elapse_pause_time;

int							pg_tx_mode;		// 0:vby1_only, 1:edp_16+vby1_16

unsigned short				pll_interval_time;
uint16_t					pwr_onoff_status;

uint8_t						bt_count;		//boot count

char 						power1_ip_info[MAX_IP_ADDR];
char			 			power2_ip_info[MAX_IP_ADDR];

uint16_t					power1_voltage, power2_voltage;

uint16_t					vde_cnt;

struct timeval				pseq_power_start_time, pseq_signal_start_time, pseq_power_pause_time;

//char						rcb_slide_ubuf[MAX_PATH], rcb_slide_dbuf[MAX_PATH];
//int							rcb_slide_ulen, rcb_slide_dlen, rcb_slide_cnt;

uint64_t 					pause_time, old_pause_time;
uint64_t 					sig_pause_time, old_sig_pause_time;
uint64_t 					elapse_time;
char						pat_roll_test;

unsigned char 				APRF;

char 						cur_group_name;

char						wtime_chk;

#define EXTERNAL_POWER_CTRL

#ifdef EXTERNAL_POWER_CTRL
uint16_t					max_pwr1_ch, max_pwr2_ch;
uint16_t					pwr1_ch_chk_done, pwr2_ch_chk_done;		//0:start or not set, 1:waiting for return, 2:done
int							pwr_ip_changed;

extern void					set_power_ip(char *ip1, char *ip2);
#endif


// Functions
extern void 				create_dir(void);
extern int 					task_thread(int priority, void *taskfunc);
extern void 				start_shell(void);
extern void 				command_system(char* cmd);

extern enum_opmode_t 		get_opmode(void);
extern int 					get_reboot(void);
extern uint64_t 			get_aging_elapse_time(void);
extern uint64_t				get_pattern_elapse_time(void);	// ms
extern uint64_t				get_sensing_elapse_time(void);	// ms
extern uint64_t				get_schedule_elapse_time(void);	// ms
extern int 					get_quhd_enable(void);

extern uint64_t				get_pattern_elapse_frame(void);	//

extern void 				set_opmode(enum_opmode_t mode);
extern void 				set_aging_start_time(void);
extern void					set_pattern_start_time(void);
extern void					set_sensing_start_time(void);
extern void					set_schedule_start_time(void);
extern void					set_reboot(int flag);
extern void 				set_quhd_enable(int enable);
extern void 				set_group_id(uint8_t id);
extern void 				set_pwr_vendor(int index);
extern void 				set_detect_start_time(void);
extern void					set_pg_tx_mode(int mode);


extern int 					get_ip_info(net_info_t *net);
extern int 					get_ip_set(net_config_t *net);
extern void 				get_eth_name(char *eth);

extern uint8_t 				get_wifi_select(void);
extern int 					get_wifi_level_int(void);
extern void 				get_wifi_level(char *str);
extern void 				get_qems_info(char *eqpid);
extern void 				get_qems_master_info(char *eqpid);
extern int 					get_ip_wlan0(net_config_t *net);
extern uint8_t 				get_wifi_info(char *ssid,char *ip,char *netmask,char *gateway,char *identity,char *password);
extern void 				get_server_ip(char *ip);

extern uint8_t 				get_group_id(void);
extern uint8_t 				get_pwr_sel(void);
extern int 					get_pwr_vendor(void);
extern uint64_t 			get_detect_elapse_time(void);
extern int					get_pg_tx_mode(void);

extern void					fpga_read_scan(void);

extern void 				delay_us(uint32_t microseconds);
extern uint64_t 			get_current_time(void);
extern int 					replace(const char* Expression, char* Dest, const char* Find, const char* Replace, unsigned int Start, unsigned int Count);



extern void					re_driver_mux_default(void);
extern void 				re_driver_mux_ACnU_set(unsigned char level);
extern void 				re_driver_mux_ACnL_set(unsigned char level);
extern void 				re_driver_mux_DCn_set(unsigned char level);

extern void					get_sub_board_id(void);
extern void					read_shmem(void);
extern void					set_progress_count(int count);

//2020.11.11
extern uint8_t				get_reboot_count(void);
extern void					set_reboot_count(int count);

//2020.11.13
extern char* 				read_storage_volume(void);
extern void					inspect_hdd_usage(void);

extern void					ushort_to_binary(uint16_t hex);

//ES628
extern void					get_power_ip(char *ip1, char *ip2);
extern void					set_power_ip(char *ip1, char *ip2);

std_pwr_seq_timing_buf_t	pseq_buf[MAX_PSEQ_CNT];

uint8_t						pseq_set_scnt, std_power_seq_run, std_power_paused;		//std_power_seq_run - 0: off(seq time end), 1:on, 2:forced off(user end)
uint16_t					flag_i2c_1_on_en, flag_i2c_1_off_en, flag_i2c_2_on_en, flag_i2c_2_off_en;
uint16_t					flag_ocmd_1_on_en, flag_ocmd_1_off_en, flag_ocmd_2_on_en, flag_ocmd_2_off_en;
uint32_t					scenario_cnt, scenario_cnt_temp; //b0: ch1, b1: ch2, b2: ch3, b3: ch4, b4: ch5, b5: ch6, b6: ch7
uint32_t					pseq_rpcnt; //
unsigned short old_scan;

extern void set_std_pseq_power_start_time(void);
extern uint64_t get_std_pseq_power_elapse_time(void);
extern void set_std_pseq_power_pause_start_time(void);
extern uint64_t get_std_pseq_power_pause_elapse_time(void);
extern void set_std_pseq_signal_start_time(void);
extern uint64_t get_std_pseq_signal_elapse_time(void);
extern void std_pseq_forced_shut_down(void);
extern void set_inc_start_time(void);
extern uint64_t get_inc_elapse_time(void);


//test
extern void set_w_time(void);
extern uint64_t get_w_time(void);

// ELVDD Alarm
int elvdd_alarm_flag_global;

extern void get_elvdd_alarm_flag_global(void);
extern void set_elvdd_alarm_flag_global(int elvdd_alarm_flag_factor);

int pwr_sequence_vx1_edp_flag;	// 0 : vx1, 1 : edp
std_aux_pwr_seq_cmd_t pwr_seq_aux_addr_data;

extern void aux_read_function(int idx_factor);

int pwr_seq_aux_state;

#endif /* GLOBAL_H_ */
