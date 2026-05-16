/*
 ============================================================================
 Name        : tcpclient.c
 Author      : jschoi
 Version     : 1.0
 Copyright   : copyright (c) 2017 ensis.co.,Ltd.
 Description : C, Ansi-style
 ============================================================================
 */

#include <pwr_tcpclient.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdlib.h>
//#include <udpclient.h>
#include <netinet/tcp.h>
#include <netinc.h>
#include <pollmanager.h>



static void on_tcpclient_poll_in(tcpclient_t *sender);
static void on_tcpclient_poll_out(tcpclient_t *sender);
static void on_tcpclient_poll_err(tcpclient_t *sender);
static void on_tcpclient_poll_hup(tcpclient_t *sender);


static tcpclient_t *new_socket(void)
{
	tcpclient_t *sock;

	sock  = (tcpclient_t *)malloc(sizeof(tcpclient_t));
	if ( NULL != sock)
	{
		sock->fd              	= -1;
		sock->tag             	= 0;
		sock->type            	= STYP_TCPIP;
		sock->on_poll_in      	= on_tcpclient_poll_in;
		sock->on_poll_out     	= on_tcpclient_poll_out;
		sock->on_poll_err     	= on_tcpclient_poll_err;
		sock->on_poll_hup    	= on_tcpclient_poll_hup;
		sock->on_read         	= NULL;
		sock->on_writable     	= NULL;

		memset(sock->ip_addr, 0, MAX_IP_ADDR);
		sock->port				= DEF_TCP_PORT;
		sock->time_out			= DEF_TIME_OUT;
	}

	return  sock;
}


static int socket_open(tcpclient_t *_sock)
{
	struct sockaddr_in 	addr;
	struct timeval 		tv;
	fd_set 				fdset;
	long				arg;
	int					res = 0;
	int					valopt;
	socklen_t			len;

	memset(&addr, 0, sizeof(addr));

	addr.sin_family 		= AF_INET;
	addr.sin_addr.s_addr 	= inet_addr(_sock->ip_addr);
	addr.sin_port 			= htons(_sock->port);

	_sock->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (0 > _sock->fd)
	{
		fprintf(stderr, "error creating socket (%d %s)\n", errno, strerror(errno));
		return NERR_NOT_OPENED;
	}

	//setsockopt(_sock->fd, SOL_SOCKET, SO_REUSEADDR, &valopt, sizeof(valopt));

	if ( (arg = fcntl(_sock->fd, F_GETFL, NULL)) < 0 )
	{
		close(_sock->fd);
		fprintf(stderr, "error fcntl(F_GETFL) %s\n", strerror(errno));
		return NERR_NOT_OPENED;
	}

	arg |= O_NONBLOCK;

	if ( fcntl(_sock->fd, F_SETFL, arg) < 0 )
	{
		close(_sock->fd);
		fprintf(stderr, "error fcntl(F_SETFL) %s\n", strerror(errno));
		return NERR_NOT_OPENED;
	}

	res = connect(_sock->fd, (struct sockaddr *)&addr, sizeof(addr));
	if(res<0)
	{
		if(errno == EINPROGRESS)
		{
//			fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
			do
			{
				tv.tv_sec 	= _sock->time_out;
				tv.tv_usec 	= 0;
				FD_ZERO(&fdset);
				FD_SET(_sock->fd, &fdset);

				res = select(_sock->fd + 1, NULL, &fdset, NULL, &tv);
				if( (res<0) && (errno!=EINTR) )
				{
					close(_sock->fd);
					fprintf(stderr, "error connecting %d - %s\n", errno, strerror(errno));
					return NERR_NOT_CONNECT;
				}
				else if(res > 0)
				{
					len = sizeof(int);
					if( getsockopt(_sock->fd, SOL_SOCKET, SO_ERROR, (void*)&valopt, &len) < 0 )
					{
						close(_sock->fd);
						fprintf(stderr, "error in getsockopt() %d - %s\n", errno, strerror(errno));
						return NERR_NOT_OPENED;
					}

					if(valopt)
					{
						close(_sock->fd);
//						fprintf(stderr, "error in delayed connection() %d - %s\n", valopt, strerror(valopt));
						return NERR_NOT_OPENED;
					}
					break;
				}
				else	// res == 0
				{
					close(_sock->fd);
//					fprintf(stderr, "timeout in select() canceling\n");
					return NERR_NOT_OPENED;
				}
			} while(1);
		}
		else
		{
			close(_sock->fd);
			fprintf(stderr, "error connecting %d - %s\n", errno, strerror(errno));
			return NERR_NOT_CONNECT;
		}
	}

	if ( (arg = fcntl(_sock->fd, F_GETFL, NULL)) < 0 )
	{
		close(_sock->fd);
		fprintf(stderr, "error fcntl(F_GETFL) %s\n", strerror(errno));
		return NERR_NOT_OPENED;
	}

	arg &= (~O_NONBLOCK);

	if ( fcntl(_sock->fd, F_SETFL, arg) < 0 )
	{
		close(_sock->fd);
		fprintf(stderr, "error fcntl(F_SETFL) %s\n", strerror(errno));
		return NERR_NOT_OPENED;
	}

	return NERR_NONE;
}


