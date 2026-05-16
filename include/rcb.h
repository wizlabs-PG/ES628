/*
 * rcb.h
 *
 *  Created on: Oct 24, 2017
 *      Author: root
 */

#ifndef RCB_H_
#define RCB_H_

#include <global.h>
#include <netinc.h>
#include <pollmanager.h>
#include <pwr_control.h>
#include <group_data.h>

#define	DEV_RCB				"/dev/ttyUSB0"
#define RCB_BAUD_RATE		57600	// only ES812K 17.10.27 V1.0.1
#define MAX_LINE_TEXT		16
#define MAX_TEXT_BUF		32
#define MAX_DISP_COUNT		8
//#define MAX_DIMM_VAL		33		// 3.3V
#define MAX_DIMM_VAL		50		// 5V		: 19-07-05 request by MJH
#define MAX_DUTY_VAL		100		// 100%
#define MAX_SCR_SPEED		99
#define MAX_SCR_DIR			4
//#define MENU_MAX_NUM 		15		// 19-07-10 13->15 : add spc on/off + power f/w update
#define MENU_MAX_NUM 		17		// 21-03-30 15->16 : add storage_info

#define MAX_I2C_REG_CNT		10-1
#define	MAX_I2C_M_CNT		6


#define STX_RCB				0x02
#define ETX_RCB				0x03
#define SRX_RCB				0x5B	// '['
#define ERX_RCB				0x5D	// ']'

#define COLOR_OFFSET		16
#define CURSOR_SPEED1		32
#define CURSOR_SPEED2		64

#define RCB_STR_SLIDE_BLK	2
#define RCB_STR_SLIDE_TIME	333		//333 = 333ms

#define RCB_LINE1			'0'
#define RCB_LINE2			'1'
#define RCB_LEDONOFF		'2'

#define RCB_CURSOR			'5'

#define KEY_ONOFF   		"00"	// old - KEY_MODEL
#define KEY_AUTOMANU		"01"	// old - KEY_CHANNEL1
#define KEY_GRAY  			"02"
#define KEY_POS				"03"
#define KEY_FILE			"04"	// old - KEY_HPOS
#define KEY_UP  			"05"
#define KEY_ESC				"06"
#define KEY_MODEL			"07"	// old - KEY_CHANNEL2
#define KEY_FREQ			"08"
#define KEY_FUNC			"09"
#define KEY_OK				"0A"	// old - KEY_VPOS
#define KEY_DOWN			"0B"
#define KEY_LONGUP			"15"
#define KEY_LONGDOWN		"1B"
#define KEY_FWD				"UP"
#define KEY_BWD				"DN"
#define KEY_LONGFWD			"U1"
#define KEY_LONGBWD			"D1"
#define KEY_RCB_INIT		"IN"

#define KEY_NEXT_PATTERN	"NP"

#define LED_QUHD   			"8K ON"
#define LED_NOT_QUHD		"8K OFF"

#define LED_ON_ONOFF   		"MODEL ON"
#define LED_ON_AUTOMANU		"CH1 ON"
#define LED_ON_GRAY  		"GRAY ON"
#define LED_ON_POS			"HTIME ON"
#define LED_ON_FILE			"HPOS ON"
#define LED_ON_UP  			"UP ON"
#define LED_ON_ESC			"ESC ON"
#define LED_ON_MODEL		"CH2 ON"
#define LED_ON_FREQ			"FREQ ON"
#define LED_ON_ETC			"VTIME ON"
#define LED_ON_OK			"VPOS ON"
#define LED_ON_DOWN			"DOWN ON"

#define LED_OFF_ONOFF   	"MODEL OFF"
#define LED_OFF_AUTOMANU	"CH1 OFF"
#define LED_OFF_GRAY  		"GRAY OFF"
#define LED_OFF_POS			"HTIME OFF"
#define LED_OFF_FILE		"HPOS OFF"
#define LED_OFF_UP  		"UP OFF"
#define LED_OFF_ESC			"ESC OFF"
#define LED_OFF_MODEL		"CH2 OFF"
#define LED_OFF_FREQ		"FREQ OFF"
#define LED_OFF_ETC			"VTIME OFF"
#define LED_OFF_OK			"VPOS OFF"
#define LED_OFF_DOWN		"DOWN OFF"

