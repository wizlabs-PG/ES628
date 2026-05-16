/*
 * osung_pwr.c
 *
 *  Created on: Oct 24, 2017
 *      Author: root
 */

#include <global.h>
#include <pwr_control.h>
#include <i2c_gpio.h>
#include <rcb.h>
#include <pattern_control.h>
#include <msg_comu.h>

static unsigned short crc_table[MAX_CRC_TABLE] = {0x0000,
	0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
	0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
	0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D,
	0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
	0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9,
	0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
	0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5,
	0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
	0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
	0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
	0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,
	0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
	0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9,
	0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
	0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176,
	0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
	0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
	0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
	0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D,
	0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
	0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A,
	0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
	0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5,
	0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
	0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
	0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
	0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E,
	0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
	0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9,
	0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
	0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5,
	0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
	0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
	0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
	0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E,
	0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
	0x820D, 0x8207, 0x0202
};


static void 			on_pwr_poll_in( pwr_t *sender);
static void 			on_pwr_poll_out( pwr_t *sender);
static void 			on_pwr_poll_err( pwr_t *sender);
static void 			on_pwr_poll_hup( pwr_t *sender);
static u16			 	crc16(u8 *pdata, size_t data_size);

static int 				check_write_data( u8 *psrc, u8 *pdest, size_t src_size);
static int 				check_read_data ( u8 *psrc, u8 *pdest);


static pwr_t *new_socket( void)
{
	pwr_t *sock;

	sock  = (pwr_t *)malloc( sizeof( pwr_t));
	if ( NULL != sock)
	{
		sock->fd              = -1;
		sock->tag             = 0;
		sock->type            = STYP_RS232;
		sock->on_poll_in      = on_pwr_poll_in;
		sock->on_poll_out     = on_pwr_poll_out;
		sock->on_poll_err     = on_pwr_poll_err;
		sock->on_poll_hup     = on_pwr_poll_hup;
		sock->on_read         = NULL;
		sock->on_writable     = NULL;
	}
	return  sock;
}


static int socket_open( pwr_t *_sock)
{
	struct termios   newtio;
/*

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
*/

//-------------------------------------------------------------------------------

	_sock->fd = open(_sock->devname, O_RDWR | O_NOCTTY | O_NONBLOCK );		//O_NONBLOCK = O_NDELAY
	if ( 0 > _sock->fd)
	{
		return NERR_NOT_OPENED;
	}

	fcntl(_sock->fd, F_SETFL, 0);

	tcgetattr(_sock->fd, &newtio);

	cfsetspeed(&newtio, _sock->baud);	//input output both

	newtio.c_cflag |= CLOCAL | CREAD;

	newtio.c_cflag &= ~PARENB;
	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8;

	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
	newtio.c_iflag |= IGNPAR;

	newtio.c_oflag &= ~OPOST;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	newtio.c_cc[VMIN] = 255;
	newtio.c_cc[VTIME] = 1;

	/* Protocol Receive Issue (0x0D turned into 0x0A automatically) 2024.05.22 KSK */
	newtio.c_iflag &= ~(OCRNL|ONLCR);
	newtio.c_oflag &= ~(ICRNL|INLCR);
	/*																	*/

	tcsetattr(_sock->fd, TCSAFLUSH, &newtio);

	sleep(1);
	tcflush(_sock->fd, TCIOFLUSH);

	return NERR_NONE;
}


static void  socket_close( pwr_t *_sock)
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


static int socket_reopen( pwr_t *_sock)
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


static void on_pwr_poll_err( pwr_t *sender)
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
		pwr_close( sender);
	}
}


static void on_pwr_poll_hup( pwr_t *sender)
{
}


static void on_pwr_poll_in( pwr_t *sender)
{
	int   sz_read;

	if ( NULL == sender->on_read)
	{
		sz_read = read( sender->fd, __pwr_sock_buf, __MAX_BUF_SIZE);
		if ( 0 >= sz_read)
		{
			if ( NULL != sender->on_error)
			{
				on_pwr_poll_err( sender);
			}
		}
	}
	else
	{
		sender->on_read( sender);
	}
}


void on_pwr_poll_out( pwr_t *sender)
{
	if ( NULL != sender->on_writable)
	{
		sender->on_writable( sender);
	}
}


void on_pwr_error( pwr_t *sender, int reopen_ok)
{
	if ( 0 == reopen_ok)
	{
		printf("%s reopened.\n", sender->devname);
	}
	else
	{
		fprintf(stderr, "on_pwr_error() error!!\n");
	}
}

// check buffer
void rs485_re( pwr_t *_sock)
{
	int tx_empty_state;
	while(1)
	{
		ioctl(_sock->fd, TIOCSERGETLSR, &tx_empty_state);
		if(tx_empty_state) break;
	}
}


pwr_t *pwr_init(void)
{
	pwr_fd = pwr_open(DEV_POWBD, POWBD_BAUD_RATE, 8, 'n', 1);
	if(pwr_fd == NULL)
	{
		fprintf(stderr, "pwr_open() failed!(%d)\n", neterr_no);
	}
	else
	{
		printf("%s pwr connected!\n", DEV_POWBD);
		pwr_fd->on_read 	= on_pwr_recv;
		pwr_fd->on_error 	= on_pwr_error;

		set_pwr_vendor((int)get_pwr_sel());
		if(get_pwr_vendor()==0)
		{
			rs485_re(pwr_fd);
			i2c_pwr_485_set(0);	// RTS control : LOW
		}
	}

	return pwr_fd;
}


pwr_t *pwr_open( char *_devname, int _baud, int _databit, int _parity, int _stopbit)
{
	pwr_t *sock;
	int i;

	neterr_no = NERR_NONE;

	if( 0 != access(_devname , F_OK ))
	{
		neterr_no = NERR_NO_DEVICE;
		return NULL;
	}

	sock = new_socket();
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

	buft_cnt 		= 0;
	version_start	= 0;
	offset_start	= 0;
	for(i=0; i<ENSIS_PWR_CH; i++) version_ack[i] = RES_NACK;
	version_osung_ack = RES_NACK;

	return sock;
}


int pwr_read( pwr_t *_sock, char *_buf, int _buf_size)
{
	int   sz_read;

	memset(_buf, 0,_buf_size);
	sz_read   = read(_sock->fd, _buf, _buf_size);

	if ( 0 >= sz_read)
	{
		on_pwr_poll_err(_sock);
	}
	return sz_read;
}


int pwr_write( pwr_t *_sock, char *_buf, int _buf_size)
{
	int		total_len 	= 0;
	int		i;
	char	crc_buf[__MAX_BUF_SIZE], txbuf[__MAX_BUF_SIZE];

	memset(crc_buf, 0, __MAX_BUF_SIZE);
	memset(txbuf, 0, __MAX_BUF_SIZE);

	total_len = check_write_data((unsigned char *)_buf, (unsigned char *)crc_buf, (size_t)_buf_size);

	txbuf[0] 			= STX;
	memcpy(txbuf+1, crc_buf, total_len);
	txbuf[total_len+1] 	= ETX;

	/*/ test
	printf("[PWR TX] ");
	for(i=0; i<total_len+2; i++)
	{
		printf("%02X ", txbuf[i]);
	}
	printf("\n");
	/*/

	if(get_pwr_vendor()==0)	i2c_pwr_485_set(1);	// RTS control : HIGH

	if(0==write(_sock->fd, txbuf, total_len+2)){}

	if(get_pwr_vendor()==0)
	{
		rs485_re(_sock);
		i2c_pwr_485_set(0);	// RTS control : LOW
	}

	return RES_ACK;
}