static void socket_close(tcpclient_t *_sock)
{
	close(_sock->fd);
	_sock->fd = -1;
}


static int socket_get_poll_ndx(int _fd)
{
	int     cnt_objs;
	int     ndx;

	cnt_objs = poll_count();

	for (ndx = 0; ndx < cnt_objs; ndx++)
	{
		if (_fd == poll_get_fd(ndx))
		{
			return ndx;
		}
	}

	return  -1;
}


static int socket_reopen(tcpclient_t *_sock)
{
	int   ndx_poll;
	int   reopen_ok;

	ndx_poll = socket_get_poll_ndx(_sock->fd);
	socket_close(_sock);
	reopen_ok = socket_open(_sock);
	if (NERR_NONE == reopen_ok)
	{
		poll_set_fd( ndx_poll, _sock->fd);
	}

	return (NERR_NONE == reopen_ok);
}


static void on_tcpclient_poll_err(tcpclient_t *sender)
{
	if ( socket_reopen( sender))
	{
		if ( NULL != sender->on_error)
		{
			sender->on_error(sender, 0);
		}
	}
	else
	{
		if ( NULL != sender->on_error)
		{
			sender->on_error(sender, -1);
		}
		tcpclient_close(sender);
	}
}


static void on_tcpclient_poll_hup(tcpclient_t *sender)
{
}


static void on_tcpclient_poll_in(tcpclient_t *sender)
{
	int   sz_read;

	if (NULL == sender->on_read)
	{
		sz_read = recv(sender->fd, __tcp_sock_buf, __MAX_BUF_SIZE, 0);
		if (0 >= sz_read)
		{
			if (NULL != sender->on_error)
			{
				on_tcpclient_poll_err(sender);
			}
		}
	}
	else
	{
		sender->on_read(sender);
	}
}


void on_tcpclient_poll_out(tcpclient_t *sender)
{
	if (NULL != sender->on_writable)
	{
		sender->on_writable(sender);
	}
}


tcpclient_t  *tcpclient_open(char *_ip_addr, int _port, int _time_out)
{
	tcpclient_t *sock;
	master = -1;

	neterr_no = NERR_NONE;

    sock      = new_socket();
	if (NULL == sock)
	{
		neterr_no	= NERR_NO_MEMORY;
		return sock;
	}

	strcpy(sock->ip_addr, _ip_addr);
	sock->port		= _port;
	sock->time_out	= _time_out;

	if (NERR_NONE != socket_open(sock) )
	{
		neterr_no	= NERR_NOT_OPENED;
		free(sock);
		return NULL;
	}

	if (0 > poll_register((net_obj_t *)sock))
	{
		neterr_no	= NERR_NO_POLL;
		free(sock);
		return NULL;
	}

	return sock;
}


int tcpclient_recv(tcpclient_t *_sock, char *_buf, int _buf_size)
{
	int   sz_read;

	memset(_buf, 0, (size_t)_buf_size);
	sz_read   = recv(_sock->fd, _buf, _buf_size, 0);

	if (0 >= sz_read)
	{
		on_tcpclient_poll_err(_sock);
	}

	return sz_read;
}


int tcpclient_send(tcpclient_t *_sock, char *_buf, int _buf_size)
{
	//int size;

	if(_sock->fd<0) return 0;

	//printf("buf_size = %d\n", _buf_size);

	//size = send(_sock->fd, _buf, _buf_size, MSG_DONTWAIT);	// MSG_DONTWAIT

	//printf("size = %d\n", size);

	return send(_sock->fd, _buf, _buf_size, MSG_DONTWAIT|MSG_NOSIGNAL);	// MSG_DONTWAIT;
}


