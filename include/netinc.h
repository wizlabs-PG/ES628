#ifndef __NET_INC_H__
#define __NET_INC_H__
                                  
#define NERR_NONE           0
#define NERR_NO_MEMORY     -1
#define NERR_NOT_OPENED    -2
#define NERR_BIND_FAILURE  -3
#define NERR_NO_POLL       -4
#define NERR_NOT_CONNECT   -5
#define NERR_NO_DEVICE     -6
                                  
#define POLL_MAX_COUNT      20
#define IP_ADDR_SIZE        20

#define __MAX_BUF_SIZE      1024

char    __tcp_sock_buf[__MAX_BUF_SIZE];
char    __pwr_sock_buf[__MAX_BUF_SIZE];
char    __rcb_sock_buf[__MAX_BUF_SIZE];
char    __rcb485_sock_buf[__MAX_BUF_SIZE];

int     neterr_no;

#endif