pwr_t *pwr_find_tag( int _tag)
{
	pwr_t  *sock;
	int       cnt_objs;
	int       ndx;

	cnt_objs = poll_count();

	for ( ndx = 0; ndx < cnt_objs; ndx++)
	{
		sock  = ( pwr_t *)poll_obj( ndx);
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


void pwr_close( pwr_t *_sock)
{
	socket_close( _sock);

	poll_unregister( (net_obj_t *)_sock);
	free(_sock);
}


int pwr_ver_req(u16 bid)
{
	int					len;

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	msg_req_ver_t	data = {0,};

	len = sizeof(msg_req_ver_t);

	if(get_pwr_vendor()==0)
	{
		memset(&data, 0, len);
		data.hdr.cmd 		= htons(CMD_VER_REQ);
		data.hdr.board_id 	= htons(bid);
		data.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		version_ack[bid-1]	= RES_NACK;
	}
	else
	{
		memset(&data, 0, len);
		data.hdr.cmd 		= CMD_VER_REQ;
		data.hdr.board_id 	= OSUNG_PWRBD_ID;
		data.hdr.len 		= len-sizeof(msg_req_head_t);

		version_osung_ack = RES_NACK;
	}

	version_ack_cnt = 0;

	return pwr_write(pwr_fd, (char*)&data, len);
}


int pwr_model_set(model_data_t *model, int flag, int sch_en)
{
	int		len, i;
	int		flag_vdd,flag_vbl,flag_pdim_freq,flag_pdim_duty,flag_adim;
	u16		adim_voltage;

	printf("adim_change_flag=%d\n", adim_change_flag);

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	if(sch_en)
	{
		flag_vdd = schedule_data_new.schedule_info[schedule_index].vdd_mode;
		flag_vbl = schedule_data_new.schedule_info[schedule_index].vbl_mode;
		flag_pdim_freq = schedule_data_new.schedule_info[schedule_index].pdim_freq_mode;
		flag_pdim_duty = schedule_data_new.schedule_info[schedule_index].pdim_duty_mode;
		flag_adim = schedule_data_new.schedule_info[schedule_index].adim_mode;
	}
	else
	{
		flag_vdd = flag;
		flag_vbl = flag;
		flag_pdim_freq = flag;
		flag_pdim_duty = flag;
		flag_adim = flag;
	}

	if(get_pwr_vendor()==0)
	{
		msg_req_ensis_composit_set_t 	comp = {0,};

		len = sizeof(msg_req_ensis_composit_set_t);

		memset(&comp, 0, len);

		comp.hdr.cmd 		= htons(CMD_COMPOSIT_SET_REQ);
		comp.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		comp.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		switch(flag_vdd)
		{
		case 1:
//			comp.vdd_val	= htons(model->vdd + (model->vdd*0.1));	// + 10%
			comp.vdd_val	= htons(var_data.vdd + (var_data.vdd*0.1));	// + 10%
			break;
		case 2:
//			comp.vdd_val	= htons(model->vdd - (model->vdd*0.1));	// - 10%
			comp.vdd_val	= htons(var_data.vdd - (var_data.vdd*0.1));	// - 10%
			break;
		default:
//			comp.vdd_val	= htons(model->vdd);
			comp.vdd_val	= htons(var_data.vdd);					// 20-05-20	Keep vdd value when enable schedule mode. Request by sdc
			break;
		}
		printf("vdd set %d	flag %d\n", comp.vdd_val, flag);

		if(sch_en)
		{
			switch(flag_vdd)
			{
			case 1:
				comp.vdd_u			= htons(model->vdd_h + (model->vdd_h*0.2));
				break;
			default:
				comp.vdd_u			= htons(model->vdd_h);
				break;
			}
		}
		else
		{
			comp.vdd_u			= htons(model->vdd_h);
		}
		comp.vdd_l			= htons(model->vdd_l);
//		comp.vdd_u			= htons(model->vdd_h);

		switch(flag_vbl)
		{
		case 1:
			comp.vbl_val	= htons(model->vbl + (model->vbl*0.1));	// + 10%
			break;
		case 2:
			comp.vbl_val	= htons(model->vbl - (model->vbl*0.1));	// - 10%
			break;
		default:
			comp.vbl_val	= htons(model->vbl);
			break;
		}
		printf("vbl set %d	flag %d\n", comp.vbl_val, schedule_data_new.schedule_info[schedule_index].vbl_mode);

		if(sch_en)
		{
			switch(flag_vbl)
			{
			case 1:
				comp.vbl_u			= htons(model->vbl_h + (model->vbl_h*0.2));
				break;
			default:
				comp.vbl_u			= htons(model->vbl_h);
				break;
			}
		}
		else
		{
			comp.vbl_u			= htons(model->vbl_h);
		}

//		comp.vbl_val		= htons(model->vbl);
		comp.vbl_l			= htons(model->vbl_l);
//		comp.vbl_u			= htons(model->vbl_h);

		switch(flag_pdim_freq)
		{
		case 1:
			comp.p_dim.freq		= htons((u16)(model->pwm_freq[0] + (model->pwm_freq[0]*0.1)));	// + 10%
			break;
		case 2:
			comp.p_dim.freq		= htons((u16)(model->pwm_freq[0] - (model->pwm_freq[0]*0.1)));	// - 10%
			break;
		default:
			comp.p_dim.freq		= htons(((u16)model->pwm_freq[0]));
			break;
		}

		switch(flag_pdim_duty)
		{
		case 1:
			comp.p_dim.duty		= htons((u8)(model->pwm_duty[0]+1));	// + 10%
			break;
		case 2:
			comp.p_dim.duty		= htons((u8)(model->pwm_duty[0]-1));	// - 10%
			break;
		default:
			comp.p_dim.duty		= htons(model->pwm_duty[0]);
			break;
		}
		printf("pdim set %d %d	flag %d %d\n", comp.p_dim.freq,comp.p_dim.duty,schedule_data_new.schedule_info[schedule_index].pdim_freq_mode, schedule_data_new.schedule_info[schedule_index].pdim_duty_mode);
//		comp.p_dim.freq		= htons(model->pwm_freq[0]);
//		comp.p_dim.duty		= htons(model->pwm_duty[0]*10);

		switch(flag_adim)
		{
		case 1:
			if((model->vbr[0] + (model->vbr[0]*0.1))>MAX_DIMM_VAL-1) 	adim_voltage = (MAX_DIMM_VAL);
			else														adim_voltage = ((u16)(model->vbr[0] + (model->vbr[0]*0.1)));
			comp.adim_vol = htons(adim_voltage);
			break;
		case 2:
			if((model->vbr[0] - (model->vbr[0]*0.1))<=0) 	adim_voltage = 0;
			else											adim_voltage = ((u16)(model->vbr[0] - (model->vbr[0]*0.1)));
			comp.adim_vol = htons(adim_voltage);
			break;
		default:
			if(adim_change_flag)	adim_voltage = var_data.adim;
			else					adim_voltage = ((u16)model->vbr[0]);
			comp.adim_vol = htons(adim_voltage);
			break;
		}
		printf("adim set %d	flag %d\n", comp.adim_vol, schedule_data_new.schedule_info[schedule_index].adim_mode);
//		comp.adim_vol		= htons(model->vbr[0]);

		comp.idd_max		= htons(model->idd_h);
		comp.ibl_max		= htons(model->ibl_h);
		comp.idd_min		= htons(model->idd_l);
		comp.ibl_min		= htons(model->ibl_l);

		pwr_write(pwr_fd, (char*)&comp, len);
	}
	else
	{
		msg_req_composit_set_t 	comp = {0,};

		len = sizeof(msg_req_composit_set_t);

		memset(&comp, 0, len);

		comp.hdr.cmd 		= CMD_COMPOSIT_SET_REQ;
		comp.hdr.board_id 	= OSUNG_PWRBD_ID;
		comp.hdr.len 		= len-sizeof(msg_req_head_t);

		switch(flag_vdd)
		{
		case 1:
//			comp.vdd_val	= model->vdd + (model->vdd*0.1);	// + 10%
			comp.vdd_val	= var_data.vdd + (var_data.vdd*0.1);	// + 10%
			break;
		case 2:
//			comp.vdd_val	= model->vdd - (model->vdd*0.1);	// - 10%
			comp.vdd_val	= var_data.vdd - (var_data.vdd*0.1);	// - 10%
			break;
		default:
//			comp.vdd_val	= model->vdd;
			comp.vdd_val	= var_data.vdd;						// 20-05-20	Keep vdd value when enable schedule mode. Request by sdc
			break;
		}
		printf("ENSIS 1 vdd set %d	flag %d\n", comp.vdd_val, flag);

		//comp.vdd_l		= model->vdd - model->vdd_l;
		//comp.vdd_u		= model->vdd_h - model->vdd;

//==================================================================================test 19-07-30
		switch(flag_vbl)
		{
		case 1:
			comp.vbl_val	= model->vbl + (model->vbl*0.1);	// + 10%
			break;
		case 2:
			comp.vbl_val	= model->vbl - (model->vbl*0.1);	// - 10%
			break;
		default:
			comp.vbl_val	= model->vbl;
			break;
		}
		printf("vbl set %d	flag %d\n", comp.vbl_val, schedule_data_new.schedule_info[schedule_index].vbl_mode);
//==================================================================================test 19-07-30
//		comp.vbl_val		= model->vbl;

		//comp.vbl_l		= model->vbl - model->vbl_l;
		//comp.vbl_u		= model->vbl_h - model->vbl;

//==================================================================================test 19-07-30
		switch(flag_pdim_freq)
		{
		case 1:
			comp.p_dim.freq		= (u16)(model->pwm_freq[0] + (model->pwm_freq[0]*0.1));	// + 10%
			break;
		case 2:
			comp.p_dim.freq		= (u16)(model->pwm_freq[0] - (model->pwm_freq[0]*0.1));	// - 10%
			break;
		default:
			comp.p_dim.freq		= ((u16)model->pwm_freq[0]);
			break;
		}

		switch(flag_pdim_duty)
		{
		case 1:
			comp.p_dim.duty		= (u8)(model->pwm_duty[0]*10 + (model->pwm_duty[0]*10*0.1));	// + 10%
			break;
		case 2:
			comp.p_dim.duty		= (u8)(model->pwm_duty[0]*10 - (model->pwm_duty[0]*10*0.1));	// - 10%
			break;
		default:
			comp.p_dim.duty		= model->pwm_duty[0]*10;
			break;
		}
		printf("pdim set %d %d	flag %d %d\n", comp.p_dim.freq,comp.p_dim.duty,schedule_data_new.schedule_info[schedule_index].pdim_freq_mode, schedule_data_new.schedule_info[schedule_index].pdim_duty_mode);
//==================================================================================test 19-07-30
//		comp.p_dim.freq		= model->pwm_freq[0];
//		comp.p_dim.duty		= model->pwm_duty[0]*10;


//==================================================================================test 19-07-30
		switch(flag_adim)
		{
		case 1:
			if((model->vbr[0] + (model->vbr[0]*0.1))>MAX_DIMM_VAL-1) 	adim_voltage = MAX_DIMM_VAL;
			else														adim_voltage = (u16)(model->vbr[0] + (model->vbr[0]*0.1));
			comp.adim_vol = adim_voltage;
			break;
		case 2:
			if((model->vbr[0] - (model->vbr[0]*0.1))<=0) 	adim_voltage = 0;
			else											adim_voltage = (u16)(model->vbr[0] - (model->vbr[0]*0.1));
			comp.adim_vol = adim_voltage;
			break;
		default:
			if(adim_change_flag)	adim_voltage = var_data.adim;
			else					adim_voltage = (u16)model->vbr[0];
			comp.adim_vol		= adim_voltage;
			break;
		}
		printf("adim set %d	flag %d\n", comp.adim_vol, schedule_data_new.schedule_info[schedule_index].adim_mode);
//==================================================================================test 19-07-30
//		comp.adim_vol		= model->vbr[0];

		comp.it_tim.h_sync	= (u16)model->h_width;
		comp.it_tim.h_bp	= (u16)model->h_bpo;
		comp.it_tim.h_dsp	= (u16)model->h_active;
		comp.it_tim.h_fp	= (u16)(model->h_total - (model->h_bpo + model->h_width + model->h_active));
		comp.it_tim.v_sync	= (u16)model->v_width;
		comp.it_tim.v_bp	= (u16)model->v_bpo;
		comp.it_tim.v_dsp	= (u16)model->v_active;
		comp.it_tim.v_fp	= (u16)(model->v_total - (model->v_bpo + model->v_width + model->v_active));

		comp.sig_mode		= model->mode & 0x7;

//		for(i=0; i<MAX_SEQ_CNT; i++)
//		{
//			if		(model->on_seq[i]==0) 		comp.vdd_delay = model->on_delay[i];
//			else if	(model->on_seq[i]==1) 		comp.sig_delay = model->on_delay[i];
//			else if	(model->on_seq[i]==2) 		comp.vbl_delay = model->on_delay[i];
//			else /*if(model->on_seq[i]==3)*/ 	comp.bl_delay = model->on_delay[i];
//		}

		for(i=0; i<PWR_MAX_CH; i++)
		{
			comp.idd_max[i]	= model->idd_h;
			comp.ibl_max[i]	= model->ibl_h;
			comp.idd_min[i]	= model->idd_l;
			comp.ibl_min[i]	= model->ibl_l;
		}

		pwr_write(pwr_fd, (char*)&comp, len);
	}

	gp.adim = var_data.adim = adim_voltage;
	memset(&rsp_detect_osung_data, 0, sizeof(rsp_detect_osung_data));			// pwr rx data initial

	return RES_ACK;
}


int pwr_pdimm_control_set(pwm_data_t pdim, int flag_freq, int flag_duty)
{
	int					len;
	u16 freq, duty;

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	if(get_pwr_vendor()==0)
	{
		msg_req_ensis_pdim_con_t	pdimm_control = {0,};

		len = sizeof(msg_req_ensis_pdim_con_t);

		memset(&pdimm_control, 0, len);
		pdimm_control.hdr.cmd 		= htons(CMD_PDIM_CON_REQ);
		pdimm_control.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		pdimm_control.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		switch(flag_freq)
		{
		case 1:
			freq = (u16)pdim.freq;
			freq = (freq + (freq*0.1));
			pdimm_control.p_dim.freq		= htons((u16)freq);	// + 10%
			break;
		case 2:
			freq = (u16)pdim.freq;
			freq = (freq - (freq*0.1));
			pdimm_control.p_dim.freq		= htons((u16)freq);	// - 10%
			break;
		default:
			freq = (u16)pdim.freq;
			pdimm_control.p_dim.freq		= htons((u16)freq);
			break;
		}

		switch(flag_duty)
		{
		case 1:
			duty = (u16)pdim.duty;
			duty = (duty + (duty*0.1));
			pdimm_control.p_dim.duty		= htons((u16)duty);	// + 10%
			break;
		case 2:
			duty = (u16)pdim.duty;
			duty = (duty - (duty*0.1));
			pdimm_control.p_dim.duty		= htons((u16)duty);	// - 10%
			break;
		default:
			duty = (u16)pdim.duty;
			pdimm_control.p_dim.duty		= htons((u16)duty);
			break;
		}

//		pdimm_control.p_dim.freq	= htons((u16)pdim.freq);
//		pdimm_control.p_dim.duty	= htons((u16)pdim.duty);

		return pwr_write(pwr_fd, (char*)&pdimm_control, len);
	}
	else
	{
		msg_req_pdim_con_t	pdimm_control = {0,};

		len = sizeof(msg_req_pdim_con_t);

		memset(&pdimm_control, 0, len);
		pdimm_control.hdr.cmd 		= CMD_PDIM_CON_REQ;
		pdimm_control.hdr.board_id 	= OSUNG_PWRBD_ID;
		pdimm_control.hdr.len 		= len-sizeof(msg_req_head_t);

		switch(flag_freq)
		{
		case 1:
			freq = (u16)pdim.freq;
			freq = (freq + (freq*0.1));
			pdimm_control.p_dim.freq		= ((u16)freq);	// + 10%
			break;
		case 2:
			freq = (u16)pdim.freq;
			freq = (freq - (freq*0.1));
			pdimm_control.p_dim.freq		= ((u16)freq);	// - 10%
			break;
		default:
			freq = (u16)pdim.freq;
			pdimm_control.p_dim.freq		= ((u16)freq);
			break;
		}

		switch(flag_duty)
		{
		case 1:
			duty = (u16)pdim.duty;
			duty = (duty + (duty*0.1));
			pdimm_control.p_dim.duty		= ((u8)duty);	// + 10%
			break;
		case 2:
			duty = (u16)pdim.duty;
			duty = (duty - (duty*0.1));
			pdimm_control.p_dim.duty		= ((u8)duty);	// - 10%
			break;
		default:
			duty = (u16)pdim.duty;
			pdimm_control.p_dim.duty		= ((u8)duty);
			break;
		}

		printf("pdim set %d	%d flag %d %d\n", freq, duty, flag_freq, flag_duty);

//		pdimm_control.p_dim.freq	= (u16)pdim.freq;
//		pdimm_control.p_dim.duty	= (u8)pdim.duty;

		return pwr_write(pwr_fd, (char*)&pdimm_control, len);
	}
}


int pwr_adimm_control_set(u16 voltage, int flag)
{
	int					len;
	msg_req_adim_con_t	adimm_control = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_adim_con_t);

	memset(&adimm_control, 0, len);

	if(get_pwr_vendor()==0)
	{
		adimm_control.hdr.cmd 		= htons(CMD_ADIM_CON_REQ);
		adimm_control.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		adimm_control.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		switch(flag)
		{
		case 1:
			if((voltage + (voltage*0.1))>MAX_DIMM_VAL-1) 	voltage = MAX_DIMM_VAL;
			else											voltage = (voltage + (voltage*0.1));
			adimm_control.voltage		= htons((u16)voltage);	// + 10%
			break;
		case 2:
			if((voltage - (voltage*0.1))<=0) 	voltage = 0;
			else								voltage = (voltage - (voltage*0.1));
			adimm_control.voltage		= htons((u16)voltage);	// - 10%
			break;
		default:
			adimm_control.voltage		= htons((u16)voltage);
			break;
		}

//		adimm_control.voltage		= htons((u16)voltage);
	}
	else
	{
		adimm_control.hdr.cmd 		= CMD_ADIM_CON_REQ;
		adimm_control.hdr.board_id 	= OSUNG_PWRBD_ID;
		adimm_control.hdr.len 		= len-sizeof(msg_req_head_t);

		switch(flag)
		{
		case 1:
			if((voltage + (voltage*0.1))>MAX_DIMM_VAL-1) 	voltage = MAX_DIMM_VAL;
			else											voltage = (voltage + (voltage*0.1));
			adimm_control.voltage		= (u16)voltage;	// + 10%
			break;
		case 2:
			if((voltage - (voltage*0.1))<=0) 	voltage = 0;
			else								voltage = (voltage - (voltage*0.1));
			adimm_control.voltage		= (u16)voltage;	// - 10%
			break;
		default:
			adimm_control.voltage		= (u16)voltage;
			break;
		}

		printf("adim set %d	flag %d\n", adimm_control.voltage, flag);

//		adimm_control.voltage		= (u16)voltage;
	}

	gp.adim = var_data.adim = voltage;		// Synchronize rcb adim & qems adim value     19-11-01 seki

	return pwr_write(pwr_fd, (char*)&adimm_control, len);
}


int pwr_vdd_set(u16 voltage, int flag)
{
	int					len;

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	if(get_pwr_vendor()==0)
	{
		msg_req_ensis_vdd_set_t	vdd_set = {0,};

		len = sizeof(msg_req_ensis_vdd_set_t);

		memset(&vdd_set, 0, len);
		vdd_set.hdr.cmd 		= htons(CMD_VDD_SET_REQ);
		vdd_set.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		vdd_set.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		switch(flag)
		{
		case 1:
			vdd_set.vdd_val		= htons(voltage + (voltage*0.1));	// + 10%
			break;
		case 2:
			vdd_set.vdd_val		= htons(voltage - (voltage*0.1));	// - 10%
			break;
		default:
			vdd_set.vdd_val		= htons(voltage);
			break;
		}

		return pwr_write(pwr_fd, (char*)&vdd_set, len);
	}
	else
	{
		msg_req_vdd_set_t	vdd_set = {0,};

		len = sizeof(msg_req_vdd_set_t);

		memset(&vdd_set, 0, len);
		vdd_set.hdr.cmd 		= CMD_VDD_SET_REQ;
		vdd_set.hdr.board_id 	= OSUNG_PWRBD_ID;
		vdd_set.hdr.len 		= len-sizeof(msg_req_head_t);

		switch(flag)
		{
		case 1:
			vdd_set.vdd_val	= voltage + (voltage*0.1);	// + 10%
			break;
		case 2:
			vdd_set.vdd_val	= voltage - (voltage*0.1);	// - 10%
			break;
		default:
			vdd_set.vdd_val	= voltage;
			break;
		}

		printf("ENSIS vdd set %d	flag %d\n", vdd_set.vdd_val, flag);

		vdd_set.vdd_l			= (u8)model_data.vdd_l;

		if( get_schedule_flag() == 1 )
		{
			switch(flag)
			{
			case 1:
				vdd_set.vdd_u	= (u8)(model_data.vdd_h + (model_data.vdd_h*0.2));	// + 20%
				break;
			default:
				vdd_set.vdd_u	= (u8)model_data.vdd_h;
				break;
			}
		}
		else
		{
			vdd_set.vdd_u	= (u8)model_data.vdd_h;
		}


		return pwr_write(pwr_fd, (char*)&vdd_set, len);
	}
}


int pwr_vbl_set(u16 voltage, int flag)
{
	int					len;

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	if(get_pwr_vendor()==0)
	{
		msg_req_ensis_vbl_set_t	vbl_set = {0,};

		len = sizeof(msg_req_ensis_vbl_set_t);

		memset(&vbl_set, 0, len);
		vbl_set.hdr.cmd 		= htons(CMD_VBL_SET_REQ);
		vbl_set.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		vbl_set.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		switch(flag)
		{
		case 1:
			vbl_set.vbl_val		= htons(voltage + (voltage*0.1));	// + 10%
			break;
		case 2:
			vbl_set.vbl_val		= htons(voltage - (voltage*0.1));	// - 10%
			break;
		default:
			vbl_set.vbl_val		= htons(voltage);
			break;
		}

//		vbl_set.vbl_val			= htons((u16)voltage);

		return pwr_write(pwr_fd, (char*)&vbl_set, len);
	}
	else
	{
		msg_req_vbl_set_t	vbl_set = {0,};

		len = sizeof(msg_req_vbl_set_t);

		memset(&vbl_set, 0, len);
		vbl_set.hdr.cmd 		= CMD_VBL_SET_REQ;
		vbl_set.hdr.board_id 	= OSUNG_PWRBD_ID;
		vbl_set.hdr.len 		= len-sizeof(msg_req_head_t);

		switch(flag)
		{
		case 1:
			vbl_set.vbl_val	= voltage + (voltage*0.1);	// + 10%
			break;
		case 2:
			vbl_set.vbl_val	= voltage - (voltage*0.1);	// - 10%
			break;
		default:
			vbl_set.vbl_val	= voltage;
			break;
		}

		printf("vbl set %d	flag %d\n", vbl_set.vbl_val, flag);

//		vbl_set.vbl_val			= (u16)voltage;
		vbl_set.vbl_l			= (u8)model_data.vbl_l;

		if( get_schedule_flag() == 1 )
		{
			switch(flag)
			{
			case 1:
				vbl_set.vbl_u	= (u8)(model_data.vbl_h + (model_data.vbl_h*0.2));	// + 20%
				break;
			default:
				vbl_set.vbl_u	= (u8)model_data.vbl_h;
				break;
			}
		}
		else
		{
			vbl_set.vbl_u	= (u8)model_data.vbl_h;
		}

//		vbl_set.vbl_u			= (u8)model_data.vbl_h;

		return pwr_write(pwr_fd, (char*)&vbl_set, len);
	}
}


int pwr_lcd_onoff_set(enum_onoff_t onoff)
{
	int					len;
	msg_req_lcd_onoff_t	lcd_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_lcd_onoff_t);
	memset(&lcd_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		lcd_onoff.hdr.cmd 		= htons(CMD_LCD_ONOFF_REQ);
		lcd_onoff.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		lcd_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		lcd_onoff.ch			= CH_ALL_T;
		lcd_onoff.onoff			= (u8)onoff;
	}
	else
	{
		lcd_onoff.hdr.cmd 		= CMD_LCD_ONOFF_REQ;
		lcd_onoff.hdr.board_id 	= OSUNG_PWRBD_ID;
		lcd_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		lcd_onoff.ch			= CH_ALL_T;
		lcd_onoff.onoff			= (u8)onoff;
	}

	return pwr_write(pwr_fd, (char*)&lcd_onoff, len);
}


