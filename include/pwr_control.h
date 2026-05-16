/*
 * pwr_control.h
 *
 *  Created on: Oct 24, 2017
 *      Author: root
 */

#ifndef OSUNG_PWR_H_
#define OSUNG_PWR_H_

#include <global.h>
#include <netinc.h>
#include <pollmanager.h>
#include <model_data.h>

#define	DEV_POWBD			"/dev/ttyTHS1"
#define POWBD_BAUD_RATE		38400
#define ENSIS_PWRBD_ID		0
#define OSUNG_PWRBD_ID		1
#define PWR_DETECT_TIME		200	// ms
//#define PWR_DETECT_TIME		500	// ms	test 1 sec

#define MAX_HW_NAME_LEN		20
#define MAX_CRC_TABLE		256
#define MAX_Q_SIZE			100
#define MAX_FW_BUF			512
#define MAX_PWR_VENDOR		2	//3-1

#define STX					0x7F
#define ETX					0x7E
#define DLE					0x7D
#define STF					0x20

#define FW_STX				0x02
#define FW_ETX				0x03

#define PWR_MAX_CH			2
#define INV_MAX_CH			2
#define IP_MAX_CH			2

typedef char 				s8;
typedef int16_t 			s16;
typedef int32_t 			s32;
typedef int64_t 			s64;
typedef uint8_t				u8;
typedef uint16_t 			u16;
typedef uint32_t 			u32;
typedef uint64_t 			u64;


typedef enum
{
	RES_NACK              	= 0,
	RES_ACK					= 1,
	RES_MAX
} enum_res_t;

typedef enum
{
	CH_1_T					= 0x01,
	CH_2_T					= 0x02,
	CH_3_T					= 0x04,
	CH_4_T					= 0x08,
	CH_ALL_T				= 0xFF
} enum_channel_t;

typedef enum
{
	ENUM_OFF				= 0,
	ENUM_ON					= 1
} enum_onoff_t;

typedef enum
{
	LCM_INCH_17_UNDER		= 0,
	LCM_INCH_19,
	LCM_INCH_20_1,
	LCM_INCH_22,
	LCM_INCH_23,
	LCM_INCH_24_OVER,
	LCM_INCH_MAX
} enum_lcm_inch_t;

typedef enum
{
	ENUM_LD_EN				= 0x01,
	ENUM_LDD				= 0x02,
	ENUM_SN_EN				= 0x04,
	ENUM_SDD				= 0x08,
	ENUM_DLC1				= 0x10,
	ENUM_DLC0				= 0x20,
	ENUM_LOCAL_SPARE1		= 0x40,
	ENUM_LOCAL_SPARE2		= 0x80
} enum_sys_info_local_t;

typedef enum
{
	ERR_VDD_MIN				= 0x01,
	ERR_VDD_MAX				= 0x02,
	ERR_IDD_MIN				= 0X04,
	ERR_IDD_MAX				= 0X08,
	ERR_VBL_MIN				= 0x10,
	ERR_VBL_MAX				= 0x20,
	ERR_IBL_MIN				= 0X40,
	ERR_IBL_MAX				= 0X80
} enum_detect_error_t;

