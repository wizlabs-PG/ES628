/*
 * rcb_485.c
 *
 *  Created on: Mar 7, 2018
 *      Author: root
 */

#include <rcb_485.h>
#include <i2c_gpio.h>
#include <rcb.h>


static void 			on_rcb485_poll_in( rcb485_t *sender);
static void 			on_rcb485_poll_out( rcb485_t *sender);
static void 			on_rcb485_poll_err( rcb485_t *sender);
static void 			on_rcb485_poll_hup( rcb485_t *sender);

static rcb485_t *new_socket( void)
{
	rcb485_t *sock;

	sock  = (rcb485_t *)malloc( sizeof( rcb485_t));
	if ( NULL != sock)
	{
		sock->fd              = -1;
		sock->tag             = 0;
		sock->type            = STYP_RS232;
		sock->on_poll_in      = on_rcb485_poll_in;
		sock->on_poll_out     = on_rcb485_poll_out;
		sock->on_poll_err     = on_rcb485_poll_err;
		sock->on_poll_hup     = on_rcb485_poll_hup;
		sock->on_read         = NULL;
		sock->on_writable     = NULL;
	}
	return  sock;
}


static int socket_open( rcb485_t *_sock)
{
	struct termios   newtio;

	_sock->fd = open(_sock->devname, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if ( 0 > _sock->fd)
	{
		return NERR_NOT_OPENED;
	}

	memset( &newtio, 0, sizeof(newtio) );

	switch(_sock->baud )
	{
	case 2400   : newtio.c_cflag = B2400  ; break;
	case 4800   : newtio.c_cflag = B4800  ; break;
	case 9600   : newtio.c_cflag = B9600  ; break;
	case 19200  : newtio.c_cflag = B19200 ; break;
	case 38400  : newtio.c_cflag = B38400 ; break;
	case 57600  : newtio.c_cflag = B57600 ; break;
	default     : newtio.c_cflag = B115200; break;
	}

	switch(_sock->databit)
	{
	case 5  : newtio.c_cflag |= CS5; break;
	case 6  : newtio.c_cflag |= CS6; break;
	case 7  : newtio.c_cflag |= CS7; break;
	default : newtio.c_cflag |= CS8;
	}

	if ( 2 == _sock->stopbit)
	{
		newtio.c_cflag |= RS_2_STOP_BIT;
	}

	newtio.c_cflag |= CLOCAL | CREAD;

	switch(_sock->parity | 0x20 )
	{
	case 'o'  : newtio.c_cflag |= RS_ODD_PARITY;   break;
	case 'e'  : newtio.c_cflag |= RS_EVEN_PARITY;  break;
	}

	newtio.c_iflag      = 0;
	newtio.c_oflag      = 0;
	newtio.c_lflag      = 0;
	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN]   = 1;

	tcflush  (_sock->fd, TCIFLUSH );
	tcsetattr(_sock->fd, TCSANOW, &newtio );

	return NERR_NONE;
}


static void  socket_close( rcb485_t *_sock)
{
	close(_sock->fd);
	_sock->fd       = -1;
}


static int  socket_get_poll_ndx( int _fd)
{
	int     cnt_objs;
	int     ndx;

	cnt_objs = poll_count();

	for ( ndx = 0; ndx < cnt_objs; ndx++)
	{
		if ( _fd == poll_get_fd(ndx))
		{
			return ndx;
		}
	}
	return  -1;
}


static int socket_reopen( rcb485_t *_sock)
{
	int   ndx_poll;
	int   reopen_ok;

	ndx_poll = socket_get_poll_ndx( _sock->fd);
	socket_close( _sock);
	reopen_ok = socket_open( _sock);
	if ( NERR_NONE == reopen_ok)
	{
		poll_set_fd( ndx_poll, _sock->fd);
	}
	return NERR_NONE == reopen_ok;
}


static void on_rcb485_poll_err( rcb485_t *sender)
{
	if ( socket_reopen( sender))
	{
		if ( NULL != sender->on_error)
		{
			sender->on_error( sender, 0);
		}
	}
	else
	{
		if ( NULL != sender->on_error)
		{
			sender->on_error( sender, -1);
		}
		rcb485_close( sender);
	}
}


static void on_rcb485_poll_hup( rcb485_t *sender)
{
}


static void on_rcb485_poll_in( rcb485_t *sender)
{
	int   sz_read;

	if ( NULL == sender->on_read)
	{
		sz_read = read( sender->fd, __rcb485_sock_buf, __MAX_BUF_SIZE);
		if ( 0 >= sz_read)
		{
			if ( NULL != sender->on_error)
			{
				on_rcb485_poll_err( sender);
			}
		}
	}
	else
	{
		sender->on_read( sender);
	}
}


void on_rcb485_poll_out( rcb485_t *sender)
{
	if ( NULL != sender->on_writable)
	{
		sender->on_writable( sender);
	}
}

void on_rcb485_error( rcb485_t *sender, int reopen_ok)
{
	if ( 0 == reopen_ok)
	{
		printf("%s reopened.\n", sender->devname);
	}
	else
	{
		fprintf(stderr, "on_rcb485_error() error!!\n");
	}
}