int pwr_vdd_onoff_set(enum_onoff_t onoff)
{
	int					len;
	msg_req_vdd_onoff_t	vdd_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_vdd_onoff_t);
	memset(&vdd_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		vdd_onoff.hdr.cmd 		= htons(CMD_VDD_ONOFF_REQ);
		vdd_onoff.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		vdd_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		vdd_onoff.ch			= CH_ALL_T;
		vdd_onoff.onoff			= (u8)onoff;
	}
	else
	{
		vdd_onoff.hdr.cmd 		= CMD_VDD_ONOFF_REQ;
		vdd_onoff.hdr.board_id 	= OSUNG_PWRBD_ID;
		vdd_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		vdd_onoff.ch			= CH_ALL_T;
		vdd_onoff.onoff			= (u8)onoff;
	}

	if(onoff==ENUM_ON)
	{
		memset(&rsp_detect_data, 0, sizeof(msg_rsp_ensis_detect_get_t)*ENSIS_PWR_CH);	// init sensing values
		memset(&rsp_detect_osung_data, 0, sizeof(msg_rsp_detect_get_t));
	}
	return pwr_write(pwr_fd, (char*)&vdd_onoff, len);
}


int pwr_vbl_onoff_set(enum_onoff_t onoff)
{
	int					len;
	msg_req_vbl_onoff_t	vbl_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_vbl_onoff_t);

	memset(&vbl_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		vbl_onoff.hdr.cmd 		= htons(CMD_VBL_ONOFF_REQ);
		vbl_onoff.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		vbl_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		vbl_onoff.ch			= CH_ALL_T;

		vbl_onoff.onoff			= (u8)onoff;
	}
	else
	{
		vbl_onoff.hdr.cmd 		= CMD_VBL_ONOFF_REQ;
		vbl_onoff.hdr.board_id 	= OSUNG_PWRBD_ID;
		vbl_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		vbl_onoff.ch			= CH_ALL_T;
		vbl_onoff.onoff			= (u8)onoff;
		printf("vbl on off vendor 1\n");
	}

	return pwr_write(pwr_fd, (char*)&vbl_onoff, len);
}

int pwr_vbl_enable_timeout(void)//2023.03.20 vbl en timeout added
{
	int					len;
	msg_req_vbl_timeout_t	vbl_timeout = {0,};
	len = sizeof(msg_req_vbl_timeout_t);
	memset(&vbl_timeout, 0, len);
	printf("VBL Enable protocol worked\n");

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	if(get_pwr_vendor()==0)
	{
		vbl_timeout.hdr.cmd 		= htons(CMD_VBL_TIMEOUT_REQ);
		vbl_timeout.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		vbl_timeout.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		vbl_timeout.timeout			= model_data.VBL_EN_timeout;
	}
	else
	{
		vbl_timeout.hdr.cmd 		= CMD_VBL_TIMEOUT_REQ;
		vbl_timeout.hdr.board_id 	= OSUNG_PWRBD_ID;
		vbl_timeout.hdr.len 		= len-sizeof(msg_req_head_t);

		vbl_timeout.timeout			= model_data.VBL_EN_timeout;
	}
	return pwr_write(pwr_fd, (char*)&vbl_timeout, len);
}

int pwr_dimm_onoff_set(enum_onoff_t onoff)
{
	int						len;
	msg_req_dimm_onoff_t	dimm_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_dimm_onoff_t);

	memset(&dimm_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		dimm_onoff.hdr.cmd 		= htons(CMD_DIMM_ONOFF_REQ);
		dimm_onoff.hdr.board_id = htons(ENSIS_PWRBD_ID);
		dimm_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		dimm_onoff.onoff		= (u8)onoff;
	}
	else
	{
		dimm_onoff.hdr.cmd 		= CMD_DIMM_ONOFF_REQ;
		dimm_onoff.hdr.board_id = OSUNG_PWRBD_ID;
		dimm_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		dimm_onoff.onoff		= (u8)onoff;
	}

	return pwr_write(pwr_fd, (char*)&dimm_onoff, len);
}


int pwr_bl_onoff_set(enum_onoff_t onoff)
{
	int					len;
	msg_req_bl_onoff_t	bl_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_vbl_onoff_t);

	memset(&bl_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		bl_onoff.hdr.cmd 		= htons(CMD_BL_ONOFF_REQ);
		bl_onoff.hdr.board_id 	= htons(ENSIS_PWRBD_ID);
		bl_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		bl_onoff.ch				= CH_ALL_T;
		bl_onoff.onoff			= (u8)onoff;
	}
	else
	{
		bl_onoff.hdr.cmd 		= CMD_BL_ONOFF_REQ;
		bl_onoff.hdr.board_id 	= OSUNG_PWRBD_ID;
		bl_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		bl_onoff.ch				= CH_ALL_T;
		bl_onoff.onoff			= (u8)onoff;
	}

	return pwr_write(pwr_fd, (char*)&bl_onoff, len);
}

int pwr_spc_onoff_set(enum_onoff_t onoff)
{
	int						len;
	msg_req_spc_onoff_t	spc_onoff = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_spc_onoff_t);

	memset(&spc_onoff, 0, len);

	if(get_pwr_vendor()==0)
	{
		spc_onoff.hdr.cmd 		= htons(CMD_SPC_ONOFF_REQ);
		spc_onoff.hdr.board_id	= htons(ENSIS_PWRBD_ID);
		spc_onoff.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		spc_onoff.ch			= CH_ALL_T;
		spc_onoff.onoff			= (u8)onoff;
	}
	else
	{
		spc_onoff.hdr.cmd 		= CMD_SPC_ONOFF_REQ;
		spc_onoff.hdr.board_id	= OSUNG_PWRBD_ID;
		spc_onoff.hdr.len 		= len-sizeof(msg_req_head_t);

		spc_onoff.ch			= CH_ALL_T;
		spc_onoff.onoff			= (u8)onoff;
	}

	return pwr_write(pwr_fd, (char*)&spc_onoff, len);
}


int pwr_detect_get(u16 bid)
{
	int						len;
	msg_req_detect_get_t	detect = {0,};

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	len = sizeof(msg_req_detect_get_t);

	memset(&detect, 0, len);

	if(get_pwr_vendor()==0)
	{
		detect.hdr.cmd 		= htons(CMD_DETECT_GET_REQ);
		detect.hdr.board_id = htons(bid);
		detect.hdr.len 		= htons(len-sizeof(msg_req_head_t));
	}
	else
	{
		detect.hdr.cmd 		= CMD_DETECT_GET_REQ;
		detect.hdr.board_id = OSUNG_PWRBD_ID;
		detect.hdr.len 		= len-sizeof(msg_req_head_t);
	}

	return pwr_write(pwr_fd, (char*)&detect, len);
}


