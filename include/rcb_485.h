/*
 * rcb_485.h
 *
 *  Created on: Mar 7, 2018
 *      Author: root
 */

#ifndef RCB_485_H_
#define RCB_485_H_

#include <global.h>
#include <netinc.h>
#include <pollmanager.h>
#include <model_data.h>

#define	DEV_RCB485			"/dev/ttyTHS3"
#define RCB485_BAUD_RATE	38400

#ifndef MAX_CRC_TABLE
#define MAX_CRC_TABLE		256
#endif

#ifndef STX
#define STX					0x7F
#endif
#ifndef ETX
#define ETX					0x7E
#endif
#ifndef DLE
#define DLE					0x7D
#endif
#ifndef STF
#define STF					0x20
#endif

typedef char 				s8;
typedef int16_t 			s16;
typedef int32_t 			s32;
typedef int64_t 			s64;
typedef uint8_t				u8;
typedef uint16_t 			u16;
typedef uint32_t 			u32;
typedef uint64_t 			u64;

typedef struct rcb485_t_ rcb485_t;
struct rcb485_t_
{
	int   			poll_ndx;
	int   			fd;
	int   			type;						// tcp/udp/uds/serial/
	int   			tag;
	void  			(*on_poll_in )( rcb485_t *);
	void  			(*on_poll_out)( rcb485_t *);
	void  			(*on_poll_err)( rcb485_t *);
	void  			(*on_poll_hup)( rcb485_t *);

	char  			devname[MAX_DEV_NAME];		// Device Name
	int   			baud;
	int   			databit;
	int   			stopbit;
	int   			parity;

	void  			(*on_read    )( rcb485_t *);
	void  			(*on_writable)( rcb485_t *);
	void  			(*on_error   )( rcb485_t *, int);
};

// Request Structure ----------------------------------------------------------------------------
typedef struct
{
	u16						cmd;						/* enum_command_t */
	u16						board_id;					/* 1 ~ 4096 */
	u16						len;						/* except for header */
} __PACKED__ req_rcb485_head_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	char					model_name[MAX_MODEL_NAME];
} __PACKED__ req_rcb485_model_sel_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	uint16_t				on_off;
	uint16_t				pat_num;
	uint16_t				type;	// 0:auto, 1:manual
} __PACKED__ req_rcb485_lcd_onoff_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	uint16_t				type;
	uint16_t				pat_num;
} __PACKED__ req_rcb485_pat_disp_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	uint16_t				type;	// 0:diable, 1:enable
	uint32_t				freq;
} __PACKED__ req_rcb485_freq_set_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	uint16_t				type;	// 0:diable, 1:enable
	uint16_t				red;
	uint16_t				green;
	uint16_t				blue;
} __PACKED__ req_rcb485_color_set_t;

typedef struct
{
	req_rcb485_head_t		hdr;
	uint8_t					type;	// 0:diable, 1:enable
	uint8_t					dir;
	uint16_t				x;
	uint16_t				y;
} __PACKED__ req_rcb485_pos_set_t;


// Response Structure ----------------------------------------------------------------------------
typedef struct
{
	u16				cmd;						/* enum_command_t */
	u16				board_id;					/* 1 ~ 4096 */
	u8				res;
	u16				cause;
	u16				len;
} __PACKED__ msg_rcb485_rsp_head_t;


// Variables
rcb485_t			*rcb485_fd;
int					rcb485_buft_cnt;
u16 				calc_crc_rs485;
char				rcb485_buft[__MAX_BUF_SIZE];


// Functions
extern rcb485_t 	*rcb485_init (void);
extern rcb485_t 	*rcb485_open( char    *, int   , int, int, int);
extern int      	rcb485_write( rcb485_t *, char *, int          );
extern int      	rcb485_read ( rcb485_t *, char *, int          );
extern void     	rcb485_close( rcb485_t *                       );
extern rcb485_t 	*rcb485_find_tag( int);
extern int 			rcb485_write( rcb485_t *_sock, char *_buf, int _buf_size);

extern void 		on_rcb485_error(rcb485_t *sender, int reopen_ok);
extern void 		on_rcb485_recv (rcb485_t *sender);

#endif /* RCB_485_H_ */