tcpclient_t *tcpclient_find_tag(int _tag)
{
	tcpclient_t	*sock;
	int       	cnt_objs;
	int       	ndx;

	cnt_objs = poll_count();

	for (ndx = 0; ndx < cnt_objs; ndx++)
	{
		sock  = (tcpclient_t *)poll_obj( ndx);
		if (NULL != sock)
		{
			if ((STYP_TCPIP == sock->type) && (_tag == sock->tag))	// STYP_TCPIP
			{
				return sock;
			}
		}
	}

	return  NULL;
}


void tcpclient_close(tcpclient_t *_sock)
{
	socket_close(_sock);

	poll_unregister((net_obj_t *)_sock);
	free(_sock);
}

// when server disconnected, this error occurred!
void on_tcp_error_pwr1(tcpclient_t *sender, int reopen_ok)
{
	if ( 0 == reopen_ok)
	{
		printf("%s reopened.\n", sender->ip_addr);
	}
	else
	{
		fprintf(stderr, "on_tcp_error_pwr1() error!!\n");
		tcp_pwr1_fd = NULL;	// Important!!
	}
}

void on_tcp_error_pwr2(tcpclient_t *sender, int reopen_ok)
{
	if ( 0 == reopen_ok)
	{
		printf("%s reopened.\n", sender->ip_addr);
	}
	else
	{
		fprintf(stderr, "on_tcp_error_pwr2() error!!\n");
		tcp_pwr2_fd = NULL;	// Important!!
	}
}
/*
int wait_ack(tcpclient_t *sender, msg_data_t *msg, int delay)
{
	int     	wait_cnt 	= 0;
	msg_data_t	rmsg 		= {0,};

	msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.

	if( (-1) != msq_send(msg) )
	{
		wait_cnt = 0;
		memset(&rmsg, 0, sizeof(msg_data_t));
		while(1)
		{
			if( (-1) != msq_recv(&rmsg) )
			{
				printf("[TCP_SEND]PWR_CONTROLLER : cmd_id=0x%X, data_len=%d\n", rmsg.header.cmd_id, rmsg.header.data_len);
				tcpclient_send(sender, (char*)&rmsg.header, sizeof(msg_header_t)+rmsg.header.data_len);
				break;
			}
			else
			{
				if( wait_cnt++ > delay )	// delay*10 = msec
				{
					fprintf(stderr, "COMMUNICATOR : cmd_id - 0x%X time out!\n", msg->header.cmd_id);

					res_common_t data_ack = {0,};
					memset(&data_ack, 0, sizeof(res_common_t));
					data_ack.hdr.cmd_id 	= RES_ID(msg->header.cmd_id);
					data_ack.hdr.data_len 	= msg->header.data_len;;
					data_ack.res 			= NACK;

					memset(&rmsg, 0, sizeof(msg_data_t));
					memcpy(&rmsg.header, &data_ack, sizeof(res_common_t));
					tcpclient_send(sender, (char*)&rmsg.header, sizeof(msg_header_t)+msg->header.data_len);

					return NACK;
				}
				usleep(10000);
			}
		}
	}

	return ACK;
}

int wait_ack_qems_data(tcpclient_t *sender, msg_data_t *msg, int delay)
{
	int     	wait_cnt 	= 0;
	int     	ret 	= 0;
	msg_data_t	rmsg 		= {0,};

	msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.

	if( (-1) != msq_send(msg) )
	{
		wait_cnt = 0;
		memset(&rmsg, 0, sizeof(msg_data_t));
		while(1)
		{
			if( (-1) != msq_recv(&rmsg) )
			{
				printf("[TCP_SEND]_PWR_CONTROLLER : QEMS, data_len=%d\n", rmsg.header.data_len);
				ret=tcpclient_send(sender, (char*)&rmsg.buf, rmsg.header.data_len);

				if(ret<=0)
				{
					printf("[TCP_SEND] disconnect link_terminated..\n");
					return DISCONNECT;
				}
				break;
			}
			else
			{
				if( wait_cnt++ > delay )	// delay*10 = msec
				{
					fprintf(stderr, "COMMUNICATOR : cmd_id - 0x%X time out!\n", msg->header.cmd_id);
					msq_send_clean();

//					res_common_t data_ack = {0,};
//					memset(&data_ack, 0, sizeof(res_common_t));
//					data_ack.hdr.cmd_id 	= RES_ID(msg->header.cmd_id);
//					data_ack.hdr.data_len 	= msg->header.data_len;;
//					data_ack.res 			= NACK;
//
//					memset(&rmsg, 0, sizeof(msg_data_t));
//					memcpy(&rmsg.header, &data_ack, sizeof(res_common_t));
//					tcpclient_send(sender, (char*)&rmsg.header, sizeof(msg_header_t)+msg->header.data_len);

					return NACK;
				}
				usleep(10000);
			}
		}
	}

	return ACK;
}

int wait_ack_qems(msg_data_t *msg, int delay)
{
	int     	wait_cnt 	= 0;
	msg_data_t	rmsg 		= {0,};

	msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.

	if( (-1) != msq_send(msg) )
	{
		wait_cnt = 0;
		memset(&rmsg, 0, sizeof(msg_data_t));
		while(1)
		{
			if( (-1) != msq_recv(&rmsg) )
			{
				break;
			}
			else
			{
				if( wait_cnt++ > delay )	// delay*10 = msec
				{
					fprintf(stderr, "COMMUNICATOR : cmd_id - 0x%X time out!\n", msg->header.cmd_id);

//					res_common_t data_ack = {0,};
//					memset(&data_ack, 0, sizeof(res_common_t));
//					data_ack.hdr.cmd_id 	= RES_ID(msg->header.cmd_id);
//					data_ack.hdr.data_len 	= msg->header.data_len;;
//					data_ack.res 			= NACK;
//
//					memset(&rmsg, 0, sizeof(msg_data_t));
//					memcpy(&rmsg.header, &data_ack, sizeof(res_common_t));
//					tcpclient_send(sender, (char*)&rmsg.header, sizeof(msg_header_t)+msg->header.data_len);

					return NACK;
				}
				usleep(10000);
			}
		}
	}

	return ACK;
}
*/