int pwr_offset_req(u16 bid)
{
	int					len;

	if(NULL == pwr_fd)
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	msg_req_offset_t	data = {0,};

	len = sizeof(msg_req_offset_t);

	if(get_pwr_vendor()==0)
	{
		memset(&data, 0, len);
		data.hdr.cmd 		= htons(CMD_OFFSET_DETECT_REQ);
		data.hdr.board_id 	= htons(bid);
		data.hdr.len 		= htons(len-sizeof(msg_req_head_t));

		offset_ack[bid-1]	= RES_NACK;
	}

	offset_ack_cnt = 0;

	return pwr_write(pwr_fd, (char*)&data, len);
}


int pwr_fw_upload( pwr_t *_sock)
{
	if(NULL == pwr_fd)	// check tegra uart to power
	{
		fprintf(stderr, "pwr_fd is NULL!\n");
		return RES_NACK;
	}

	fw_upload_req_t data = {0,};
	FILE 	*fp;
	char 	path[MAX_PATH], buf[MAX_FW_BUF], crc_buf;
	long 	file_size;
	int		total_send_bytes, send_bytes, write_size, i;
	u32		cs = 0x0;

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_FW, PWR_FW_FILE);

	fp = fopen(path, "rb");
	if(fp == NULL)
	{
		fprintf(stderr, "%s file not exist!\n", path);
		return RES_NACK;
	}

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	total_send_bytes = 0;

	for(i=0; i<file_size; i++)
	{
		send_bytes = fread(&crc_buf, sizeof(char), 1, fp);
		cs += crc_buf;
		cs &= 0xFFFFFFFF;
	}
	printf("file_size = %ld, checksum = 0x%X\n", file_size, cs);

	if(get_pwr_vendor()==0)	i2c_pwr_485_set(1);	// RTS control : HIGH


	pwr_upload_flag=1;
	printf("power upload flag on\n");

	// protocol
	memset(&data, 0, sizeof(fw_upload_req_t));
	data.stx 		= FW_STX;
	data.len 		= sizeof(fw_upload_req_t);
	data.type 		= 0x00;
	data.cmd 		= 0x90;
	data.file_size 	= file_size;
	data.check_sum 	= cs;
	data.etx 		= FW_ETX;
	if(0==write(_sock->fd, &data, data.len)) {}

	sleep(1);	// delay(1sec)

	fseek(fp, 0, SEEK_SET);
	while((send_bytes = fread(buf, 1, MAX_FW_BUF, fp))>0)
	{
		write_size = write(_sock->fd, buf, send_bytes);
		total_send_bytes += send_bytes;
		rs485_re(_sock);	// if you do not run this func, write() func will return -1
		printf("write(%d), %d/%ld bytes\n", write_size, total_send_bytes, file_size);
	}

	if(get_pwr_vendor()==0)
	{
		rs485_re(_sock);
		i2c_pwr_485_set(0);	// RTS control : LOW
	}

	fclose(fp);




	pwr_upload_flag=0;
	printf("power upload flag off\n");
	return RES_ACK;
}


static u16 crc16( unsigned char *pdata, size_t data_size)
{
	u16				crc = 0x0;
	register u16 	i, j;

	for(j=0; j<(int)data_size; j++)
	{
		i	= ((u16)(crc >> 8) ^ *pdata++) & 0xff;
		crc	= (crc << 8) ^ crc_table[i];
	}

	return crc;
}

// psrc : except to crc data
static int check_write_data( u8 *psrc, u8 *pdest, size_t src_size)
{
	int		i, j=0;
	u16		crc;
	u8		temp;

	for(i=0; i<(int)src_size; i++)
	{
		if( (psrc[i]==STX) || (psrc[i]==ETX) || (psrc[i]==DLE) )
		{
			pdest[j++]	= DLE;
			pdest[j++]	= psrc[i] ^ STF;
		}
		else
		{
			pdest[j++]	= psrc[i];
		}
	}

	crc = crc16(psrc, src_size);

	temp = (crc>>8) & 0xff;
	if( temp==STX || temp==ETX || temp==DLE )
	{
		pdest[j++]	= DLE;
		pdest[j++]	= temp ^ STF;
	}
	else
	{
		pdest[j++]	= temp;
	}

	temp = crc & 0xff;
	if( temp==STX || temp==ETX || temp==DLE )
	{
		pdest[j++]	= DLE;
		pdest[j++]	= temp ^ STF;				// encode
	}
	else
	{
		pdest[j++]	= temp;
	}

	return j;
}

// psrc : include to crc data
static int check_read_data( u8 *psrc, u8 *pdest)
{
	int i, j=0;

	for(i=0; ;i++)
	{
		if (i > __MAX_BUF_SIZE)		break;

		if		(psrc[i] == STX) 	continue;
		else if	(psrc[i] == ETX) 	break;
		else if	(psrc[i] == DLE) 	pdest[j++] = psrc[++i] | STF;		// decode
		else						pdest[j++] = psrc[i];
	}

	return j;
}


void on_pwr_recv( pwr_t *sender)
{
	int				sz_read, data_len, bd_id, cmd_id, i, j;
	msg_rsp_head_t	hdr = {0,};
	char			rx_buf[__MAX_BUF_SIZE], buf[__MAX_BUF_SIZE];
	memset(rx_buf, 0, __MAX_BUF_SIZE);
	u16 			buf_crc_chk=0, crc_chk_result=0;

	sz_read = pwr_read(sender, rx_buf, __MAX_BUF_SIZE);
	if (0 < sz_read)
	{
		for(i=0; i<sz_read; i++)
		{
			if(pwr_upload_flag==0)
			{
				if(rx_buf[i] == STX)
				{
					buft_cnt 			= 0;
					memset(rx_buft, 0, __MAX_BUF_SIZE);
					rx_buft[buft_cnt++] = rx_buf[i];
				}
				else if(rx_buf[i] == ETX)
				{
					rx_buft[buft_cnt++]	= rx_buf[i];

					// analyzing data
					data_len 			= check_read_data((u8*)rx_buft, (u8*)buf);
					calc_crc 			= crc16((u8*)buf, data_len-2);

					// check crc	add 19-07-05
					buf_crc_chk |=  buf[data_len-2];
					buf_crc_chk = ((buf_crc_chk<<8)&0xff00) | buf[data_len-1];
					if (calc_crc==buf_crc_chk)	crc_chk_result=0x0001;
					else 						crc_chk_result=0x0000;

					/*/ debug
					printf("[PWR RX] ");
					for(j=0; j<data_len; j++)
					{
						printf("%02X ", buf[j]);
					}
					printf("\n");
					/*/

					memset(&hdr, 0, sizeof(msg_rsp_head_t));
					memcpy(&hdr, buf, sizeof(msg_rsp_head_t));


					if(get_pwr_vendor()==0)
					{
						cmd_id 	= htons(hdr.cmd);
						bd_id 	= htons(hdr.board_id);
						if(bd_id<1 || bd_id>ENSIS_PWR_CH)
						{
							return;
						}
					}
					else
					{
						cmd_id 	= hdr.cmd;
						bd_id 	= hdr.board_id;
					}

					switch(cmd_id)
					{
					case CMD_VER_RSP:
						{
							if(get_pwr_vendor()==0)
							{
								memset(&rsp_ver_data[bd_id-1], 0, sizeof(msg_rsp_ensis_ver_t));
								memcpy(&rsp_ver_data[bd_id-1], buf, sizeof(msg_rsp_ensis_ver_t));
								version_ack[bd_id-1] = RES_ACK;
							}
							else
							{
								memset(&rsp_ver_osung_data, 0, sizeof(msg_rsp_osung_ver_t));
								memcpy(&rsp_ver_osung_data, buf, sizeof(msg_rsp_osung_ver_t));
								version_osung_ack = RES_ACK;
							}
						}
						break;
					case CMD_SYNC_RSP:
					case CMD_LCD_ONOFF_RSP:
					case CMD_VBL_ONOFF_RSP:
					case CMD_VDD_ONOFF_RSP:
					case CMD_BL_ONOFF_RSP:
					case CMD_DIMM_ONOFF_RSP:
					case CMD_PDIM_CON_RSP:
					case CMD_ADIM_CON_RSP:
					case CMD_VDD_SET_RSP:
					case CMD_VBL_SET_RSP:
					case CMD_COMPOSIT_SET_RSP:
						break;
					case CMD_DETECT_GET_RSP:
						{
							if(get_pwr_vendor()==0)
							{
								//memset(&rsp_detect_data[bd_id-1], 0, sizeof(msg_rsp_ensis_detect_get_t));
	//							memcpy(&rsp_detect_data[bd_id-1], buf, sizeof(msg_rsp_ensis_detect_get_t));
								if (crc_chk_result==0x0001)	memcpy(&rsp_detect_data[bd_id-1], buf, sizeof(msg_rsp_ensis_detect_get_t));
								else 						memcpy(&rsp_detect_data[bd_id-1], rsp_detect_data, sizeof(msg_rsp_ensis_detect_get_t));
							}
							else
							{
	//							memcpy(&rsp_detect_osung_data, buf, sizeof(msg_rsp_detect_get_t));
								if (crc_chk_result==0x0001)	memcpy(&rsp_detect_osung_data, buf, sizeof(msg_rsp_detect_get_t));
								else						memcpy(&rsp_detect_osung_data, &rsp_detect_osung_data, sizeof(msg_rsp_detect_get_t));
							}
						}
						break;
					case CMD_OFFSET_DETECT_RSP:
						{
							if(get_pwr_vendor()==0)
							{
								offset_ack[bd_id-1] = RES_ACK;
							}
						}
						break;
					default:
						fprintf(stderr, "[pwr] command:0x%X - unknown!\n", hdr.cmd);
						break;
					}

					// initialize
					buft_cnt 			= 0;
					memset(rx_buft, 0, __MAX_BUF_SIZE);
				}
				else
				{
					rx_buft[buft_cnt++] = rx_buf[i];
//					printf("%02X\n",rx_buf[i]);
				}
			}
			else	//power f/w update ack
			{
				if(rx_buf[i] == FW_STX)
				{
					buft_cnt 			= 0;
					memset(rx_buft, 0, __MAX_BUF_SIZE);
					rx_buft[buft_cnt++] = rx_buf[i];
				}
				else if(rx_buf[i] == FW_ETX)
				{
					rx_buft[buft_cnt++]	= rx_buf[i];

					printf("POWER F/W UPLOAD ACk :");
					for (j=0;j<6;j++) printf(" %02X", rx_buft[j]);
					printf("\n");
				}
				else
				{
					rx_buft[buft_cnt++] = rx_buf[i];
//					printf("ack %02X\n",rx_buf[i]);
				}
			}
		}
		//printf("\n");
	}
	else
	{
		fprintf(stderr, "on_pwr_recv() error\n");
	}
}