void rcb485_re( rcb485_t *_sock)
{
	int tx_empty_state;
	while(1)
	{
		ioctl(_sock->fd, TIOCSERGETLSR, &tx_empty_state);
		if(tx_empty_state) break;
	}
}


rcb485_t *rcb485_init(void)
{
	rcb485_fd = rcb485_open(DEV_RCB485, RCB485_BAUD_RATE, 8, 'n', 1);
	if(rcb485_fd == NULL)
	{
		fprintf(stderr, "rcb485_open() failed!(%d)\n", neterr_no);
	}
	else
	{
		printf("%s rcb485 connected!\n", DEV_RCB485);
		rcb485_fd->on_read 	= on_rcb485_recv;
		rcb485_fd->on_error = on_rcb485_error;

		i2c_pg_485_set(0);	// RTS control : LOW
	}

	return rcb485_fd;
}


rcb485_t *rcb485_open( char *_devname, int _baud, int _databit, int _parity, int _stopbit)
{
	rcb485_t *sock;

	neterr_no = NERR_NONE;

	if( 0 != access(_devname , F_OK ))
	{
		neterr_no = NERR_NO_DEVICE;
		return NULL;
	}

	sock      = new_socket();
	if ( NULL == sock)
	{
		neterr_no   = NERR_NO_MEMORY;
		return sock;
	}

	strcpy( sock->devname, _devname);
	sock->baud		= _baud;
	sock->databit	= _databit;
	sock->parity	= _parity;
	sock->stopbit	= _stopbit;

	if ( NERR_NONE != socket_open( sock) )
	{
		neterr_no   = NERR_NOT_OPENED;
		free(sock);
		return NULL;
	}

	if ( 0 > poll_register( (net_obj_t *)sock))
	{
		neterr_no   = NERR_NO_POLL;
		free( sock);
		return NULL;
	}

	rcb485_buft_cnt = 0;

	return sock;
}


int rcb485_read( rcb485_t *_sock, char *_buf, int _buf_size)
{
	int   sz_read;

	memset(_buf, 0, _buf_size);
	sz_read   = read(_sock->fd, _buf, _buf_size);

	if ( 0 >= sz_read)
	{
		on_rcb485_poll_err(_sock);
	}
	return sz_read;
}


int rcb485_write( rcb485_t *_sock, char *_buf, int _buf_size)
{

	if(NULL == rcb485_fd)
	{
		fprintf(stderr, "rcb485_fd is NULL!\n");
		return 0;
	}

	int		i;
	char	txbuf[__MAX_BUF_SIZE];

	memset(txbuf, 0, __MAX_BUF_SIZE);

	txbuf[0] 			= SRX_RCB;
	memcpy(txbuf+1, _buf, _buf_size);
	txbuf[_buf_size+1] 	= ERX_RCB;

	// test
	printf("[RCB485 TX] ");
	for(i=0; i<_buf_size+2; i++)
	{
		printf("%02X ", txbuf[i]);
	}
	printf("\n");
	//

	i2c_pg_485_set(1);	// RTS control : HIGH
	usleep(100);

	if(0==write(_sock->fd, txbuf, _buf_size+2)){}

	rcb485_re(_sock);
	i2c_pg_485_set(0);	// RTS control : LOW

	return 1;
}


rcb485_t *rcb485_find_tag( int _tag)
{
	rcb485_t  *sock;
	int       cnt_objs;
	int       ndx;

	cnt_objs = poll_count();

	for ( ndx = 0; ndx < cnt_objs; ndx++)
	{
		sock  = ( rcb485_t *)poll_obj( ndx);
		if ( NULL != sock)
		{
			if ( ( STYP_RS232 == sock->type) && (_tag == sock->tag))
			{
				return sock;
			}
		}
	}
	return  NULL;
}


void rcb485_close( rcb485_t *_sock)
{
	socket_close( _sock);

	poll_unregister( (net_obj_t *)_sock);
	free(_sock);
}


void on_rcb485_recv( rcb485_t *sender)
{

	int				sz_read, i, j;
	char			rx_buf[__MAX_BUF_SIZE];

	memset(rx_buf, 0, __MAX_BUF_SIZE);

	sz_read = rcb485_read(sender, rx_buf, __MAX_BUF_SIZE);
	if (0 < sz_read)
	{
		for(i=0; i<sz_read; i++)
		{
			if(rx_buf[i] == SRX_RCB)
			{
				rcb485_buft_cnt = 0;
				memset(rcb485_buft, 0, __MAX_BUF_SIZE);
				rcb485_buft[rcb485_buft_cnt++] = rx_buf[i];
			}
			else if(rx_buf[i] == ERX_RCB)
			{
				rcb485_buft[rcb485_buft_cnt++] = rx_buf[i];

				// test
				printf("[RCB485 RX] ");
				for(j=0; j<rcb485_buft_cnt; j++)
				{
					printf("%02X ", rx_buf[j]);
				}
				printf("\n");
				//

				rcb_proc(rcb485_buft+1);

				// initialize
				rcb485_buft_cnt = 0;
				memset(rcb485_buft, 0, __MAX_BUF_SIZE);
			}
			else
			{
				rcb485_buft[rcb485_buft_cnt++] = rx_buf[i];
			}
		}
	}
	else
	{
		fprintf(stderr, "on_rcb485_recv() error\n");
	}
}