void dir_info_data(char *path)
{
	DIR 			*dir_info;
	struct dirent 	*dir_entry;
	char 			str[MAX_PATH];
	struct stat 	buf;

	memset(&path_dir_data, 0, sizeof(path_dir_data));
	dir_info = opendir(path);
	if(NULL != dir_info)
	{
		while((dir_entry=readdir(dir_info)))
		{
			if(path_dir_data.cnt>=MAX_FILE_CNT)
				break;

			memset(str, 0, sizeof(str));
			sprintf(str, "%s%s", path, dir_entry->d_name);
			lstat(str,&buf);
			switch (buf.st_mode & S_IFMT)
			{
				case S_IFBLK:  printf("block device\n");            break;
				case S_IFCHR:  printf("character device\n");        break;
				case S_IFDIR:
					//printf("directory %s\n",dir_entry->d_name);
					path_dir_data.f[path_dir_data.cnt].type = 0;
					break;
				case S_IFIFO:  printf("FIFO/pipe\n");               break;
				case S_IFLNK:  printf("symlink\n");                 break;
				case S_IFREG:
					//printf("regular file %s |size %ld\n",dir_entry->d_name,(long)buf.st_size);
					path_dir_data.f[path_dir_data.cnt].type = 1;
					break;
				case S_IFSOCK: printf("socket\n");                  break;
				default:       printf("unknown?\n");                break;
			}

			if(path_dir_data.f[path_dir_data.cnt].type==1)
			{
				if(buf.st_size==0)
				{
					if(0==remove(str)) printf("%s size is zero.. delete!\n",str);
					continue;
				}
				strcpy(path_dir_data.f[path_dir_data.cnt].name, dir_entry->d_name);
				path_dir_data.f[path_dir_data.cnt].size = (uint32_t)buf.st_size;
				path_dir_data.cnt++;
			}
			else if(path_dir_data.f[path_dir_data.cnt].type==0)
			{
				strcpy(path_dir_data.f[path_dir_data.cnt].name, dir_entry->d_name);
				path_dir_data.f[path_dir_data.cnt].size = (uint32_t)buf.st_blksize;
				path_dir_data.cnt++;
			}
		}
	}
	closedir(dir_info);
}


void tcpclient_send_big(tcpclient_t *_sock, unsigned short id, unsigned int len, unsigned char *ptr)
{
	msg_header_t packet;
	packet.cmd_id 	= RES_ID(id);
	packet.data_len	= 4;

	send(_sock->fd, &packet.cmd_id, sizeof(msg_header_t), MSG_NOSIGNAL);

	if(len>0) send(_sock->fd, ptr, len, MSG_NOSIGNAL);
}


