/*
============================================================================
 Name        : msg_comu.h
 Author      : jschoi
 Version     : 1.0
 Copyright   : copyright (c) 2017 ensis.co.,Ltd.
 Description : C, Ansi-style
 ============================================================================
 */

#ifndef MSG_COMU_H_
#define MSG_COMU_H_

#include <global.h>

#define MSQ_KEY			8000
#define MAX_MSG_BUF		4096-20 //1024

typedef enum
{
	DEV_NONE,
	DEV_MAIN,
	DEV_COMU,
	DEV_PWRC
} enum_device_t;

typedef struct
{
	unsigned short		cmd_id;
	unsigned short		data_len;
} msg_header_t;

typedef struct
{
	long int			target_type;
	long int			source_type;
	msg_header_t		header;
	unsigned char		buf[MAX_MSG_BUF];
} msg_data_t;

// Functions
extern int 				msq_init(void);
extern int 				msq_send_comm(msg_data_t* pdata);
extern int 				msq_send_pwrc(msg_data_t* pdata);
extern int 				msq_recv(msg_data_t* pdata);
extern int 				msq_clean();
extern int 				msq_close();
extern void 			msg_analyze(void);

extern void 			pwr_version_checking(void);
extern void 			pwr_offset_checking(void);
extern void 			pwr_seq_check(void);

//for test
extern int 				wait_ack_pwrc(msg_data_t *msg, int delay);

// Variables
int 					msq_id;

#endif /* MSG_COMU_H_ */