typedef enum
{
	/*==================================*/
	/*			REQUEST COMMAND			*/
	/*==================================*/
	CMD_SYNC_REQ			= 0x1011,
	CMD_VER_REQ,
	CMD_LCD_ONOFF_REQ		= 0x1020,
	CMD_VBL_ONOFF_REQ,
	CMD_VDD_ONOFF_REQ,
	CMD_BL_ONOFF_REQ		= 0x1024,
	CMD_SPC_ONOFF_REQ,
	CMD_DIMM_ONOFF_REQ		= 0x102E,
	CMD_PDIM_CON_REQ		= 0x1030,
	CMD_ADIM_CON_REQ,
	CMD_VDD_SET_REQ			= 0x1042,
	CMD_VBL_SET_REQ,
	CMD_COMPOSIT_SET_REQ	= 0x1048,
	CMD_DETECT_GET_REQ		= 0x1051,
	CMD_OFFSET_DETECT_REQ,
	CMD_VBL_TIMEOUT_REQ     = 0x1053,//2023.03.20 vbl en timeout added

	/*==================================*/
	/*			RESPONSE COMMAND		*/
	/*==================================*/
	CMD_SYNC_RSP			= 0x2011,
	CMD_VER_RSP,
	CMD_LCD_ONOFF_RSP		= 0x2020,
	CMD_VBL_ONOFF_RSP,
	CMD_VDD_ONOFF_RSP,
	CMD_BL_ONOFF_RSP		= 0x2024,
	CMD_SPC_ONOFF_RSP,
	CMD_DIMM_ONOFF_RSP		= 0x202E,
	CMD_PDIM_CON_RSP		= 0x2030,
	CMD_ADIM_CON_RSP,
	CMD_VDD_SET_RSP			= 0x2042,
	CMD_VBL_SET_RSP,
	CMD_COMPOSIT_SET_RSP	= 0x2048,
	CMD_DETECT_GET_RSP		= 0x2051,
	CMD_OFFSET_DETECT_RSP,
	CMD_VBL_TIMEOUT_RSP     = 0x2053
} enum_command_t;


// Request Structure ----------------------------------------------------------------------------
typedef struct
{
	u16				cmd;						/* enum_command_t */
	u16				board_id;					/* 1 ~ 4096 */
	u16				len;						/* except for header */
} __PACKED__ msg_req_head_t;

typedef struct
{
	msg_req_head_t	hdr;
} __PACKED__ msg_req_sync_t;

typedef struct
{
	msg_req_head_t	hdr;
} __PACKED__ msg_req_ver_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				ch;							/* select channel (enum_channel_t) ex)CH_1_T|CH_2_T */
	u8				onoff;						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_lcd_onoff_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				ch;   						/* select channel (enum_channel_t) ex)CH_1_T|CH_2_T */
	u8				onoff; 						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_vdd_onoff_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				ch;   						/* select channel (enum_channel_t) ex)CH_1_T|CH_2_T */
	u8				onoff; 						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_vbl_onoff_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				timeout; 						/* timeout 0~65535*/
} __PACKED__ msg_req_vbl_timeout_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				ch;   						/* select channel (enum_channel_t) ex)CH_1_T|CH_2_T */
	u8				onoff; 						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_bl_onoff_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				onoff; 						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_dimm_onoff_t;

typedef struct
{
	msg_req_head_t	hdr;
	u8				ch;   						/* select channel (enum_channel_t) ex)CH_1_T|CH_2_T */
	u8				onoff; 						/* on/off control (enum_onoff_t)*/
} __PACKED__ msg_req_spc_onoff_t;

typedef struct
{
	u16				freq;						/* frequency */
	u8				duty;						/* duty rate 0~100% */
} __PACKED__ pwm_data_t;

typedef struct
{
	u16				freq;						/* frequency */
	u16				duty;						/* duty rate 0~100% */
} __PACKED__ pwm_ensis_data_t;

typedef struct
{
	msg_req_head_t	hdr;
	pwm_data_t		p_dim;						/* pwm dimming */
} __PACKED__ msg_req_pdim_con_t;

typedef struct
{
	msg_req_head_t	hdr;
	pwm_ensis_data_t p_dim;						/* pwm dimming */
} __PACKED__ msg_req_ensis_pdim_con_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				voltage;					/* analog voltage : 0.1V step (5V->50, 0.1V->1, 0.5V->5) */
} __PACKED__ msg_req_adim_con_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				vdd_val;					/* LCD vdd value (5V->50) */
	u8				vdd_l;						/* minus deviation (5V->50) */
	u8				vdd_u;						/* plus deviation (5V->50) */
} __PACKED__ msg_req_vdd_set_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				vdd_val;					/* LCD vdd value (5V->50) */
} __PACKED__ msg_req_ensis_vdd_set_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				vbl_val;					/* Inverter vbl value (0.5V->5) */
	u8				vbl_l;						/* minus deviation (0.5V->5) */
	u8				vbl_u;						/* plus deviation (0.5V->5) */
} __PACKED__ msg_req_vbl_set_t;