#define FILE_BUF_SIZE	4096


void on_tcp_recv_pwr1(tcpclient_t *sender)
{
	int     	sz_read;
	char		buf[128];
	int			i;

	msg_data_t	smsg = {0,};

	//seochihong 20180405
	memset(&smsg, 0, sizeof(msg_data_t));

	char dir_path[MAX_PATH];
	unsigned int totalbytes=0;
//	res_common_t data_ack = {0,};

	memset(buf, 0, 128);

	sz_read = tcpclient_recv(sender, buf, sizeof(buf));
	if (0 < sz_read)
	{
		/*/
		printf("[TCP_RX] %d\n", sz_read);
		for(i=0; i<sz_read; i++){
			printf("%02X ", buf[i]);
		}
		printf("\n");
		/*/

		if(pwr1_ch_chk_done==1)
		{
			for(i=0;i<128;i++)
			{
				if( buf[i]>='1' && buf[i]<='9' )
				{
					max_pwr1_ch = buf[i]-'0';
					printf("[TCP_RECV]PCTLR : Power1 output channel count=%d\n", max_pwr1_ch);
					pwr1_ch_chk_done=2;
					break;
				}

				if(i==127)
				{
					printf("[TCP_RECV]PCTLR : Power1 output channel count= not found\n");
				}
			}
		}


//		printf("[TCP_RECV]PWR_CONTROLLER : cmd_id - 0x%X\tdata_length - 0x%X\n", smsg.header.cmd_id, smsg.header.data_len);

//		switch(smsg.header.cmd_id)
//		{
//		case SID_CONN_REQ:
//			{
//				res_head_t data = {0,};
//				data.cmd_id 	= RES_ID(SID_CONN_REQ);
//				data.data_len 	= 0;
//				tcpclient_send(sender, (char*)&data, sizeof(res_head_t));
//			}
//			break;
//		default:
//			break;
//		}
	}
	else
	{
		fprintf(stderr, "tcpclient_recv() error\n");
	}

}

void on_tcp_recv_pwr2(tcpclient_t *sender)
{
	int     	sz_read;
	char		buf[128];
	int			i;

	msg_data_t	smsg = {0,};

	//seochihong 20180405
	memset(&smsg, 0, sizeof(msg_data_t));

	char dir_path[MAX_PATH];
	unsigned int totalbytes=0;
//	res_common_t data_ack = {0,};

	memset(buf, 0, 128);

	sz_read = tcpclient_recv(sender, buf, sizeof(buf));
	if (0 < sz_read)
	{
		/*/
		printf("[TCP_RX] %d\n", sz_read);
		for(i=0; i<sz_read; i++){
			printf("%02X ", buf[i]);
		}
		printf("\n");
		/*/

		if(pwr2_ch_chk_done==1)
		{
			for(i=0;i<128;i++)
			{
				if( buf[i]>='1' && buf[i]<='9' )
				{
					max_pwr2_ch = buf[i]-'0';
					printf("[TCP_RECV]PCTLR : Power2 output channel count=%d\n", max_pwr2_ch);
					pwr2_ch_chk_done=2;
					break;
				}

				if(i==127)
				{
					printf("[TCP_RECV]PCTLR : Power2 output channel count= not found\n");
				}
			}
		}


//		printf("[TCP_RECV]PWR_CONTROLLER : cmd_id - 0x%X\tdata_length - 0x%X\n", smsg.header.cmd_id, smsg.header.data_len);

//		switch(smsg.header.cmd_id)
//		{
//		case SID_CONN_REQ:
//			{
//				res_head_t data = {0,};
//				data.cmd_id 	= RES_ID(SID_CONN_REQ);
//				data.data_len 	= 0;
//				tcpclient_send(sender, (char*)&data, sizeof(res_head_t));
//			}
//			break;
//		default:
//			break;
//		}
	}
	else
	{
		fprintf(stderr, "tcpclient_recv() error\n");
	}

}