#define TITLE_PG_START		"  P/G STARTED  "
#define TITLE_PG_END		"  P/G CLOSED   "
#define TITLE_MENU			"      MENU     "
#define TITLE_READY			"   R E A D Y   "
#define TITLE_MODEL_SELECT	"  MODEL SELECT  "
#define TITLE_GROUP_SELECT	"  GROUP SELECT  "
#define TITLE_SERVER_IP		"SERVER IP SET"
#define TITLE_WIFI_IP		"WIFI IP SET"
#define TITLE_WIFI_NETMASK	"WIFI NETMASK SET"
#define TITLE_WIFI_GATEWAY	"WIFI GATEWAY SET"
#define TITLE_WIFI_SSID		"SSID SET"
#define TITLE_QEMS_EQPID	"EQPID SET"
#define TITLE_QEMS_MASTER_EQPID	"MASTER EQPID SET"
#define TITLE_WIFI_IDENTITY	"IDENTITY SET"
#define TITLE_WIFI_PASSWORD	"PASSWORD SET"
#define TITLE_GROUP_ID		"GROUP ID SET"
#define TITLE_PWR_SELECT	"POWER SELECT"

#define TITLE_WIFI_SELECT	"WIFI SELECT"
#define TITLE_AUTO_MODE		"AUTO MODE..."
#define TITLE_MANU_MODE		"MANU MODE..."
#define TITLE_SCHE_MODE		"SCHE MODE..."
#define TITLE_FREQ_CHANGE_S	"FREQ CHANGE:SLOW"
#define TITLE_FREQ_CHANGE_F	"FREQ CHANGE:FAST"
#define TITLE_FREQ_CHANGE_FF	"FREQ CHANGE:FF"
#define TITLE_GRAY_CHANGE_S	"GRAY CHANGE:SLOW"
#define TITLE_GRAY_CHANGE_F	"GRAY CHANGE:FAST"
#define TITLE_LEVEL_CHANGE	"LEVEL CHANGE"
#define TITLE_CURSOR_S		"CURSOR:SLOW"
#define TITLE_CURSOR_F		"CURSOR:FAST"
#define TITLE_CURSOR_FF		"CURSOR:FF"
#define TITLE_CURSOR_COLOR_S	"CURSOR COLOR:SLO"
#define TITLE_CURSOR_COLOR_F	"CURSOR COLOR:FAS"
#define TITLE_HTIME_CHANGE	"1.HTIME CHANGE"
#define TITLE_VTIME_CHANGE	"2.VTIME CHANGE"
#define TITLE_CH_SHIFT		"3.CHANNEL SHIFT"
#define TITLE_BIT_SHIFT		"4.BIT SHIFT"
#define TITLE_DIMM_CHANGE	"5.DIMM CHANGE"
#define TITLE_PWM_CHANGE	"6.PWM CHANGE"
#define TITLE_SCROLL_CHANGE	"7.SCROLL TEST"
#define TITLE_I2C_SET		"8.I2C "
#define TITLE_VBY1_EQUAL_SET	"9.Vby1 EQUAL OPT"
#define TITLE_VDD_CHANGE	"10.VDD CHANGE"
#define TITLE_PRE_EMPHASIS	"11.Pre-emphasis"
#define TITLE_VBL_CHANGE	"12.VBL CHANGE"

