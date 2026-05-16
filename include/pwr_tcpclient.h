/*
 ============================================================================
 Name        : tcpclient.h
 Author      : jschoi
 Version     : 1.0
 Copyright   : copyright (c) 2017 ensis.co.,Ltd.
 Description : C, Ansi-style
 ============================================================================
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <global.h>
#include "msg_comu.h"

#define	DEF_TCP_PORT		5025	//keysight tcp port

#define DEF_TIME_OUT		1000

#define	MAX_PROC_CNT		10
#define	MAX_HEADER_SIZE		4

#define MAX_FILE_CNT		1024
#define MIDDLETEXT 			128



#define RES_ID(x)			(x+0x1000)


typedef struct tcpclient_t_ tcpclient_t;
struct tcpclient_t_
{
	int   					poll_ndx;
	int   					fd;
	int   					type;									// tcp/udp/uds/serial/
	int   					tag;
	void  					(*on_poll_in )(tcpclient_t *);
	void  					(*on_poll_out)(tcpclient_t *);
	void  					(*on_poll_err)(tcpclient_t *);
	void  					(*on_poll_hup)(tcpclient_t *);

	void  					(*on_read    )(tcpclient_t *);
	void  					(*on_writable)(tcpclient_t *);
	void  					(*on_error   )(tcpclient_t *, int);

	char					ip_addr[MAX_IP_ADDR];
	int						port;
	int						time_out;								// second
};


typedef struct _FILE_INFO_DATA{
	unsigned short 			type;
	char 					name[MIDDLETEXT];
	unsigned int 			size;
} __PACKED__ FILE_INFO_DATA;

typedef struct _DIR_DATA{
	unsigned int 			cnt;
	FILE_INFO_DATA 			f[MAX_FILE_CNT];
} __PACKED__ DIR_DATA;

typedef struct _DISK_DATA{
	unsigned int			disk_size;
	unsigned int			free_size;
} __PACKED__ DISK_DATA;

// Variables
tcpclient_t 				*tcp_pwr1_fd;
tcpclient_t 				*tcp_pwr2_fd;

int							master;
//int							pwr_onoff_status;	// jschoi 2019.10.25( 0:off, 1:on)
DIR_DATA 					path_dir_data;
FILE_INFO_DATA 				finfo;


// Functions
extern tcpclient_t 			*tcpclient_open (char 		*, int	 , int	);
extern int      			tcpclient_send (tcpclient_t *, char *, int	);
extern int      			tcpclient_recv (tcpclient_t *, char *, int	);
extern void     			tcpclient_close(tcpclient_t *				);
extern tcpclient_t 			*tcpclient_find_tag(int);
extern int 					wait_ack(tcpclient_t *sender, msg_data_t *msg, int delay);
extern int					wait_ack_qems_data(tcpclient_t *sender, msg_data_t *msg, int delay);
extern void recv_data_proc(char *ptr);

extern void 				on_tcp_error_pwr1(tcpclient_t *sender, int reopen_ok);
extern void 				on_tcp_error_pwr2(tcpclient_t *sender, int reopen_ok);
extern void 				on_tcp_recv_pwr1(tcpclient_t *sender);
extern void 				on_tcp_recv_pwr2(tcpclient_t *sender);

extern int					pwr_ctrl_onoff(uint16_t, uint16_t, uint16_t);
extern int					pwr_ctrl_vol_cur(uint16_t, uint16_t, uint16_t, uint16_t);
extern int					pwr_ctrl_slew(uint16_t, uint16_t, uint32_t);

extern int					check_output_channel_count(uint16_t id);


#endif /* TCPCLIENT_H_ */