int pwr_ctrl_onoff(uint16_t id, uint16_t ch, uint16_t onoff)
{
	char	str[MAX_PATH];

	memset(str, 0, MAX_PATH);

	if(id==ID_PWR1)
	{
		if(tcp_pwr1_fd != NULL)
		{
			if(onoff==1)
			{
				if(max_pwr1_ch>1)
				{
					if( (ch>=0)&&(ch<4) )	sprintf(str, "OUTP 1,(@%d)\r\n", ch+1);
//					else if(data->ch == 4)	sprintf(str, "OUTP 1,(@1:4)\r\n");
					else if(ch==4)			sprintf(str, "OUTP 1,(@1:%d)\r\n", max_pwr1_ch);
				}
				else
				{
					sprintf(str, "OUTP 1\r\n");
				}
			}
			else
			{
				if(max_pwr1_ch>1)
				{
					if( (ch>=0)&&(ch<4) )	sprintf(str, "OUTP 0,(@%d)\r\n", ch+1);
//					else if(data->ch == 4)	sprintf(str, "OUTP 0,(@1:4)\r\n");
					else if(ch==4)			sprintf(str, "OUTP 0,(@1:%d)\r\n", max_pwr1_ch);
				}
				else
				{
					sprintf(str, "OUTP 0\r\n");
				}
			}

			printf("%s", str);
			tcpclient_send(tcp_pwr1_fd, (char*)&str, MAX_PATH);
		}
	}
	else if(id==ID_PWR2)
	{
		if(tcp_pwr2_fd != NULL)
		{
			if(onoff==1)
			{
				if(max_pwr2_ch>1)
				{
					if( (ch>=0)&&(ch<4) )	sprintf(str, "OUTP 1,(@%d)\r\n", ch+1);
//					else if(data->ch == 4)	sprintf(str, "OUTP 1,(@1:4)\r\n");
					else if(ch==4)			sprintf(str, "OUTP 1,(@1:%d)\r\n", max_pwr2_ch);
				}
				else
				{
					sprintf(str, "OUTP 1\r\n");
				}
			}
			else
			{
				if(max_pwr2_ch>1)
				{
					if( (ch>=0)&&(ch<4) )	sprintf(str, "OUTP 0,(@%d)\r\n", ch+1);
//					else if(data->ch == 4)	sprintf(str, "OUTP 0,(@1:4)\r\n");
					else if(ch==4)			sprintf(str, "OUTP 0,(@1:%d)\r\n", max_pwr2_ch);
				}
				else
				{
					sprintf(str, "OUTP 0\r\n");
				}
			}

			printf("%s", str);
			tcpclient_send(tcp_pwr2_fd, (char*)&str, MAX_PATH);
		}
	}
	else
	{

	}

	return ACK;
}

int pwr_ctrl_vol_cur(uint16_t id, uint16_t ch, uint16_t volt, uint16_t curr)
{
	char	str[MAX_PATH];

	memset(str, 0, MAX_PATH);

	if(id==ID_PWR1)
	{
		if(tcp_pwr1_fd != NULL)
		{
			if(max_pwr1_ch>1)
			{
				if( (ch>=0)&&(ch<4) ) {
					if(curr>0)	sprintf(str, "VOLT %.2f,(@%d);CURR:LIM %.2f,(@%d)\r\n", (float)(volt/100.0), ch+1, (float)(curr/100.0), ch+1);
					else		sprintf(str, "VOLT %.2f,(@%d)\r\n", (float)(volt/100.0), ch+1);
				}
				else if(ch==4) {
					if(curr>0)	sprintf(str, "VOLT %.2f,(@1:%d);CURR:LIM %.2f,(@1:%d)\r\n", (float)(volt/100.0), max_pwr1_ch, (float)(curr/100.0), max_pwr1_ch);
					else		sprintf(str, "VOLT %.2f,(@1:%d)\r\n", (float)(volt/100.0), max_pwr1_ch);
				}
			}
			else
			{
				if(curr>0)		sprintf(str, "VOLT %.2f;CURR:LIM %.2f\r\n", (float)(volt/100.0), (float)(curr/100.0));
				else			sprintf(str, "VOLT %.2f\r\n", (float)(volt/100.0));
			}

			printf("%s", str);

			power1_voltage = volt;

			tcpclient_send(tcp_pwr1_fd, (char*)&str, MAX_PATH);
		}
	}
	else if(id==ID_PWR2)
	{
		if(tcp_pwr2_fd != NULL)
		{
			if(max_pwr2_ch>1)
			{
				if( (ch>=0)&&(ch<4) ) {
					if(curr>0)	sprintf(str, "VOLT %.2f,(@%d);CURR:LIM %.2f,(@%d)\r\n", (float)(volt/100.0), ch+1, (float)(curr/100.0), ch+1);
					else		sprintf(str, "VOLT %.2f,(@%d)\r\n", (float)(volt/100.0), ch+1);
				}
				else if(ch==4) {
					if(curr>0)	sprintf(str, "VOLT %.2f,(@1:%d);CURR:LIM %.2f,(@1:%d)\r\n", (float)(volt/100.0), max_pwr2_ch, (float)(curr/100.0), max_pwr2_ch);
					else		sprintf(str, "VOLT %.2f,(@1:%d)\r\n", (float)(volt/100.0), max_pwr2_ch);
				}
			}
			else
			{
				if(curr>0)		sprintf(str, "VOLT %.2f;CURR:LIM %.2f\r\n", (float)(volt/100.0), (float)(curr/100.0));
				else			sprintf(str, "VOLT %.2f\r\n", (float)(volt/100.0));
			}

			printf("%s", str);

			power2_voltage = volt;

			tcpclient_send(tcp_pwr2_fd, (char*)&str, MAX_PATH);
		}
	}
	else
	{

	}

	return ACK;
}