void std_power_sequence(req_std_pwr_seq_test_t* ps)
{

	uint16_t	i=0;
	uint32_t	seq_sel=0;

	//initial
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing_buf_t)*MAX_PSEQ_CNT);
	memset(&pseq_buf, 0, sizeof(pseq_buf));
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing)*MAX_PSEQ_CNT);
	pseq_set_scnt=0;

	seq_sel=ps->sel; // clock of I2C

	for(i=0;i<MAX_PSEQ_CNT;i++)//start from the point that seq was paused
	{
		if( (seq_sel>>i)&0x00000001 )	//find selected sequence
		{
			++pseq_set_scnt;
			memcpy(&pseq_buf[pseq_set_scnt-1], &ps->timing_info[i], sizeof(std_pwr_seq_timing));	//copy t1 ~ repeat value

			pseq_buf[pseq_set_scnt-1].pwr1_off_time	= (ps->timing_info[i].t1 + ps->timing_info[i].t2 + ps->timing_info[i].t5			\
													+ ps->timing_info[i].tb1 + ps->timing_info[i].ton + ps->timing_info[i].tb2		\
													+ ps->timing_info[i].t6 + ps->timing_info[i].t3);	//100us
			pseq_buf[pseq_set_scnt-1].sequence_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time + ps->timing_info[i].tf + ps->timing_info[i].t4);
			pseq_buf[pseq_set_scnt-1].signal_on_time = (ps->timing_info[i].t1 + ps->timing_info[i].t2);
			pseq_buf[pseq_set_scnt-1].signal_off_time	= (pseq_buf[pseq_set_scnt-1].signal_on_time + ps->timing_info[i].t5 + ps->timing_info[i].tb1	\
													+ ps->timing_info[i].ton + ps->timing_info[i].tb2 + ps->timing_info[i].t6);
			pseq_buf[pseq_set_scnt-1].pwr2_on_time	= (pseq_buf[pseq_set_scnt-1].signal_on_time + ps->timing_info[i].t5);
			pseq_buf[pseq_set_scnt-1].pwr2_off_time	= (pseq_buf[pseq_set_scnt-1].pwr2_on_time + ps->timing_info[i].tb1 + ps->timing_info[i].ton);
			pseq_buf[pseq_set_scnt-1].acdet_on_time	= (ps->timing_info[i].t1 + ps->timing_info[i].acdet_on);
			pseq_buf[pseq_set_scnt-1].acdet_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].acdet_off);
			/* 2022.04.20 ksk i2c 2 */
			pseq_buf[pseq_set_scnt-1].i2c_on_time_1		= (ps->timing_info[i].t1 + ps->timing_info[i].t_i2c_on_1);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_1	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].t_i2c_off_1);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_2		= (ps->timing_info[i].t1 + ps->timing_info[i].t_i2c_on_2);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_2	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].t_i2c_off_2);

			printf("[%d] %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n\r", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t1, pseq_buf[pseq_set_scnt-1].t2, \
					pseq_buf[pseq_set_scnt-1].t5, pseq_buf[pseq_set_scnt-1].tb1, pseq_buf[pseq_set_scnt-1].ton, pseq_buf[pseq_set_scnt-1].tb2, pseq_buf[pseq_set_scnt-1].t6, pseq_buf[pseq_set_scnt-1].t3, \
					pseq_buf[pseq_set_scnt-1].tf, pseq_buf[pseq_set_scnt-1].t4, pseq_buf[pseq_set_scnt-1].tlds, pseq_buf[pseq_set_scnt-1].tldo, pseq_buf[pseq_set_scnt-1].t_i2c_on_1, pseq_buf[pseq_set_scnt-1].t_i2c_off_1, \
					pseq_buf[pseq_set_scnt-1].acdet_on, pseq_buf[pseq_set_scnt-1].acdet_off, pseq_buf[pseq_set_scnt-1].repeat, \
					pseq_buf[pseq_set_scnt-1].pwr1_off_time, pseq_buf[pseq_set_scnt-1].sequence_time, pseq_buf[pseq_set_scnt-1].signal_on_time, pseq_buf[pseq_set_scnt-1].signal_off_time, \
					pseq_buf[pseq_set_scnt-1].pwr2_on_time, pseq_buf[pseq_set_scnt-1].pwr2_off_time, pseq_buf[pseq_set_scnt-1].acdet_on_time, pseq_buf[pseq_set_scnt-1].acdet_off_time, \
					pseq_buf[pseq_set_scnt-1].i2c_on_time_1, pseq_buf[pseq_set_scnt-1].i2c_off_time_1, pseq_buf[pseq_set_scnt-1].i2c_on_time_2, pseq_buf[pseq_set_scnt-1].i2c_off_time_2);
		}
	}

	pwr_sequence_vx1_edp_flag = 0;

//	if(pseq_set_scnt>0)	std_power_seq_run=1;

}