typedef struct
{
	msg_req_head_t	hdr;
	u16				vbl_val;					/* Inverter vbl value (0.5V->5) */
} __PACKED__ msg_req_ensis_vbl_set_t;

typedef struct
{
	u8				enable;						/* unload command use or not */
	u16				idd_min;					/* current minimum value */
} __PACKED__ uload_con_t;

typedef struct
{
	u8				error_en;
	u32				error_lvl;					/* error reporting unload command use or not */
	u16				error_ind_delay;
} __PACKED__ err_ind_t;

typedef struct
{
	u16				h_sync;
	u16				h_bp;
	u16				h_dsp;
	u16				h_fp;
	u16				v_sync;
	u16				v_bp;
	u16				v_dsp;
	u16				v_fp;
} __PACKED__ it_timing_t;

typedef struct
{
	msg_req_head_t	hdr;
	/* VDD */
	u16				vdd_val;   					/* LCD vdd value */
	u8				vdd_l;     					/* minus deviation */
	u8				vdd_u;     					/* plus deviation */
	/* VBL */
	u16				vbl_val;   					/* Inverter vbl value */
	u8				vbl_l;     					/* minus deviation */
	u8				vbl_u;    					/* plus deviation */
	/* Lamp select */
	u8				lamp_sel;  					/* 0: Inverter, 1:IP  */
	/* Inverter value */
	u8				inv_val[INV_MAX_CH];   		/* inverter lamp selecting value */
	/* IP select value */
	u8				ip_val[IP_MAX_CH];    		/* IP index value */
	/* PWM dimming */
	pwm_data_t		p_dim;						/* pwm dimming */
	/* Analog dimming */
	u16				adim_vol;  					/* analog voltage */
	/* unload function */
	uload_con_t		unload;     				/* unload command */
	/* Error detecting enable */
	err_ind_t		error_report;

	/* Zone value */
	u8				zone_val;  					/* zone value */
	/* Timing setting */
	it_timing_t		it_tim;     				/* it timing value */
	/* Pll clock value */
	u16				clk_val;    				/* clk value */
	/* singnal mode select */
	u8				sig_mode;   				/* single,dual,quad */
	/* Channel sequence order */
	u8				ch_seq;     				/* channel sequence */
	/* Bits mode */
	u8				bits_mode;					/* 8bits normal, jeida etc ... */
	/* Block mode setting */
	u8				block_mode;					/* mode select BMP or Internal logic */
	/* HVS mode setting */
	u8				hvs_io;						/* HVS model select */
	/* delay time */
	u16				vdd_delay;					/* vdd delay milisecond step*/
	u16				vbl_delay;					/* vbl delay */
	u16				sig_delay;					/* signal delay */
	u16				bl_delay;					/* bl delay */
	u16				vdd_rise_tim[PWR_MAX_CH];	/* vdd rising time per CH */
	u16				vbl_rise_tim[PWR_MAX_CH];	/* vbl rising time per CH */
	u16				idd_max[PWR_MAX_CH];		/* IDD max value per CH */
	u16				ibl_max[PWR_MAX_CH];		/* IBL max value per CH */
	u16				idd_min[PWR_MAX_CH];		/* IDD min value per CH */
	u16				ibl_min[PWR_MAX_CH];		/* IBL min value per CH */
	u16				dimm_delay;					/* dimming delay */
	u8				model_inch;					/* LCD model inch information */
	u8				local_dimming[PWR_MAX_CH];  /* local dimming setting IO */
	u8				l3d_io[PWR_MAX_CH];
} __PACKED__ msg_req_composit_set_t;