#define TITLE_SCHEDULE_X	"POWER SCHEDUL[X]"
#define TITLE_SCHEDULE_O	"POWER SCHEDUL[O]"
#define TITLE_MODEL_DEL_ONE	"MODEL DEL   [OK]"
#define TITLE_MODEL_DEL_ALL	"MODEL DEL   [OK]"
#define TITLE_PAT_DEL_ONE	"PATTERN DEL [OK]"
#define TITLE_PAT_DEL_ALL	"PATTERN DEL [OK]"
#define TITLE_BMP_DEL_ONE	"BMP DEL     [OK]"
#define TITLE_BMP_DEL_ALL	"BMP DEL     [OK]"
#define TITLE_GROUP_DEL_ONE	"GROUP DEL   [OK]"
#define TITLE_GROUP_DEL_ALL	"GROUP DEL   [OK]"

#define TITLE_PWR_SPC_SET	"POWER SPC SET"
#define TITLE_PWR_UPLOAD	"POWER F/W"
#define TITLE_STORAGE_INFO	"STORAGE INFO    "

// Enumerations
typedef enum
{
    COLOR_MODE_RED			= 0x01,
    COLOR_MODE_GREEN		= 0x02,
    COLOR_MODE_BLUE			= 0x04,
    COLOR_MODE_RED_GREEN	= 0x03,
    COLOR_MODE_GREEN_BLUE	= 0x06,
    COLOR_MODE_BLUE_RED		= 0x05,
    COLOR_MODE_WHITE		= 0x07
} enum_color_mode_t;

typedef enum
{
	COLOR_SPEED_SLOW		= 1,
	COLOR_SPEED_FAST		= 8
} enum_color_speed_t;

typedef enum
{
	FREQ_SPEED_SLOW		= 1000000,
	FREQ_SPEED_FAST		= 10000000,
	FREQ_SPEED_FFAST	= 50000000
} enum_freq_speed_t;

typedef enum
{
	RCB_HTIME				= 0,
	RCB_VTIME,
	RCB_HV_RESET
} enum_hvtime_t;

typedef enum
{
    CURSOR_X 				= 0,
    CURSOR_Y
} enum_cursor_t;

typedef enum
{
    FILE_ONE 				= 0,
    FILE_ALL
} enum_file_t;

typedef enum
{
	IDX_VOD				= 0,
	IDX_1PRE,
	IDX_2PRE,
	IDX_1POST,
	IDX_2POST,
	IDX_DEFAULT,
	_VOD_MAX			= 31,
	_1POST_MAX			= 25,
	_2POST_MAX			= 12,
	_1PRE_MAX			= 16,
	_2PRE_MAX			= 7,
	_PREM_REF_VALUE		= 31

} enum_pre_emph_t;


// Structures
typedef struct rcb_t_ rcb_t;
struct rcb_t_
{
	int   			poll_ndx;
	int   			fd;
	int   			type;						// tcp/udp/uds/serial/
	int   			tag;
	void  			(*on_poll_in )( rcb_t *);
	void  			(*on_poll_out)( rcb_t *);
	void  			(*on_poll_err)( rcb_t *);
	void  			(*on_poll_hup)( rcb_t *);

	char  			devname[MAX_DEV_NAME];		// Device Name
	int   			baud;
	int   			databit;
	int   			stopbit;
	int   			parity;

	void  			(*on_read    )( rcb_t *);
	void  			(*on_writable)( rcb_t *);
	void  			(*on_error   )( rcb_t *, int);
};

typedef struct var_t_ var_t;
struct var_t_
{
	uint64_t		freq;						// main clock
	uint32_t		h_total;
	uint32_t		h_active;
	uint32_t		h_bpo;
	uint32_t		h_fpo;
	uint32_t		h_width;
	uint32_t		v_total;
	uint32_t		v_active;
	uint32_t		v_bpo;
	uint32_t		v_fpo;
	uint32_t		v_width;

	uint16_t		twist;						// 0:JEIDA, 1:VESA
	uint16_t		vdd;						// 12.00V -> 1200
	uint16_t		vbl;
	uint32_t		v_freq;						// Hz
	uint32_t		freq_speed;

	uint16_t		adim;						// 3.3V -> 33 (0~3.3V)
	uint8_t			duty;						// 0 ~ 100%