int pwr_ctrl_slew(uint16_t id, uint16_t ch, uint32_t slew)
{
	char	str[MAX_PATH];

	memset(str, 0, MAX_PATH);

	if(id==ID_PWR1)
	{
		if(tcp_pwr1_fd != NULL)
		{
			if(max_pwr1_ch>1)
			{
				if( (ch>=0)&&(ch<4) ) {
					if( slew>0 )	sprintf(str, "VOLT:SLEW %.2f,(@%d)\r\n", (float)(slew/100.0), ch+1);
					else			sprintf(str, "VOLT:SLEW MAX,(@%d)\r\n", ch+1);
				}
				else if(ch == 4) {
					if( slew>0 )	sprintf(str, "VOLT:SLEW %.2f,(@1:%d)\r\n", (float)(slew/100.0), max_pwr1_ch);
					else			sprintf(str, "VOLT:SLEW MAX,(@1:%d)\r\n", max_pwr1_ch);
				}
			}
			else
			{
				if( slew>0 )		sprintf(str, "VOLT:SLEW %.2f\r\n", (float)(slew/100.0));
				else				sprintf(str, "VOLT:SLEW MAX\r\n");
			}

			printf("%s", str);
			tcpclient_send(tcp_pwr1_fd, (char*)&str, MAX_PATH);
		}
	}
	else if(id==ID_PWR2)
	{
		if(tcp_pwr2_fd != NULL)
		{
			if(max_pwr2_ch>1)
			{
				if( (ch>=0)&&(ch<4) ) {
					if( (slew)>0 )	sprintf(str, "VOLT:SLEW %.2f,(@%d)\r\n", (float)(slew/100.0), ch+1);
					else			sprintf(str, "VOLT:SLEW MAX,(@%d)\r\n", ch+1);
				}
				else if(ch==4) {
					if( slew>0 )	sprintf(str, "VOLT:SLEW %.2f,(@1:%d)\r\n", (float)(slew/100.0), max_pwr2_ch);
					else			sprintf(str, "VOLT:SLEW MAX,(@1:%d)\r\n", max_pwr2_ch);
				}
			}
			else
			{
				if( slew>0 )		sprintf(str, "VOLT:SLEW %.2f\r\n", (float)(slew/100.0));
				else				sprintf(str, "VOLT:SLEW MAX\r\n");
			}

			printf("%s", str);
			tcpclient_send(tcp_pwr2_fd, (char*)&str, MAX_PATH);
		}
	}
	else
	{

	}

	return ACK;
}

int check_output_channel_count(uint16_t id)
{
	char	str[MAX_PATH];

	memset(str, 0, MAX_PATH);

	sprintf(str, "SYST:CHAN?\r\n");

	if		( (id==ID_PWR1) && (tcp_pwr1_fd != NULL) )
	{
		tcpclient_send(tcp_pwr1_fd, (char*)&str, MAX_PATH);
		pwr1_ch_chk_done=1;
	}
	else if	( (id==ID_PWR2) && (tcp_pwr2_fd != NULL) )
	{
		tcpclient_send(tcp_pwr2_fd, (char*)&str, MAX_PATH);
		pwr2_ch_chk_done=1;
	}
	else {}

	return ACK;
}