typedef struct
{
	msg_req_head_t	hdr;
	/* VDD */
	u16				vdd_val;   					/* LCD vdd value */
	u16				vdd_l;     					/* low limit */
	u16				vdd_u;     					/* high limit */
	/* VBL */
	u16				vbl_val;   					/* Inverter vbl value */
	u16				vbl_l;     					/* low limit */
	u16				vbl_u;    					/* high limit */
	/* PWM dimming */
	pwm_ensis_data_t p_dim;						/* pwm dimming */
	/* Analog dimming */
	u16				adim_vol;  					/* analog voltage */
	u16				idd_max;					/* IDD max value */
	u16				ibl_max;					/* IBL max value */
	u16				idd_min;					/* IDD min value */
	u16				ibl_min;					/* IBL min value */
} __PACKED__ msg_req_ensis_composit_set_t;

typedef struct
{
	msg_req_head_t	hdr;
} __PACKED__ msg_req_detect_get_t;

typedef struct
{
	u8				stx;
	u8				len;						/* packet length */
	u8				type;						/* packet type (0:request) */
	u8				cmd;						/* command */
	u32				file_size;
	u32				check_sum;
	u8				etx;
} __PACKED__ fw_upload_req_t;

typedef struct
{
	msg_req_head_t	hdr;
} __PACKED__ msg_req_offset_t;

// Response Structure ----------------------------------------------------------------------------
typedef struct
{
	u16				cmd;						/* enum_command_t */
	u16				board_id;					/* 1 ~ 4096 */
	u8				res;						// ACK=1, NACK=0
	u16				cause;
	u16				len;
} __PACKED__ msg_rsp_head_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_sync_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	u16				fw_ver;						/* firmware version ex) rev 1.01 -> 101 */
	u16				hw_ver;						/* hardware version ex) rev 1.01 -> 101 */
	u16				prot_ver;					/* protocol version ex) rev 1.01 -> 101 */
	u8				hw_name[MAX_HW_NAME_LEN];
	u8				date[MAX_HW_NAME_LEN];
	u8				time[MAX_HW_NAME_LEN];
	u8				ch_num;						/* channel count for board */
} __PACKED__ msg_rsp_ver_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_lcd_onoff_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_vbl_onoff_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_vdd_onoff_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_bl_onoff_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_dimm_onoff_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_pdim_con_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_adim_con_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_vdd_set_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_vbl_set_t;

typedef struct
{
	msg_rsp_head_t	hdr;
} __PACKED__ msg_rsp_composit_set_t;

typedef struct
{
	u16				vdd;						/* vdd value from ADC ex) 5.00V->500 */
	u16				idd;						/* idd value from ADC ex) 50mA->50 */
	u16				vbl;						/* vbl value from ADC ex) 5.00V->500 */
	u16				ibl;						/* ibl value from ADC ex) 50mA->50 */
} det_adc_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	u16				fw_ver;						/* firmware version ex) rev 1.01 -> 101 */
} __PACKED__ msg_rsp_ensis_ver_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	u16				fw_ver;						/* firmware version ex) rev 1.01 -> 101 */
} __PACKED__ msg_rsp_osung_ver_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	u8				ch_num;
	det_adc_t		det_val[4];
	u8				error[4];					/* enum_detect_error_t */
} __PACKED__ msg_rsp_detect_get_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	det_adc_t		det_val;
	u8				error;						/* enum_detect_error_t */
} __PACKED__ msg_rsp_ensis_detect_get_t;

typedef struct
{
	msg_rsp_head_t	hdr;
	u16				idd_offset;
	u16				ibl_offset;
} __PACKED__ msg_rsp_offset_detect_t;


typedef struct pwr_t_ pwr_t;
struct pwr_t_
{
	int   			poll_ndx;
	int   			fd;
	int   			type;						// tcp/udp/uds/serial/
	int   			tag;
	void  			(*on_poll_in )( pwr_t *);
	void  			(*on_poll_out)( pwr_t *);
	void  			(*on_poll_err)( pwr_t *);
	void  			(*on_poll_hup)( pwr_t *);