	uint16_t		cursor_speed;
	uint32_t		gray_scale;					// gray Level

	uint16_t		color_mode;
	uint16_t		color_speed;
	color_t			color;						// color change

	uint16_t		port;						// port change
	pos_data_t		cursor;
	uint16_t		cursor_color_mode;
	uint16_t		cursor_color_speed;

	int32_t			info_idx;

	uint16_t		scr_dir;
	uint16_t		scr_speed;
};

typedef struct preem_t_ preem_t;
struct preem_t_
{
	uint16_t		_vod;
	uint16_t		_1post;
	uint16_t		_2post;
	uint16_t		_1pre;
	uint16_t		_2pre;

};


// Variables
rcb_t				*rcb_fd;
int					buft_cnt;
int					rx_push;
int					data_len;
int					menu_idx, lcd_cursor_pos;
int					server_ip[4], var_server_ip[4];
char				qems_eqpid[16],var_qems_eqpid[16];
char				qems_master_eqpid[16],var_qems_master_eqpid[16];
int					wifi_ip[4], var_wifi_ip[4];
int					wifi_netmask[4], var_wifi_netmask[4];
int					wifi_gateway[4], var_wifi_gateway[4];
char				wifi_ssid[16],var_wifi_ssid[16];
char				wifi_identity[16],var_wifi_identity[16];
char				wifi_password[16],var_wifi_password[16];
int					var_wifi_sel;
int					group_id, var_group_id;
int					var_pwr_sel;
int					var_spc_en;
int					vby1_equal_opt_index;
int					tcon_i2c_reg_index;
short				tcon_i2c_reg_data[12]; //[0]~[5]: master, [6]~[11]: slave
short				tcon_i2c_data_min[12]; //[0]~[5]: master, [6]~[11]: slave
short				tcon_i2c_data_max[12]; //[0]~[5]: master, [6]~[11]: slave
char				rx_buft[__MAX_BUF_SIZE];
int					onoff_flag, cursor_flag, file_flag;
int					color_done, cursor_done, freq_done;
int					reload_model_flag;
int					adim_change_flag;


var_t				var_data;

uint				pre_emp_index;
ushort				tmp_pre_emp;
preem_t				prm_data;
struct timeval		rcb_slide_start_time;


// Functions
extern rcb_t 		*rcb_init (void);
extern rcb_t 		*rcb_open( char  *, int   , int   , int, int);
extern rcb_t 		*rcb_find_tag( int);
extern int 			rcb_write( rcb_t *, char  , char *);
extern int      	rcb_read ( rcb_t *, char *, int          );
extern void     	rcb_close( rcb_t *                       );

extern void 		on_rcb_error(rcb_t *sender, int reopen_ok);
extern void 		on_rcb_recv (rcb_t *sender);

void 				rcb_start(void);
extern void 		rcb_proc(char *);
extern void 		rcb_server_ip_proc(char *);
extern void 		rcb_group_id_proc(char *c);
extern void 		rcb_pwr_select_proc(char *c);
extern void 		rcb_menu_proc(char *);
extern void 		rcb_ready_screen(int);
extern void 		rcb_model_change(void);
extern void 		rcb_model_change_proc(char *);
extern void 		rcb_ready_proc(char *);
extern void 		rcb_ready_func_proc(char *);
extern void 		rcb_auto_run_proc(char *);
extern void 		rcb_manu_run_proc(char *);
extern void 		rcb_freq_proc(char *);
extern void 		rcb_htime_proc(char *);
extern void 		rcb_vtime_proc(char *);
extern void 		rcb_cursor_proc(char *);
extern void 		rcb_vdd_change_proc(char *);
extern void 		rcb_color_change_proc(char *);
extern void 		rcb_gray_scale_proc(char *);
extern void 		rcb_onoff_control(int_pat_idx_factor, int_auto_manu_mode_factor);
extern void 		rcb_ch_shift_proc(char *);
extern void 		rcb_bit_shift_proc(char *);
extern void 		rcb_dim_change_proc(char *c);
extern void 		rcb_pwm_change_proc(char *c);
extern void 		rcb_scroll_test_proc(char *c);
extern void 		rcb_model_delete_proc(char *);
extern void 		rcb_pattern_delete_proc(char *);
extern void 		rcb_bitmap_delete_proc(char *);
extern void 		rcb_usb_detect_proc(char *);
extern void 		rcb_network_info_proc(char *c);
extern void 		rcb_wifi_ssid_proc(char *c);
extern void 		rcb_wifi_password_proc(char *c);
extern void			rcb_wifi_ip_proc(char *c);
extern void 		rcb_wifi_select_proc(char *c);