void std_edp_power_sequence(req_std_aux_i2c_pwr_seq_test_t* ps)
{

	uint16_t	i=0;
	uint32_t	seq_sel=0;

	//initial
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing_buf_t)*MAX_PSEQ_CNT);
	memset(&pseq_buf, 0, sizeof(pseq_buf));
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing)*MAX_PSEQ_CNT);
	pseq_set_scnt=0;

	seq_sel=ps->sel; // clock of I2C

	for(i=0;i<MAX_PSEQ_CNT;i++)//start from the point that seq was paused
	{
		if( (seq_sel>>i)&0x00000001 )	//find selected sequence
		{
			++pseq_set_scnt;
//			memcpy(&pseq_buf[pseq_set_scnt-1], &ps->pwr_seq_timing[i], sizeof(std_pwr_seq_timing));	//copy t1 ~ repeat value

			pseq_buf[pseq_set_scnt-1].t1 = ps->pwr_seq_timing[i].t1;
//			pseq_buf[pseq_set_scnt-1].t2 = ps->pwr_seq_timing[i].t2;
//			pseq_buf[pseq_set_scnt-1].t3 = ps->pwr_seq_timing[i].t3;
//			pseq_buf[pseq_set_scnt-1].t4 = ps->pwr_seq_timing[i].t4;
//			pseq_buf[pseq_set_scnt-1].t5 = ps->pwr_seq_timing[i].t5;
			pseq_buf[pseq_set_scnt-1].ton = ps->pwr_seq_timing[i].ton;
//			pseq_buf[pseq_set_scnt-1].acdet_on = ps->pwr_seq_timing[i].acdet_on;
//			pseq_buf[pseq_set_scnt-1].acdet_off = ps->pwr_seq_timing[i].acdet_off;
			pseq_buf[pseq_set_scnt-1].repeat = ps->pwr_seq_timing[i].repeat;
			pseq_buf[pseq_set_scnt-1].t3_1 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_1);
			pseq_buf[pseq_set_scnt-1].t3_2 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_2);
			pseq_buf[pseq_set_scnt-1].t3_3 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_3);
			pseq_buf[pseq_set_scnt-1].t3_4 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_4);
			// prw1 off time not operate
			pseq_buf[pseq_set_scnt-1].pwr1_off_time	= (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].ton);
			pseq_buf[pseq_set_scnt-1].sequence_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time + ps->pwr_seq_timing[i].t4);
			pseq_buf[pseq_set_scnt-1].signal_on_time = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t2);
			pseq_buf[pseq_set_scnt-1].signal_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t5);
			pseq_buf[pseq_set_scnt-1].acdet_on_time	= (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].acdet_on);
			pseq_buf[pseq_set_scnt-1].acdet_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].acdet_off);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_1 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t_i2c_on_1);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_1 = (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t_i2c_off_1);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_2 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t_i2c_on_2);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_2 = (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t_i2c_off_2);

			printf("pseq_buf[%d].t1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t1);
//			printf("pseq_buf[%d].t2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t2);
//			printf("pseq_buf[%d].t3 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3);
//			printf("pseq_buf[%d].t4 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t4);
//			printf("pseq_buf[%d].t5 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t5);
			printf("pseq_buf[%d].ton : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].ton);
//			printf("pseq_buf[%d].acdet_on : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_on);
//			printf("pseq_buf[%d].acdet_off : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_off);
			printf("pseq_buf[%d].pwr1_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].pwr1_off_time);
			printf("pseq_buf[%d].signal_on_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].signal_on_time);
			printf("pseq_buf[%d].signal_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].signal_off_time);
			printf("pseq_buf[%d].t3_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_1);
			printf("pseq_buf[%d].t3_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_2);
			printf("pseq_buf[%d].t3_3 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_3);
			printf("pseq_buf[%d].t3_4 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_4);
			printf("pseq_buf[%d].acdet_on_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_on_time);
			printf("pseq_buf[%d].acdet_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_off_time);
			printf("pseq_buf[%d].i2c_on_time_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_on_time_1);
			printf("pseq_buf[%d].i2c_off_time_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_off_time_1);
			printf("pseq_buf[%d].i2c_on_time_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_on_time_2);
			printf("pseq_buf[%d].i2c_off_time_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_off_time_2);
			printf("pseq_buf[%d].repeat : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].repeat);
			printf("pseq_buf[%d].sequence_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].sequence_time);
		}
	}

	pwr_sequence_vx1_edp_flag = 1;

//	if(pseq_set_scnt>0)	std_power_seq_run=1;

}

void std_ocmd_power_sequence(req_std_ocmd_pwr_seq_test_t* ps)
{

	uint16_t	i=0;
	uint32_t	seq_sel=0;

	//initial
	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing_buf_t)*MAX_PSEQ_CNT);
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing)*MAX_PSEQ_CNT);
	pseq_set_scnt=0;

	seq_sel=ps->sel; // clock of OCMD

	for(i=0;i<MAX_PSEQ_CNT;i++)
	{
		if( (seq_sel>>i)&0x00000001 )	//find selected sequence
		{
			++pseq_set_scnt;
			memcpy(&pseq_buf[pseq_set_scnt-1], &ps->timing_info[i], sizeof(std_pwr_seq_timing));	//copy t1 ~ repeat value

			pseq_buf[pseq_set_scnt-1].pwr1_off_time	= (ps->timing_info[i].t1 + ps->timing_info[i].t2 + ps->timing_info[i].t5			\
													+ ps->timing_info[i].tb1 + ps->timing_info[i].ton + ps->timing_info[i].tb2		\
													+ ps->timing_info[i].t6 + ps->timing_info[i].t3);	//100us
			pseq_buf[pseq_set_scnt-1].sequence_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time + ps->timing_info[i].tf + ps->timing_info[i].t4);
			pseq_buf[pseq_set_scnt-1].signal_on_time = (ps->timing_info[i].t1 + ps->timing_info[i].t2);
			pseq_buf[pseq_set_scnt-1].signal_off_time	= (pseq_buf[pseq_set_scnt-1].signal_on_time + ps->timing_info[i].t5 + ps->timing_info[i].tb1	\
													+ ps->timing_info[i].ton + ps->timing_info[i].tb2 + ps->timing_info[i].t6);
			pseq_buf[pseq_set_scnt-1].pwr2_on_time	= (pseq_buf[pseq_set_scnt-1].signal_on_time + ps->timing_info[i].t5);
			pseq_buf[pseq_set_scnt-1].pwr2_off_time	= (pseq_buf[pseq_set_scnt-1].pwr2_on_time + ps->timing_info[i].tb1 + ps->timing_info[i].ton);
			pseq_buf[pseq_set_scnt-1].acdet_on_time	= (ps->timing_info[i].t1 + ps->timing_info[i].acdet_on);
			pseq_buf[pseq_set_scnt-1].acdet_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].acdet_off);
			/* 2022.04.20 ksk i2c 2 */
			pseq_buf[pseq_set_scnt-1].i2c_on_time_1		= (ps->timing_info[i].t1 + ps->timing_info[i].t_i2c_on_1);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_1	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].t_i2c_off_1);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_2		= (ps->timing_info[i].t1 + ps->timing_info[i].t_i2c_on_2);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_2	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->timing_info[i].t_i2c_off_2);

			printf("[%d] %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n\r", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t1, pseq_buf[pseq_set_scnt-1].t2, \
					pseq_buf[pseq_set_scnt-1].t5, pseq_buf[pseq_set_scnt-1].tb1, pseq_buf[pseq_set_scnt-1].ton, pseq_buf[pseq_set_scnt-1].tb2, pseq_buf[pseq_set_scnt-1].t6, pseq_buf[pseq_set_scnt-1].t3, \
					pseq_buf[pseq_set_scnt-1].tf, pseq_buf[pseq_set_scnt-1].t4, pseq_buf[pseq_set_scnt-1].tlds, pseq_buf[pseq_set_scnt-1].tldo, pseq_buf[pseq_set_scnt-1].t_i2c_on_1, pseq_buf[pseq_set_scnt-1].t_i2c_off_1, \
					pseq_buf[pseq_set_scnt-1].acdet_on, pseq_buf[pseq_set_scnt-1].acdet_off, pseq_buf[pseq_set_scnt-1].repeat, \
					pseq_buf[pseq_set_scnt-1].pwr1_off_time, pseq_buf[pseq_set_scnt-1].sequence_time, pseq_buf[pseq_set_scnt-1].signal_on_time, pseq_buf[pseq_set_scnt-1].signal_off_time, \
					pseq_buf[pseq_set_scnt-1].pwr2_on_time, pseq_buf[pseq_set_scnt-1].pwr2_off_time, pseq_buf[pseq_set_scnt-1].acdet_on_time, pseq_buf[pseq_set_scnt-1].acdet_off_time, \
					pseq_buf[pseq_set_scnt-1].i2c_on_time_1, pseq_buf[pseq_set_scnt-1].i2c_off_time_1, pseq_buf[pseq_set_scnt-1].i2c_on_time_2, pseq_buf[pseq_set_scnt-1].i2c_off_time_2);
		}
	}

	pwr_sequence_vx1_edp_flag = 0;
}

void std_edp_ocmd_power_sequence(req_std_aux_ocmd_pwr_seq_test_t* ps)
{

	uint16_t	i=0;
	uint32_t	seq_sel=0;

	//initial
	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing_buf_t)*MAX_PSEQ_CNT);
//	memset(&pseq_buf, 0, sizeof(std_pwr_seq_timing)*MAX_PSEQ_CNT);
	pseq_set_scnt=0;

	seq_sel=ps->sel; // clock of OCMD

	for(i=0;i<MAX_PSEQ_CNT;i++)
	{
		if( (seq_sel>>i)&0x00000001 )	//find selected sequence
		{
			++pseq_set_scnt;
//			memcpy(&pseq_buf[pseq_set_scnt-1], &ps->pwr_seq_timing[i], sizeof(std_pwr_seq_timing));	//copy t1 ~ repeat value

			pseq_buf[pseq_set_scnt-1].t1 = ps->pwr_seq_timing[i].t1;
//			pseq_buf[pseq_set_scnt-1].t2 = ps->pwr_seq_timing[i].t2;
//			pseq_buf[pseq_set_scnt-1].t3 = ps->pwr_seq_timing[i].t3;
//			pseq_buf[pseq_set_scnt-1].t4 = ps->pwr_seq_timing[i].t4;
//			pseq_buf[pseq_set_scnt-1].t5 = ps->pwr_seq_timing[i].t5;
			pseq_buf[pseq_set_scnt-1].ton = ps->pwr_seq_timing[i].ton;
//			pseq_buf[pseq_set_scnt-1].acdet_on = ps->pwr_seq_timing[i].acdet_on;
//			pseq_buf[pseq_set_scnt-1].acdet_off = ps->pwr_seq_timing[i].acdet_off;
			pseq_buf[pseq_set_scnt-1].repeat = ps->pwr_seq_timing[i].repeat;
			pseq_buf[pseq_set_scnt-1].t3_1 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_1);
			pseq_buf[pseq_set_scnt-1].t3_2 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_2);
			pseq_buf[pseq_set_scnt-1].t3_3 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_3);
			pseq_buf[pseq_set_scnt-1].t3_4 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t3_4);
			// prw1 off time not operate
			pseq_buf[pseq_set_scnt-1].pwr1_off_time	= (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].ton);
			pseq_buf[pseq_set_scnt-1].sequence_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time + ps->pwr_seq_timing[i].t4);
			pseq_buf[pseq_set_scnt-1].signal_on_time = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t2);
			pseq_buf[pseq_set_scnt-1].signal_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t5);
			pseq_buf[pseq_set_scnt-1].acdet_on_time	= (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].acdet_on);
			pseq_buf[pseq_set_scnt-1].acdet_off_time	= (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].acdet_off);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_1 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t_i2c_on_1);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_1 = (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t_i2c_off_1);
			pseq_buf[pseq_set_scnt-1].i2c_on_time_2 = (ps->pwr_seq_timing[i].t1 + ps->pwr_seq_timing[i].t_i2c_on_2);
			pseq_buf[pseq_set_scnt-1].i2c_off_time_2 = (pseq_buf[pseq_set_scnt-1].pwr1_off_time - ps->pwr_seq_timing[i].t_i2c_off_2);

			printf("pseq_buf[%d].t1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t1);
//			printf("pseq_buf[%d].t2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t2);
//			printf("pseq_buf[%d].t3 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3);
//			printf("pseq_buf[%d].t4 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t4);
//			printf("pseq_buf[%d].t5 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t5);
			printf("pseq_buf[%d].ton : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].ton);
//			printf("pseq_buf[%d].acdet_on : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_on);
//			printf("pseq_buf[%d].acdet_off : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_off);
			printf("pseq_buf[%d].pwr1_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].pwr1_off_time);
			printf("pseq_buf[%d].signal_on_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].signal_on_time);
			printf("pseq_buf[%d].signal_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].signal_off_time);
			printf("pseq_buf[%d].t3_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_1);
			printf("pseq_buf[%d].t3_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_2);
			printf("pseq_buf[%d].t3_3 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_3);
			printf("pseq_buf[%d].t3_4 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].t3_4);
			printf("pseq_buf[%d].acdet_on_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_on_time);
			printf("pseq_buf[%d].acdet_off_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].acdet_off_time);
			printf("pseq_buf[%d].i2c_on_time_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_on_time_1);
			printf("pseq_buf[%d].i2c_off_time_1 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_off_time_1);
			printf("pseq_buf[%d].i2c_on_time_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_on_time_2);
			printf("pseq_buf[%d].i2c_off_time_2 : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].i2c_off_time_2);
			printf("pseq_buf[%d].repeat : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].repeat);
			printf("pseq_buf[%d].sequence_time : %d\n", pseq_set_scnt-1, pseq_buf[pseq_set_scnt-1].sequence_time);
		}
	}

	pwr_sequence_vx1_edp_flag = 1;
}

void std_power_onoff_set(uint16_t id, uint16_t ch, uint16_t onoff)
{
	msg_data_t	smsg;
	req_pwrc_onoff_t	pwrc_data;

	//to PWRC
	memset(&pwrc_data, 0, sizeof(req_pwrc_onoff_t));
	pwrc_data.hdr.cmd_id = SID_PWR_CTRL_ONOFF_REQ;
	pwrc_data.hdr.data_len = sizeof(req_pwrc_onoff_t)-sizeof(res_head_t);

	pwrc_data.pwr_id = id;
	pwrc_data.pwr_ch = ch;
	pwrc_data.onoff = onoff;

	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &pwrc_data, sizeof(req_pwrc_onoff_t));
//	msq_send_pwrc(&smsg);

	if( (id==0)&&(onoff==1) ) printf("power1 on\n");
	else if( (id==0)&&(onoff==0) ) printf("power1 off\n");
	else if( (id==1)&&(onoff==1) ) printf("power2 on\n");
	else if( (id==1)&&(onoff==0) ) printf("power2 off\n");
	wait_ack_pwrc(&smsg, 2000);	//2000ms time out
}

void std_power_volt_curr_set(req_ip_pwr_onoff_t *data)
{
	msg_data_t	smsg;
	req_pwrc_vol_cur_set_t	pwrc_data;

	//to PWRC
	memset(&pwrc_data, 0, sizeof(req_pwrc_vol_cur_set_t));
	pwrc_data.hdr.cmd_id = SID_PWR_CTRL_VOL_CUR_SET_REQ;
	pwrc_data.hdr.data_len = sizeof(req_pwrc_vol_cur_set_t)-sizeof(res_head_t);

	pwrc_data.pwr_id = data->pwr_id;
	pwrc_data.pwr_ch = data->pwr_ch;
	pwrc_data.voltage = data->voltage;
	pwrc_data.current = data->current;

	if(data->pwr_id==0)			power1_voltage=data->voltage;
	else if(data->pwr_id==1)	power2_voltage=data->voltage;


	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &pwrc_data, sizeof(req_pwrc_vol_cur_set_t));
	msq_send_pwrc(&smsg);
}

void std_power_slew_set(uint16_t pwr_id, uint16_t ch, uint32_t slew)
{
	msg_data_t	smsg;
	req_pwrc_slew_set_t	pwrc_data;

	//to PWRC
	memset(&pwrc_data, 0, sizeof(req_pwrc_slew_set_t));
	pwrc_data.hdr.cmd_id = SID_PWR_CTRL_SLEW_REQ;
	pwrc_data.hdr.data_len = sizeof(req_pwrc_slew_set_t)-sizeof(res_head_t);

	pwrc_data.id = pwr_id;
	pwrc_data.ch = ch;
	pwrc_data.slew = slew;

	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &pwrc_data, sizeof(req_pwrc_slew_set_t));
	msq_send_pwrc(&smsg);
}

int std_power_seq_i2c_set(req_std_pwr_seq_test_t *data)
{
	uint16_t	on_cnt_1=0, on_cnt_2=0, off_cnt_1=0, off_cnt_2=0, i;

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data->i2c_info[0].dev_addr)&0x00fe); // 2022.08.17 ksk repeating device addr (& 0x00ff -> 0x00fe)
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc9, (data->i2c_info[0].reg_addr_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data->i2c_info[0].data_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data->i2c_info[0].byte_num)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data->i2c_info[0].i2c_clock_sel)&0x00ff);



	printf("power sequence i2c reg check\n");
	printf("on i2c 1 device addr 0x%04x\t->\t0x%04x\n", data->i2c_info[0].dev_addr, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
	printf("on i2c 1 reg size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[0].reg_addr_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc9));
	printf("on i2c 1 data size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[0].data_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
	printf("on i2c 1 byte num 0x%04x\t->\t0x%04x\n", 	data->i2c_info[0].byte_num, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));
	printf("on i2c 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->i2c_info[0].i2c_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));

	on_cnt_1=data->i2c_info[0].byte_num;
	if(on_cnt_1>10)	on_cnt_1=10;
	for(i=0;i<on_cnt_1;i++)
	{
		FPGA_I2C_1_ON_ADDR_CTRL_Write(FPGA_I2C_1_ON_REG_ADDR+i,(data->i2c_info[0].reg_addr[i]));
		FPGA_I2C_1_ON_DATA_Write(FPGA_I2C_1_ON_REG_DATA+i,(data->i2c_info[0].reg_data[i]));

		printf("I2C 1 on[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->i2c_info[0].reg_addr[i], data->i2c_info[0].reg_data[i], FPGA_I2C_1_ON_ADDR_CTRL_Read(i), FPGA_I2C_1_ON_DATA_Read(i));
	}
	printf("\n");

	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc8, (data->i2c_info[1].dev_addr)&0x00fe);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc9, (data->i2c_info[1].reg_addr_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xca, (data->i2c_info[1].data_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcb, (data->i2c_info[1].byte_num)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcc, (data->i2c_info[1].i2c_clock_sel)&0x00ff);

	printf("off i2c 1 device addr 0x%04x\t->\t0x%04x\n",data->i2c_info[1].dev_addr, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc8));
	printf("off i2c 1 reg size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[1].reg_addr_size, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc9));
	printf("off i2c 1 data size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[1].data_size, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xca));
	printf("off i2c 1 byte num 0x%04x\t->\t0x%04x\n", 	data->i2c_info[1].byte_num, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcb));
	printf("off i2c 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->i2c_info[1].i2c_clock_sel, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcc));

	off_cnt_1=data->i2c_info[1].byte_num;
	if(off_cnt_1>10)	off_cnt_1=10;
	for(i=0;i<off_cnt_1;i++)
	{
		FPGA_I2C_1_OFF_ADDR_CTRL_Write(FPGA_I2C_1_OFF_REG_ADDR+i,(data->i2c_info[1].reg_addr[i]));
		FPGA_I2C_1_OFF_DATA_Write(FPGA_I2C_1_OFF_REG_DATA+i,(data->i2c_info[1].reg_data[i]));

		printf("I2c 1 off[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->i2c_info[1].reg_addr[i], data->i2c_info[1].reg_data[i], FPGA_I2C_1_OFF_ADDR_CTRL_Read(i), FPGA_I2C_1_OFF_DATA_Read(i));
	}
	printf("\n");

	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc8, (data->i2c_info[2].dev_addr)&0x00fe);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc9, (data->i2c_info[2].reg_addr_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xca, (data->i2c_info[2].data_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcb, (data->i2c_info[2].byte_num)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcc, (data->i2c_info[2].i2c_clock_sel)&0x00ff);


	printf("on i2c 2 device addr 0x%04x\t->\t0x%04x\n", data->i2c_info[2].dev_addr, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc8));
	printf("on i2c 2 reg size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[2].reg_addr_size, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc9));
	printf("on i2c 2 data size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[2].data_size, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xca));
	printf("on i2c 2 byte num 0x%04x\t->\t0x%04x\n", 	data->i2c_info[2].byte_num, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcb));
	printf("on i2c 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->i2c_info[2].i2c_clock_sel, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcc));


	on_cnt_2=data->i2c_info[2].byte_num;
	if(on_cnt_2>10)	on_cnt_2=10;
	for(i=0;i<on_cnt_2;i++)
	{
		FPGA_I2C_2_ON_ADDR_CTRL_Write(FPGA_I2C_2_ON_REG_ADDR+i,(data->i2c_info[2].reg_addr[i]));
		FPGA_I2C_2_ON_DATA_Write(FPGA_I2C_2_ON_REG_DATA+i,(data->i2c_info[2].reg_data[i]));

		printf("I2C 2 on[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->i2c_info[2].reg_addr[i], data->i2c_info[2].reg_data[i], FPGA_I2C_2_ON_ADDR_CTRL_Read(i), FPGA_I2C_2_ON_DATA_Read(i));
	}
	printf("\n");


	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc8, (data->i2c_info[3].dev_addr)&0x00fe);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc9, (data->i2c_info[3].reg_addr_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xca, (data->i2c_info[3].data_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcb, (data->i2c_info[3].byte_num)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcc, (data->i2c_info[3].i2c_clock_sel)&0x00ff);


	printf("off i2c 2 device addr 0x%04x\t->\t0x%04x\n",data->i2c_info[3].dev_addr, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc8));
	printf("off i2c 2 reg size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[3].reg_addr_size, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc9));
	printf("off i2c 2 data size 0x%04x\t->\t0x%04x\n", 	data->i2c_info[3].data_size, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xca));
	printf("off i2c 2 byte num 0x%04x\t->\t0x%04x\n", 	data->i2c_info[3].byte_num, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcb));
	printf("off i2c 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->i2c_info[3].i2c_clock_sel, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcc));


	off_cnt_2=data->i2c_info[3].byte_num;
	if(off_cnt_2>10)	off_cnt_2=10;
	for(i=0;i<off_cnt_2;i++)
	{
		FPGA_I2C_2_OFF_ADDR_CTRL_Write(FPGA_I2C_2_OFF_REG_ADDR+i,(data->i2c_info[3].reg_addr[i]));
		FPGA_I2C_2_OFF_DATA_Write(FPGA_I2C_2_OFF_REG_DATA+i,(data->i2c_info[3].reg_data[i]));

		printf("I2c 2 off[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->i2c_info[3].reg_addr[i], data->i2c_info[3].reg_data[i], FPGA_I2C_2_OFF_ADDR_CTRL_Read(i), FPGA_I2C_2_OFF_DATA_Read(i));
	}
	printf("\n");



	return ACK;
}

int std_edp_power_seq_i2c_set(req_std_aux_i2c_pwr_seq_test_t *data)
{
	uint16_t	on_cnt_1=0, on_cnt_2=0, off_cnt_1=0, off_cnt_2=0, i;

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data->pwr_seq_i2c[0].dev_addr)&0x00fe); // 2022.08.17 ksk repeating device addr (& 0x00ff -> 0x00fe)
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc9, (data->pwr_seq_i2c[0].reg_addr_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data->pwr_seq_i2c[0].data_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data->pwr_seq_i2c[0].byte_num)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data->pwr_seq_i2c[0].i2c_clock_sel)&0x00ff);

	printf("power sequence i2c reg check\n");
	printf("on i2c 1 device addr 0x%04x\t->\t0x%04x\n", data->pwr_seq_i2c[0].dev_addr, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
	printf("on i2c 1 reg size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[0].reg_addr_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc9));
	printf("on i2c 1 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[0].data_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
	printf("on i2c 1 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[0].byte_num, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));
	printf("on i2c 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[0].i2c_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));

	on_cnt_1=data->pwr_seq_i2c[0].byte_num;

	if(on_cnt_1>10)	on_cnt_1=10;

	for(i=0;i<on_cnt_1;i++)
	{
		FPGA_I2C_1_ON_ADDR_CTRL_Write(FPGA_I2C_1_ON_REG_ADDR+i,(data->pwr_seq_i2c[0].reg_addr[i]));
		FPGA_I2C_1_ON_DATA_Write(FPGA_I2C_1_ON_REG_DATA+i,(data->pwr_seq_i2c[0].reg_data[i]));

		printf("I2C 1 on[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->pwr_seq_i2c[0].reg_addr[i], data->pwr_seq_i2c[0].reg_data[i], FPGA_I2C_1_ON_ADDR_CTRL_Read(i), FPGA_I2C_1_ON_DATA_Read(i));
	}

	printf("\n");

	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc8, (data->pwr_seq_i2c[1].dev_addr)&0x00fe);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc9, (data->pwr_seq_i2c[1].reg_addr_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xca, (data->pwr_seq_i2c[1].data_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcb, (data->pwr_seq_i2c[1].byte_num)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcc, (data->pwr_seq_i2c[1].i2c_clock_sel)&0x00ff);

	printf("off i2c 1 device addr 0x%04x\t->\t0x%04x\n",data->pwr_seq_i2c[1].dev_addr, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc8));
	printf("off i2c 1 reg size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[1].reg_addr_size, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc9));
	printf("off i2c 1 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[1].data_size, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xca));
	printf("off i2c 1 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[1].byte_num, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcb));
	printf("off i2c 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[1].i2c_clock_sel, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcc));

	off_cnt_1=data->pwr_seq_i2c[1].byte_num;

	if(off_cnt_1>10)	off_cnt_1=10;

	for(i=0;i<off_cnt_1;i++)
	{
		FPGA_I2C_1_OFF_ADDR_CTRL_Write(FPGA_I2C_1_OFF_REG_ADDR+i,(data->pwr_seq_i2c[1].reg_addr[i]));
		FPGA_I2C_1_OFF_DATA_Write(FPGA_I2C_1_OFF_REG_DATA+i,(data->pwr_seq_i2c[1].reg_data[i]));

		printf("I2c 1 off[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->pwr_seq_i2c[1].reg_addr[i], data->pwr_seq_i2c[1].reg_data[i], FPGA_I2C_1_OFF_ADDR_CTRL_Read(i), FPGA_I2C_1_OFF_DATA_Read(i));
	}

	printf("\n");

	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc8, (data->pwr_seq_i2c[2].dev_addr)&0x00fe);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc9, (data->pwr_seq_i2c[2].reg_addr_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xca, (data->pwr_seq_i2c[2].data_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcb, (data->pwr_seq_i2c[2].byte_num)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcc, (data->pwr_seq_i2c[2].i2c_clock_sel)&0x00ff);


	printf("on i2c 2 device addr 0x%04x\t->\t0x%04x\n", data->pwr_seq_i2c[2].dev_addr, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc8));
	printf("on i2c 2 reg size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[2].reg_addr_size, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc9));
	printf("on i2c 2 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[2].data_size, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xca));
	printf("on i2c 2 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[2].byte_num, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcb));
	printf("on i2c 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[2].i2c_clock_sel, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcc));

	on_cnt_2=data->pwr_seq_i2c[2].byte_num;

	if(on_cnt_2>10)	on_cnt_2=10;

	for(i=0;i<on_cnt_2;i++)
	{
		FPGA_I2C_2_ON_ADDR_CTRL_Write(FPGA_I2C_2_ON_REG_ADDR+i,(data->pwr_seq_i2c[2].reg_addr[i]));
		FPGA_I2C_2_ON_DATA_Write(FPGA_I2C_2_ON_REG_DATA+i,(data->pwr_seq_i2c[2].reg_data[i]));

		printf("I2C 2 on[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->pwr_seq_i2c[2].reg_addr[i], data->pwr_seq_i2c[2].reg_data[i], FPGA_I2C_2_ON_ADDR_CTRL_Read(i), FPGA_I2C_2_ON_DATA_Read(i));
	}

	printf("\n");

	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc8, (data->pwr_seq_i2c[3].dev_addr)&0x00fe);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc9, (data->pwr_seq_i2c[3].reg_addr_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xca, (data->pwr_seq_i2c[3].data_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcb, (data->pwr_seq_i2c[3].byte_num)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcc, (data->pwr_seq_i2c[3].i2c_clock_sel)&0x00ff);

	printf("off i2c 2 device addr 0x%04x\t->\t0x%04x\n",data->pwr_seq_i2c[3].dev_addr, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc8));
	printf("off i2c 2 reg size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[3].reg_addr_size, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc9));
	printf("off i2c 2 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[3].data_size, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xca));
	printf("off i2c 2 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[3].byte_num, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcb));
	printf("off i2c 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_i2c[3].i2c_clock_sel, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcc));

	off_cnt_2=data->pwr_seq_i2c[3].byte_num;

	if(off_cnt_2>10)	off_cnt_2=10;

	for(i=0;i<off_cnt_2;i++)
	{
		FPGA_I2C_2_OFF_ADDR_CTRL_Write(FPGA_I2C_2_OFF_REG_ADDR+i,(data->pwr_seq_i2c[3].reg_addr[i]));
		FPGA_I2C_2_OFF_DATA_Write(FPGA_I2C_2_OFF_REG_DATA+i,(data->pwr_seq_i2c[3].reg_data[i]));

		printf("I2c 2 off[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data->pwr_seq_i2c[3].reg_addr[i], data->pwr_seq_i2c[3].reg_data[i], FPGA_I2C_2_OFF_ADDR_CTRL_Read(i), FPGA_I2C_2_OFF_DATA_Read(i));
	}

	printf("\n");

	return ACK;
}

int std_power_seq_ocmd_set(req_std_ocmd_pwr_seq_test_t *data)
{
	uint16_t	on_cnt_1=0, on_cnt_2=0, off_cnt_1=0, off_cnt_2=0, i;

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data->ocmd_info[0].dev_addr)&0x00fe); // 2022.08.17 ksk repeating device addr (& 0x00ff -> 0x00fe)
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data->ocmd_info[0].data_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data->ocmd_info[0].byte_num)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data->ocmd_info[0].ocmd_clock_sel)&0x00ff);



	printf("power sequence OCMD reg check\n");
	printf("on ocmd 1 device addr 0x%04x\t->\t0x%04x\n",data->ocmd_info[0].dev_addr, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
	printf("on ocmd 1 data size 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[0].data_size, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
	printf("on ocmd 1 byte num 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[0].byte_num, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));
	printf("on ocmd 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[0].ocmd_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));

	on_cnt_1=data->ocmd_info[0].byte_num;
	if(on_cnt_1>10)	on_cnt_1=10;
	for(i=0;i<on_cnt_1;i++)
	{
		FPGA_I2C_1_ON_ADDR_CTRL_Write		(i,	(data->ocmd_info[0].sub_addr[i]));
		FPGA_OCMD_1_ON_TABLE_DATA_H_Write	(i, (data->ocmd_info[0].table_data_h[i]));
		FPGA_OCMD_1_ON_TABLE_DATA_L_Write	(i, (data->ocmd_info[0].table_data_l[i]));
		FPGA_OCMD_1_ON_REG_DATA_H_Write		(i, (data->ocmd_info[0].reg_data_h[i]));
		FPGA_OCMD_1_ON_REG_DATA_L_Write		(i, (data->ocmd_info[0].reg_data_l[i]));

		printf("OCMD 1 on[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
				i, data->ocmd_info[0].sub_addr[i], \
				data->ocmd_info[0].table_data_h[i],	data->ocmd_info[0].table_data_l[i],\
				data->ocmd_info[0].reg_data_h[i],	data->ocmd_info[0].reg_data_l[i],\
				FPGA_OCMD_1_ON_TABLE_DATA_H_Read(i),FPGA_OCMD_1_ON_TABLE_DATA_L_Read(i),\
				FPGA_OCMD_1_ON_REG_DATA_H_Read(i), FPGA_OCMD_1_ON_REG_DATA_L_Read(i));
	}
	printf("\n");

	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc8, (data->ocmd_info[1].dev_addr)&0x00fe);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xca, (data->ocmd_info[1].data_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcb, (data->ocmd_info[1].byte_num)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcc, (data->ocmd_info[1].ocmd_clock_sel)&0x00ff);

	printf("off ocmd 1 device addr 0x%04x\t->\t0x%04x\n",data->ocmd_info[1].dev_addr, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc8));
	printf("off ocmd 1 data size 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[1].data_size, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xca));
	printf("off ocmd 1 byte num 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[1].byte_num, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcb));
	printf("off ocmd 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[1].ocmd_clock_sel, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcc));

	off_cnt_1=data->ocmd_info[1].byte_num;
	if(off_cnt_1>10)	off_cnt_1=10;
	for(i=0;i<off_cnt_1;i++)
	{
		FPGA_I2C_1_OFF_ADDR_CTRL_Write		(i,	(data->ocmd_info[1].sub_addr[i]));
		FPGA_OCMD_1_OFF_TABLE_DATA_H_Write	(i, (data->ocmd_info[1].table_data_h[i]));
		FPGA_OCMD_1_OFF_TABLE_DATA_L_Write	(i, (data->ocmd_info[1].table_data_l[i]));
		FPGA_OCMD_1_OFF_REG_DATA_H_Write	(i, (data->ocmd_info[1].reg_data_h[i]));
		FPGA_OCMD_1_OFF_REG_DATA_L_Write	(i, (data->ocmd_info[1].reg_data_l[i]));

		printf("OCMD 1 off[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
						i, data->ocmd_info[1].sub_addr[i], \
						data->ocmd_info[1].table_data_h[i],	data->ocmd_info[1].table_data_l[i],\
						data->ocmd_info[1].reg_data_h[i],	data->ocmd_info[1].reg_data_l[i],\
						FPGA_OCMD_1_OFF_TABLE_DATA_H_Read(i),FPGA_OCMD_1_OFF_TABLE_DATA_L_Read(i),\
						FPGA_OCMD_1_OFF_REG_DATA_H_Read(i), FPGA_OCMD_1_OFF_REG_DATA_L_Read(i));
	}
	printf("\n");

	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc8, (data->ocmd_info[2].dev_addr)&0x00fe);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xca, (data->ocmd_info[2].data_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcb, (data->ocmd_info[2].byte_num)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcc, (data->ocmd_info[2].ocmd_clock_sel)&0x00ff);


	printf("on ocmd 2 device addr 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[2].dev_addr, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc8));
	printf("on ocmd 2 data size 0x%04x\t->\t0x%04x\n", 		data->ocmd_info[2].data_size, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xca));
	printf("on ocmd 2 byte num 0x%04x\t->\t0x%04x\n", 		data->ocmd_info[2].byte_num, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcb));
	printf("on ocmd 2 clock sel 0x%04x\t->\t0x%04x\n", 		data->ocmd_info[2].ocmd_clock_sel, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcc));


	on_cnt_2=data->ocmd_info[2].byte_num;
	if(on_cnt_2>10)	on_cnt_2=10;
	for(i=0;i<on_cnt_2;i++)
	{
		FPGA_I2C_2_ON_ADDR_CTRL_Write		(i,	(data->ocmd_info[2].sub_addr[i]));
		FPGA_OCMD_2_ON_TABLE_DATA_H_Write	(i, (data->ocmd_info[2].table_data_h[i]));
		FPGA_OCMD_2_ON_TABLE_DATA_L_Write	(i, (data->ocmd_info[2].table_data_l[i]));
		FPGA_OCMD_2_ON_REG_DATA_H_Write		(i, (data->ocmd_info[2].reg_data_h[i]));
		FPGA_OCMD_2_ON_REG_DATA_L_Write		(i, (data->ocmd_info[2].reg_data_l[i]));

		printf("OCMD 2 on[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
						i, data->ocmd_info[2].sub_addr[i], \
						data->ocmd_info[2].table_data_h[i],	data->ocmd_info[2].table_data_l[i],\
						data->ocmd_info[2].reg_data_h[i],	data->ocmd_info[2].reg_data_l[i],\
						FPGA_OCMD_2_ON_TABLE_DATA_H_Read(i),FPGA_OCMD_2_ON_TABLE_DATA_L_Read(i),\
						FPGA_OCMD_2_ON_REG_DATA_H_Read(i), FPGA_OCMD_2_ON_REG_DATA_L_Read(i));
	}
	printf("\n");


	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc8, (data->ocmd_info[3].dev_addr)&0x00fe);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xca, (data->ocmd_info[3].data_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcb, (data->ocmd_info[3].byte_num)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcc, (data->ocmd_info[3].ocmd_clock_sel)&0x00ff);


	printf("off ocmd 2 device addr 0x%04x\t->\t0x%04x\n",	data->ocmd_info[3].dev_addr, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc8));
	printf("off ocmd 2 data size 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[3].data_size, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xca));
	printf("off ocmd 2 byte num 0x%04x\t->\t0x%04x\n", 		data->ocmd_info[3].byte_num, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcb));
	printf("off ocmd 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->ocmd_info[3].ocmd_clock_sel, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcc));


	off_cnt_2=data->ocmd_info[3].byte_num;
	if(off_cnt_2>10)	off_cnt_2=10;
	for(i=0;i<off_cnt_2;i++)
	{
		FPGA_I2C_2_OFF_ADDR_CTRL_Write		(i,	(data->ocmd_info[3].sub_addr[i]));
		FPGA_OCMD_2_OFF_TABLE_DATA_H_Write	(i, (data->ocmd_info[3].table_data_h[i]));
		FPGA_OCMD_2_OFF_TABLE_DATA_L_Write	(i, (data->ocmd_info[3].table_data_l[i]));
		FPGA_OCMD_2_OFF_REG_DATA_H_Write	(i, (data->ocmd_info[3].reg_data_h[i]));
		FPGA_OCMD_2_OFF_REG_DATA_L_Write	(i, (data->ocmd_info[3].reg_data_l[i]));

		printf("OCMD 2 off[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
								i, data->ocmd_info[3].sub_addr[i], \
								data->ocmd_info[3].table_data_h[i],	data->ocmd_info[3].table_data_l[i],\
								data->ocmd_info[3].reg_data_h[i],	data->ocmd_info[3].reg_data_l[i],\
								FPGA_OCMD_2_OFF_TABLE_DATA_H_Read(i),FPGA_OCMD_2_OFF_TABLE_DATA_L_Read(i),\
								FPGA_OCMD_2_OFF_REG_DATA_H_Read(i), FPGA_OCMD_2_OFF_REG_DATA_L_Read(i));
	}
	printf("\n");



	return ACK;
}

int std_edp_power_seq_ocmd_set(req_std_aux_ocmd_pwr_seq_test_t *data)
{
	uint16_t	on_cnt_1=0, on_cnt_2=0, off_cnt_1=0, off_cnt_2=0, i;

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data->pwr_seq_ocmd[0].dev_addr)&0x00fe); // 2022.08.17 ksk repeating device addr (& 0x00ff -> 0x00fe)
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data->pwr_seq_ocmd[0].data_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data->pwr_seq_ocmd[0].byte_num)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data->pwr_seq_ocmd[0].ocmd_clock_sel)&0x00ff);



	printf("power sequence OCMD reg check\n");
	printf("on ocmd 1 device addr 0x%04x\t->\t0x%04x\n",data->pwr_seq_ocmd[0].dev_addr, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
	printf("on ocmd 1 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[0].data_size, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
	printf("on ocmd 1 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[0].byte_num, 	FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));
	printf("on ocmd 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[0].ocmd_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));

	on_cnt_1=data->pwr_seq_ocmd[0].byte_num;
	if(on_cnt_1>10)	on_cnt_1=10;
	for(i=0;i<on_cnt_1;i++)
	{
		FPGA_I2C_1_ON_ADDR_CTRL_Write		(i,	(data->pwr_seq_ocmd[0].sub_addr[i]));
		FPGA_OCMD_1_ON_TABLE_DATA_H_Write	(i, (data->pwr_seq_ocmd[0].table_data_h[i]));
		FPGA_OCMD_1_ON_TABLE_DATA_L_Write	(i, (data->pwr_seq_ocmd[0].table_data_l[i]));
		FPGA_OCMD_1_ON_REG_DATA_H_Write		(i, (data->pwr_seq_ocmd[0].reg_data_h[i]));
		FPGA_OCMD_1_ON_REG_DATA_L_Write		(i, (data->pwr_seq_ocmd[0].reg_data_l[i]));

		printf("OCMD 1 on[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
				i, data->pwr_seq_ocmd[0].sub_addr[i], \
				data->pwr_seq_ocmd[0].table_data_h[i],	data->pwr_seq_ocmd[0].table_data_l[i],\
				data->pwr_seq_ocmd[0].reg_data_h[i],	data->pwr_seq_ocmd[0].reg_data_l[i],\
				FPGA_OCMD_1_ON_TABLE_DATA_H_Read(i),FPGA_OCMD_1_ON_TABLE_DATA_L_Read(i),\
				FPGA_OCMD_1_ON_REG_DATA_H_Read(i), FPGA_OCMD_1_ON_REG_DATA_L_Read(i));
	}
	printf("\n");

	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xc8, (data->pwr_seq_ocmd[1].dev_addr)&0x00fe);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xca, (data->pwr_seq_ocmd[1].data_size)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcb, (data->pwr_seq_ocmd[1].byte_num)&0x00ff);
	FPGA_I2C_1_OFF_ADDR_CTRL_Write(0xcc, (data->pwr_seq_ocmd[1].ocmd_clock_sel)&0x00ff);

	printf("off ocmd 1 device addr 0x%04x\t->\t0x%04x\n",data->pwr_seq_ocmd[1].dev_addr, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xc8));
	printf("off ocmd 1 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[1].data_size, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xca));
	printf("off ocmd 1 byte num 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[1].byte_num, 	FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcb));
	printf("off ocmd 1 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[1].ocmd_clock_sel, FPGA_I2C_1_OFF_ADDR_CTRL_Read(0xcc));

	off_cnt_1=data->pwr_seq_ocmd[1].byte_num;
	if(off_cnt_1>10)	off_cnt_1=10;
	for(i=0;i<off_cnt_1;i++)
	{
		FPGA_I2C_1_OFF_ADDR_CTRL_Write		(i,	(data->pwr_seq_ocmd[1].sub_addr[i]));
		FPGA_OCMD_1_OFF_TABLE_DATA_H_Write	(i, (data->pwr_seq_ocmd[1].table_data_h[i]));
		FPGA_OCMD_1_OFF_TABLE_DATA_L_Write	(i, (data->pwr_seq_ocmd[1].table_data_l[i]));
		FPGA_OCMD_1_OFF_REG_DATA_H_Write	(i, (data->pwr_seq_ocmd[1].reg_data_h[i]));
		FPGA_OCMD_1_OFF_REG_DATA_L_Write	(i, (data->pwr_seq_ocmd[1].reg_data_l[i]));

		printf("OCMD 1 off[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
						i, data->pwr_seq_ocmd[1].sub_addr[i], \
						data->pwr_seq_ocmd[1].table_data_h[i],	data->pwr_seq_ocmd[1].table_data_l[i],\
						data->pwr_seq_ocmd[1].reg_data_h[i],	data->pwr_seq_ocmd[1].reg_data_l[i],\
						FPGA_OCMD_1_OFF_TABLE_DATA_H_Read(i),FPGA_OCMD_1_OFF_TABLE_DATA_L_Read(i),\
						FPGA_OCMD_1_OFF_REG_DATA_H_Read(i), FPGA_OCMD_1_OFF_REG_DATA_L_Read(i));
	}
	printf("\n");

	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xc8, (data->pwr_seq_ocmd[2].dev_addr)&0x00fe);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xca, (data->pwr_seq_ocmd[2].data_size)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcb, (data->pwr_seq_ocmd[2].byte_num)&0x00ff);
	FPGA_I2C_2_ON_ADDR_CTRL_Write(0xcc, (data->pwr_seq_ocmd[2].ocmd_clock_sel)&0x00ff);


	printf("on ocmd 2 device addr 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[2].dev_addr, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xc8));
	printf("on ocmd 2 data size 0x%04x\t->\t0x%04x\n", 		data->pwr_seq_ocmd[2].data_size, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xca));
	printf("on ocmd 2 byte num 0x%04x\t->\t0x%04x\n", 		data->pwr_seq_ocmd[2].byte_num, 	FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcb));
	printf("on ocmd 2 clock sel 0x%04x\t->\t0x%04x\n", 		data->pwr_seq_ocmd[2].ocmd_clock_sel, FPGA_I2C_2_ON_ADDR_CTRL_Read(0xcc));


	on_cnt_2=data->pwr_seq_ocmd[2].byte_num;
	if(on_cnt_2>10)	on_cnt_2=10;
	for(i=0;i<on_cnt_2;i++)
	{
		FPGA_I2C_2_ON_ADDR_CTRL_Write		(i,	(data->pwr_seq_ocmd[2].sub_addr[i]));
		FPGA_OCMD_2_ON_TABLE_DATA_H_Write	(i, (data->pwr_seq_ocmd[2].table_data_h[i]));
		FPGA_OCMD_2_ON_TABLE_DATA_L_Write	(i, (data->pwr_seq_ocmd[2].table_data_l[i]));
		FPGA_OCMD_2_ON_REG_DATA_H_Write		(i, (data->pwr_seq_ocmd[2].reg_data_h[i]));
		FPGA_OCMD_2_ON_REG_DATA_L_Write		(i, (data->pwr_seq_ocmd[2].reg_data_l[i]));

		printf("OCMD 2 on[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
						i, data->pwr_seq_ocmd[2].sub_addr[i], \
						data->pwr_seq_ocmd[2].table_data_h[i],	data->pwr_seq_ocmd[2].table_data_l[i],\
						data->pwr_seq_ocmd[2].reg_data_h[i],	data->pwr_seq_ocmd[2].reg_data_l[i],\
						FPGA_OCMD_2_ON_TABLE_DATA_H_Read(i),FPGA_OCMD_2_ON_TABLE_DATA_L_Read(i),\
						FPGA_OCMD_2_ON_REG_DATA_H_Read(i), FPGA_OCMD_2_ON_REG_DATA_L_Read(i));
	}
	printf("\n");


	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xc8, (data->pwr_seq_ocmd[3].dev_addr)&0x00fe);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xca, (data->pwr_seq_ocmd[3].data_size)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcb, (data->pwr_seq_ocmd[3].byte_num)&0x00ff);
	FPGA_I2C_2_OFF_ADDR_CTRL_Write(0xcc, (data->pwr_seq_ocmd[3].ocmd_clock_sel)&0x00ff);


	printf("off ocmd 2 device addr 0x%04x\t->\t0x%04x\n",	data->pwr_seq_ocmd[3].dev_addr, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xc8));
	printf("off ocmd 2 data size 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[3].data_size, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xca));
	printf("off ocmd 2 byte num 0x%04x\t->\t0x%04x\n", 		data->pwr_seq_ocmd[3].byte_num, 	FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcb));
	printf("off ocmd 2 clock sel 0x%04x\t->\t0x%04x\n", 	data->pwr_seq_ocmd[3].ocmd_clock_sel, FPGA_I2C_2_OFF_ADDR_CTRL_Read(0xcc));


	off_cnt_2=data->pwr_seq_ocmd[3].byte_num;
	if(off_cnt_2>10)	off_cnt_2=10;
	for(i=0;i<off_cnt_2;i++)
	{
		FPGA_I2C_2_OFF_ADDR_CTRL_Write		(i,	(data->pwr_seq_ocmd[3].sub_addr[i]));
		FPGA_OCMD_2_OFF_TABLE_DATA_H_Write	(i, (data->pwr_seq_ocmd[3].table_data_h[i]));
		FPGA_OCMD_2_OFF_TABLE_DATA_L_Write	(i, (data->pwr_seq_ocmd[3].table_data_l[i]));
		FPGA_OCMD_2_OFF_REG_DATA_H_Write	(i, (data->pwr_seq_ocmd[3].reg_data_h[i]));
		FPGA_OCMD_2_OFF_REG_DATA_L_Write	(i, (data->pwr_seq_ocmd[3].reg_data_l[i]));

		printf("OCMD 2 off[%d] 0x%04x: 0x%02x%02x - 0x%02x%02x\t->\t0x%04x%04x - 0x%02x%02x\n",\
								i, data->pwr_seq_ocmd[3].sub_addr[i], \
								data->pwr_seq_ocmd[3].table_data_h[i],	data->pwr_seq_ocmd[3].table_data_l[i],\
								data->pwr_seq_ocmd[3].reg_data_h[i],	data->pwr_seq_ocmd[3].reg_data_l[i],\
								FPGA_OCMD_2_OFF_TABLE_DATA_H_Read(i),FPGA_OCMD_2_OFF_TABLE_DATA_L_Read(i),\
								FPGA_OCMD_2_OFF_REG_DATA_H_Read(i), FPGA_OCMD_2_OFF_REG_DATA_L_Read(i));
	}
	printf("\n");



	return ACK;
}