	char  			devname[MAX_DEV_NAME];		// Device Name
	int   			baud;
	int   			databit;
	int   			stopbit;
	int   			parity;

	void  			(*on_read    )( pwr_t *);
	void  			(*on_writable)( pwr_t *);
	void  			(*on_error   )( pwr_t *, int);
};

// Variables
pwr_t				*pwr_fd;
int					buft_cnt;
int					pwr_upload_flag;
u16					calc_crc;
char				rx_buft[__MAX_BUF_SIZE];
int					bd_idx;

int					version_start, offset_start;
int					version_ack_cnt, offset_ack_cnt;
enum_res_t			version_ack[ENSIS_PWR_CH], version_end[ENSIS_PWR_CH], version_osung_ack, version_osung_end;
enum_res_t			offset_ack[ENSIS_PWR_CH], offset_end[ENSIS_PWR_CH];

msg_rsp_ensis_ver_t		 	rsp_ver_data[ENSIS_PWR_CH];
msg_rsp_osung_ver_t			rsp_ver_osung_data;
msg_rsp_detect_get_t		rsp_detect_osung_data;
msg_rsp_ensis_detect_get_t 	rsp_detect_data[ENSIS_PWR_CH];


// Functions
extern pwr_t 		*pwr_init (void);
extern pwr_t 		*pwr_open( char  *, int   , int, int, int);
extern int      	pwr_write( pwr_t *, char *, int          );
extern int      	pwr_read ( pwr_t *, char *, int          );
extern void     	pwr_close( pwr_t *                       );
extern pwr_t 		*pwr_find_tag( int);
extern int 			pwr_ver_req(u16 bid);
extern int 			pwr_model_set(model_data_t *, int, int);
extern int 			pwr_lcd_onoff_set(enum_onoff_t);
extern int 			pwr_vdd_onoff_set(enum_onoff_t);
extern int 			pwr_vbl_onoff_set(enum_onoff_t);
extern int 			pwr_bl_onoff_set(enum_onoff_t);
extern int 			pwr_pdimm_control_set(pwm_data_t, int, int);
extern int 			pwr_adimm_control_set(u16, int);
extern int 			pwr_dimm_onoff_set(enum_onoff_t);
extern int 			pwr_vdd_set(u16, int);
extern int 			pwr_vbl_set(u16, int);
extern int			pwr_vbl_enable_timeout(void);
extern int 			pwr_detect_get(u16);
extern int 			pwr_fw_upload(pwr_t *);
extern int 			pwr_offset_req(u16);
extern int 			pwr_spc_onoff_set(enum_onoff_t);



extern void 		on_pwr_error(pwr_t *sender, int reopen_ok);
extern void 		on_pwr_recv (pwr_t *sender);

extern void			std_power_sequence(req_std_pwr_seq_test_t *);
extern void			std_ocmd_power_sequence(req_std_ocmd_pwr_seq_test_t *);
extern int			std_power_seq_i2c_set(req_std_pwr_seq_test_t *);
extern int			std_power_seq_ocmd_set(req_std_ocmd_pwr_seq_test_t *);
extern void			std_power_onoff_set(uint16_t id, uint16_t ch, uint16_t onoff);
extern void			std_power_volt_curr_set(req_ip_pwr_onoff_t *);
extern void			std_power_slew_set(uint16_t, uint16_t, uint32_t);

extern void			std_edp_power_sequence(req_std_aux_i2c_pwr_seq_test_t *);
extern void			std_edp_ocmd_power_sequence(req_std_aux_ocmd_pwr_seq_test_t *);
extern int			std_edp_power_seq_i2c_set(req_std_aux_i2c_pwr_seq_test_t *);
extern int			std_edp_power_seq_ocmd_set(req_std_aux_ocmd_pwr_seq_test_t *);

#endif /* OSUNG_PWR_H_ */