extern void			rcb_wifi_netmask_proc(char *c);
extern void			rcb_wifi_gateway_proc(char *c);
extern void			rcb_qems_eqpid_proc(char *c);
extern void			rcb_wifi_identity_proc(char *c);

extern void 		freq_set_from_pc(req_freq_set_t *pdata);
extern void 		vsync_set_from_pc(req_vsync_set_t *pdata);
extern void 		color_set_from_pc(req_color_set_t *pdata);
extern void 		pos_set_from_pc(req_pos_set_t *pdata);
extern uint64_t 	vsync_to_freq(uint16_t vsync);
extern float 		freq_to_vsync(uint64_t freq);
extern float		freq_to_mhz(uint64_t freq);

extern int			get_onoff_flag(void);
extern void 		set_onoff_flag(int);

extern void 		model_variable_reset(void);

extern void 		rcb_quhd_mode_select(int select);

extern void 		set_wifi_info(int select,char *ssid,int *ip,int *netmask,int *gateway,char *identity,char *password);

extern void 		rcb_pwr_spc_set_proc(char *c);
extern void 		rcb_pwr_update_proc(char *c);
extern void 		rcb_tcon_i2c_ctrl_proc(char *c);

extern void			rcb_vby1_equal_option_proc(char *c);


extern void 		rcb_menu(void);
extern void 		rcb_server_ip_set(void);
extern void 		rcb_group_id_set(void);
extern void			rcb_pwr_select_set(void);
extern void			rcb_pwr_spc_onoff_set(void);
extern void 		rcb_pwr_fw_upload(void);
extern void			rcb_tcon_i2c_ctrl(void);
extern void 		send_tcon_i2c(void);

#ifdef GROUP_SELECT
extern void			rcb_group_change(int flag);
extern void			rcb_group_change_proc(char *c);
extern void			rcb_group_delete_proc(char *c);
#endif


#ifdef USE_PRE_EMPHASIS
void 				rcb_pre_emphasis_proc(char *c);
unsigned short 		fpga_pre_emphasis_check(unsigned int index, unsigned short pre_value);
void 				set_pre_emphasis_default(void);
ushort 				increase_pre_emp_setting(unsigned int index, unsigned short value);
ushort 				decrease_pre_emp_setting(unsigned int index, unsigned short value);
unsigned int 		set_this_pre_emp_index(unsigned int index, unsigned short value);
void 				display_pre_emphasis(unsigned int index, unsigned short value);
#endif

extern int 			pre_emphasis_set_from_pc(char vod, char post_1st, char post_2nd, char pre_1st, char pre_2nd);
extern void 		rcb_vbl_change_proc(char *c);		//20-04-01	request by sonk, sdc
extern void			color_cursor_from_pc(req_color_cursor_t *pdata);

extern void			rcb_storage_info(void);
extern void			rcb_storage_info_proc(char *c);

extern void			rcb_string_slide();
extern void			rcb_cursor_color_proc(char *c);

int					saving_pat_idx_rcb;
int					saving_auto_manu_mode_rcb;

// ELVDD Alarm
int elvdd_alarm_flag_buf_rcb;

extern void 		rcb_vbl_check_warning_proc(char *);
extern void rcb_elvdd_alarm_set_info(void);
extern void rcb_elvdd_alarm_set_info_proc(char *c);

#endif /* RCB_H_ */
