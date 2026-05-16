/*
 * rcb.c
 *
 *  Created on: Oct 24, 2017
 *      Author: root
 */

#include <rcb.h>
#include <i2c_lcd.h>
#include <usb_storage.h>
#include <pattern_control.h>
#include <file_manager.h>
#include <rcb_485.h>
#include <global.h>

int 		net_info_cur=0;
int 		net_info_str_max=30;
char 		net_info_str[30][16];
net_info_t 	net_info[10];

//static char *g_menu[]			= { "1.MODEL SELECT", "2.SERVER IP SET", "3.GROUP ID SET","4.EQPID SET", "5.POWER SELECT", "6.WIFI SELECT", "7.WIFI SSID", "8.WIFI IDENTITY",  "9.WIFI PASSWORD", "10.WIFI IP", "11.WIFI NETMASK", "12.WIFI GATEWAY", "13.NETWORK INFO", "14.REBOOT", NULL };
static char *g_menu[]			= { "1.MODEL SELECT", "2.SERVER IP SET", "3.GROUP ID SET","4.EQPID SET", "5.POWER SELECT", "6.WIFI SELECT", "7.WIFI SSID", "8.WIFI IDENTITY",  "9.WIFI PASSWORD", "10.WIFI IP", "11.WIFI NETMASK", "12.WIFI GATEWAY", "13.NETWORK INFO", "14.POWER SPC SET", "15.POWER F/W","16.STORAGE INFO","17.ELVDD ALARM", "18.REBOOT", NULL };
static char *g_wifi_select[]	= { "0:OFF", "1:STATIC", "2:DHCP","3:ETH+WIFI", NULL };
static char *g_vendor[]			= { "EU1150", "ES620P","OSUNG", NULL };
static char *g_pwr_spc[]		= { "DISABLE", "SPC ENABLE", "OTC ENABLE", NULL };
static char *g_port_name[] 		= { "1-2-3-4", "2-4-1-3", "1-3-2-4", "3-1-4-2", "4-3-2-1", "3-4-1-2", "2-1-4-3" };
static char *g_out_name[] 		= { "SING", "DUAL", "QUAD", "OCTA", "HEXA", "32LN", "64LN", "4LN4", "16LN2" };
static char *g_bit_type[] 		= { "6B", "8B", "10B" };
static char *g_twist_name[] 	= { "JEI", "VES" };
static char *g_divide_name[]	= { "NO", "2D", "4D", "8D" };
static char *g_scroll_dir[]		= { "NONE", "UP", "DOWN", "LEFT", "RIGHT" };
static char *g_model_err[] 		= { "NACK",
									"ACK",
									"MODEL NOT EXIST!",
									"CURMODEL FAIL!",
									"FPGA FAIL!",
									"POWER FAIL!",
									"LIMIT SET FAIL!",
									"XRANDR FAIL!",
									"DRAW FAIL!",
									"FILE NOT FOUND!",
									"VDD LOW LIMIT",
									"VDD HIGH LIMIT",
									"IDD LOW LIMIT",
									"IDD HIGH LIMIT",
									"VBL LOW LIMIT",
									"VBL HIGH LIMIT",
									"IBL LOW LIMIT",
									"IBL HIGH LIMIT",
									"SPC OCP LIMIT"};
static char *g_i2c_reg_name[]			= {"[M][AVDD1]","[M][AVDD2]","[M][VON  ]","[M][VSS1 ]","[M][VSS2 ]","[M][VSS3 ]","[S][VON  ]","[S][VCLT ]","[S][VOFF ]","[S][VCLN ]"};
static char *g_vby1_equal_opt_name[]	= {"EQDC :", "EQACL:", "EQACU:"};
static char *g_pre_emphasis_name[]		= {"VOD      :", "1Pre_tap :", "2Pre_tap :", "1Post_tap:", "2Post_tap:", "Default set"};


static void 			on_rcb_poll_in( rcb_t *sender);
static void 			on_rcb_poll_out( rcb_t *sender);
static void 			on_rcb_poll_err( rcb_t *sender);
static void 			on_rcb_poll_hup( rcb_t *sender);


unsigned int	fwd_long_cnt, bwd_long_cnt;
int				var_freq_changed=0;
int 			var_vdd_changed=0;

char			rcb_slide_upline_buf[MAX_PATH], rcb_slide_downline_buf[MAX_PATH];
uint			rcb_slide_upline_length, rcb_slide_downline_length;
uint			rcb_slide_upline_cnt, rcb_slide_downline_cnt;
uint			rcb_slide_upline_inc, rcb_slide_downline_inc;



static rcb_t *new_socket( void)
{
	rcb_t *sock;

	sock  = (rcb_t *)malloc( sizeof( rcb_t));
	if ( NULL != sock)
	{
		sock->fd              = -1;
		sock->tag             = 0;
		sock->type            = STYP_RS232;
		sock->on_poll_in      = on_rcb_poll_in;
		sock->on_poll_out     = on_rcb_poll_out;
		sock->on_poll_err     = on_rcb_poll_err;
		sock->on_poll_hup     = on_rcb_poll_hup;
		sock->on_read         = NULL;
		sock->on_writable     = NULL;
	}
	return  sock;
}


static int socket_open( rcb_t *_sock)
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

	newtio.c_iflag 		= 0;
	newtio.c_oflag      = 0;
	newtio.c_lflag      = 0;
	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN]   = 1;

	tcflush  (_sock->fd, TCIFLUSH );
	tcsetattr(_sock->fd, TCSANOW, &newtio );

	return NERR_NONE;
}


static void  socket_close( rcb_t *_sock)
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


static int socket_reopen( rcb_t *_sock)
{
	int   ndx_poll;
	int   reopen_ok;

	ndx_poll 	= socket_get_poll_ndx( _sock->fd);
	socket_close( _sock);
	reopen_ok 	= socket_open( _sock);
	if ( NERR_NONE == reopen_ok)
	{
		poll_set_fd( ndx_poll, _sock->fd);
	}
	return NERR_NONE == reopen_ok;
}


static void on_rcb_poll_err( rcb_t *sender)
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
		rcb_close( sender);
	}
}


static void on_rcb_poll_hup( rcb_t *sender)
{
}


static void on_rcb_poll_in( rcb_t *sender)
{
	int   sz_read;

	if ( NULL == sender->on_read)
	{
		sz_read = read( sender->fd, __rcb_sock_buf, __MAX_BUF_SIZE);
		if ( 0 >= sz_read)
		{
			if ( NULL != sender->on_error)
			{
				on_rcb_poll_err( sender);
			}
		}
	}
	else
	{
		sender->on_read( sender);
	}
}


void on_rcb_poll_out( rcb_t *sender)
{
	if ( NULL != sender->on_writable)
	{
		sender->on_writable( sender);
	}
}


rcb_t *rcb_init(void)
{
	rcb_fd = rcb_open(DEV_RCB, RCB_BAUD_RATE, 8, 'n', 1);
	if(rcb_fd == NULL)
	{
		fprintf(stderr, "rcb_open() failed!(%d)\n", neterr_no);
	}
	else
	{
		printf("%s rcb connected!\n", DEV_RCB);
		rcb_fd->on_read 	= on_rcb_recv;
		rcb_fd->on_error 	= on_rcb_error;

		//rcb_start();
	}

	set_onoff_flag(ENUM_OFF);
	cursor_flag = CURSOR_X;
	file_flag	= FILE_ONE;

	return rcb_fd;
}


rcb_t *rcb_open( char *_devname, int _baud, int _databit, int _parity, int _stopbit)
{
	rcb_t *sock;

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
		free( sock);
		return NULL;
	}

	if ( 0 > poll_register( (net_obj_t *)sock))
	{
		neterr_no   = NERR_NO_POLL;
		free( sock);
		return NULL;
	}

	buft_cnt 	= 0;
	rx_push		= 0;

	return sock;
}


int rcb_read( rcb_t *_sock, char *_buf, int _buf_size)
{
	int   sz_read;

	memset(_buf, 0, _buf_size);
	sz_read   = read(_sock->fd, _buf, _buf_size);

	if ( 0 >= sz_read)
	{
		on_rcb_poll_err(_sock);
	}
	return sz_read;
}

void set_rcb_slide_start_time(void)
{
	gettimeofday(&rcb_slide_start_time, NULL);
}

uint64_t get_rcb_slide_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &rcb_slide_start_time, &result_time);

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}

// | STX | mode | data | ETX |
// |  1  |   1  |   n  |  1  |
int rcb_write( rcb_t *_sock, char mode, char *_buf)
{
	if(_sock == NULL) return NACK;

	char	txbuf[__MAX_BUF_SIZE];
	int		data_len 	= strlen(_buf);
	int		total_len 	= data_len + 4;

	memset(txbuf, 0, __MAX_BUF_SIZE);

	txbuf[0] 			= STX_RCB;
	txbuf[1] 			= mode;
	if(data_len>0) memcpy(txbuf+2, _buf, (data_len>MAX_LINE_TEXT) ? MAX_LINE_TEXT : data_len); // must be less than MAX_LINE_TEXT
	txbuf[total_len-2] 	= ETX_RCB;
	txbuf[total_len-1] 	= '\n';

	if(1)
	{
		rcb_slide_upline_cnt=0;
		rcb_slide_downline_cnt=0;
		rcb_slide_upline_inc=1;
		rcb_slide_downline_inc=1;

//		printf("rcb string length = %d, %c, %s\n", data_len, mode, _buf);

		if( (data_len>MAX_LINE_TEXT) )
		{
			if(data_len>MAX_PATH)	data_len = MAX_PATH;

			if(mode==RCB_LINE1)
			{
//				if(strncmp(rcb_slide_upline_buf, _buf, data_len))
//				{
					memset(rcb_slide_upline_buf, 0, MAX_PATH);
					memcpy(rcb_slide_upline_buf, _buf, data_len);
					rcb_slide_upline_length = data_len-MAX_LINE_TEXT;
//					dprintf("rcb upline over length = %d, %s\n", rcb_slide_upline_length, rcb_slide_upline_buf);
//				}
			}
			else if(mode==RCB_LINE2)
			{
//				if(strncmp(rcb_slide_downline_buf, _buf, data_len))
//				{
					memset(rcb_slide_downline_buf, 0, MAX_PATH);
					memcpy(rcb_slide_downline_buf, _buf, data_len);
					rcb_slide_downline_length = data_len-MAX_LINE_TEXT;
//					dprintf("rcb downline over length = %d, %s\n", rcb_slide_downline_length, rcb_slide_downline_buf);
//				}
			}
			//When rcb_slide_length over MAX_LINE_TEXT in model_manu, output slide string.

//			set_rcb_slide_start_time();
		}
		else
		{
			if(mode==RCB_LINE1)			rcb_slide_upline_length=0;
			else if (mode==RCB_LINE2)	rcb_slide_downline_length=0;
//			printf("rcb slide length reset\n");
		}
		set_rcb_slide_start_time();
	}


#if defined(USE_I2C_LCD)
	if		(mode==RCB_LINE1)	i2c_lcd_set_data(0, _buf);	// P/G LCD 1st Line
	else if	(mode==RCB_LINE2)	i2c_lcd_set_data(1, _buf);	// P/G LCD 2nd Line
#endif

	return  write(_sock->fd, txbuf, total_len);		// RCB LCD
}


rcb_t *rcb_find_tag( int _tag)
{
	rcb_t  *sock;
	int       cnt_objs;
	int       ndx;

	cnt_objs = poll_count();

	for ( ndx = 0; ndx < cnt_objs; ndx++)
	{
		sock  = ( rcb_t *)poll_obj( ndx);
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


void rcb_close( rcb_t *_sock)
{
	char ver[MAX_TEXT_BUF];

	memset(ver, 0, sizeof(ver));
	sprintf(ver, "F/W R%d.%d", FW_VERSION/10, FW_VERSION%10);

	rcb_write(rcb_fd, RCB_LINE1, TITLE_PG_END);
	rcb_write(rcb_fd, RCB_LINE2, ver);

	socket_close( _sock);

	poll_unregister( (net_obj_t *)_sock);
	free(_sock);
}


void on_rcb_error( rcb_t *sender, int reopen_ok)
{
	if ( 0 == reopen_ok)
	{
		printf("%s reopened.\n", sender->devname);
	}
	else
	{
		fprintf(stderr, "on_rcb_error() error!!\n");
	}
}

void on_rcb_recv( rcb_t *sender)
{
	int				sz_read, i;
	char			rx_buf[__MAX_BUF_SIZE]/*, buf[__MAX_BUF_SIZE]*/;

	memset(rx_buf, 0, __MAX_BUF_SIZE);

	sz_read = rcb_read(sender, rx_buf, __MAX_BUF_SIZE);
	if (0 < sz_read)
	{
		for(i=0; i<sz_read; i++)
		{
			if(rx_buf[i] == SRX_RCB)		// '['
			{
				buft_cnt 			= 0;
				memset(rx_buft, 0, __MAX_BUF_SIZE);
				rx_buft[buft_cnt++] = rx_buf[i];
			}
			else if(rx_buf[i] == ERX_RCB)	// ']'
			{
				rx_buft[buft_cnt++]	= rx_buf[i];
				printf("[RCB RX] : %s \n", rx_buft);

				rcb_proc(rx_buft+1);

#if defined(USE_RCB485)
				rcb485_write(rcb485_fd, rx_buft+1, 2); // to slave
#endif

				// initialize
				buft_cnt 			= 0;
				memset(rx_buft, 0, __MAX_BUF_SIZE);
			}
			else
			{
				rx_buft[buft_cnt++] = rx_buf[i];
			}
		}
	}
	else
	{
		fprintf(stderr, "on_rcb_recv() error\n");
	}
}

void rcb_proc(char *c)
{
	switch(get_opmode())
	{
		case MENU			: rcb_menu_proc(c);				break;
		case SERVER_IP		: rcb_server_ip_proc(c);		break;
		case GROUP_ID		: rcb_group_id_proc(c);			break;
		case PWR_SELECT		: rcb_pwr_select_proc(c);		break;
		case MODEL_CHANGE	: rcb_model_change_proc(c);		break;
		case READY			: rcb_ready_proc(c);			break;
		case READY_FUNC		: rcb_ready_func_proc(c);		break;
		case AUTO_RUN		: rcb_auto_run_proc(c);			break;
		case MANU_RUN		: rcb_manu_run_proc(c);			break;
		case CURSOR			: rcb_cursor_proc(c);			break;
		case GRAY_CHANGE	: rcb_color_change_proc(c);		break;
		case GRAY_SCALE		: rcb_gray_scale_proc(c);		break;
		case FREQUENCY		: rcb_freq_proc(c);				break;
		case HTIME_CHANGE	: rcb_htime_proc(c);			break;
		case VTIME_CHANGE	: rcb_vtime_proc(c);			break;
		case CH_SHIFT		: rcb_ch_shift_proc(c);			break;
		case BIT_CHANGE		: rcb_bit_shift_proc(c);		break;
		case DIMM_CHANGE	: rcb_dim_change_proc(c);		break;
		case PWM_CHANGE		: rcb_pwm_change_proc(c);		break;
		case SCROLL			: rcb_scroll_test_proc(c);		break;
		case MODEL_DELETE	: rcb_model_delete_proc(c);		break;
		case PATTERN_DELETE	: rcb_pattern_delete_proc(c);	break;
		case BITMAP_DELETE	: rcb_bitmap_delete_proc(c);	break;
		case USB_DETECTED	: rcb_usb_detect_proc(c);		break;
		case NETWORK_INFO	: rcb_network_info_proc(c);		break;
		case WIFI_IP		: rcb_wifi_ip_proc(c);			break;
		case WIFI_NETMASK	: rcb_wifi_netmask_proc(c);		break;
		case WIFI_GATEWAY	: rcb_wifi_gateway_proc(c);		break;
		case WIFI_SSID		: rcb_wifi_ssid_proc(c);		break;
		case WIFI_IDENTITY	: rcb_wifi_identity_proc(c);	break;
		case WIFI_PASSWORD	: rcb_wifi_password_proc(c);	break;
		case WIFI_SELECT	: rcb_wifi_select_proc(c);		break;
		case QEMS_EQPID		: rcb_qems_eqpid_proc(c);		break;
		case SPC_SET		: rcb_pwr_spc_set_proc(c);		break;
		case PWR_FW_UPLOAD	: rcb_pwr_update_proc(c);		break;
		case I2C_CTRL		: rcb_tcon_i2c_ctrl_proc(c);	break;
		case VBY1_EQUAL		: rcb_vby1_equal_option_proc(c);	break;
#ifdef GROUP_SELECT
		case GROUP_CHANGE	: rcb_group_change_proc(c);		break;
		case GROUP_DELETE	: rcb_group_delete_proc(c);		break;
#endif
		case VDD_CHANGE		: rcb_vdd_change_proc(c);		break;
		case VBL_CHANGE		: rcb_vbl_change_proc(c);		break;

		case STORAGE_INFO	: rcb_storage_info_proc(c);		break;

#ifdef USE_PRE_EMPHASIS
		case PRE_EMPHASIS	: rcb_pre_emphasis_proc(c);		break;
#endif
		case CURSOR_COLOR	: rcb_cursor_color_proc(c);		break;

		// ELVDD Alarm
		case VBL_CHECK_WARN	:	rcb_vbl_check_warning_proc(c);		break;
		case VBL_CHECK_INFO	:	rcb_elvdd_alarm_set_info_proc(c);		break;

		default: break;
	}
}

float freq_to_mhz(uint64_t freq)
{
	return (float)(freq/1000000.0);
}

float freq_to_vsync(uint64_t freq)
{
	float vsync_hz	= 0;
	float total 	= model_data.h_total * model_data.v_total;
	if(total > 0)
	{
		vsync_hz = freq/(float)total;
	}
	return vsync_hz;
}

uint64_t vsync_to_freq(uint16_t vsync)
{
	uint64_t freq = 0;

	uint64_t total = model_data.h_total * model_data.v_total;
	if(total>0)
	{
		freq = (vsync/10.0) * total;
	}

	return freq;
}

void set_qems_info(char *eqpid)
{
	FILE 	*fp;
	char 	path[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, QEMS_INFO_FILE);
	fp = fopen(path, "w");
	if(fp!=NULL)
	{
		memset(qems_eqpid, 0, MAX_QEMS_EQPID);
		memcpy(qems_eqpid,eqpid,MAX_QEMS_EQPID);
		fputs(qems_eqpid, fp);
		fclose(fp);
		sync();

		rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
	}

	sleep(1);
	rcb_write(rcb_fd, RCB_LINE2, qems_eqpid);
}

void set_qems8k_info(char *eqpid)
{
	FILE 	*fp;
	char 	path[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, QEMS_8K_INFO_FILE);
	fp = fopen(path, "w");
	if(fp!=NULL)
	{
		memset(qems_master_eqpid, 0, MAX_QEMS_EQPID);
		memcpy(qems_master_eqpid,eqpid,MAX_QEMS_EQPID);
		fputs(qems_master_eqpid, fp);
		fclose(fp);
		sync();

		rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
	}

	sleep(1);
	rcb_write(rcb_fd, RCB_LINE2, qems_master_eqpid);
}

void set_wifi_info(int select,char *ssid,int *ip,int *netmask,int *gateway,char *identity,char *password)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, WIFI_INFO_FILE);


	wifi_select = select;
	memcpy(wifi_ssid, ssid, MAX_WIFI_SSID);

	wifi_ip[0] = ip[0];
	wifi_ip[1] = ip[1];
	wifi_ip[2] = ip[2];
	wifi_ip[3] = ip[3];

	wifi_netmask[0] = netmask[0];
	wifi_netmask[1] = netmask[1];
	wifi_netmask[2] = netmask[2];
	wifi_netmask[3] = netmask[3];

	wifi_gateway[0] = gateway[0];
	wifi_gateway[1] = gateway[1];
	wifi_gateway[2] = gateway[2];
	wifi_gateway[3] = gateway[3];

	memcpy(wifi_identity,identity,MAX_WIFI_IDENTITY);
	memcpy(wifi_password,password,MAX_WIFI_PASSWORD);

	fp = fopen(path, "w");

	if(fp!=NULL)
	{
		sprintf(str, "%d\r\n", select);
		fputs(str, fp);

		sprintf(str, "%s\r\n", ssid);
		fputs(str, fp);

		sprintf(str, "%d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
		fputs(str, fp);
		sprintf(str, "%d.%d.%d.%d\r\n", netmask[0], netmask[1], netmask[2], netmask[3]);
		fputs(str, fp);
		sprintf(str, "%d.%d.%d.%d\r\n", gateway[0], gateway[1], gateway[2], gateway[3]);
		fputs(str, fp);
		sprintf(str, "%s\r\n", identity);
		fputs(str, fp);
		sprintf(str, "%s\r\n", password);
		fputs(str, fp);
		fclose(fp);
		sync();
		rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
	}
	sleep(1);
}

void set_server_ip(int *pip)
{
	FILE 	*fp;
	char 	path[MAX_PATH], ip[MAX_IP_ADDR];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, SERVER_IP_FILE);
	fp = fopen(path, "w");
	if(fp!=NULL)
	{
		server_ip[0] = pip[0];
		server_ip[1] = pip[1];
		server_ip[2] = pip[2];
		server_ip[3] = pip[3];

		memset(ip, 0, MAX_IP_ADDR);
		sprintf(ip, "%d.%d.%d.%d", server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
		fputs(ip, fp);
		fclose(fp);
		sync();

		rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
	}

	sleep(1);
	memset(ip, 0, MAX_IP_ADDR);
	sprintf(ip, "%03d.%03d.%03d.%03d", server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
	rcb_write(rcb_fd, RCB_LINE2, ip);
}

void set_group_ip(uint8_t id)
{
	FILE 	*fp;
	char 	data[MAX_PATH], eth_name[MAX_PATH];
	uint8_t	dipsw_ip;

	memset(eth_name, 0, MAX_PATH);
	get_eth_name(eth_name);

	dipsw_ip 	= i2c_gpio_get_ipsw();

	fp = fopen(NET_INTERFACE_FILE, "wb");
	if(fp!=NULL)
	{
		memset(data, 0, sizeof(data));
		sprintf(data, "# interfaces(5) file used by ifup(8) and ifdown(8)\n");
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data, "source-directory /etc/network/interfaces.d\n");
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data, "auto %s\n", eth_name);
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data,"iface %s inet static\n", eth_name);
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data,"address %s%d.%d\n", DEF_PREFIX_IP, id, dipsw_ip);
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data,"netmask %s\n", DEF_SUBNET_MASK);
		fwrite(data, strlen(data), 1, fp);

		memset(data, 0, sizeof(data));
		sprintf(data,"gateway %s%d.1\n", DEF_PREFIX_IP, id);
		fwrite(data, strlen(data), 1, fp);

		fclose(fp);

		set_group_id(id);	// groupid.txt

		rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
	}
	else
	{
		rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
	}
	sleep(1);
}

void rcb_start(void)
{
	if(rcb_fd == NULL) return;

	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));
	sprintf(str, "%s STARTED", PG_NAME);
	rcb_write(rcb_fd, RCB_LINE1, str);

	memset(str, 0, sizeof(str));
	sprintf(str, "F/W R%d.%d", FW_VERSION/10, FW_VERSION%10);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(MODEL_CHANGE);
}

void rcb_model_change(void)
{
	rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
	rcb_write(rcb_fd, RCB_LINE2, model_list[model_idx]);

	reload_model_flag=0;
//	memset(old_model_name,0,sizeof(old_model_name));
//	memcpy(old_model_name, model_name, sizeof(model_name));

	set_opmode(MODEL_CHANGE);
}

void rcb_menu(void)
{
	menu_idx = 0;
	rcb_write(rcb_fd, RCB_LINE1, TITLE_MENU);
	rcb_write(rcb_fd, RCB_LINE2, g_menu[menu_idx]);

	set_opmode(MENU);
}


void rcb_server_ip_set(void)
{
	char 	ip[MAX_IP_ADDR];
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;

	// default value
	lcd_cursor_pos 		= 0;
	server_ip[0]		= DEF_IP_CLASS0;
	server_ip[1]		= DEF_IP_CLASS1;
	server_ip[2]		= DEF_IP_CLASS2;
	server_ip[3]		= DEF_IP_CLASS3;
	var_server_ip[0]	= DEF_IP_CLASS0;
	var_server_ip[1]	= DEF_IP_CLASS1;
	var_server_ip[2]	= DEF_IP_CLASS2;
	var_server_ip[3]	= DEF_IP_CLASS3;

	memset(ip, 0, MAX_IP_ADDR);
	get_server_ip(ip);

	ptr = strtok(ip, ".");
	while(ptr!=NULL)
	{
		server_ip[i] 		= atoi(ptr);
		var_server_ip[i] 	= server_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}

	rcb_write(rcb_fd, RCB_LINE1, TITLE_SERVER_IP);

	memset(ip, 0, MAX_IP_ADDR);
	sprintf(ip, "%03d.%03d.%03d.%03d", server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
	rcb_write(rcb_fd, RCB_LINE2, ip);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);

	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);
	set_opmode(SERVER_IP);
}

void rcb_group_id_set(void)
{
	char 	str[MAX_TEXT_BUF];

	group_id 		= get_group_id();
	var_group_id 	= group_id;

	rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_ID);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, "%d", group_id);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(GROUP_ID);
}

void rcb_pwr_select_set(void)
{
	char 	str[MAX_TEXT_BUF];

	var_pwr_sel		= get_pwr_vendor();

	rcb_write(rcb_fd, RCB_LINE1, TITLE_PWR_SELECT);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, "%s", g_vendor[var_pwr_sel]);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(PWR_SELECT);
}

void rcb_wifi_select_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char 	str[MAX_TEXT_BUF];
	char 	*ptr;
	int 	i=0;

	var_wifi_sel		= get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);
	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_SELECT);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, "%s", g_wifi_select[var_wifi_sel]);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(WIFI_SELECT);
}

void rcb_wifi_ip_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;

	// default value
	lcd_cursor_pos 		= 0;
	wifi_ip[0]		= DEF_IP_CLASS0;
	wifi_ip[1]		= DEF_IP_CLASS1;
	wifi_ip[2]		= DEF_IP_CLASS2;
	wifi_ip[3]		= DEF_IP_CLASS3;
	var_wifi_ip[0]	= DEF_IP_CLASS0;
	var_wifi_ip[1]	= DEF_IP_CLASS1;
	var_wifi_ip[2]	= DEF_IP_CLASS2;
	var_wifi_ip[3]	= DEF_IP_CLASS3;

	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);

	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_IP);

	memset(ip1, 0, MAX_IP_ADDR);
	sprintf(ip1, "%03d.%03d.%03d.%03d", wifi_ip[0], wifi_ip[1], wifi_ip[2], wifi_ip[3]);
	rcb_write(rcb_fd, RCB_LINE2, ip1);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);
	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);

	set_opmode(WIFI_IP);

}

void rcb_wifi_netmask_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;

	// default value
	lcd_cursor_pos 		= 0;
	wifi_netmask[0]		= DEF_IP_CLASS0;
	wifi_netmask[1]		= DEF_IP_CLASS1;
	wifi_netmask[2]		= DEF_IP_CLASS2;
	wifi_netmask[3]		= DEF_IP_CLASS3;
	var_wifi_netmask[0]	= DEF_IP_CLASS0;
	var_wifi_netmask[1]	= DEF_IP_CLASS1;
	var_wifi_netmask[2]	= DEF_IP_CLASS2;
	var_wifi_netmask[3]	= DEF_IP_CLASS3;

	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);

	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_NETMASK);

	memset(ip1, 0, MAX_IP_ADDR);
	sprintf(ip1, "%03d.%03d.%03d.%03d", wifi_netmask[0], wifi_netmask[1], wifi_netmask[2], wifi_netmask[3]);
	rcb_write(rcb_fd, RCB_LINE2, ip1);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);

	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);
	set_opmode(WIFI_NETMASK);

}

void rcb_wifi_gateway_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;

	// default value
	lcd_cursor_pos 		= 0;
	wifi_gateway[0]		= DEF_IP_CLASS0;
	wifi_gateway[1]		= DEF_IP_CLASS1;
	wifi_gateway[2]		= DEF_IP_CLASS2;
	wifi_gateway[3]		= DEF_IP_CLASS3;
	var_wifi_gateway[0]	= DEF_IP_CLASS0;
	var_wifi_gateway[1]	= DEF_IP_CLASS1;
	var_wifi_gateway[2]	= DEF_IP_CLASS2;
	var_wifi_gateway[3]	= DEF_IP_CLASS3;

	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);

	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}

	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_GATEWAY);

	memset(ip1, 0, MAX_IP_ADDR);
	sprintf(ip1, "%03d.%03d.%03d.%03d", wifi_gateway[0], wifi_gateway[1], wifi_gateway[2], wifi_gateway[3]);
	rcb_write(rcb_fd, RCB_LINE2, ip1);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);

	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);
	set_opmode(WIFI_GATEWAY);

}

void rcb_wifi_ssid_set(void)
{

	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;
	// default value
	lcd_cursor_pos 		= 0;

	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);


	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}

	memcpy(var_wifi_ssid,wifi_ssid,sizeof(wifi_ssid));
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_SSID);

	rcb_write(rcb_fd, RCB_LINE2, wifi_ssid);
	i2c_lcd_change_cursor(lcd_cursor_pos, 1);
	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);

	set_opmode(WIFI_SSID);
}

void rcb_qems_eqpid_set(void)
{
	char	cursor_command[4];
	// default value
	lcd_cursor_pos 		= 0;

	get_qems_info(var_qems_eqpid);
	memcpy(var_qems_eqpid,qems_eqpid,sizeof(qems_eqpid));
	rcb_write(rcb_fd, RCB_LINE1, TITLE_QEMS_EQPID);

	rcb_write(rcb_fd, RCB_LINE2, var_qems_eqpid);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);
	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);

	set_opmode(QEMS_EQPID);
}

void rcb_qems_master_eqpid_set(void)
{

	char 	cursor_comand[4];
	// default value
	lcd_cursor_pos 		= 0;

	get_qems_master_info(var_qems_master_eqpid);
	memcpy(var_qems_master_eqpid,qems_master_eqpid,sizeof(qems_master_eqpid));
	rcb_write(rcb_fd, RCB_LINE1, TITLE_QEMS_MASTER_EQPID);

	rcb_write(rcb_fd, RCB_LINE2, var_qems_master_eqpid);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);
	sprintf(cursor_comand,"%d,%d",lcd_cursor_pos,1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_comand);

	set_opmode(QEMS_MASTER_EQPID);
}

void rcb_wifi_identity_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;
	// default value
	lcd_cursor_pos 		= 0;

	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);
	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}

	memcpy(var_wifi_identity,wifi_identity,sizeof(wifi_identity));
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_IDENTITY);

	rcb_write(rcb_fd, RCB_LINE2, wifi_identity);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);
	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);

	set_opmode(WIFI_IDENTITY);
}

void rcb_wifi_password_set(void)
{
	char 	ip1[MAX_IP_ADDR]={0,};
	char 	ip2[MAX_IP_ADDR]={0,};
	char 	ip3[MAX_IP_ADDR]={0,};
	char	cursor_command[4];
	char 	*ptr;
	int 	i=0;

	// default value
	lcd_cursor_pos 		= 0;
	wifi_select=get_wifi_info(wifi_ssid,ip1,ip2,ip3,wifi_identity,wifi_password);

	ptr = strtok(ip1, ".");
	while(ptr!=NULL)
	{
		wifi_ip[i] 		= atoi(ptr);
		var_wifi_ip[i] 	= wifi_ip[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip2, ".");
	while(ptr!=NULL)
	{
		wifi_netmask[i] 		= atoi(ptr);
		var_wifi_netmask[i] 	= wifi_netmask[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	i=0;
	ptr = strtok(ip3, ".");
	while(ptr!=NULL)
	{
		wifi_gateway[i] 		= atoi(ptr);
		var_wifi_gateway[i] 	= wifi_gateway[i];
		ptr = strtok(NULL, ".");
		i++;
	}
	memcpy(var_wifi_password,wifi_password,sizeof(wifi_password));
	rcb_write(rcb_fd, RCB_LINE1, TITLE_WIFI_PASSWORD);

	rcb_write(rcb_fd, RCB_LINE2, wifi_password);

	i2c_lcd_change_cursor(lcd_cursor_pos, 1);

	sprintf(cursor_command, "%d,%d", lcd_cursor_pos, 1);
	rcb_write(rcb_fd, RCB_CURSOR, cursor_command);
	set_opmode(WIFI_PASSWORD);
}

void rcb_pwr_spc_onoff_set(void)
{
	char 	str[MAX_TEXT_BUF];

	rcb_write(rcb_fd, RCB_LINE1, TITLE_PWR_SPC_SET);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, "%s", g_pwr_spc[var_spc_en]);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(SPC_SET);
}

void rcb_pwr_fw_upload(void)
{
	char 	str[MAX_TEXT_BUF];

//	memset(str, 0, MAX_TEXT_BUF);
//	sprintf(str, "%s R%d", TITLE_PWR_UPLOAD, rsp_ver_osung_data.fw_ver);
//	rcb_write(rcb_fd, RCB_LINE1, str);
	rcb_write(rcb_fd, RCB_LINE1, TITLE_PWR_UPLOAD);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, "%s", "OK TO UPDATE");
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(PWR_FW_UPLOAD);
}

void rcb_tcon_i2c_ctrl(void)
{
	char 	str[MAX_TEXT_BUF];
	int		i;

	for(i=0;i<(MAX_I2C_REG_CNT+1);i++)
	{
		if(i<6)
		{
			if(tcon_i2c_reg_data[i] <= model_data.tcon_i2c_test[0].data_min[i]) tcon_i2c_reg_data[i] = model_data.tcon_i2c_test[0].data_min[i];
			else if (tcon_i2c_reg_data[i] >= model_data.tcon_i2c_test[0].data_max[i]) tcon_i2c_reg_data[i] = model_data.tcon_i2c_test[0].data_max[i];

			tcon_i2c_data_min[i] = model_data.tcon_i2c_test[0].data_min[i];
			tcon_i2c_data_max[i] = model_data.tcon_i2c_test[0].data_max[i];
		}
		else
		{
			if(tcon_i2c_reg_data[i] <= model_data.tcon_i2c_test[1].data_min[i-6]) tcon_i2c_reg_data[i] = model_data.tcon_i2c_test[1].data_min[i-6];
			else if (tcon_i2c_reg_data[i] >= model_data.tcon_i2c_test[1].data_max[i-6]) tcon_i2c_reg_data[i] = model_data.tcon_i2c_test[1].data_max[i-6];

			tcon_i2c_data_min[i] = model_data.tcon_i2c_test[1].data_min[i-6];
			tcon_i2c_data_max[i] = model_data.tcon_i2c_test[1].data_max[i-6];
		}
	}
	sprintf(str, "%s%s", TITLE_I2C_SET, g_i2c_reg_name[tcon_i2c_reg_index]);
	rcb_write(rcb_fd, RCB_LINE1, str);

	memset(str, 0, MAX_TEXT_BUF);
	sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
	rcb_write(rcb_fd, RCB_LINE2, str);

	set_opmode(I2C_CTRL);
}

void send_tcon_i2c(void)
{
	unsigned char slave_index=0;

	if(tcon_i2c_reg_index<MAX_I2C_M_CNT)
	{
		//master
		i2c_gpio_set(GPIO_EX16, 0x01);		// Master WP high

		FPGA_Write(FPGA_I2C_MASTER_CLK_SEL, model_data.tcon_i2c_test[0].i2c_clock_sel & 0x00ff);
		FPGA_Write(FPGA_I2C_MASTER_REG_ADDR_SIZE, (model_data.tcon_i2c_test[0].reg_addr_size + 1) & 0x00ff);
		FPGA_Write(FPGA_I2C_MASTER_REG_DATA_SIZE, (model_data.tcon_i2c_test[0].data_size + 1) & 0x00ff);
		FPGA_Write(FPGA_I2C_MASTER_DEV_ADDR, model_data.tcon_i2c_test[0].dev_addr & 0x00ff);
		if(model_data.tcon_i2c_test[0].reg_addr_size == 0)
		{
			FPGA_Write(FPGA_I2C_MASTER_REG_ADDR, model_data.tcon_i2c_test[0].reg_addr[tcon_i2c_reg_index] & 0x00ff);
		}
		else
		{
			FPGA_Write(FPGA_I2C_MASTER_REG_ADDR, model_data.tcon_i2c_test[0].reg_addr[tcon_i2c_reg_index] & 0xffff);
		}

		if(model_data.tcon_i2c_test[0].data_size == 0)
		{
			FPGA_Write(FPGA_I2C_MASTER_REG_DATA, tcon_i2c_reg_data[tcon_i2c_reg_index] & 0x00ff);
		}
		else
		{
			FPGA_Write(FPGA_I2C_MASTER_REG_DATA, tcon_i2c_reg_data[tcon_i2c_reg_index] & 0xffff);
		}
		printf("master ack : %d\n", FPGA_Read(FPGA_I2C_MASTER_ACK));
		usleep(100);
		FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);
		FPGA_Write(FPGA_I2C_MASTER_EN, 0x0001);
		usleep(2000);

		i2c_gpio_set(GPIO_EX16, 0x00);		// Master WP low
	}
	else
	{
		//slave
		slave_index = tcon_i2c_reg_index-MAX_I2C_M_CNT;

		i2c_gpio_set(GPIO_EX17, 0x01);		// Slave WP high

		FPGA_Write(FPGA_I2C_SLAVE_CLK_SEL, model_data.tcon_i2c_test[1].i2c_clock_sel & 0x00ff);
		FPGA_Write(FPGA_I2C_SLAVE_REG_ADDR_SIZE, (model_data.tcon_i2c_test[1].reg_addr_size + 1) & 0x00ff);
		FPGA_Write(FPGA_I2C_SLAVE_REG_DATA_SIZE, (model_data.tcon_i2c_test[1].data_size + 1) & 0x00ff);
		FPGA_Write(FPGA_I2C_SLAVE_DEV_ADDR, model_data.tcon_i2c_test[1].dev_addr & 0x00ff);
		if(model_data.tcon_i2c_test[1].reg_addr_size == 0)
		{
			FPGA_Write(FPGA_I2C_SLAVE_REG_ADDR, model_data.tcon_i2c_test[1].reg_addr[slave_index] & 0x00ff);
		}
		else
		{
			FPGA_Write(FPGA_I2C_SLAVE_REG_ADDR, model_data.tcon_i2c_test[1].reg_addr[slave_index] & 0xffff);
		}

		if(model_data.tcon_i2c_test[1].data_size == 0)
		{
			FPGA_Write(FPGA_I2C_SLAVE_REG_DATA, tcon_i2c_reg_data[tcon_i2c_reg_index] & 0x00ff);
		}
		else
		{
			FPGA_Write(FPGA_I2C_SLAVE_REG_DATA, tcon_i2c_reg_data[tcon_i2c_reg_index] & 0xffff);
		}
		printf("slave ack : %d\n", FPGA_Read(FPGA_I2C_SLAVE_ACK));
		usleep(100);
		FPGA_Write(FPGA_I2C_SLAVE_EN, 0x0000);
		FPGA_Write(FPGA_I2C_SLAVE_EN, 0x0001);
		usleep(2000);

		i2c_gpio_set(GPIO_EX17, 0x00);		// Slave WP low

	}
}

void rcb_group_change(int flag)
{
	rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
	if(flag)	rcb_write(rcb_fd, RCB_LINE2, group_list[group_idx]);
	else		rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");

	set_opmode(GROUP_CHANGE);
}



void rcb_server_ip_proc(char *c)
{
	char str[MAX_TEXT_BUF];
	int num = 0;

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_server_ip[0]/100;
			if(--num<0) var_server_ip[0] += 900;
			else 		var_server_ip[0] -= 100;
			break;
		case 1:
			num = (var_server_ip[0]/10)%10;
			if(--num<0) var_server_ip[0] += 90;
			else 		var_server_ip[0] -= 10;
			break;
		case 2:
			num = var_server_ip[0]%10;
			if(--num<0) var_server_ip[0] += 9;
			else 		var_server_ip[0] -= 1;
			break;
		case 4:
			num = var_server_ip[1]/100;
			if(--num<0) var_server_ip[1] += 900;
			else 		var_server_ip[1] -= 100;
			break;
		case 5:
			num = (var_server_ip[1]/10)%10;
			if(--num<0) var_server_ip[1] += 90;
			else 		var_server_ip[1] -= 10;
			break;
		case 6:
			num = var_server_ip[1]%10;
			if(--num<0) var_server_ip[1] += 9;
			else 		var_server_ip[1] -= 1;
			break;
		case 8:
			num = var_server_ip[2]/100;
			if(--num<0) var_server_ip[2] += 900;
			else 		var_server_ip[2] -= 100;
			break;
		case 9:
			num = (var_server_ip[2]/10)%10;
			if(--num<0) var_server_ip[2] += 90;
			else 		var_server_ip[2] -= 10;
			break;
		case 10:
			num = var_server_ip[2]%10;
			if(--num<0) var_server_ip[2] += 9;
			else 		var_server_ip[2] -= 1;
			break;
		case 12:
			num = var_server_ip[3]/100;
			if(--num<0)	var_server_ip[3] += 900;
			else 		var_server_ip[3] -= 100;
			break;
		case 13:
			num = (var_server_ip[3]/10)%10;
			if(--num<0)	var_server_ip[3] += 90;
			else 		var_server_ip[3] -= 10;
			break;
		case 14:
			num = var_server_ip[3]%10;
			if(--num<0)	var_server_ip[3] += 9;
			else 		var_server_ip[3] -= 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_server_ip[0], var_server_ip[1], var_server_ip[2], var_server_ip[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_server_ip(var_server_ip);
		rcb_menu();
		//set_reboot(1);	// reboot
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_server_ip[0]/100;
			if(++num>9) var_server_ip[0] -= 900;
			else 		var_server_ip[0] += 100;
			break;
		case 1:
			num = (var_server_ip[0]/10)%10;
			if(++num>9) var_server_ip[0] -= 90;
			else 		var_server_ip[0] += 10;
			break;
		case 2:
			num = var_server_ip[0]%10;
			if(++num>9) var_server_ip[0] -= 9;
			else 		var_server_ip[0] += 1;
			break;
		case 4:
			num = var_server_ip[1]/100;
			if(++num>9) var_server_ip[1] -= 900;
			else 		var_server_ip[1] += 100;
			break;
		case 5:
			num = (var_server_ip[1]/10)%10;
			if(++num>9) var_server_ip[1] -= 90;
			else 		var_server_ip[1] += 10;
			break;
		case 6:
			num = var_server_ip[1]%10;
			if(++num>9) var_server_ip[1] -= 9;
			else 		var_server_ip[1] += 1;
			break;
		case 8:
			num = var_server_ip[2]/100;
			if(++num>9) var_server_ip[2] -= 900;
			else 		var_server_ip[2] += 100;
			break;
		case 9:
			num = (var_server_ip[2]/10)%10;
			if(++num>9) var_server_ip[2] -= 90;
			else 		var_server_ip[2] += 10;
			break;
		case 10:
			num = var_server_ip[2]%10;
			if(++num>9) var_server_ip[2] -= 9;
			else 		var_server_ip[2] += 1;
			break;
		case 12:
			num = var_server_ip[3]/100;
			if(++num>9)	var_server_ip[3] -= 900;
			else 		var_server_ip[3] += 100;
			break;
		case 13:
			num = (var_server_ip[3]/10)%10;
			if(++num>9)	var_server_ip[3] -= 90;
			else 		var_server_ip[3] += 10;
			break;
		case 14:
			num = var_server_ip[3]%10;
			if(++num>9)	var_server_ip[3] -= 9;
			else 		var_server_ip[3] += 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_server_ip[0], var_server_ip[1], var_server_ip[2], var_server_ip[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_server_ip_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_wifi_ip_proc(char *c)
{
	char str[MAX_TEXT_BUF];
	int num = 0;

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_ip[0]/100;
			if(--num<0) var_wifi_ip[0] += 900;
			else 		var_wifi_ip[0] -= 100;
			break;
		case 1:
			num = (var_wifi_ip[0]/10)%10;
			if(--num<0) var_wifi_ip[0] += 90;
			else 		var_wifi_ip[0] -= 10;
			break;
		case 2:
			num = var_wifi_ip[0]%10;
			if(--num<0) var_wifi_ip[0] += 9;
			else 		var_wifi_ip[0] -= 1;
			break;
		case 4:
			num = var_wifi_ip[1]/100;
			if(--num<0) var_wifi_ip[1] += 900;
			else 		var_wifi_ip[1] -= 100;
			break;
		case 5:
			num = (var_wifi_ip[1]/10)%10;
			if(--num<0) var_wifi_ip[1] += 90;
			else 		var_wifi_ip[1] -= 10;
			break;
		case 6:
			num = var_wifi_ip[1]%10;
			if(--num<0) var_wifi_ip[1] += 9;
			else 		var_wifi_ip[1] -= 1;
			break;
		case 8:
			num = var_wifi_ip[2]/100;
			if(--num<0) var_wifi_ip[2] += 900;
			else 		var_wifi_ip[2] -= 100;
			break;
		case 9:
			num = (var_wifi_ip[2]/10)%10;
			if(--num<0) var_wifi_ip[2] += 90;
			else 		var_wifi_ip[2] -= 10;
			break;
		case 10:
			num = var_wifi_ip[2]%10;
			if(--num<0) var_wifi_ip[2] += 9;
			else 		var_wifi_ip[2] -= 1;
			break;
		case 12:
			num = var_wifi_ip[3]/100;
			if(--num<0)	var_wifi_ip[3] += 900;
			else 		var_wifi_ip[3] -= 100;
			break;
		case 13:
			num = (var_wifi_ip[3]/10)%10;
			if(--num<0)	var_wifi_ip[3] += 90;
			else 		var_wifi_ip[3] -= 10;
			break;
		case 14:
			num = var_wifi_ip[3]%10;
			if(--num<0)	var_wifi_ip[3] += 9;
			else 		var_wifi_ip[3] -= 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_ip[0], var_wifi_ip[1], var_wifi_ip[2], var_wifi_ip[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,wifi_ssid,var_wifi_ip,wifi_netmask,wifi_gateway,wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_ip[0]/100;
			if(++num>9) var_wifi_ip[0] -= 900;
			else 		var_wifi_ip[0] += 100;
			break;
		case 1:
			num = (var_wifi_ip[0]/10)%10;
			if(++num>9) var_wifi_ip[0] -= 90;
			else 		var_wifi_ip[0] += 10;
			break;
		case 2:
			num = var_wifi_ip[0]%10;
			if(++num>9) var_wifi_ip[0] -= 9;
			else 		var_wifi_ip[0] += 1;
			break;
		case 4:
			num = var_wifi_ip[1]/100;
			if(++num>9) var_wifi_ip[1] -= 900;
			else 		var_wifi_ip[1] += 100;
			break;
		case 5:
			num = (var_wifi_ip[1]/10)%10;
			if(++num>9) var_wifi_ip[1] -= 90;
			else 		var_wifi_ip[1] += 10;
			break;
		case 6:
			num = var_wifi_ip[1]%10;
			if(++num>9) var_wifi_ip[1] -= 9;
			else 		var_wifi_ip[1] += 1;
			break;
		case 8:
			num = var_wifi_ip[2]/100;
			if(++num>9) var_wifi_ip[2] -= 900;
			else 		var_wifi_ip[2] += 100;
			break;
		case 9:
			num = (var_wifi_ip[2]/10)%10;
			if(++num>9) var_wifi_ip[2] -= 90;
			else 		var_wifi_ip[2] += 10;
			break;
		case 10:
			num = var_wifi_ip[2]%10;
			if(++num>9) var_wifi_ip[2] -= 9;
			else 		var_wifi_ip[2] += 1;
			break;
		case 12:
			num = var_wifi_ip[3]/100;
			if(++num>9)	var_wifi_ip[3] -= 900;
			else 		var_wifi_ip[3] += 100;
			break;
		case 13:
			num = (var_wifi_ip[3]/10)%10;
			if(++num>9)	var_wifi_ip[3] -= 90;
			else 		var_wifi_ip[3] += 10;
			break;
		case 14:
			num = var_wifi_ip[3]%10;
			if(++num>9)	var_wifi_ip[3] -= 9;
			else 		var_wifi_ip[3] += 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_ip[0], var_wifi_ip[1], var_wifi_ip[2], var_wifi_ip[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_ip_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_wifi_netmask_proc(char *c)
{
	char str[MAX_TEXT_BUF];
	int num = 0;

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_netmask[0]/100;
			if(--num<0) var_wifi_netmask[0] += 900;
			else 		var_wifi_netmask[0] -= 100;
			break;
		case 1:
			num = (var_wifi_netmask[0]/10)%10;
			if(--num<0) var_wifi_netmask[0] += 90;
			else 		var_wifi_netmask[0] -= 10;
			break;
		case 2:
			num = var_wifi_netmask[0]%10;
			if(--num<0) var_wifi_netmask[0] += 9;
			else 		var_wifi_netmask[0] -= 1;
			break;
		case 4:
			num = var_wifi_netmask[1]/100;
			if(--num<0) var_wifi_netmask[1] += 900;
			else 		var_wifi_netmask[1] -= 100;
			break;
		case 5:
			num = (var_wifi_netmask[1]/10)%10;
			if(--num<0) var_wifi_netmask[1] += 90;
			else 		var_wifi_netmask[1] -= 10;
			break;
		case 6:
			num = var_wifi_netmask[1]%10;
			if(--num<0) var_wifi_netmask[1] += 9;
			else 		var_wifi_netmask[1] -= 1;
			break;
		case 8:
			num = var_wifi_netmask[2]/100;
			if(--num<0) var_wifi_netmask[2] += 900;
			else 		var_wifi_netmask[2] -= 100;
			break;
		case 9:
			num = (var_wifi_netmask[2]/10)%10;
			if(--num<0) var_wifi_netmask[2] += 90;
			else 		var_wifi_netmask[2] -= 10;
			break;
		case 10:
			num = var_wifi_netmask[2]%10;
			if(--num<0) var_wifi_netmask[2] += 9;
			else 		var_wifi_netmask[2] -= 1;
			break;
		case 12:
			num = var_wifi_netmask[3]/100;
			if(--num<0)	var_wifi_netmask[3] += 900;
			else 		var_wifi_netmask[3] -= 100;
			break;
		case 13:
			num = (var_wifi_netmask[3]/10)%10;
			if(--num<0)	var_wifi_netmask[3] += 90;
			else 		var_wifi_netmask[3] -= 10;
			break;
		case 14:
			num = var_wifi_netmask[3]%10;
			if(--num<0)	var_wifi_netmask[3] += 9;
			else 		var_wifi_netmask[3] -= 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_netmask[0], var_wifi_netmask[1], var_wifi_netmask[2], var_wifi_netmask[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,wifi_ssid,wifi_ip,var_wifi_netmask,wifi_gateway,wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_netmask[0]/100;
			if(++num>9) var_wifi_netmask[0] -= 900;
			else 		var_wifi_netmask[0] += 100;
			break;
		case 1:
			num = (var_wifi_netmask[0]/10)%10;
			if(++num>9) var_wifi_netmask[0] -= 90;
			else 		var_wifi_netmask[0] += 10;
			break;
		case 2:
			num = var_wifi_netmask[0]%10;
			if(++num>9) var_wifi_netmask[0] -= 9;
			else 		var_wifi_netmask[0] += 1;
			break;
		case 4:
			num = var_wifi_netmask[1]/100;
			if(++num>9) var_wifi_netmask[1] -= 900;
			else 		var_wifi_netmask[1] += 100;
			break;
		case 5:
			num = (var_wifi_netmask[1]/10)%10;
			if(++num>9) var_wifi_netmask[1] -= 90;
			else 		var_wifi_netmask[1] += 10;
			break;
		case 6:
			num = var_wifi_netmask[1]%10;
			if(++num>9) var_wifi_netmask[1] -= 9;
			else 		var_wifi_netmask[1] += 1;
			break;
		case 8:
			num = var_wifi_netmask[2]/100;
			if(++num>9) var_wifi_netmask[2] -= 900;
			else 		var_wifi_netmask[2] += 100;
			break;
		case 9:
			num = (var_wifi_netmask[2]/10)%10;
			if(++num>9) var_wifi_netmask[2] -= 90;
			else 		var_wifi_netmask[2] += 10;
			break;
		case 10:
			num = var_wifi_netmask[2]%10;
			if(++num>9) var_wifi_netmask[2] -= 9;
			else 		var_wifi_netmask[2] += 1;
			break;
		case 12:
			num = var_wifi_netmask[3]/100;
			if(++num>9)	var_wifi_netmask[3] -= 900;
			else 		var_wifi_netmask[3] += 100;
			break;
		case 13:
			num = (var_wifi_netmask[3]/10)%10;
			if(++num>9)	var_wifi_netmask[3] -= 90;
			else 		var_wifi_netmask[3] += 10;
			break;
		case 14:
			num = var_wifi_netmask[3]%10;
			if(++num>9)	var_wifi_netmask[3] -= 9;
			else 		var_wifi_netmask[3] += 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_netmask[0], var_wifi_netmask[1], var_wifi_netmask[2], var_wifi_netmask[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_netmask_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_wifi_gateway_proc(char *c)
{
	char str[MAX_TEXT_BUF];
	int num = 0;

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_gateway[0]/100;
			if(--num<0) var_wifi_gateway[0] += 900;
			else 		var_wifi_gateway[0] -= 100;
			break;
		case 1:
			num = (var_wifi_gateway[0]/10)%10;
			if(--num<0) var_wifi_gateway[0] += 90;
			else 		var_wifi_gateway[0] -= 10;
			break;
		case 2:
			num = var_wifi_gateway[0]%10;
			if(--num<0) var_wifi_gateway[0] += 9;
			else 		var_wifi_gateway[0] -= 1;
			break;
		case 4:
			num = var_wifi_gateway[1]/100;
			if(--num<0) var_wifi_gateway[1] += 900;
			else 		var_wifi_gateway[1] -= 100;
			break;
		case 5:
			num = (var_wifi_gateway[1]/10)%10;
			if(--num<0) var_wifi_gateway[1] += 90;
			else 		var_wifi_gateway[1] -= 10;
			break;
		case 6:
			num = var_wifi_gateway[1]%10;
			if(--num<0) var_wifi_gateway[1] += 9;
			else 		var_wifi_gateway[1] -= 1;
			break;
		case 8:
			num = var_wifi_gateway[2]/100;
			if(--num<0) var_wifi_gateway[2] += 900;
			else 		var_wifi_gateway[2] -= 100;
			break;
		case 9:
			num = (var_wifi_gateway[2]/10)%10;
			if(--num<0) var_wifi_gateway[2] += 90;
			else 		var_wifi_gateway[2] -= 10;
			break;
		case 10:
			num = var_wifi_gateway[2]%10;
			if(--num<0) var_wifi_gateway[2] += 9;
			else 		var_wifi_gateway[2] -= 1;
			break;
		case 12:
			num = var_wifi_gateway[3]/100;
			if(--num<0)	var_wifi_gateway[3] += 900;
			else 		var_wifi_gateway[3] -= 100;
			break;
		case 13:
			num = (var_wifi_gateway[3]/10)%10;
			if(--num<0)	var_wifi_gateway[3] += 90;
			else 		var_wifi_gateway[3] -= 10;
			break;
		case 14:
			num = var_wifi_gateway[3]%10;
			if(--num<0)	var_wifi_gateway[3] += 9;
			else 		var_wifi_gateway[3] -= 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_gateway[0], var_wifi_gateway[1], var_wifi_gateway[2], var_wifi_gateway[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,wifi_ssid,wifi_ip,wifi_netmask,var_wifi_gateway,wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		switch(lcd_cursor_pos)
		{
		case 0:
			num = var_wifi_gateway[0]/100;
			if(++num>9) var_wifi_gateway[0] -= 900;
			else 		var_wifi_gateway[0] += 100;
			break;
		case 1:
			num = (var_wifi_gateway[0]/10)%10;
			if(++num>9) var_wifi_gateway[0] -= 90;
			else 		var_wifi_gateway[0] += 10;
			break;
		case 2:
			num = var_wifi_gateway[0]%10;
			if(++num>9) var_wifi_gateway[0] -= 9;
			else 		var_wifi_gateway[0] += 1;
			break;
		case 4:
			num = var_wifi_gateway[1]/100;
			if(++num>9) var_wifi_gateway[1] -= 900;
			else 		var_wifi_gateway[1] += 100;
			break;
		case 5:
			num = (var_wifi_gateway[1]/10)%10;
			if(++num>9) var_wifi_gateway[1] -= 90;
			else 		var_wifi_gateway[1] += 10;
			break;
		case 6:
			num = var_wifi_gateway[1]%10;
			if(++num>9) var_wifi_gateway[1] -= 9;
			else 		var_wifi_gateway[1] += 1;
			break;
		case 8:
			num = var_wifi_gateway[2]/100;
			if(++num>9) var_wifi_gateway[2] -= 900;
			else 		var_wifi_gateway[2] += 100;
			break;
		case 9:
			num = (var_wifi_gateway[2]/10)%10;
			if(++num>9) var_wifi_gateway[2] -= 90;
			else 		var_wifi_gateway[2] += 10;
			break;
		case 10:
			num = var_wifi_gateway[2]%10;
			if(++num>9) var_wifi_gateway[2] -= 9;
			else 		var_wifi_gateway[2] += 1;
			break;
		case 12:
			num = var_wifi_gateway[3]/100;
			if(++num>9)	var_wifi_gateway[3] -= 900;
			else 		var_wifi_gateway[3] += 100;
			break;
		case 13:
			num = (var_wifi_gateway[3]/10)%10;
			if(++num>9)	var_wifi_gateway[3] -= 90;
			else 		var_wifi_gateway[3] += 10;
			break;
		case 14:
			num = var_wifi_gateway[3]%10;
			if(++num>9)	var_wifi_gateway[3] -= 9;
			else 		var_wifi_gateway[3] += 1;
			break;
		default:
			break;
		}
		sprintf(str, "%03d.%03d.%03d.%03d", var_wifi_gateway[0], var_wifi_gateway[1], var_wifi_gateway[2], var_wifi_gateway[3]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_gateway_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void rcb_qems_eqpid_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if((var_qems_eqpid[lcd_cursor_pos]>0x21) &&(var_qems_eqpid[lcd_cursor_pos]<=0x7E))
			var_qems_eqpid[lcd_cursor_pos]--;
		else if(var_qems_eqpid[lcd_cursor_pos]==0)
			var_qems_eqpid[lcd_cursor_pos]=0x7E;
		else var_qems_eqpid[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_qems_eqpid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGUP, 2)==0)
	{

		if((var_qems_eqpid[lcd_cursor_pos]>0x21) &&(var_qems_eqpid[lcd_cursor_pos]<=0x7E))
			var_qems_eqpid[lcd_cursor_pos]--;
		else if(var_qems_eqpid[lcd_cursor_pos]==0)
			var_qems_eqpid[lcd_cursor_pos]=0x7E;
		else var_qems_eqpid[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_qems_eqpid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_qems_info(var_qems_eqpid);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if((var_qems_eqpid[lcd_cursor_pos]>=0x21) &&(var_qems_eqpid[lcd_cursor_pos]<0x7E))
			var_qems_eqpid[lcd_cursor_pos]++;
		else if(var_qems_eqpid[lcd_cursor_pos]==0x7E)
			var_qems_eqpid[lcd_cursor_pos]=0;
		else var_qems_eqpid[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_qems_eqpid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGDOWN, 2)==0)
	{
		if((var_qems_eqpid[lcd_cursor_pos]>=0x21) &&(var_qems_eqpid[lcd_cursor_pos]<0x7E))
			var_qems_eqpid[lcd_cursor_pos]++;
		else if(var_qems_eqpid[lcd_cursor_pos]==0x7E)
			var_qems_eqpid[lcd_cursor_pos]=0;
		else var_qems_eqpid[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_qems_eqpid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_qems_eqpid_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_wifi_ssid_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{/*
		if(var_wifi_ssid[lcd_cursor_pos]>'0'&&var_wifi_ssid[lcd_cursor_pos]<='9')
			var_wifi_ssid[lcd_cursor_pos]--;
		else if(var_wifi_ssid[lcd_cursor_pos]>'A'&&var_wifi_ssid[lcd_cursor_pos]<='Z')
			var_wifi_ssid[lcd_cursor_pos]--;
		else if(var_wifi_ssid[lcd_cursor_pos]>'a'&&var_wifi_ssid[lcd_cursor_pos]<='z')
			var_wifi_ssid[lcd_cursor_pos]--;
		else if(var_wifi_ssid[lcd_cursor_pos]=='0')
			var_wifi_ssid[lcd_cursor_pos]=' ';
		else if(var_wifi_ssid[lcd_cursor_pos]==' ')
			var_wifi_ssid[lcd_cursor_pos]='_';
		else if(var_wifi_ssid[lcd_cursor_pos]=='_')
			var_wifi_ssid[lcd_cursor_pos]= 0;
		else if(var_wifi_ssid[lcd_cursor_pos]=='A')
			var_wifi_ssid[lcd_cursor_pos]='9';
		else if(var_wifi_ssid[lcd_cursor_pos]=='a')
			var_wifi_ssid[lcd_cursor_pos]='Z';
		else var_wifi_ssid[lcd_cursor_pos]='z';*/


		if((var_wifi_ssid[lcd_cursor_pos]>0x21) &&(var_wifi_ssid[lcd_cursor_pos]<=0x7E))
			var_wifi_ssid[lcd_cursor_pos]--;
		else if(var_wifi_ssid[lcd_cursor_pos]==0)
			var_wifi_ssid[lcd_cursor_pos]=0x7E;
		else var_wifi_ssid[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_ssid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGUP, 2)==0)
	{
		if((var_wifi_ssid[lcd_cursor_pos]>0x21) &&(var_wifi_ssid[lcd_cursor_pos]<=0x7E))
			var_wifi_ssid[lcd_cursor_pos]--;
		else if(var_wifi_ssid[lcd_cursor_pos]==0)
			var_wifi_ssid[lcd_cursor_pos]=0x7E;
		else var_wifi_ssid[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_ssid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,var_wifi_ssid,wifi_ip,wifi_netmask,wifi_gateway,wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		/*
		if(var_wifi_ssid[lcd_cursor_pos]>='0'&&var_wifi_ssid[lcd_cursor_pos]<'9')
			var_wifi_ssid[lcd_cursor_pos]++;
		else if(var_wifi_ssid[lcd_cursor_pos]>='A'&&var_wifi_ssid[lcd_cursor_pos]<'Z')
			var_wifi_ssid[lcd_cursor_pos]++;
		else if(var_wifi_ssid[lcd_cursor_pos]>='a'&&var_wifi_ssid[lcd_cursor_pos]<'z')
			var_wifi_ssid[lcd_cursor_pos]++;
		else if(var_wifi_ssid[lcd_cursor_pos]=='9')
			var_wifi_ssid[lcd_cursor_pos]='A';
		else if(var_wifi_ssid[lcd_cursor_pos]=='Z')
			var_wifi_ssid[lcd_cursor_pos]='a';
		else if(var_wifi_ssid[lcd_cursor_pos]=='z')
			var_wifi_ssid[lcd_cursor_pos]=' ';
		else if(var_wifi_ssid[lcd_cursor_pos]==' ')
			var_wifi_ssid[lcd_cursor_pos]='_';
		else if(var_wifi_ssid[lcd_cursor_pos]=='_')
			var_wifi_ssid[lcd_cursor_pos]=0;
		else var_wifi_ssid[lcd_cursor_pos]='0';
*/
		if((var_wifi_ssid[lcd_cursor_pos]>=0x21) &&(var_wifi_ssid[lcd_cursor_pos]<0x7E))
			var_wifi_ssid[lcd_cursor_pos]++;
		else if(var_wifi_ssid[lcd_cursor_pos]==0x7E)
			var_wifi_ssid[lcd_cursor_pos]=0;
		else var_wifi_ssid[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_ssid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGDOWN, 2)==0)
	{
		if((var_wifi_ssid[lcd_cursor_pos]>=0x21) &&(var_wifi_ssid[lcd_cursor_pos]<0x7E))
			var_wifi_ssid[lcd_cursor_pos]++;
		else if(var_wifi_ssid[lcd_cursor_pos]==0x7E)
			var_wifi_ssid[lcd_cursor_pos]=0;
		else var_wifi_ssid[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_ssid);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_ssid_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_wifi_identity_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if((var_wifi_identity[lcd_cursor_pos]>0x21) &&(var_wifi_identity[lcd_cursor_pos]<=0x7E))
			var_wifi_identity[lcd_cursor_pos]--;
		else if(var_wifi_identity[lcd_cursor_pos]==0)
			var_wifi_identity[lcd_cursor_pos]=0x7E;
		else var_wifi_identity[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_identity);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGUP, 2)==0)
	{
		if((var_wifi_identity[lcd_cursor_pos]>0x21) &&(var_wifi_identity[lcd_cursor_pos]<=0x7E))
			var_wifi_identity[lcd_cursor_pos]--;
		else if(var_wifi_identity[lcd_cursor_pos]==0)
			var_wifi_identity[lcd_cursor_pos]=0x7E;
		else var_wifi_identity[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_identity);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,wifi_ssid,wifi_ip,wifi_netmask,wifi_gateway,var_wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if((var_wifi_identity[lcd_cursor_pos]>=0x21) &&(var_wifi_identity[lcd_cursor_pos]<0x7E))
			var_wifi_identity[lcd_cursor_pos]++;
		else if(var_wifi_identity[lcd_cursor_pos]==0x7E)
			var_wifi_identity[lcd_cursor_pos]=0;
		else var_wifi_identity[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_identity);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_LONGDOWN, 2)==0)
	{
		if((var_wifi_identity[lcd_cursor_pos]>=0x21) &&(var_wifi_identity[lcd_cursor_pos]<0x7E))
			var_wifi_identity[lcd_cursor_pos]++;
		else if(var_wifi_identity[lcd_cursor_pos]==0x7E)
			var_wifi_identity[lcd_cursor_pos]=0;
		else var_wifi_identity[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_identity);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_identity_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void rcb_wifi_password_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if((var_wifi_password[lcd_cursor_pos]>0x21) &&(var_wifi_password[lcd_cursor_pos]<=0x7E))
			var_wifi_password[lcd_cursor_pos]--;
		else if(var_wifi_password[lcd_cursor_pos]==0)
			var_wifi_password[lcd_cursor_pos]=0x7E;
		else var_wifi_password[lcd_cursor_pos]=0;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_password);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(wifi_select,wifi_ssid,wifi_ip,wifi_netmask,wifi_gateway,wifi_identity,var_wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if((var_wifi_password[lcd_cursor_pos]>=0x21) &&(var_wifi_password[lcd_cursor_pos]<0x7E))
			var_wifi_password[lcd_cursor_pos]++;
		else if(var_wifi_password[lcd_cursor_pos]==0x7E)
			var_wifi_password[lcd_cursor_pos]=0;
		else var_wifi_password[lcd_cursor_pos]=0x21;

		rcb_write(rcb_fd, RCB_LINE2, var_wifi_password);
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(++lcd_cursor_pos>=MAX_LINE_TEXT-1) lcd_cursor_pos = MAX_LINE_TEXT - 2;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(--lcd_cursor_pos<0) lcd_cursor_pos = 0;
		i2c_lcd_change_cursor(lcd_cursor_pos, 1);
		sprintf(str,"%d,%d",lcd_cursor_pos,1);
		rcb_write(rcb_fd, RCB_CURSOR, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_password_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void rcb_wifi_select_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(--var_wifi_sel<0) var_wifi_sel = 3;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_wifi_select[var_wifi_sel]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_wifi_info(var_wifi_sel,wifi_ssid,wifi_ip,wifi_netmask,wifi_gateway,wifi_identity,wifi_password);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(++var_wifi_sel>3) var_wifi_sel = 0;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_wifi_select[var_wifi_sel]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_wifi_select_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_network_info(void)
{
	int i=0;
	int str_cnt=0;
	int net_info_cnt=get_ip_info(net_info);

	for(i=0;i<net_info_cnt;i++)
	{
		sprintf(net_info_str[str_cnt++], "%s", net_info[i].name);
		sprintf(net_info_str[str_cnt++], "%d.%d.%d.%d", net_info[i].myip[0],net_info[i].myip[1],net_info[i].myip[2],net_info[i].myip[3]);
		sprintf(net_info_str[str_cnt++], "%02X%02X%02X%02X%02X%02X", net_info[i].mac[0],net_info[i].mac[1],net_info[i].mac[2],net_info[i].mac[3],net_info[i].mac[4],net_info[i].mac[5]);

	}

	sprintf(net_info_str[str_cnt++], "---WIFI STATE---");
	if(gp.network_connect==0)
	{
		sprintf(net_info_str[str_cnt++], "STATE:DISCONNECT");
	}
	else if(gp.network_connect==1)
	{
		sprintf(net_info_str[str_cnt++], "STATE:RECONNECT");
	}
	else
	{
		sprintf(net_info_str[str_cnt++], "STATE:CONNECT");
	}
	get_wifi_level(net_info_str[str_cnt++]);
	sprintf(net_info_str[str_cnt++], "----------------");

	net_info_str_max=(net_info_cnt*3)+4;
	rcb_write(rcb_fd, RCB_LINE1, net_info_str[net_info_cur]);
	rcb_write(rcb_fd, RCB_LINE2, net_info_str[net_info_cur+1]);

	set_opmode(NETWORK_INFO);
}

//wifi list
//nmcli dev wifi list

//wifi connect
//nmcli dev wifi con 'ssid' password 'wifi-password'

//auto wlan0 iface wlan0 inet dhcp
//wpa-ssid "xxxxxx"
//wpa-psk "xxxxx"

void rcb_network_info_proc(char *c)
{

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(net_info_cur>0)
		{
			net_info_cur--;
		}
		else if(net_info_cur==0)
		{
			net_info_cur=net_info_str_max-2;
		}
		rcb_write(rcb_fd, RCB_LINE1, net_info_str[net_info_cur]);
		rcb_write(rcb_fd, RCB_LINE2, net_info_str[net_info_cur+1]);

	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		rcb_network_info();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(net_info_cur<(net_info_str_max-2))
		{
			net_info_cur++;
		}
		else if(net_info_cur==(net_info_str_max-2))
		{
			net_info_cur=0;
		}
		rcb_write(rcb_fd, RCB_LINE1, net_info_str[net_info_cur]);
		rcb_write(rcb_fd, RCB_LINE2, net_info_str[net_info_cur+1]);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
		rcb_write(rcb_fd, RCB_LINE1, net_info_str[net_info_cur]);
		rcb_write(rcb_fd, RCB_LINE2, net_info_str[net_info_cur+1]);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
	}
}

void rcb_group_id_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(--var_group_id<0) var_group_id = 255;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%d", var_group_id);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_group_ip(var_group_id);
		rcb_menu();
		//set_reboot(1);	// reboot
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(++var_group_id>255) var_group_id = 0;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%d", var_group_id);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_group_id_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_pwr_select_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(--var_pwr_sel<0) var_pwr_sel = MAX_PWR_VENDOR;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_vendor[var_pwr_sel]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_pwr_vendor(var_pwr_sel);
		set_reboot(1);	// reboot
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(++var_pwr_sel>MAX_PWR_VENDOR) var_pwr_sel = 0;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_vendor[var_pwr_sel]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_pwr_select_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_menu_proc(char *c)
{
	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(--menu_idx<0) 	menu_idx = MENU_MAX_NUM;

		rcb_write(rcb_fd, RCB_LINE1, TITLE_MENU);
		rcb_write(rcb_fd, RCB_LINE2, g_menu[menu_idx]);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		switch(menu_idx)
		{
		case 0:	// model select
			model_init();
			rcb_model_change();
			break;
		case 1:	// server ip set
			rcb_server_ip_set();
			break;
		case 2:	// group id set
			rcb_group_id_set();
			break;
		case 3:	// qems eqpid set
			rcb_qems_eqpid_set();
			break;
		case 4:	// power select
			rcb_pwr_select_set();
			break;
		case 5:
			rcb_wifi_select_set();
			break;
		case 6:
			rcb_wifi_ssid_set();
			break;
		case 7:
			rcb_wifi_identity_set();
			break;
		case 8:
			rcb_wifi_password_set();
			break;
		case 9:
			rcb_wifi_ip_set();
			break;
		case 10:
			rcb_wifi_netmask_set();
			break;
		case 11:
			rcb_wifi_gateway_set();
			break;
		case 12:	// netowrk info select
			net_info_cur=0;
			rcb_network_info();
			break;
		case 13:
			rcb_pwr_spc_onoff_set();
			break;
		case 14:
			version_start = 1;
			rcb_pwr_fw_upload();
			break;
		case 15:
			rcb_storage_info();
			break;
		case 16:
			rcb_elvdd_alarm_set_info();
			break;
		case 17:
			set_reboot(1);	// reboot
			break;

		default:
			break;
		}
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(++menu_idx>MENU_MAX_NUM) menu_idx=0;

		if(g_menu[menu_idx]==NULL) menu_idx--;

		rcb_write(rcb_fd, RCB_LINE1, TITLE_MENU);
		rcb_write(rcb_fd, RCB_LINE2, g_menu[menu_idx]);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_menu();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_onoff_control(int pat_idx_factor, int auto_manu_mode_factor)
{
	if(get_onoff_flag()==ENUM_OFF)
	{
		/* 2022.12.07 RCB on off index ksk	*/
		if(get_schedule_flag()==0) {

			if(adim_change_flag)	gp.adim = var_data.adim;
			else					gp.adim = var_data.adim = model_data.vbr[0];

			set_pattern_index(pat_idx_factor);
			onoff_by_power_seq(ENUM_ON);
			if(adim_change_flag)	pwr_adimm_control_set(var_data.adim,0);		//20-03-30  request by sdc
		}
		reset_schedule_func();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		//rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
		//set_opmode(MANU_RUN);
		//20181030 seochihong
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
		if(auto_manu_mode_factor == 0){
			set_opmode(AUTO_RUN);
		}
		else if(auto_manu_mode_factor == 1){
			set_opmode(MANU_RUN);
		}
//		else{
//			set_opmode(AUTO_RUN);
//		}
		set_onoff_flag(ENUM_ON);
	}
	else
	{
		onoff_by_power_seq(ENUM_OFF);
		set_pattern_index(0);
		set_schedule_index(0);
		rcb_ready_screen(ACK);
		set_opmode(READY);
		set_onoff_flag(ENUM_OFF);
	}
}

int	get_onoff_flag(void)
{
	return onoff_flag;
}

void set_onoff_flag(int flag)
{
	onoff_flag = flag;

	if(onoff_flag==ENUM_ON)
	{
		color_done 	= 0;
		freq_done 	= 0;
		cursor_done = 0;
	}
}

void rcb_quhd_mode_select(int select)
{
	if(select>0) rcb_write(rcb_fd, RCB_LEDONOFF, LED_QUHD);
	else rcb_write(rcb_fd, RCB_LEDONOFF, LED_NOT_QUHD);
}

void rcb_ready_screen(int result)
{
	if(rcb_fd == NULL) return;

	rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
	rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
	rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_GRAY);
	rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_FREQ);

	rcb_write(rcb_fd, RCB_LINE1, model_name);

	switch(result)
	{
	case ACK:
		rcb_write(rcb_fd, RCB_LINE2, TITLE_READY);
		break;
	case ERR_DRAW_FAIL:
		rcb_model_change();
		rcb_write(rcb_fd, RCB_LINE2, g_model_err[result]);
		break;
	case ERR_FILE_ERROR:
		//rcb_model_change();
		rcb_write(rcb_fd, RCB_LINE1, g_model_err[result]);
		break;
	default:
		rcb_write(rcb_fd, RCB_LINE2, g_model_err[result]);
		break;
	}

	if( (result<10)&&(result!=ACK) )
	{
		memset(old_model_name,0,sizeof(old_model_name));
		model_selection_end=NACK;
		group_selection_end=NACK;
	}
	else if( (result==10) )
	{
		group_selection_end=NACK;
	}
}

void set_var_data(void)
{
	uint64_t freq64 = model_data.freq_high;

	var_data.freq			= (freq64<<32) | model_data.freq;
	var_data.h_total		= model_data.h_total;
	var_data.h_active		= model_data.h_active;
	var_data.h_bpo			= model_data.h_bpo;
	var_data.h_width		= model_data.h_width;
	var_data.h_fpo			= model_data.h_total-(model_data.h_active+model_data.h_bpo+model_data.h_width);		//bug fix 210218

	var_data.v_total		= model_data.v_total;
	var_data.v_active		= model_data.v_active;
	var_data.v_bpo			= model_data.v_bpo;
	var_data.v_width		= model_data.v_width;
	var_data.v_fpo			= model_data.v_total-(model_data.v_active+model_data.v_bpo+model_data.v_width);		//bug fix 210218

	var_data.vdd			= model_data.vdd;
	var_data.vbl			= model_data.vbl;				//20-04-01	request by sonk, sdc
	var_data.v_freq 		= var_data.freq/(model_data.h_total*model_data.v_total);
	var_data.port			= model_data.port;
	var_data.twist			= (model_data.mode>>8) & 0x3;

	if(model_data.pwm_duty[0]>10)	var_data.duty = 100;							// 0~10 ?
	else							var_data.duty = model_data.pwm_duty[0]*10;		// 0~10 ?


	var_data.adim			= model_data.vbr[0];

	var_data.freq_speed		= FREQ_SPEED_SLOW;

	var_data.color_mode 	= COLOR_MODE_RED;
	var_data.color_speed	= COLOR_SPEED_FAST;
	var_data.color.red		= 4095;
	var_data.color.green	= 0;
	var_data.color.blue		= 0;
	var_data.info_idx		= 0;

	var_data.cursor_speed	= 1;
	var_data.cursor.red		= 4095;
	var_data.cursor.green	= 0;
	var_data.cursor.blue	= 0;
	var_data.cursor_color_mode	= COLOR_MODE_RED;
	var_data.cursor_color_speed	= COLOR_SPEED_FAST;

	printf("var data freq = %d\n", var_data.v_freq);

	printf("var data pwm duty = %d\n", var_data.duty);		// delete

}

void display_model_info(int32_t n)
{
	char str[MAX_TEXT_BUF];
	uint64_t	freq=0, freq_h=0;

	memset(str, 0, sizeof(str));

	switch(n)
	{
	case 1:	// ex) 10B SING JEI NO
		sprintf(str, "%s %s %s %s", g_bit_type[(model_data.mode>>4)&0x3], g_out_name[model_data.mode&0xf], g_twist_name[(model_data.mode>>8)&0x3], g_divide_name[(model_data.mode>>12)&0x3]);
		break;
	case 2:	// ex) 1188.00MHz
		freq_h = model_data.freq_high;
		freq = (freq_h<<32) | model_data.freq;
		sprintf(str, "%.2fMHz", freq*0.000001);
		break;
	case 3:	// ex) TOTAL 3840 2160
		sprintf(str, "TOTAL %d %d", model_data.h_total, model_data.v_total);
		break;
	case 4:	// ex) ACTIV 3840 2160
		sprintf(str, "ACTIV %d %d", model_data.h_active, model_data.v_active);
		break;
	case 5:	// ex) BPORC 3840 2160
		sprintf(str, "BPORC %d %d", model_data.h_bpo, model_data.v_bpo);
		break;
	case 6:	// ex) WIDTH 3840 2160
		sprintf(str, "WIDTH %d %d", model_data.h_width, model_data.v_width);
		break;
	case 7:	// ex) F/W R1.4
		sprintf(str, "F/W R%d.%d", FW_VERSION/10, FW_VERSION%10);
		break;
	default:
		sprintf(str, "%s", TITLE_READY);
		break;
	}

	rcb_write(rcb_fd, RCB_LINE2, str);
}

void model_variable_reset(void)
{
	set_var_data();
	set_pattern_time_offset(0);
	set_schedule_flag(0);
}

void rcb_model_change_proc(char *c)
{
	if(strncmp(c, KEY_FILE, 2)==0)
	{
		file_flag = FILE_ONE;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ONE);
		if(get_file_list(DEL_TYPE_MODEL_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		set_opmode(MODEL_DELETE);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
#ifndef GROUP_SELECT
		model_update();
		if(model_idx <= 0)				model_idx = model_cnt - 1;
		else							model_idx--;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
		rcb_write(rcb_fd, RCB_LINE2, model_list[model_idx]);
#else
//		if( (model_selection_end==ACK)&&(model_data.use_dvi != 1) ) rcb_group_change(group_init());
		if(model_selection_end==ACK)	rcb_group_change(group_init());
#endif
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
#ifndef GROUP_SELECT
		if (model_selection_end==ACK){
			rcb_ready_screen(ACK);
			set_opmode(READY);
		}
		else rcb_menu();
#else
		if( (model_selection_end==ACK)&&(group_selection_end==ACK) )
		{
			rcb_ready_screen(ACK);
			set_opmode(READY);
		}
		else if ( (model_selection_end==ACK)&&(group_selection_end==NACK) )
		{
			rcb_group_change(group_init());
		}
		else rcb_menu();
#endif
	}
	else if(strncmp(c, KEY_MODEL, 2)==0)
	{
#ifdef GROUP_SELECT
		if(model_cnt>0)
		{
			memset(model_name,0,sizeof(model_name));
			memcpy(model_name, model_list[model_idx], sizeof(model_name));

			reload_model_flag=1;
			rcb_group_change(group_init());
		}
		else
		{
			reload_model_flag=0;
			memset(old_model_name,0,sizeof(old_model_name));
		}
#else
				model_selection_end = model_select(model_list[model_idx], 0);
				model_variable_reset();
#endif
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
#ifdef GROUP_SELECT
		if(model_cnt>0)
		{
			memset(model_name,0,sizeof(model_name));
			memcpy(model_name, model_list[model_idx], sizeof(model_name));

			reload_model_flag=1;
			rcb_group_change(group_init());
		}
		else
		{
			reload_model_flag=0;
			memset(old_model_name,0,sizeof(old_model_name));
		}
#else
				model_selection_end = model_select(model_list[model_idx], 0);
				model_variable_reset();
#endif
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
#ifndef GROUP_SELECT
		model_update();
		if(model_idx >= model_cnt-1)	model_idx = 0;
		else							model_idx++;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
		rcb_write(rcb_fd, RCB_LINE2, model_list[model_idx]);
#else

//		if( (model_selection_end==ACK)&&(model_data.use_dvi != 1) ) rcb_group_change(group_init());
		if(model_selection_end==ACK)	rcb_group_change(group_init());
#endif
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		model_update();
		if(model_cnt>0)
		{
			if(model_idx >= model_cnt-1)	model_idx = 0;
			else							model_idx++;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, model_list[model_idx]);
		}
		else
		{
			model_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO MODEL FILE!");
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		model_update();
		if(model_cnt>0)
		{
			if(model_idx <= 0)				model_idx = model_cnt - 1;
			else							model_idx--;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, model_list[model_idx]);
		}
		else
		{
			model_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO MODEL FILE!");
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_model_change();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_ready_proc(char *c)
{
	if(strncmp(c, KEY_ONOFF, 2)==0)
	{
		if(elvdd_alarm_flag_global == 1)
		{
			char vbl_temp[32] = {0, };

			memset(vbl_temp, 0, sizeof(vbl_temp));

			sprintf(vbl_temp, "ELVDD : %d.%d V", model_data.vbl/100, model_data.vbl%100);

			rcb_write(rcb_fd, RCB_LINE1, vbl_temp);
			rcb_write(rcb_fd, RCB_LINE2, "OK ?");

			set_opmode(VBL_CHECK_WARN);
		}
		else
		{
			if(get_schedule_flag() == 0){
				rcb_onoff_control(saving_pat_idx_rcb,saving_auto_manu_mode_rcb);
			}
			else{
				rcb_onoff_control(0,0);
			}

//			pattern_change(get_pattern_index());
		}
	}
	else if(strncmp(c, KEY_FILE, 2)==0)
	{
		file_flag = FILE_ONE;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ONE);
		if(get_file_list(DEL_TYPE_MODEL_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		set_opmode(MODEL_DELETE);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		//var_data.info_idx = 0;
		//display_model_info(var_data.info_idx);
		rcb_menu();
	}
	else if(strncmp(c, KEY_MODEL, 2)==0)
	{
		model_init();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, get_schedule_flag() ? TITLE_SCHEDULE_O : TITLE_SCHEDULE_X);
		schedule_list_load();
		schedule_list_index=0;
		if(schedule_list_count>0)	rcb_write(rcb_fd, RCB_LINE2, schedule_list[0]);
		else						rcb_write(rcb_fd, RCB_LINE2, "NO FILE!");
		set_opmode(READY_FUNC);
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.info_idx>=MAX_DISP_COUNT-1) var_data.info_idx = 0;
		else									var_data.info_idx += 1;
		display_model_info(var_data.info_idx);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(var_data.info_idx<=0) 				var_data.info_idx = MAX_DISP_COUNT-1;
		else									var_data.info_idx -= 1;
		display_model_info(var_data.info_idx);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_ready_screen(ACK);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		set_opmode(READY);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_ready_func_proc(char *c)		// schedule on/off
{
	if(strncmp(c, KEY_UP, 2)==0)
	{
		schedule_list_load();
		if (schedule_list_count>0)
		{
			if(schedule_list_index <= 0)	schedule_list_index = schedule_list_count - 1;
			else							schedule_list_index--;
			rcb_write(rcb_fd, RCB_LINE2, schedule_list[schedule_list_index]);
		}
		else
		{
			schedule_list_index=0;
			rcb_write(rcb_fd, RCB_LINE2, "NO SCHED FILE!");
		}
	}
	else if(strncmp(c, KEY_ESC,2)==0)
	{
		rcb_ready_screen(ACK);
		set_opmode(READY);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		schedule_list_load();
		if (schedule_list_count>0)
		{
			if(get_schedule_flag()) set_schedule_flag(0);
			else					set_schedule_flag(1);
		}
		else	set_schedule_flag(0);

		rcb_write(rcb_fd, RCB_LINE1, get_schedule_flag() ? TITLE_SCHEDULE_O : TITLE_SCHEDULE_X);
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		if(get_schedule_flag()) get_schedule_info(schedule_list[schedule_list_index]);
		else
		{
//			uint64_t freq_high 	= model_data.freq_high;
//			var_data.freq = (freq_high << 32) |  model_data.freq;
			freq_set_func(var_data.freq);


			adim_change_flag = 0;
			memset(&schedule_data_new, 0, sizeof(schedule_data_t_new));
			pwr_model_set(&model_data, 0, 0);
		}
		schedule_list_index=0;
		rcb_ready_screen(ACK);
		set_opmode(READY);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		schedule_list_load();
		if (schedule_list_count>0)
		{
			if(schedule_list_index >= schedule_list_count-1)	schedule_list_index = 0;
			else												schedule_list_index++;
			rcb_write(rcb_fd, RCB_LINE2, schedule_list[schedule_list_index]);
		}
		else
		{
			schedule_list_index=0;
			rcb_write(rcb_fd, RCB_LINE2, "NO SCHED FILE!");
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		schedule_list_load();
		if (schedule_list_count>0)
		{
			if(schedule_list_index >= schedule_list_count-1)	schedule_list_index = 0;
			else												schedule_list_index++;
			rcb_write(rcb_fd, RCB_LINE2, schedule_list[schedule_list_index]);
		}
		else
		{
			schedule_list_index=0;
			rcb_write(rcb_fd, RCB_LINE2, "NO SCHED FILE!");
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		schedule_list_load();
		if (schedule_list_count>0)
		{
			if(schedule_list_index <= 0)	schedule_list_index = schedule_list_count - 1;
			else							schedule_list_index--;
			rcb_write(rcb_fd, RCB_LINE2, schedule_list[schedule_list_index]);
		}
		else
		{
			schedule_list_index=0;
			rcb_write(rcb_fd, RCB_LINE2, "NO SCHED FILE!");
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, get_schedule_flag() ? TITLE_SCHEDULE_O : TITLE_SCHEDULE_X);
		schedule_list_load();
		if(schedule_list_count>0)	rcb_write(rcb_fd, RCB_LINE2, schedule_list[schedule_list_index]);
		else						rcb_write(rcb_fd, RCB_LINE2, "NO FILE!");
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_auto_run_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ONOFF, 2)==0)
	{
		if(get_schedule_flag()==0){
			saving_pat_idx_rcb = get_pattern_index();
			saving_auto_manu_mode_rcb = 0;
			rcb_onoff_control(0,0);
			printf("~~~~~~~~~~~~~~~~~~~~~auto run\r\n");
		}
		else
		{
			if( (schedule_move_menu || schedule_move_check)==0 ) rcb_onoff_control(0,0);
			printf("~~~~~~~~~~~~~~~~~~~~~auto run 2\r\n");
		}
	}
	else if(strncmp(c, KEY_AUTOMANU, 2)==0)
	{
		if(get_schedule_flag()==0)
		{
			set_opmode(MANU_RUN);
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
			rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
		}

	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		if(get_schedule_flag()==0)
		{
			pattern_time_offset_dec();
			memset(str, 0, sizeof(str));
			sprintf(str, "[+%.1fsec]", (float)get_pattern_time_offset()/1000);
			rcb_write(rcb_fd, RCB_LINE2, str);
		}
		else
		{
			schedule_menu_step_dec();
		}
	}
	else if(strncmp(c, KEY_LONGUP, 2)==0)
	{
		if(get_schedule_flag()==1)	schedule_menu_step_dec();
	}
	else if(strncmp(c, KEY_ESC,2)==0)
	{
		if(get_schedule_flag()==1) schedule_menu_reset();
	}
	else if(strncmp(c, KEY_NEXT_PATTERN, 2)==0)
	{
		if(get_schedule_flag()==0)	pattern_inc();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(get_schedule_flag()==0)
		{
			pattern_time_offset_inc();
			memset(str, 0, sizeof(str));
			sprintf(str, "[+%.1fsec]", (float)get_pattern_time_offset()/1000);
			rcb_write(rcb_fd, RCB_LINE2, str);
		}
		else
		{
			schedule_menu_step_inc();
		}
	}
	else if(strncmp(c, KEY_LONGDOWN, 2)==0)
	{
		if(get_schedule_flag()==1)	schedule_menu_step_inc();
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(get_schedule_flag()==0)
		{
			set_opmode(MANU_RUN);
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
			rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
			pattern_inc();
		}
		else
		{
			schedule_menu_index_inc();
		}
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		if(get_schedule_flag()==1)	schedule_menu_index_inc();
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(get_schedule_flag()==0)
		{
			set_opmode(MANU_RUN);
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
			rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
			pattern_dec();
		}
		else
		{
			schedule_menu_index_dec();
		}
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		if(get_schedule_flag()==1)	schedule_menu_index_dec();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		if(get_schedule_flag()==1) schedule_menu_ok();
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
		schedule_move_menu=0;
		schedule_move_check=0;
//		pattern_dec();
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_manu_run_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];
	color_t		cr;
	int			idx=0;

	if(strncmp(c, KEY_ONOFF, 2)==0)
	{
		if(get_schedule_flag() == 0){
			saving_pat_idx_rcb = get_pattern_index();
			saving_auto_manu_mode_rcb = 1;
			printf("~~~~~~~~~~~~~~~~~~~~~manu run\r\n");
		}
		rcb_onoff_control(0,0);
	}
	else if(strncmp(c, KEY_AUTOMANU, 2)==0)
	{
		set_opmode(AUTO_RUN);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
		pattern_inc();
	}
	else if(strncmp(c, KEY_GRAY, 2)==0)
	{

		var_data.color_mode 	= COLOR_MODE_RED;
		var_data.color_speed 	= COLOR_SPEED_FAST;
		var_data.color.red		= 4095;
		var_data.color.green	= 0;
		var_data.color.blue		= 0;

		gray_change_init();

		set_opmode(GRAY_CHANGE);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_F);

//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
		switch(model_data.mode & 0xf)
		{
			case MODE_HEXA :
				gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue);//2022.04.07 ksk 11bit gray
//				printf("gray change rcb 1 hexa\n");
				break;
			default :
		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
				break;
		}


		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_GRAY);
	}
	else if(strncmp(c, KEY_POS, 2)==0)
	{
		var_data.cursor_speed	= 1;
		var_data.cursor.type	= 1;
		var_data.cursor.x 		= 0;
		var_data.cursor.y 		= 0;
		cursor_func(&var_data.cursor);
		cursor_flag	= CURSOR_X;
		memset(str, 0, sizeof(str));
		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(CURSOR);
	}
	else if(strncmp(c, KEY_FREQ, 2)==0)
	{
		var_data.freq_speed = FREQ_SPEED_FAST;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_FREQ_CHANGE_F);
		memset(str, 0, sizeof(str));

		//test	20-05-21
		idx=get_pattern_index();
		if(group_data.pat[idx].vsync != 0){
			sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(vsync_to_freq(group_data.pat[idx].vsync *10)), (float)group_data.pat[idx].vsync);
			printf("indicate pattern_freq\n");
		}
		else {
			sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
			printf("indicate var_freq\n");
		}
		//test	20-05-21

//		sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_FREQ);
		set_opmode(FREQUENCY);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(HTIME_CHANGE);
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		set_opmode(AUTO_RUN);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
		pattern_dec();
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		set_opmode(AUTO_RUN);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
		pattern_inc();
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		pattern_inc();
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		pattern_dec();
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LEDONOFF, (get_onoff_flag()==ENUM_ON) ? LED_ON_ONOFF : LED_OFF_ONOFF);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void freq_set_from_pc(req_freq_set_t *pdata)
{
	if(pdata->type==1)
	{
		var_data.freq = pdata->freq;
		freq_set_func(var_data.freq);
		set_opmode(FREQUENCY);
		if(freq_done==0)
		{
			freq_done = 1;
			rcb_write(rcb_fd, RCB_LINE2, TITLE_FREQ_CHANGE_F);
		}
	}
	else
	{
		uint64_t freq_high 	= model_data.freq_high;
		var_data.freq = (freq_high << 32) |  model_data.freq;

		freq_set_func(var_data.freq);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
		freq_done = 0;
	}
}

void vsync_set_from_pc(req_vsync_set_t *pdata)
{
	if(pdata->type==1)
	{
		var_data.freq = vsync_to_freq(pdata->vsync);
		freq_set_func(var_data.freq);
		set_opmode(FREQUENCY);
		if(freq_done==0)
		{
			freq_done = 1;
			rcb_write(rcb_fd, RCB_LINE2, TITLE_FREQ_CHANGE_F);
		}
	}
	else
	{
		uint64_t freq_high 	= model_data.freq_high;
		var_data.freq = (freq_high << 32) |  model_data.freq;
		freq_set_func(var_data.freq);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
		freq_done = 0;
	}
}

void rcb_freq_proc(char *c)
{
	char	str[MAX_TEXT_BUF];
	int		idx=0;

	if(strncmp(c, KEY_ESC, 2)==0)
	{
//		uint64_t freq_high 	= model_data.freq_high;
//		var_data.freq = (freq_high << 32) |  model_data.freq;		//20-05-19 keep frequency setting, request by sdc

		freq_set_func(var_data.freq);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_FREQ);
		set_opmode(MANU_RUN);

		fwd_long_cnt=0;		//test 19-09-26
		bwd_long_cnt=0;		//test 19-09-26
		var_freq_changed=0;		//test 20-05-21
	}
	else if(strncmp(c, KEY_FREQ, 2)==0)
	{
		if(var_data.freq_speed>FREQ_SPEED_FAST)
		{
			var_data.freq_speed = FREQ_SPEED_SLOW;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_FREQ_CHANGE_S);
		}
		else if(var_data.freq_speed<FREQ_SPEED_FAST)
		{
			var_data.freq_speed = FREQ_SPEED_FAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_FREQ_CHANGE_F);
		}
		else
		{
			var_data.freq_speed = FREQ_SPEED_FFAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_FREQ_CHANGE_FF);
		}

		fwd_long_cnt=0;		//test 19-09-26
		bwd_long_cnt=0;		//test 19-09-26
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		//test	20-05-21
		if(!var_freq_changed)
		{
			idx=get_pattern_index();
			if(group_data.pat[idx].vsync != 0)
			{
				printf("copy pattern vsync to var_vsync\n");
				var_data.freq = vsync_to_freq(group_data.pat[idx].vsync *10);
			}
			var_freq_changed = 1;
		}
		//test	20-05-21

//		var_data.freq += (var_data.freq_speed>FREQ_SPEED_SLOW) ? FREQ_SPEED_FAST : FREQ_SPEED_SLOW;
		var_data.freq += (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;
		freq_set_func(var_data.freq);
		memset(str, 0, sizeof(str));
		sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		if( (fwd_long_cnt++) > 2 )
		{
			//test	20-05-21
			if(!var_freq_changed)
			{
				idx=get_pattern_index();
				if(group_data.pat[idx].vsync != 0)
				{
					printf("copy pattern vsync to var_vsync\n");
					var_data.freq = vsync_to_freq(group_data.pat[idx].vsync *10);
				}
				var_freq_changed = 1;
			}
			//test	20-05-21


//			var_data.freq += (var_data.freq_speed>FREQ_SPEED_SLOW) ? FREQ_SPEED_FAST : FREQ_SPEED_SLOW;
			var_data.freq += (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;
			freq_set_func(var_data.freq);
			memset(str, 0, sizeof(str));
			sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
			rcb_write(rcb_fd, RCB_LINE2, str);

			fwd_long_cnt=0;
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		//test	20-05-21
		if(!var_freq_changed)
		{
			idx=get_pattern_index();
			if(group_data.pat[idx].vsync != 0)
			{
				printf("copy pattern vsync to var_vsync\n");
				var_data.freq = vsync_to_freq(group_data.pat[idx].vsync *10);
			}
			var_freq_changed = 1;
		}
		//test	20-05-21

//		if(var_data.freq>10000000) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? FREQ_SPEED_FAST : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
//		if(var_data.freq>10000000) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
		if(var_data.freq_speed<FREQ_SPEED_FAST){
			if(var_data.freq>FREQ_SPEED_SLOW) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
		}
		else if(var_data.freq_speed>FREQ_SPEED_FAST){
			if(var_data.freq>FREQ_SPEED_FFAST) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
		}
		else{
			if(var_data.freq>FREQ_SPEED_FAST) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
		}



		freq_set_func(var_data.freq);
		memset(str, 0, sizeof(str));
		sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		if( (bwd_long_cnt++) > 2 )
		{
			//test	20-05-21
			if(!var_freq_changed)
			{
				idx=get_pattern_index();
				if(group_data.pat[idx].vsync != 0)
				{
					printf("copy pattern vsync to var_vsync\n");
					var_data.freq = vsync_to_freq(group_data.pat[idx].vsync *10);
				}
				var_freq_changed = 1;
			}
			//test	20-05-21

//			if(var_data.freq>10000000) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? FREQ_SPEED_FAST : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
//			if(var_data.freq>10000000) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
			if(var_data.freq_speed<FREQ_SPEED_FAST){
				if(var_data.freq>FREQ_SPEED_SLOW) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
			}
			else if(var_data.freq_speed>FREQ_SPEED_FAST){
				if(var_data.freq>FREQ_SPEED_FFAST) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
			}
			else{
				if(var_data.freq>FREQ_SPEED_FAST) var_data.freq -= (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? FREQ_SPEED_FFAST : FREQ_SPEED_FAST) : FREQ_SPEED_SLOW;		//error in speed fast(-10MHz)
			}
			freq_set_func(var_data.freq);
			memset(str, 0, sizeof(str));
			sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
			rcb_write(rcb_fd, RCB_LINE2, str);

			bwd_long_cnt=0;
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
//		rcb_write(rcb_fd, RCB_LINE1, (var_data.freq_speed>FREQ_SPEED_SLOW) ? TITLE_FREQ_CHANGE_F : TITLE_FREQ_CHANGE_S);
		rcb_write(rcb_fd, RCB_LINE1, (var_data.freq_speed>FREQ_SPEED_SLOW) ? ((var_data.freq_speed>FREQ_SPEED_FAST) ? TITLE_FREQ_CHANGE_FF : TITLE_FREQ_CHANGE_F) : TITLE_FREQ_CHANGE_S);
		memset(str, 0, sizeof(str));
		sprintf(str, "%.2fMHz/%.1f", freq_to_mhz(var_data.freq), freq_to_vsync(var_data.freq));
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_FREQ);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong

		fwd_long_cnt=0;		//test 19-09-26
		bwd_long_cnt=0;		//test 19-09-26
	}
}

void rcb_htime_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.v_bpo);
//		bporch_set_func(RCB_VTIME, &var_data);				//20-05-21		commentted
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(VTIME_CHANGE);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		var_data.h_total 	+= 1;
		var_data.h_bpo 		+= 1;
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		bporch_set_func(RCB_HTIME, &var_data);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		var_data.h_total 	-= 1;
		var_data.h_bpo 		-= 1;
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		bporch_set_func(RCB_HTIME, &var_data);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_vtime_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CH_SHIFT);
		memset(str, 0, sizeof(str));
		sprintf(str, "CH: %s", g_port_name[var_data.port]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(CH_SHIFT);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		var_data.v_total 	+= 1;
		var_data.v_bpo 		+= 1;
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.v_bpo);
		bporch_set_func(RCB_VTIME, &var_data);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		var_data.v_total 	-= 1;
		var_data.v_bpo 		-= 1;
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.v_bpo);
		bporch_set_func(RCB_VTIME, &var_data);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.v_bpo);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_ch_shift_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_BIT_SHIFT);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %s", g_twist_name[var_data.twist]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(BIT_CHANGE);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		var_data.port += 1;
		if(var_data.port>=MAX_PORT_CNT) var_data.port = 0;
		set_portmap(var_data.port);
		memset(str, 0, sizeof(str));
		sprintf(str, "CH: %s", g_port_name[var_data.port]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		var_data.port -= 1;
		if(var_data.port<0 || var_data.port>=MAX_PORT_CNT) var_data.port = MAX_PORT_CNT - 1;
		set_portmap(var_data.port);
		memset(str, 0, sizeof(str));
		sprintf(str, "CH: %s", g_port_name[var_data.port]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CH_SHIFT);
		memset(str, 0, sizeof(str));
		sprintf(str, "CH: %s", g_port_name[var_data.port]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_bit_shift_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_DIMM_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %.1f V", var_data.adim/10.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(DIMM_CHANGE);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.twist==TWIST_JEIDA) var_data.twist = TWIST_VESA;
		else							var_data.twist = TWIST_JEIDA;
		set_mode_by_twist(var_data.twist);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %s", g_twist_name[var_data.twist]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(var_data.twist==TWIST_JEIDA) var_data.twist = TWIST_VESA;
		else							var_data.twist = TWIST_JEIDA;
		set_mode_by_twist(var_data.twist);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %s", g_twist_name[var_data.twist]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_BIT_SHIFT);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %s", g_twist_name[var_data.twist]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_dim_change_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		adim_change_flag = 1;
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_PWM_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d %%", var_data.duty);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(PWM_CHANGE);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.adim>MAX_DIMM_VAL-1) 					var_data.adim = MAX_DIMM_VAL;
		else												var_data.adim += 1;
		adim_change_flag = 1;
		pwr_adimm_control_set(var_data.adim,0);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %.1f V", var_data.adim/10.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(var_data.adim<=0 || var_data.adim>MAX_DIMM_VAL) 	var_data.adim = 0;
		else												var_data.adim -= 1;
		adim_change_flag = 1;
		pwr_adimm_control_set(var_data.adim,0);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %.1f V", var_data.adim/10.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_DIMM_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %.1f V", var_data.adim/10.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(DIMM_CHANGE);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_pwm_change_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];
	pwm_data_t	pwm = {0,};

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_SCROLL_CHANGE);
		var_data.scr_dir 	= 0;
		var_data.scr_speed 	= 1;
		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(SCROLL);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.duty>MAX_DUTY_VAL-1) 					var_data.duty = MAX_DUTY_VAL;
		else												var_data.duty += 1;
		pwm.freq = model_data.pwm_freq[0];	// fix
		pwm.duty = var_data.duty;
		pwr_pdimm_control_set(pwm,0,0);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d %%", var_data.duty);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(var_data.duty<=0 || var_data.duty>MAX_DUTY_VAL) 	var_data.duty = 0;
		else												var_data.duty -= 1;
		pwm.freq = model_data.pwm_freq[0];	// fix
		pwm.duty = var_data.duty;
		pwr_pdimm_control_set(pwm,0,0);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d %%", var_data.duty);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_PWM_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d %%", var_data.duty);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(PWM_CHANGE);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_scroll_test_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_UP, 2)==0)
	{
		var_data.scr_dir -= 1;
		if(var_data.scr_dir<0 || var_data.scr_dir>MAX_SCR_DIR) var_data.scr_dir = MAX_SCR_DIR;
		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);

		FPGA_OR_SET(FPGA_PARAM_LATCH_EN, 0x0002);
		FPGA_AND_SET(FPGA_PARAM_LATCH_EN, 0x0002);

		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		var_data.scr_dir 	= 0;
		var_data.scr_speed 	= 1;
//		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);
		pattern_change(pattern_index);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
//		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
//		memset(str, 0, sizeof(str));
//		sprintf(str, "VALUE: %d", var_data.h_bpo);
//		rcb_write(rcb_fd, RCB_LINE2, str);
//		set_opmode(HTIME_CHANGE);

		var_data.scr_dir 	= 0;
		var_data.scr_speed 	= 1;
		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);

		tcon_i2c_reg_index=0;
		rcb_tcon_i2c_ctrl();
		set_opmode(I2C_CTRL);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		var_data.scr_dir += 1;
		if(var_data.scr_dir>MAX_SCR_DIR) var_data.scr_dir = 0;
//		FPGA_scroll_ctrl(var_data.scr_dir, 0x0001, var_data.scr_speed);
		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);

		FPGA_OR_SET(FPGA_PARAM_LATCH_EN, 0x0002);
		FPGA_AND_SET(FPGA_PARAM_LATCH_EN, 0x0002);

		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		var_data.scr_speed += 1;
		if(var_data.scr_speed>MAX_SCR_SPEED) var_data.scr_speed = 1;
		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);

		FPGA_OR_SET(FPGA_PARAM_LATCH_EN, 0x0002);
		FPGA_AND_SET(FPGA_PARAM_LATCH_EN, 0x0002);

		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		var_data.scr_speed -= 1;
		if(var_data.scr_speed<1 || var_data.scr_speed>MAX_SCR_SPEED) var_data.scr_speed = MAX_SCR_SPEED;
		FPGA_scroll_ctrl(var_data.scr_dir, model_data.mode, var_data.scr_speed);

		FPGA_OR_SET(FPGA_PARAM_LATCH_EN, 0x0002);
		FPGA_AND_SET(FPGA_PARAM_LATCH_EN, 0x0002);

		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_SCROLL_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "DIR:%s,SPD:%d", g_scroll_dir[var_data.scr_dir], var_data.scr_speed);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(SCROLL);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void pos_set_from_pc(req_pos_set_t *pdata)
{
	char 		str[MAX_TEXT_BUF];

	if(pdata->type==1)
	{
		var_data.cursor_speed	= 1;
		var_data.cursor.type	= pdata->type;
		var_data.cursor.x 		= pdata->x;
		var_data.cursor.y 		= pdata->y;
		var_data.cursor.red		= pdata->red;
		var_data.cursor.green	= pdata->green;
		var_data.cursor.blue	= pdata->blue;
		cursor_func(&var_data.cursor);
		set_opmode(CURSOR);
//		if(cursor_done == 0)
//		{
//			cursor_done = 1;
//			rcb_write(rcb_fd, RCB_LINE2, TITLE_CURSOR_F);
//		}
		memset(str, 0, sizeof(str));
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else
	{
		var_data.cursor.type	= 0;
		cursor_func(&var_data.cursor);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
		cursor_done = 0;
	}
}

void rcb_cursor_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];
	pos_t		pos_limit;

	switch(model_data.rgb_set)
	{
		case RGB_HORZ:
		case BGR_HORZ:
			pos_limit.x = model_data.h_active*3;
			pos_limit.y = model_data.v_active;
			break;
		case RGB_VERT:
		case BGR_VERT:
			pos_limit.x = model_data.h_active;
			pos_limit.y = model_data.v_active*3;
			break;
		default:
			pos_limit.x = model_data.h_active;
			pos_limit.y = model_data.v_active;
			break;
	}

	if(strncmp(c, KEY_POS, 2)==0)
	{
		//speed
		if(var_data.cursor_speed==0)
		{
			var_data.cursor_speed = 1;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		}
		else if(var_data.cursor_speed==1)
		{
			var_data.cursor_speed = 2;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_FF);
		}
		else
		{
			var_data.cursor_speed = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_S);
		}
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		// y pos up
//		if		(var_data.speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
//		else 							var_data.cursor.y--;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y--;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y -= 3;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y--;
				break;
		}

		if( (var_data.cursor.y < 0) || (var_data.cursor.y >= 65280) ) 	var_data.cursor.y = 0;

		var_data.cursor.type	= 1;
		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		var_data.cursor.type	= 0;
		cursor_func(&var_data.cursor);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		// y pos down
//		if		(var_data.speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
//		else 							var_data.cursor.y++;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y++;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y += 3;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y++;
				break;
		}

		if(var_data.cursor.y >= pos_limit.y) var_data.cursor.y = pos_limit.y-1;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGUP, 2)==0)
	{
		// y pos up
//		if		(var_data.speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
//		else 							var_data.cursor.y--;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y--;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y -= 3;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y -= CURSOR_SPEED2;
				else 								var_data.cursor.y--;
				break;
		}

		if( (var_data.cursor.y < 0) || (var_data.cursor.y >= 65280) ) 	var_data.cursor.y = 0;

		var_data.cursor.type	= 1;
		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGDOWN, 2)==0)
	{
		// y pos down
//		if		(var_data.speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
//		else 							var_data.cursor.y++;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y++;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y += 3;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.y += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.y += CURSOR_SPEED2;
				else 								var_data.cursor.y++;
				break;
		}

		if(var_data.cursor.y >= pos_limit.y) var_data.cursor.y = pos_limit.y-1;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		// x pos right
//		if		(var_data.speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
//		else 							var_data.cursor.x++;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x += 3;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x ++;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x++;
				break;
		}

		if(var_data.cursor.x >= pos_limit.x) var_data.cursor.x = pos_limit.x-1;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		// x pos right
//		if		(var_data.speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
//		else 							var_data.cursor.x++;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x += 3;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x ++;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x += CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x += CURSOR_SPEED2;
				else 								var_data.cursor.x++;
				break;
		}

		if(var_data.cursor.x >= pos_limit.x) var_data.cursor.x = pos_limit.x-1;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		// x pos left
//		if		(var_data.speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
//		else 							var_data.cursor.x--;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x -= 3;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x--;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x--;
				break;
		}

		if( (var_data.cursor.x < 0) || (var_data.cursor.x >= 65280) ) 	var_data.cursor.x = 0;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);

	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		// x pos left
//		if		(var_data.speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
//		else if	(var_data.speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
//		else 							var_data.cursor.x--;
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x -= 3;
				break;
			case RGB_VERT:
			case BGR_VERT:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x--;
				break;
			default:
				if		(var_data.cursor_speed==1) 	var_data.cursor.x -= CURSOR_SPEED1;
				else if	(var_data.cursor_speed==2) 	var_data.cursor.x -= CURSOR_SPEED2;
				else 								var_data.cursor.x--;
				break;
		}

		if( (var_data.cursor.x < 0) || (var_data.cursor.x >= 65280) ) 	var_data.cursor.x = 0;

		var_data.cursor.type	= 1;

		cursor_func(&var_data.cursor);
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);

	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{

		var_data.cursor_color_speed = COLOR_SPEED_FAST;

		set_opmode(CURSOR_COLOR);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_F);

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", var_data.cursor.red/COLOR_OFFSET, var_data.cursor.green/COLOR_OFFSET, var_data.cursor.blue /COLOR_OFFSET);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
//		if		(var_data.speed==1) 	rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
//		else if	(var_data.speed==2) 	rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_FF);
//		else 							rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_S);
		if		(var_data.cursor_speed==1) 	rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		else if	(var_data.cursor_speed==2) 	rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_FF);
		else 								rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_S);

		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_cursor_color_proc(char *c)
{
	color_t	cr;
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_FUNC, 2)==0)
	{
		switch(var_data.cursor_color_mode)
		{
		case COLOR_MODE_RED:
			var_data.cursor_color_mode = COLOR_MODE_GREEN;
			break;

		case COLOR_MODE_GREEN:
			var_data.cursor_color_mode = COLOR_MODE_BLUE;
			break;

		case COLOR_MODE_BLUE:
			var_data.cursor_color_mode = COLOR_MODE_WHITE;
			break;

		default:
			var_data.cursor_color_mode = COLOR_MODE_RED;
			break;
		}

		if(var_data.cursor_color_mode & COLOR_MODE_RED  ) 	var_data.cursor.red 	= 4095;
		else											var_data.cursor.red 	= 0;
		if(var_data.cursor_color_mode & COLOR_MODE_GREEN) 	var_data.cursor.green 	= 4095;
		else											var_data.cursor.green 	= 0;
		if(var_data.cursor_color_mode & COLOR_MODE_BLUE ) 	var_data.cursor.blue 	= 4095;
		else											var_data.cursor.blue 	= 0;


//		FPGA_Write(FPGA_POSION_R, (var_data.cursor.red)>>2);
//		FPGA_Write(FPGA_POSION_G, (var_data.cursor.green)>>2);
//		FPGA_Write(FPGA_POSION_B, (var_data.cursor.blue)>>2);

		cursor_func(&var_data.cursor);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		if(var_data.cursor_color_speed>COLOR_SPEED_SLOW)
		{
			var_data.cursor_color_speed = COLOR_SPEED_SLOW;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_S);
		}
		else
		{
			var_data.cursor_color_speed = COLOR_SPEED_FAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_F);
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		var_data.cursor_speed = 1;
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(CURSOR);
	}
	else if(strncmp(c, KEY_POS, 2)==0)
	{
		var_data.cursor_speed = 1;
		memset(str, 0, sizeof(str));
//		sprintf(str,"X:%d  Y:%d", var_data.cursor.x+1,var_data.cursor.y+1);
		switch(model_data.rgb_set)
		{
			case RGB_HORZ:
			case BGR_HORZ:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x/3)+1,(var_data.cursor.y)+1);
				break;
			case RGB_VERT:
			case BGR_VERT:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y/3)+1);
				break;
			default:
				sprintf(str,"X:%d  Y:%d", (var_data.cursor.x)+1,(var_data.cursor.y)+1);
				break;
		}
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_F);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(CURSOR);
	}

	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(var_data.cursor_color_speed>COLOR_SPEED_SLOW)
		{
			var_data.cursor_color_speed = COLOR_SPEED_SLOW;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_S);
		}
		else
		{
			var_data.cursor_color_speed = COLOR_SPEED_FAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_F);
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if( (var_data.cursor_color_mode&COLOR_MODE_RED) )
			var_data.cursor.red 	+= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_GREEN) )
			var_data.cursor.green 	+= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_BLUE) )
			var_data.cursor.blue 	+= (COLOR_OFFSET * var_data.cursor_color_speed);

		if( (var_data.cursor.red<0) || (var_data.cursor.red>4095) )
			var_data.cursor.red 	= 4095;
		if( (var_data.cursor.green<0) || (var_data.cursor.green>4095) )
			var_data.cursor.green	= 4095;
		if( (var_data.cursor.blue<0) || (var_data.cursor.blue>4095) )
			var_data.cursor.blue 	= 4095;


//		FPGA_Write(FPGA_POSION_R, (var_data.cursor.red)>>2);
//		FPGA_Write(FPGA_POSION_G, (var_data.cursor.green)>>2);
//		FPGA_Write(FPGA_POSION_B, (var_data.cursor.blue)>>2);

		cursor_func(&var_data.cursor);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		if( (var_data.cursor_color_mode&COLOR_MODE_RED) )
			var_data.cursor.red 	+= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_GREEN) )
			var_data.cursor.green 	+= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_BLUE) )
			var_data.cursor.blue 	+= (COLOR_OFFSET * var_data.cursor_color_speed);

		if( (var_data.cursor.red<0) || (var_data.cursor.red>4095) )
			var_data.cursor.red 	= 4095;
		if( (var_data.cursor.green<0) || (var_data.cursor.green>4095) )
			var_data.cursor.green	= 4095;
		if( (var_data.cursor.blue<0) || (var_data.cursor.blue>4095) )
			var_data.cursor.blue 	= 4095;


//		FPGA_Write(FPGA_POSION_R, (var_data.cursor.red)>>2);
//		FPGA_Write(FPGA_POSION_G, (var_data.cursor.green)>>2);
//		FPGA_Write(FPGA_POSION_B, (var_data.cursor.blue)>>2);

		cursor_func(&var_data.cursor);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if( (var_data.cursor_color_mode&COLOR_MODE_RED) )
			var_data.cursor.red 		-= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_GREEN) )
			var_data.cursor.green 	-= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_BLUE))
			var_data.cursor.blue 	-= (COLOR_OFFSET * var_data.cursor_color_speed);

		if( (var_data.cursor.red<0) || (var_data.cursor.red>4095) )
			var_data.cursor.red 	= 0;
		if( (var_data.cursor.green<0) || (var_data.cursor.green>4095) )
			var_data.cursor.green	= 0;
		if( (var_data.cursor.blue<0) || (var_data.cursor.blue>4095) )
			var_data.cursor.blue 	= 0;

//		FPGA_Write(FPGA_POSION_R, (var_data.cursor.red)>>2);
//		FPGA_Write(FPGA_POSION_G, (var_data.cursor.green)>>2);
//		FPGA_Write(FPGA_POSION_B, (var_data.cursor.blue)>>2);

		cursor_func(&var_data.cursor);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		if( (var_data.cursor_color_mode&COLOR_MODE_RED) )
			var_data.cursor.red 	-= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_GREEN) )
			var_data.cursor.green 	-= (COLOR_OFFSET * var_data.cursor_color_speed);
		if( (var_data.cursor_color_mode&COLOR_MODE_BLUE))
			var_data.cursor.blue 	-= (COLOR_OFFSET * var_data.cursor_color_speed);

		if( (var_data.cursor.red<0) || (var_data.cursor.red>4095) )
			var_data.cursor.red 	= 0;
		if( (var_data.cursor.green<0) || (var_data.cursor.green>4095) )
			var_data.cursor.green	= 0;
		if( (var_data.cursor.blue<0) || (var_data.cursor.blue>4095) )
			var_data.cursor.blue 	= 0;

//		FPGA_Write(FPGA_POSION_R, (var_data.cursor.red)>>2);
//		FPGA_Write(FPGA_POSION_G, (var_data.cursor.green)>>2);
//		FPGA_Write(FPGA_POSION_B, (var_data.cursor.blue)>>2);

		cursor_func(&var_data.cursor);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		var_data.cursor_color_speed = COLOR_SPEED_FAST;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_CURSOR_COLOR_F);

		cr.red 		= var_data.cursor.red  /COLOR_OFFSET;
		cr.green 	= var_data.cursor.green/COLOR_OFFSET;
		cr.blue 	= var_data.cursor.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_vdd_change_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];
	int		idx=0;

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		var_vdd_changed = 0;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
#ifndef USE_PRE_EMPHASIS
		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(HTIME_CHANGE);
#else
		var_vdd_changed = 0;
		pre_emp_index=0;
		tmp_pre_emp=0;		//ushort
		rcb_write(rcb_fd, RCB_LINE1, TITLE_PRE_EMPHASIS);
		if		(pre_emp_index==IDX_VOD)		tmp_pre_emp=prm_data._vod;
		else if (pre_emp_index==IDX_1PRE)		tmp_pre_emp=prm_data._1pre;
		else if (pre_emp_index==IDX_2PRE)		tmp_pre_emp=prm_data._2pre;
		else if (pre_emp_index==IDX_1POST)		tmp_pre_emp=prm_data._1post;
		else if (pre_emp_index==IDX_2POST)		tmp_pre_emp=prm_data._2post;
		else if (pre_emp_index==IDX_DEFAULT)	tmp_pre_emp=_VOD_MAX;
		else									tmp_pre_emp=0;
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
		set_opmode(PRE_EMPHASIS);
#endif
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		//test 20-05-21
		if(!var_vdd_changed)
		{
			idx=get_pattern_index();
			if(group_data.pat[idx].vdd != 0)
			{
				var_data.vdd = group_data.pat[idx].vdd;
			}
			var_vdd_changed = 1;
		}
		//test 20-05-21


		if(var_data.vdd<(model_data.vdd_h -10) )
		{
			var_data.vdd += 10;
//			vdd_set_func(var_data.vdd);
			pwr_vdd_set(var_data.vdd, 0);
		}

		memset(str, 0, sizeof(str));
		sprintf(str, "VDD: %.2fV", var_data.vdd/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		//test 20-05-21
		if(!var_vdd_changed)
		{
			idx=get_pattern_index();
			if(group_data.pat[idx].vdd != 0)
			{
				var_data.vdd = group_data.pat[idx].vdd;
			}
			var_vdd_changed = 1;
		}
		//test 20-05-21

		if( (var_data.vdd>(model_data.vdd_l +10)) && (var_data.vdd<model_data.vdd_h) )
		{
			var_data.vdd -= 10;
//			vdd_set_func(var_data.vdd);
			pwr_vdd_set(var_data.vdd, 0);
		}

		memset(str, 0, sizeof(str));
		sprintf(str, "VDD: %.2fV", var_data.vdd/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VDD_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VDD: %.2fV", var_data.vdd/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_vbl_change_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VALUE: %d", var_data.h_bpo);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(HTIME_CHANGE);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.vbl<(model_data.vbl_h -10) )
		{
			var_data.vbl += 10;
			pwr_vbl_set(var_data.vbl, 0);
		}

		memset(str, 0, sizeof(str));
		sprintf(str, "VBL: %.2fV", var_data.vbl/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if( (var_data.vbl>(model_data.vbl_l +10)) && (var_data.vbl<model_data.vbl_h) )
		{
			var_data.vbl -= 10;
			pwr_vbl_set(var_data.vbl, 0);
		}

		memset(str, 0, sizeof(str));
		sprintf(str, "VBL: %.2fV", var_data.vbl/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VBL_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VBL: %.2fV", var_data.vbl/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void color_set_from_pc(req_color_set_t *pdata)
{
	color_t	cr;
	char 	str[MAX_TEXT_BUF];

	if(pdata->type==1)
	{
		FPGA_Write(FPGA_MEM_RD_CTRL, 0x0008);		//gray_change enable

		set_opmode(GRAY_CHANGE);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_F);

		var_data.color.red		= pdata->red;
		var_data.color.green	= pdata->green;
		var_data.color.blue		= pdata->blue;
		gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue); //2022.05.11 ksk 11bit Scroll
		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
//		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_GRAY);


//		if(color_done==0)
//		{
//			color_done = 1;
//			rcb_write(rcb_fd, RCB_LINE2, TITLE_GRAY_CHANGE_F);
//		}

	}
	else
	{
		FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0008);

		var_data.color_speed = COLOR_SPEED_FAST;
		pattern_change(get_pattern_index());
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
//		color_done = 0;
	}
}

void rcb_color_change_proc(char *c)
{
	color_t	cr;
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_GRAY, 2)==0)
	{
		switch(var_data.color_mode)
		{
		case COLOR_MODE_RED:
			var_data.color_mode = COLOR_MODE_GREEN;
//			gray_color_set(COLOR_MODE_GREEN);
			break;

		case COLOR_MODE_GREEN:
			var_data.color_mode = COLOR_MODE_BLUE;
//			gray_color_set(COLOR_MODE_BLUE);
			break;

		case COLOR_MODE_BLUE:
			var_data.color_mode = COLOR_MODE_RED_GREEN;
//			gray_color_set(COLOR_MODE_RED_GREEN);
			break;

		case COLOR_MODE_RED_GREEN:
			var_data.color_mode = COLOR_MODE_GREEN_BLUE;
//			gray_color_set(COLOR_MODE_GREEN_BLUE);
			break;

		case COLOR_MODE_GREEN_BLUE:
			var_data.color_mode = COLOR_MODE_BLUE_RED;
//			gray_color_set(COLOR_MODE_BLUE_RED);
			break;

		case COLOR_MODE_BLUE_RED:
			var_data.color_mode = COLOR_MODE_WHITE;
//			gray_color_set(COLOR_MODE_WHITE);
			break;

		default:
			var_data.color_mode = COLOR_MODE_RED;
//			gray_color_set(COLOR_MODE_RED);
			break;
		}

		if(var_data.color_mode & COLOR_MODE_RED  ) 	var_data.color.red 		= 4095;
		else										var_data.color.red 		= 0;
		if(var_data.color_mode & COLOR_MODE_GREEN) 	var_data.color.green 	= 4095;
		else										var_data.color.green 	= 0;
		if(var_data.color_mode & COLOR_MODE_BLUE ) 	var_data.color.blue 	= 4095;
		else										var_data.color.blue 	= 0;

//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);

		switch(model_data.mode & 0xf){
			case MODE_HEXA :	gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue); // 2022.04.07 ksk 11bit gray when initiating gray change with rcb
								printf("11bit rcb HEXA\n");
								break;//2022.04.07 ksk 11bit gray
			default			:	gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
								printf("11bit rcb 32lane\n");
								break;
		}

		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		if(var_data.color_speed>COLOR_SPEED_SLOW)
		{
			var_data.color_speed = COLOR_SPEED_SLOW;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_S);
		}
		else
		{
			var_data.color_speed = COLOR_SPEED_FAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_F);
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0008);
		var_data.color_speed = COLOR_SPEED_FAST;
		pattern_change(get_pattern_index());
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_GRAY);
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(var_data.color_speed>COLOR_SPEED_SLOW)
		{
			var_data.color_speed = COLOR_SPEED_SLOW;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_S);
		}
		else
		{
			var_data.color_speed = COLOR_SPEED_FAST;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_F);
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
//		if( (var_data.color_mode&COLOR_MODE_RED) )
//			var_data.color.red 		+= (COLOR_OFFSET * var_data.color_speed);
//		if( (var_data.color_mode&COLOR_MODE_GREEN) )
//			var_data.color.green 	+= (COLOR_OFFSET * var_data.color_speed);
//		if( (var_data.color_mode&COLOR_MODE_BLUE) )
//			var_data.color.blue 	+= (COLOR_OFFSET * var_data.color_speed);

		if( (var_data.color_mode&COLOR_MODE_RED) ){
			if(var_data.color.red < 16)	var_data.color.red 		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.red 		+= (COLOR_OFFSET * var_data.color_speed);
		}
		if( (var_data.color_mode&COLOR_MODE_GREEN) ){
			if(var_data.color.green < 16)	var_data.color.green 		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.green 		+= (COLOR_OFFSET * var_data.color_speed);
		}
		if( (var_data.color_mode&COLOR_MODE_BLUE) ){
			if(var_data.color.blue < 16)	var_data.color.blue		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.blue 		+= (COLOR_OFFSET * var_data.color_speed);
		}

		if( (var_data.color.red<0) || (var_data.color.red>4095) )
			var_data.color.red 	= 4095;
		if( (var_data.color.green<0) || (var_data.color.green>4095) )
			var_data.color.green = 4095;
		if( (var_data.color.blue<0) || (var_data.color.blue>4095) )
			var_data.color.blue 	= 4095;




//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
		switch(model_data.mode & 0xf){
			case MODE_HEXA :	gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue); //2022.04.07 ksk 11bit gray
								break;//2022.04.07 ksk 11bit gray
			default			:	gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
								break;
		}
		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		if( (var_data.color_mode&COLOR_MODE_RED) ){
			if(var_data.color.red < 16)	var_data.color.red 		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.red 		+= (COLOR_OFFSET * var_data.color_speed);
		}
		if( (var_data.color_mode&COLOR_MODE_GREEN) ){
			if(var_data.color.green < 16)	var_data.color.green 		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.green 		+= (COLOR_OFFSET * var_data.color_speed);
		}
		if( (var_data.color_mode&COLOR_MODE_BLUE) ){
			if(var_data.color.blue < 16)	var_data.color.blue		+= (COLOR_OFFSET * var_data.color_speed)+15;
			else	var_data.color.blue 		+= (COLOR_OFFSET * var_data.color_speed);
		}

		if( (var_data.color.red<0) || (var_data.color.red>4095) )
			var_data.color.red 	= 4095;
		if( (var_data.color.green<0) || (var_data.color.green>4095) )
			var_data.color.green = 4095;
		if( (var_data.color.blue<0) || (var_data.color.blue>4095) )
			var_data.color.blue 	= 4095;


//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
		switch(model_data.mode & 0xf){
			case MODE_HEXA :	gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue); break;//2022.04.07 ksk 11bit gray
			default			:	gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2); break;
		}

		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if( (var_data.color_mode&COLOR_MODE_RED) )
			var_data.color.red 		-= (COLOR_OFFSET * var_data.color_speed);
		if( (var_data.color_mode&COLOR_MODE_GREEN) )
			var_data.color.green 	-= (COLOR_OFFSET * var_data.color_speed);
		if( (var_data.color_mode&COLOR_MODE_BLUE))
			var_data.color.blue 	-= (COLOR_OFFSET * var_data.color_speed);

		if( (var_data.color.red<0) || (var_data.color.red>4095) )
			var_data.color.red 	= 0;
		if( (var_data.color.green<0) || (var_data.color.green>4095) )
			var_data.color.green = 0;
		if( (var_data.color.blue<0) || (var_data.color.blue>4095) )
			var_data.color.blue 	= 0;

		if( var_data.color.red<16) var_data.color.red = 0;
		if( var_data.color.green<16) var_data.color.green = 0;
		if( var_data.color.blue<16) var_data.color.blue = 0;


//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
		switch(model_data.mode & 0xf){
			case MODE_HEXA :	gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue); break;//2022.04.07 ksk 11bit gray
			default			:	gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2); break;
		}

		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		if( (var_data.color_mode&COLOR_MODE_RED) )
			var_data.color.red 		-= (COLOR_OFFSET * var_data.color_speed);
		if( (var_data.color_mode&COLOR_MODE_GREEN) )
			var_data.color.green 	-= (COLOR_OFFSET * var_data.color_speed);
		if( (var_data.color_mode&COLOR_MODE_BLUE))
			var_data.color.blue 	-= (COLOR_OFFSET * var_data.color_speed);

		if( (var_data.color.red<0) || (var_data.color.red>4095) )
			var_data.color.red 	= 0;
		if( (var_data.color.green<0) || (var_data.color.green>4095) )
			var_data.color.green = 0;
		if( (var_data.color.blue<0) || (var_data.color.blue>4095) )
			var_data.color.blue 	= 0;

		if( var_data.color.red<16) var_data.color.red = 0;
		if( var_data.color.green<16) var_data.color.green = 0;
		if( var_data.color.blue<16) var_data.color.blue = 0;


//		gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
		switch(model_data.mode & 0xf){
			case MODE_HEXA :	gray_change_func(var_data.color.red, var_data.color.green, var_data.color.blue);//2022.04.07 ksk 11bit gray
								break;//2022.04.07 ksk 11bit gray
			default			:	gray_change_func(var_data.color.red>>2, var_data.color.green>>2, var_data.color.blue>>2);
								break;
		}

		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		gray_change_init();

		rcb_write(rcb_fd, RCB_LINE1, (var_data.color_speed>COLOR_SPEED_SLOW) ? TITLE_GRAY_CHANGE_F : TITLE_GRAY_CHANGE_S);

		cr.red 		= var_data.color.red  /COLOR_OFFSET;
		cr.green 	= var_data.color.green/COLOR_OFFSET;
		cr.blue 	= var_data.color.blue /COLOR_OFFSET;

		memset(str, 0, sizeof(str));
		sprintf(str, "%d %d %d", cr.red, cr.green, cr.blue);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_GRAY);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_gray_scale_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];
	int 	max_level;

	switch(model_data.mode&0x30)
	{
		case 0x00: 	max_level = 64; 	break;	// 6bit
		case 0x10: 	max_level = 256; 	break;	// 8bit
		case 0x20: 	max_level = 1024; 	break;	// 10bit
		default:	max_level = 4096; 	break;
	}

	if(strncmp(c, KEY_GRAY, 2)==0)
	{
		var_data.gray_scale = get_gray_scale_value(get_pattern_index());
		pattern_change(get_pattern_index());
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		var_data.gray_scale = get_gray_scale_value(get_pattern_index());
		pattern_change(get_pattern_index());
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(var_data.gray_scale >= max_level)	var_data.gray_scale = 8;
		else									var_data.gray_scale *= 2;

		gray_level_change_func();

		memset(str, 0, sizeof(str));
		sprintf(str, "LEVEL = %d", var_data.gray_scale);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(var_data.gray_scale < 16)			var_data.gray_scale = max_level;
		else									var_data.gray_scale /= 2;

		gray_level_change_func();

		memset(str, 0, sizeof(str));
		sprintf(str, "LEVEL = %d", var_data.gray_scale);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE1, TITLE_GRAY_CHANGE_S);
		memset(str, 0, sizeof(str));
		sprintf(str, "LEVEL = %d", var_data.gray_scale);
		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_model_delete_proc(char *c)
{
	if(strncmp(c, KEY_FILE, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_flag = FILE_ALL;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
		}
		else
		{
#ifndef GROUP_SELECT
				file_flag = FILE_ONE;
				rcb_write(rcb_fd, RCB_LINE1, TITLE_PAT_DEL_ONE);
				if(get_file_list(DEL_TYPE_PATTERN_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
				set_opmode(PATTERN_DELETE);
#else
				file_flag = FILE_ONE;
				rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_DEL_ONE);
				if(get_file_list(DEL_TYPE_GROUP_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
				set_opmode(GROUP_DELETE);
#endif
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		free_file_list();
		free_sub_bmp_list();
		model_init();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		char path[MAX_PATH];
		memset(path, 0, sizeof(path));
		if(file_flag==FILE_ONE)
		{
			sprintf(path, "%s%s/%s", DIR_ROOT, DIR_MODEL, get_cur_file_name());
			if(0==remove(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				if(get_file_list(DEL_TYPE_MODEL_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
		}
		else
		{
			sprintf(path, "%s%s", DIR_ROOT, DIR_MODEL);
			if(0<remove_dirs(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_inc();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_dec();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ONE);
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
		else
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");

		}
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_pattern_delete_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_FILE, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_flag = FILE_ALL;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_PAT_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
		}
		else
		{
			file_flag = FILE_ONE;
			if(get_file_list(DEL_TYPE_BITMAP_DIR))
			{
				memset(str, 0, sizeof(str));
				sprintf(str, "BMP %s/", get_cur_file_name());
				rcb_write(rcb_fd, RCB_LINE1, str);
				if(get_sub_bmp_list(get_cur_file_name())) rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE1, TITLE_BMP_DEL_ONE);
				rcb_write(rcb_fd, RCB_LINE2, "NO FILE");
			}
			set_opmode(BITMAP_DELETE);
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		free_file_list();
		free_sub_bmp_list();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		char path[MAX_PATH];
		memset(path, 0, sizeof(path));
		if(file_flag==FILE_ONE)
		{
			sprintf(path, "%s%s/%s", DIR_ROOT, DIR_PATTERN, get_cur_file_name());
			if(0==remove(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				if(get_file_list(DEL_TYPE_PATTERN_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
		}
		else
		{
			sprintf(path, "%s%s", DIR_ROOT, DIR_PATTERN);
			if(0<remove_dirs(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_inc();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_dec();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_PAT_DEL_ONE);
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
		else
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_PAT_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
		}
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_bitmap_delete_proc(char *c)
{
	char 	str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_FILE, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_flag = FILE_ALL;
			if(get_file_list(DEL_TYPE_BITMAP_DIR))
			{
				memset(str, 0, sizeof(str));
				sprintf(str, "BMP %s/", get_cur_file_name());
				rcb_write(rcb_fd, RCB_LINE1, str);
			}
			rcb_write(rcb_fd, RCB_LINE2, "DELETE DIR  [OK]");
		}
		else
		{
			file_flag = FILE_ONE;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ONE);
			if(get_file_list(DEL_TYPE_MODEL_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			set_opmode(MODEL_DELETE);
		}
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		file_list_dec();
		memset(str, 0, sizeof(str));
		sprintf(str, "BMP %s/", get_cur_file_name());
		rcb_write(rcb_fd, RCB_LINE1, str);
		if(file_flag==FILE_ONE)
		{
			if(get_sub_bmp_list(get_cur_file_name())) 	rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
			else										rcb_write(rcb_fd, RCB_LINE2, "NO FILE!");
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		free_file_list();
		free_sub_bmp_list();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		char path[MAX_PATH];
		memset(path, 0, sizeof(path));
		if(file_flag==FILE_ONE)
		{
			sprintf(path, "%s%s/%s/%s", DIR_ROOT, DIR_BMP, get_cur_file_name(), get_sub_bmp_name());
			if(0==remove(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				if(get_sub_bmp_list(get_cur_file_name())) rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
			}
		}
		else
		{
			sprintf(path, "%s%s/%s", DIR_ROOT, DIR_BMP, get_cur_file_name());
			if(0<remove_dirs(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
		}
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		file_list_inc();
		memset(str, 0, sizeof(str));
		sprintf(str, "BMP %s/", get_cur_file_name());
		rcb_write(rcb_fd, RCB_LINE1, str);
		if(file_flag==FILE_ONE)
		{
			if(get_sub_bmp_list(get_cur_file_name())) 	rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
			else										rcb_write(rcb_fd, RCB_LINE2, "NO FILE!");
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			sub_bmp_list_inc();
			rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			sub_bmp_list_dec();
			rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			memset(str, 0, sizeof(str));
			sprintf(str, "BMP %s/", get_cur_file_name());
			rcb_write(rcb_fd, RCB_LINE1, str);
			rcb_write(rcb_fd, RCB_LINE2, get_sub_bmp_name());
		}
		else
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_BMP_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
		}
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_usb_detect_proc(char *c)
{
	if(strncmp(c, KEY_UP, 2)==0)
	{
		usb_title_change(1);
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		usb_operation();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		usb_title_change(0);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		usb_default();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void rcb_pwr_spc_set_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(--var_spc_en<0) var_spc_en = 2;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_pwr_spc[var_spc_en]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		pwr_spc_onoff_set(var_spc_en);
		memset(str, 0, MAX_TEXT_BUF);
		if (var_spc_en==1)		sprintf(str, "%s", "SPC enabled");
		else if (var_spc_en==2)
		{
			sprintf(str, "%s", "OTC enabled");
			pwr_model_set(&model_data, 0, 0);
		}
		else
		{
			sprintf(str, "%s", "Disabled");
			pwr_model_set(&model_data, 0, 0);
		}
		rcb_write(rcb_fd, RCB_LINE2, str);
		sleep(1);
		rcb_menu();
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(++var_spc_en>2) var_spc_en = 0;

		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s", g_pwr_spc[var_spc_en]);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_pwr_spc_onoff_set();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_pwr_update_proc(char *c)
{
	char str[MAX_TEXT_BUF];

	memset(str, 0, sizeof(str));

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LINE2, "PLEASE WAIT..");
		if(pwr_fw_upload(pwr_fd)==RES_ACK)
		{
			pwr_upload_flag=0;
			printf("power upload flag off\n");
			rcb_write(rcb_fd, RCB_LINE2, "UPDATE FINISHED");
			sleep(2);
			rcb_menu();
		}
		else
		{
			rcb_write(rcb_fd, RCB_LINE2, "FILE NOT FOUND");
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_pwr_fw_upload();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_tcon_i2c_ctrl_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_UP, 2)==0)
	{
		if(tcon_i2c_reg_index <= 0)	tcon_i2c_reg_index = MAX_I2C_REG_CNT;
		else 						tcon_i2c_reg_index--;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s%s", TITLE_I2C_SET, g_i2c_reg_name[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE1, str);
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		tcon_i2c_reg_index=0;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
//		tcon_i2c_reg_index=0;
//		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
//		memset(str, 0, sizeof(str));
//		sprintf(str, "VALUE: %d", var_data.h_bpo);
//		rcb_write(rcb_fd, RCB_LINE2, str);
//		set_opmode(HTIME_CHANGE);

		vby1_equal_opt_index=0;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VBY1_EQUAL_SET);
		memset(str, 0, MAX_TEXT_BUF);
		if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
		else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
		else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
		else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(VBY1_EQUAL);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if(tcon_i2c_reg_index >= MAX_I2C_REG_CNT)	tcon_i2c_reg_index = 0;
		else 							tcon_i2c_reg_index++;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, "%s%s", TITLE_I2C_SET, g_i2c_reg_name[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE1, str);
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(tcon_i2c_reg_data[tcon_i2c_reg_index] >= tcon_i2c_data_max[tcon_i2c_reg_index])	tcon_i2c_reg_data[tcon_i2c_reg_index] = tcon_i2c_data_max[tcon_i2c_reg_index];
		else																				tcon_i2c_reg_data[tcon_i2c_reg_index]++;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(tcon_i2c_reg_data[tcon_i2c_reg_index] <= tcon_i2c_data_min[tcon_i2c_reg_index])	tcon_i2c_reg_data[tcon_i2c_reg_index] = tcon_i2c_data_min[tcon_i2c_reg_index];
		else																				tcon_i2c_reg_data[tcon_i2c_reg_index]--;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		if(tcon_i2c_reg_data[tcon_i2c_reg_index] >= tcon_i2c_data_max[tcon_i2c_reg_index])	tcon_i2c_reg_data[tcon_i2c_reg_index] = tcon_i2c_data_max[tcon_i2c_reg_index];
		else																				tcon_i2c_reg_data[tcon_i2c_reg_index]++;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		if(tcon_i2c_reg_data[tcon_i2c_reg_index] <= tcon_i2c_data_min[tcon_i2c_reg_index])	tcon_i2c_reg_data[tcon_i2c_reg_index] = tcon_i2c_data_min[tcon_i2c_reg_index];
		else																				tcon_i2c_reg_data[tcon_i2c_reg_index]--;
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		send_tcon_i2c();
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		sprintf(str, "%s%s", TITLE_I2C_SET, g_i2c_reg_name[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE1, str);
		memset(str, 0, MAX_TEXT_BUF);
		sprintf(str, " %04x /%04x~%04x", tcon_i2c_reg_data[tcon_i2c_reg_index], tcon_i2c_data_min[tcon_i2c_reg_index], tcon_i2c_data_max[tcon_i2c_reg_index]);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(I2C_CTRL);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}


void rcb_vby1_equal_option_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];
	int			idx=0;

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		vby1_equal_opt_index=0;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		vby1_equal_opt_index=0;
//		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
//		memset(str, 0, sizeof(str));
//		sprintf(str, "VALUE: %d", var_data.h_bpo);
//		rcb_write(rcb_fd, RCB_LINE2, str);
//		set_opmode(HTIME_CHANGE);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VDD_CHANGE);
		memset(str, 0, sizeof(str));

		idx=get_pattern_index();
		if(group_data.pat[idx].vdd>0)	sprintf(str, "VDD: %.2fV", group_data.pat[idx].vdd/100.0);
		else							sprintf(str, "VDD: %.2fV", var_data.vdd/100.0);

		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(VDD_CHANGE);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		if (vby1_equal_opt_index <= 0)	vby1_equal_opt_index=2;
		else							--vby1_equal_opt_index;

		memset(str, 0, MAX_TEXT_BUF);
		if(vby1_equal_opt_index==0)
		{
			if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==1)
		{
			if		(re_driver_eqacl==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacl==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacl==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==2)
		{
			if		(re_driver_eqacu==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacu==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacu==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}

		rcb_write(rcb_fd, RCB_LINE2, str);

		re_driver_mux_ACnU_set(re_driver_eqacu);
		re_driver_mux_ACnL_set(re_driver_eqacl);
		re_driver_mux_DCn_set(re_driver_eqdc);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if (vby1_equal_opt_index >= 2)	vby1_equal_opt_index=0;
		else							++vby1_equal_opt_index;

		memset(str, 0, MAX_TEXT_BUF);
		if(vby1_equal_opt_index==0)
		{
			if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==1)
		{
			if		(re_driver_eqacl==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacl==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacl==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==2)
		{
			if		(re_driver_eqacu==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacu==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacu==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}

		rcb_write(rcb_fd, RCB_LINE2, str);

		re_driver_mux_ACnU_set(re_driver_eqacu);
		re_driver_mux_ACnL_set(re_driver_eqacl);
		re_driver_mux_DCn_set(re_driver_eqdc);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		memset(str, 0, MAX_TEXT_BUF);

		if(vby1_equal_opt_index==0)
		{
			if(re_driver_eqdc >= 3)	re_driver_eqdc=0;
			else					++re_driver_eqdc;

			if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==1)
		{
			if(re_driver_eqacl >= 3)	re_driver_eqacl=0;
			else						++re_driver_eqacl;

			if		(re_driver_eqacl==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacl==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacl==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==2)
		{
			if(re_driver_eqacu >= 3)	re_driver_eqacu=0;
			else						++re_driver_eqacu;

			if		(re_driver_eqacu==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacu==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacu==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}

		rcb_write(rcb_fd, RCB_LINE2, str);

		re_driver_mux_ACnU_set(re_driver_eqacu);
		re_driver_mux_ACnL_set(re_driver_eqacl);
		re_driver_mux_DCn_set(re_driver_eqdc);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		memset(str, 0, MAX_TEXT_BUF);

		if(vby1_equal_opt_index==0)
		{
			if(re_driver_eqdc <= 0)	re_driver_eqdc=3;
			else					--re_driver_eqdc;

			if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==1)
		{
			if(re_driver_eqacl <= 0)	re_driver_eqacl=3;
			else						--re_driver_eqacl;

			if		(re_driver_eqacl==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacl==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacl==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==2)
		{
			if(re_driver_eqacu <= 0)	re_driver_eqacu=3;
			else						--re_driver_eqacu;

			if		(re_driver_eqacu==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacu==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacu==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}

		rcb_write(rcb_fd, RCB_LINE2, str);

		re_driver_mux_ACnU_set(re_driver_eqacu);
		re_driver_mux_ACnL_set(re_driver_eqacl);
		re_driver_mux_DCn_set(re_driver_eqdc);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LEDONOFF, (get_onoff_flag()==ENUM_ON) ? LED_ON_ONOFF : LED_OFF_ONOFF);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VBY1_EQUAL_SET);
		memset(str, 0, MAX_TEXT_BUF);
		if(vby1_equal_opt_index==0)
		{
			if		(re_driver_eqdc==0)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqdc==1)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqdc==2)		sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==1)
		{
			if		(re_driver_eqacl==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacl==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacl==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}
		else if(vby1_equal_opt_index==2)
		{
			if		(re_driver_eqacu==0)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"VCC");
			else if (re_driver_eqacu==1)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"OPEN");
			else if (re_driver_eqacu==2)	sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"PULL-DOWN");
			else							sprintf(str, "%s%s", g_vby1_equal_opt_name[vby1_equal_opt_index],"GND");
		}

		rcb_write(rcb_fd, RCB_LINE2, str);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);

		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}

}
#ifdef GROUP_SELECT
void rcb_group_change_proc(char *c)
{
	if(strncmp(c, KEY_FILE, 2)==0)
	{
		file_flag = FILE_ONE;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MODEL_DEL_ONE);
		if(get_file_list(DEL_TYPE_MODEL_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		set_opmode(MODEL_DELETE);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		model_init();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		if( (model_selection_end==ACK)&&(group_selection_end==ACK) ) {
			rcb_ready_screen(ACK);
			set_opmode(READY);
		}
		else rcb_menu();
	}
	else if(strncmp(c, KEY_MODEL, 2)==0)
	{
		group_update();
		if(group_cnt>0)
		{
			if( (strcmp(old_model_name,model_name)) || (reload_model_flag) )
			{
				model_selection_end = model_select(model_name, 0);
				DP_EDID_change();
				model_variable_reset();
			}

			if (model_selection_end==ACK)
			{
				set_model_init_status(ACK);
				group_selection_end = group_select(group_list[group_idx]);
			}
			else
			{
				set_model_init_status(NACK);
				memset(old_model_name,0,sizeof(old_model_name));
				model_init();
				rcb_model_change();
			}
		}
		else
		{
			group_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
		}
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		group_update();
		if(group_cnt>0)
		{
			if( (strcmp(old_model_name,model_name)) || (reload_model_flag) )
			{
				model_selection_end = model_select(model_name, 0);
				DP_EDID_change();
				model_variable_reset();
			}

			if (model_selection_end==ACK)
			{
				set_model_init_status(ACK);
				group_selection_end = group_select(group_list[group_idx]);
			}
			else
			{
				set_model_init_status(NACK);
				memset(old_model_name,0,sizeof(old_model_name));
				model_init();
				rcb_model_change();
			}
		}
		else
		{
			group_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
		}
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		model_init();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		group_update();
		if(group_cnt>0)
		{
			if(group_idx >= group_cnt-1)	group_idx = 0;
			else							group_idx++;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, group_list[group_idx]);
		}
		else
		{
			group_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		group_update();
		if(group_cnt>0)
		{
			if(group_idx <= 0)				group_idx = group_cnt - 1;
			else							group_idx--;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, group_list[group_idx]);
		}
		else
		{
			group_idx = 0;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
			rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_group_change(group_init());
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_group_delete_proc(char *c)
{
	if(strncmp(c, KEY_ONOFF, 2)==0)
	{
	}
	else if(strncmp(c, KEY_FILE, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_flag = FILE_ALL;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
		}
		else
		{
			file_flag = FILE_ONE;
			rcb_write(rcb_fd, RCB_LINE1, TITLE_PAT_DEL_ONE);
			if(get_file_list(DEL_TYPE_PATTERN_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			set_opmode(PATTERN_DELETE);
		}
	}
	else if(strncmp(c, KEY_ESC, 2)==0)
	{
		free_file_list();
		free_sub_bmp_list();
		model_init();
		rcb_model_change();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		char path[MAX_PATH];
		memset(path, 0, sizeof(path));
		if(file_flag==FILE_ONE)
		{
			sprintf(path, "%s%s/%s", DIR_ROOT, DIR_GROUP, get_cur_file_name());
			if(0==remove(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				if(get_file_list(DEL_TYPE_GROUP_FILE)) rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
			}
		}
		else
		{
			sprintf(path, "%s%s", DIR_ROOT, DIR_GROUP);
			if(0<remove_dirs(path))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SUCCESS!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
			else
			{
				rcb_write(rcb_fd, RCB_LINE2, "FAILED!");
				delay_us(1000000);
				rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");
			}
		}
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_inc();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			file_list_dec();
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		if(file_flag==FILE_ONE)
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_DEL_ONE);
			rcb_write(rcb_fd, RCB_LINE2, get_cur_file_name());
		}
		else
		{
			rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_DEL_ALL);
			rcb_write(rcb_fd, RCB_LINE2, "DELETE ALL");

		}
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}
#endif



#ifdef USE_PRE_EMPHASIS

void rcb_pre_emphasis_proc(char *c)
{
	char 		str[MAX_TEXT_BUF];

	if(strncmp(c, KEY_ESC, 2)==0)
	{
		pre_emp_index=0;
		rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
	}
	else if(strncmp(c, KEY_FUNC, 2)==0)
	{
		pre_emp_index=0;
//		rcb_write(rcb_fd, RCB_LINE1, TITLE_HTIME_CHANGE);
//		memset(str, 0, sizeof(str));
//		sprintf(str, "VALUE: %d", var_data.h_bpo);
//		rcb_write(rcb_fd, RCB_LINE2, str);
//		set_opmode(HTIME_CHANGE);

		//20-04-01	add vbl change, request by sonk, sdc
		rcb_write(rcb_fd, RCB_LINE1, TITLE_VBL_CHANGE);
		memset(str, 0, sizeof(str));
		sprintf(str, "VBL: %.2fV", var_data.vbl/100.0);
		rcb_write(rcb_fd, RCB_LINE2, str);
		set_opmode(VBL_CHANGE);
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_this_pre_emp_index(pre_emp_index, tmp_pre_emp);
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		if (pre_emp_index <= 0)	pre_emp_index=5;
		else					--pre_emp_index;

		tmp_pre_emp=0;		//ushort

		if		(pre_emp_index==IDX_VOD)		tmp_pre_emp=prm_data._vod;
		else if (pre_emp_index==IDX_1PRE)		tmp_pre_emp=prm_data._1pre;
		else if (pre_emp_index==IDX_2PRE)		tmp_pre_emp=prm_data._2pre;
		else if (pre_emp_index==IDX_1POST)		tmp_pre_emp=prm_data._1post;
		else if (pre_emp_index==IDX_2POST)		tmp_pre_emp=prm_data._2post;
		else if (pre_emp_index==IDX_DEFAULT)	tmp_pre_emp=_VOD_MAX;
		else									tmp_pre_emp=0;

		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		if (pre_emp_index >= 5)	pre_emp_index=0;
		else					++pre_emp_index;

		tmp_pre_emp=0;		//ushort

		if		(pre_emp_index==IDX_VOD)		tmp_pre_emp=prm_data._vod;
		else if (pre_emp_index==IDX_1PRE)		tmp_pre_emp=prm_data._1pre;
		else if (pre_emp_index==IDX_2PRE)		tmp_pre_emp=prm_data._2pre;
		else if (pre_emp_index==IDX_1POST)		tmp_pre_emp=prm_data._1post;
		else if (pre_emp_index==IDX_2POST)		tmp_pre_emp=prm_data._2post;
		else if (pre_emp_index==IDX_DEFAULT)	tmp_pre_emp=_VOD_MAX;
		else									tmp_pre_emp=0;

		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_FWD, 2)==0)
	{
		tmp_pre_emp = increase_pre_emp_setting(pre_emp_index, tmp_pre_emp);
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_BWD, 2)==0)
	{
		tmp_pre_emp = decrease_pre_emp_setting(pre_emp_index, tmp_pre_emp);
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_LONGFWD, 2)==0)
	{
		tmp_pre_emp = increase_pre_emp_setting(pre_emp_index, tmp_pre_emp);
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_LONGBWD, 2)==0)
	{
		tmp_pre_emp = decrease_pre_emp_setting(pre_emp_index, tmp_pre_emp);
		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_write(rcb_fd, RCB_LEDONOFF, (get_onoff_flag()==ENUM_ON) ? LED_ON_ONOFF : LED_OFF_ONOFF);
		rcb_write(rcb_fd, RCB_LINE1, TITLE_PRE_EMPHASIS);

		tmp_pre_emp=0;		//ushort

		if		(pre_emp_index==IDX_VOD)		tmp_pre_emp=prm_data._vod;
		else if (pre_emp_index==IDX_1PRE)		tmp_pre_emp=prm_data._1pre;
		else if (pre_emp_index==IDX_2PRE)		tmp_pre_emp=prm_data._2pre;
		else if (pre_emp_index==IDX_1POST)		tmp_pre_emp=prm_data._1post;
		else if (pre_emp_index==IDX_2POST)		tmp_pre_emp=prm_data._2post;
		else if (pre_emp_index==IDX_DEFAULT)	tmp_pre_emp=_VOD_MAX;
		else									tmp_pre_emp=0;

		display_pre_emphasis(pre_emp_index, tmp_pre_emp);
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);

		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}

}


void display_pre_emphasis(unsigned int index, unsigned short value)
{
	char 	str[MAX_TEXT_BUF];

	memset(str, 0, MAX_TEXT_BUF);

	if (index < IDX_DEFAULT)
	{
		if(value & 0x8000)	//negative
		{
			if (fpga_pre_emphasis_check(index, value))	sprintf(str, "%s%c%02d%s", g_pre_emphasis_name[index],'-',tmp_pre_emp&0x001f," OK");
			else										sprintf(str, "%s%c%02d", g_pre_emphasis_name[index],'-',tmp_pre_emp&0x001f);
		}
		else	//positive
		{
			if (fpga_pre_emphasis_check(index, value))	sprintf(str, "%s%c%02d%s", g_pre_emphasis_name[index],'+',tmp_pre_emp&0x001f," OK");
			else										sprintf(str, "%s%c%02d", g_pre_emphasis_name[index],'+',tmp_pre_emp&0x001f);
		}
	}
	else
	{
//		if (fpga_pre_emphasis_check(index, value))		sprintf(str, "%s%s", g_pre_emphasis_name[index],"   OK");
//		else											sprintf(str, "%s", g_pre_emphasis_name[index]);
		sprintf(str, "%s", g_pre_emphasis_name[index]);
	}
	rcb_write(rcb_fd, RCB_LINE2, str);
}


unsigned short fpga_pre_emphasis_check(unsigned int index, unsigned short pre_value)
{
	unsigned short buf=0, data=0, polar=0;

	switch(index)
	{
		case IDX_VOD :
			buf 	= FPGA_Read(FPGA_XCVR_VOD);
			data	= buf & 0x001f;
			polar	= 0;
			break;
		case IDX_1PRE :
			buf 	= FPGA_Read(FPGA_XCVR_1PRE_TAP);
			data	= buf & 0x001f;
			polar	= (buf & 0x0020)>>5;
			break;
		case IDX_2PRE :
			buf 	= FPGA_Read(FPGA_XCVR_2PRE_TAP);
			data	= buf & 0x0007;
			polar	= (buf & 0x0010)>>4;
			break;
		case IDX_1POST :
			buf		= FPGA_Read(FPGA_XCVR_1POST_TAP);
			data	= buf & 0x001f;
			polar	= (buf & 0x0040)>>6;
			break;
		case IDX_2POST :
			buf		= FPGA_Read(FPGA_XCVR_2POST_TAP);
			data	= buf & 0x000f;
			polar	= (buf & 0x0020)>>5;
			break;
//		case IDX_DEFAULT :
//			buf		= FPGA_Read(FPGA_XCVR_VOD)|FPGA_Read(FPGA_XCVR_1PRE_TAP)|FPGA_Read(FPGA_XCVR_2PRE_TAP)|FPGA_Read(FPGA_XCVR_1POST_TAP)|FPGA_Read(FPGA_XCVR_2POST_TAP);
//			data	= buf & 0x001f;
//			polar	= ((buf & 0xffe0)? 1:0);
//			break;
		default :
			break;
	}

	if( ((pre_value & 0x001f) == data) && (((pre_value & 0x8000)>>15)==polar) )	return 1;
	else																		return 0;
}

void set_pre_emphasis_default(void)
{
	prm_data._1pre	=0;
	prm_data._2pre	=0;
	prm_data._1post	=0;
	prm_data._2post	=0;
	prm_data._vod	=_VOD_MAX;

	FPGA_Write(FPGA_XCVR_VOD, prm_data._vod);
	FPGA_Write(FPGA_XCVR_1POST_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_2POST_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_1PRE_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_2PRE_TAP, 0x0000);

	printf("Set pre-emphasis default\tvod= 31, 1post= +0, 2post= +0, 1pre= +0, 2pre= +0\n");
}

ushort increase_pre_emp_setting(unsigned int index, unsigned short value)
{
	uint16_t	buf=0, result=0;

	if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
	else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33

	switch(index)
	{
	case IDX_VOD : //VOD
//		buf=value+_PREM_REF_VALUE;	// (current value + ref) <= max 62
		if (buf<(_VOD_MAX+_PREM_REF_VALUE))		buf++;
		else									buf = _VOD_MAX + _PREM_REF_VALUE;		//62
		break;
	case IDX_1PRE : //1pre-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf<(_1PRE_MAX+_PREM_REF_VALUE))	buf++;
		else									buf = _1PRE_MAX+_PREM_REF_VALUE;	//47
		break;
	case IDX_2PRE : //2pre-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf<(_2PRE_MAX+_PREM_REF_VALUE))	buf++;
		else									buf = _2PRE_MAX+_PREM_REF_VALUE;	//38
		break;
	case IDX_1POST : //1post-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf<(_1POST_MAX+_PREM_REF_VALUE))	buf++;
		else									buf = _1POST_MAX+_PREM_REF_VALUE;	//56
		break;
	case IDX_2POST : //2post-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf<(_2POST_MAX+_PREM_REF_VALUE))	buf++;
		else									buf = _2POST_MAX+_PREM_REF_VALUE;	//43
		break;
	default :
//		vod_val		=MAX_VOD;
//		1pre_val	=0;
//		2pre_val	=0;
//		1post_val	=0;
//		2post_val	=0;
		break;
	}


	if (buf < _PREM_REF_VALUE)	result = (_PREM_REF_VALUE - buf) | 0x8000;
	else						result = buf - _PREM_REF_VALUE;

	return result;
}

ushort decrease_pre_emp_setting(unsigned int index, unsigned short value)
{
	uint16_t	buf=0, result=0;

	if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
	else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33

	switch(index)
	{
	case IDX_VOD : //VOD
//		buf=value+_PREM_REF_VALUE;	// (current value + ref) <= max 62
		if (buf>(_PREM_REF_VALUE+1))			buf--;
		else									buf = _PREM_REF_VALUE+1;		//31	min 1
		break;
	case IDX_1PRE : //1pre-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf>(_PREM_REF_VALUE-_1PRE_MAX))	buf--;
		else									buf = _PREM_REF_VALUE-_1PRE_MAX;	//15
		break;
	case IDX_2PRE : //2pre-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf>(_PREM_REF_VALUE-_2PRE_MAX))	buf--;
		else									buf = _PREM_REF_VALUE-_2PRE_MAX;	//24
		break;
	case IDX_1POST : //1post-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf>(_PREM_REF_VALUE-_1POST_MAX))	buf--;
		else									buf = _PREM_REF_VALUE-_1POST_MAX;	//6
		break;
	case IDX_2POST : //2post-tap
//		if(value & 0x8000)						buf=_PREM_REF_VALUE - (value & 0x001f);		//neg 	-2 -> 29
//		else									buf=_PREM_REF_VALUE + (value & 0x001f);		//pos	+2 -> 33
		if (buf>(_PREM_REF_VALUE-_2POST_MAX))	buf--;
		else									buf = _PREM_REF_VALUE-_2POST_MAX;	//19
		break;
	default :
//		vod_val		=MAX_VOD;
//		1pre_val	=0;
//		2pre_val	=0;
//		1post_val	=0;
//		2post_val	=0;
		break;
	}


	if (buf < _PREM_REF_VALUE)	result = (_PREM_REF_VALUE - buf) | 0x8000;
	else						result = buf - _PREM_REF_VALUE;

	return result;
}

unsigned int set_this_pre_emp_index(unsigned int index, unsigned short value)
{
	switch(index)
	{
		case IDX_VOD :
			FPGA_Write(FPGA_XCVR_VOD, value & 0x001f);
			usleep(100);
			prm_data._vod = value;
			printf("Set Vod : %d\t0x%04x\n", value, FPGA_Read(FPGA_XCVR_VOD));
			break;
		case IDX_1PRE :
			if (value & 0x8000)	FPGA_Write(FPGA_XCVR_1PRE_TAP, (value & 0x001f) | 0x0020 );
			else				FPGA_Write(FPGA_XCVR_1PRE_TAP, (value & 0x001f));
			usleep(100);
			prm_data._1pre = value;
			printf("Set 1pre tap : %c0x%04x\t0x%04x\n", (value & 0x8000)?'-':'+', value&0x001f, FPGA_Read(FPGA_XCVR_1PRE_TAP));
			break;
		case IDX_2PRE :
			if (value & 0x8000)	FPGA_Write(FPGA_XCVR_2PRE_TAP, (value & 0x0007) | 0x0010 );
			else				FPGA_Write(FPGA_XCVR_2PRE_TAP, (value & 0x0007));
			usleep(100);
			prm_data._2pre = value;
			printf("Set 2pre tap : %c0x%04x\t0x%04x\n", (value & 0x8000)?'-':'+', value&0x001f, FPGA_Read(FPGA_XCVR_2PRE_TAP));
			break;
		case IDX_1POST :
			if (value & 0x8000)	FPGA_Write(FPGA_XCVR_1POST_TAP, (value & 0x001f) | 0x0040 );
			else				FPGA_Write(FPGA_XCVR_1POST_TAP, (value & 0x001f));
			usleep(100);
			prm_data._1post = value;
			printf("Set 1post tap : %c0x%04x\t0x%04x\n", (value & 0x8000)?'-':'+', value&0x001f, FPGA_Read(FPGA_XCVR_1POST_TAP));
			break;
		case IDX_2POST:
			if (value & 0x8000)	FPGA_Write(FPGA_XCVR_2POST_TAP, (value & 0x000f) | 0x0020 );
			else				FPGA_Write(FPGA_XCVR_2POST_TAP, (value & 0x000f));
			usleep(100);
			prm_data._2post = value;
			printf("Set 1post tap : %c0x%04x\t0x%04x\n", (value & 0x8000)?'-':'+', value&0x001f, FPGA_Read(FPGA_XCVR_2POST_TAP));
			break;
		case IDX_DEFAULT :
			set_pre_emphasis_default();
			break;
		default :
			break;
	}

	return 0;
}

int pre_emphasis_set_from_pc(char vod, char post_1st, char post_2nd, char pre_1st, char pre_2nd)
{
	uint16_t	buf_vod=0, buf_post_1st=0, buf_post_2nd=0, buf_pre_1st=0, buf_pre_2nd=0;

	// check range
	if(vod & 0x80)	//negative value
	{
		buf_vod = ((-(unsigned short)(vod)) & 0x00ff);
		if(buf_vod > _VOD_MAX)	buf_vod = _VOD_MAX;
		else if(buf_vod < 1)	buf_vod = 1;
	}
	else
	{
		buf_vod = (unsigned short)(vod);
		if(buf_vod > _VOD_MAX)	buf_vod = _VOD_MAX;
		else if(buf_vod < 1)	buf_vod = 1;
	}

	if(post_1st & 0x80)
	{
		buf_post_1st = ((-(unsigned short)(post_1st)) & 0x00ff);
		if(buf_post_1st > _1POST_MAX)	buf_post_1st = (_1POST_MAX | 0x8000);
		else							buf_post_1st |= 0x8000;
	}
	else
	{
		buf_post_1st = (unsigned short)(post_1st);
		if(buf_post_1st > _1POST_MAX)	buf_post_1st = _1POST_MAX;
	}

	if(post_2nd & 0x80)
	{
		buf_post_2nd = ((-(unsigned short)(post_2nd)) & 0x00ff);
		if(buf_post_2nd > _2POST_MAX)	buf_post_2nd = (_2POST_MAX | 0x8000);
		else							buf_post_2nd |= 0x8000;
	}
	else
	{
		buf_post_2nd = (unsigned short)(post_2nd);
		if(buf_post_2nd > _2POST_MAX)	buf_post_2nd = _2POST_MAX;
	}

	if(pre_1st & 0x80)
	{
		buf_pre_1st = ((-(unsigned short)(pre_1st)) & 0x00ff);
		if(buf_pre_1st > _1PRE_MAX)		buf_pre_1st = (_1PRE_MAX | 0x8000);
		else							buf_pre_1st |= 0x8000;
	}
	else
	{
		buf_pre_1st = (unsigned short)(pre_1st);
		if(buf_pre_1st > _1PRE_MAX)	buf_pre_1st = _1PRE_MAX;
	}

	if(pre_2nd & 0x80)
	{
		buf_pre_2nd = ((-(unsigned short)(pre_2nd)) & 0x00ff);
		if(buf_pre_2nd > _2PRE_MAX)		buf_pre_2nd = (_2PRE_MAX | 0x8000);
		else							buf_pre_2nd |= 0x8000;
	}
	else
	{
		buf_pre_2nd = (unsigned short)(pre_2nd);
		if(buf_pre_2nd > _2PRE_MAX)		buf_pre_2nd = _2PRE_MAX;
	}


//	printf("[PC] pre_emphasis vod   = %d	0x%x\n", buf_vod,buf_vod);
//	printf("[PC] pre_emphasis 1post = %c%d	0x%x\n", (buf_post_1st&0x8000)?'-':'+', (buf_post_1st)&0x7fff, (buf_post_1st)&0x7fff);
//	printf("[PC] pre_emphasis 2post = %c%d	0x%x\n", (buf_post_2nd&0x8000)?'-':'+', (buf_post_2nd)&0x7fff, (buf_post_2nd)&0x7fff);
//	printf("[PC] pre_emphasis 1pre  = %c%d	0x%x\n", (buf_pre_1st&0x8000)?'-':'+', (buf_pre_1st)&0x7fff, (buf_pre_1st)&0x7fff);
//	printf("[PC] pre_emphasis 2pre  = %c%d	0x%x\n", (buf_pre_2nd&0x8000)?'-':'+', (buf_pre_2nd)&0x7fff, (buf_pre_2nd)&0x7fff);

									FPGA_Write(FPGA_XCVR_VOD, buf_vod & 0x001f);
	if (buf_pre_1st & 0x8000)		FPGA_Write(FPGA_XCVR_1PRE_TAP, (buf_pre_1st & 0x001f) | 0x0020 );
	else							FPGA_Write(FPGA_XCVR_1PRE_TAP, (buf_pre_1st & 0x001f));
	if (buf_pre_2nd & 0x8000)		FPGA_Write(FPGA_XCVR_2PRE_TAP, (buf_pre_2nd & 0x0007) | 0x0010 );
	else							FPGA_Write(FPGA_XCVR_2PRE_TAP, (buf_pre_2nd & 0x0007));
	if (buf_post_1st & 0x8000)		FPGA_Write(FPGA_XCVR_1POST_TAP, (buf_post_1st & 0x001f) | 0x0040 );
	else							FPGA_Write(FPGA_XCVR_1POST_TAP, (buf_post_1st & 0x001f));
	if (buf_post_2nd & 0x8000)		FPGA_Write(FPGA_XCVR_2POST_TAP, (buf_post_2nd & 0x000f) | 0x0020 );
	else							FPGA_Write(FPGA_XCVR_2POST_TAP, (buf_post_2nd & 0x000f));

	/*
	_VOD_MAX			= 31,
	_1POST_MAX			= 25,
	_2POST_MAX			= 12,
	_1PRE_MAX			= 16,
	_2PRE_MAX			= 7,
	_PREM_REF_VALUE		= 31
	*/

	return 0;
}
#endif


void color_cursor_from_pc(req_color_cursor_t *pdata)
{
	if(pdata->type==1)
	{
		var_data.cursor_speed	= 1;
		var_data.cursor.type	= pdata->type;
		var_data.cursor.x 		= pdata->x;
		var_data.cursor.y 		= pdata->y;

		var_data.cursor.red		= pdata->red;
		var_data.cursor.green	= pdata->green;
		var_data.cursor.blue	= pdata->blue;

		cursor_func_for_donga(&var_data.cursor);
		set_opmode(CURSOR);
		if(cursor_done == 0)
		{
			cursor_done = 1;
			rcb_write(rcb_fd, RCB_LINE2, TITLE_CURSOR_F);
		}
	}
	else
	{
		var_data.cursor.type	= 0;
		cursor_func_for_donga(&var_data.cursor);
		rcb_write(rcb_fd, RCB_LINE1, model_name);
		rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		set_opmode(MANU_RUN);
		cursor_done = 0;
	}
}


void rcb_storage_info(void)
{
	char disp_str[32];
	int str_cnt=0;
	char buffer[128];
	char perc[128];
	char *ptr;
	uint32_t	stor_prec, stor_used, stor_max;

	memset(buffer, 0, sizeof(buffer));


	memcpy(buffer, read_storage_volume(), sizeof(buffer));

	ptr = strtok(buffer, "%");

	while(ptr != NULL)
	{
		memset(perc, 0, sizeof(perc));
		memcpy(perc, ptr, 128);

		if(str_cnt==0)		stor_prec=(uint32_t)atoi(perc);
		else if(str_cnt==1)	stor_used=(uint32_t)atoi(perc);
		else if(str_cnt==2)	stor_max=(uint32_t)atoi(perc);

//		printf("%s\n", ptr);
		ptr = strtok(NULL, "M");
		str_cnt++;
	}

//	printf("storage using percent:%d\n", stor_prec);
//	printf("storage using volume:%d\n", stor_used);
//	printf("storage   max volume:%d\n", stor_max);

	memset(disp_str, 0, sizeof(disp_str));
	sprintf(disp_str, "%d%% %.1f/%.1fGB", stor_prec, (float)(stor_used/1024.0), (float)(stor_max/1024.0));

	rcb_write(rcb_fd, RCB_LINE1, TITLE_STORAGE_INFO);
	rcb_write(rcb_fd, RCB_LINE2, disp_str);

	set_opmode(STORAGE_INFO);
}

void rcb_storage_info_proc(char *c)
{
	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		rcb_storage_info();
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
		rcb_storage_info();
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
	}
}


void rcb_string_slide(void)
{
	uint64_t elapse_time;

	if(rcb_fd == NULL) return;

	elapse_time 	= get_rcb_slide_elapse_time();

	if(elapse_time>=RCB_STR_SLIDE_TIME)
	{
		char	txbuf[__MAX_BUF_SIZE];
		uint	total_len 	= MAX_LINE_TEXT + 4;
		uint	i;

//		dprintf("loop rcb slide up length, down length = %d, %d\n", rcb_slide_upline_length, rcb_slide_downline_length);

		if(rcb_slide_upline_length>0)
		{
			memset(txbuf, 0, __MAX_BUF_SIZE);
			txbuf[0] 			= STX_RCB;
			txbuf[1] 			= RCB_LINE1;

			if(rcb_slide_upline_cnt<RCB_STR_SLIDE_BLK)				i=0;
			else if(rcb_slide_upline_cnt>rcb_slide_upline_length)	i=rcb_slide_upline_length;
			else													i=rcb_slide_upline_cnt-1;

//			memcpy(txbuf, &rcb_slide_upline_buf[i], MAX_LINE_TEXT); // must be less than MAX_LINE_TEXT
//			dprintf("rcb slide upline count, i = %02d, %02d, %s\n", rcb_slide_upline_cnt, i, txbuf);

			memcpy(txbuf+2, &rcb_slide_upline_buf[i], MAX_LINE_TEXT); // must be less than MAX_LINE_TEXT
			txbuf[total_len-2] 	= ETX_RCB;
			txbuf[total_len-1] 	= '\n';
			write(rcb_fd->fd, txbuf, total_len);

			if(rcb_slide_upline_inc)
			{
				if( rcb_slide_upline_cnt<(rcb_slide_upline_length+RCB_STR_SLIDE_BLK) )	rcb_slide_upline_cnt++;
				else
				{
					rcb_slide_upline_inc=0;
					rcb_slide_upline_cnt--;
				}
			}
			else
			{
				if( rcb_slide_upline_cnt>0 )	rcb_slide_upline_cnt--;
				else
				{
					rcb_slide_upline_inc=1;
					rcb_slide_upline_cnt++;
				}
			}
		}

		if(rcb_slide_downline_length>0)
		{
			memset(txbuf, 0, __MAX_BUF_SIZE);
			txbuf[0] 			= STX_RCB;
			txbuf[1] 			= RCB_LINE2;

			if(rcb_slide_downline_cnt<RCB_STR_SLIDE_BLK)				i=0;
			else if(rcb_slide_downline_cnt>rcb_slide_downline_length)	i=rcb_slide_downline_length;
			else														i=rcb_slide_downline_cnt-1;

//			memcpy(txbuf, &rcb_slide_downline_buf[i], MAX_LINE_TEXT); // must be less than MAX_LINE_TEXT
//			dprintf("rcb slide downline count, i = %02d, %02d, %s\n", rcb_slide_downline_cnt, i, txbuf);

			memcpy(txbuf+2, &rcb_slide_downline_buf[i], MAX_LINE_TEXT); // must be less than MAX_LINE_TEXT
			txbuf[total_len-2] 	= ETX_RCB;
			txbuf[total_len-1] 	= '\n';
			write(rcb_fd->fd, txbuf, total_len);

			if(rcb_slide_downline_inc)
			{
				if( rcb_slide_downline_cnt<(rcb_slide_downline_length+RCB_STR_SLIDE_BLK) )	rcb_slide_downline_cnt++;
				else
				{
					rcb_slide_downline_inc=0;
					rcb_slide_downline_cnt--;
				}
			}
			else
			{
				if( rcb_slide_downline_cnt>0 )	rcb_slide_downline_cnt--;
				else
				{
					rcb_slide_downline_inc=1;
					rcb_slide_downline_cnt++;
				}
			}
		}

		set_rcb_slide_start_time();
	}
}

// ELVDD Alarm
void rcb_vbl_check_warning_proc(char *c)
{
	if(strncmp(c, KEY_ESC, 2)==0)
	{
		rcb_ready_screen(ACK);
		set_opmode(READY);
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		if(get_schedule_flag() == 0){
			rcb_onoff_control(saving_pat_idx_rcb,saving_auto_manu_mode_rcb);
		}
		else{
			rcb_onoff_control(0,0);
		}
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		char vbl_temp[32] = {0, };

		memset(vbl_temp, 0, sizeof(vbl_temp));

		sprintf(vbl_temp, "ELVDD : %d.%d V", model_data.vbl/100, model_data.vbl%100);

		rcb_write(rcb_fd, RCB_LINE1, vbl_temp);
		rcb_write(rcb_fd, RCB_LINE2, "OK ?");

		set_opmode(VBL_CHECK_WARN);
		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
	}
}

void rcb_elvdd_alarm_set_info(void)
{
	char flag_temp[16];

	memset(flag_temp, 0, 16);

	if(elvdd_alarm_flag_global == 1)
	{
		strcpy(flag_temp, "ON      [OK?]");
	}
	else if(elvdd_alarm_flag_global == 0)
	{
		strcpy(flag_temp, "OFF     [OK?]");
	}

	rcb_write(rcb_fd, RCB_LINE1, "ELVDD ALRAM SET");
	rcb_write(rcb_fd, RCB_LINE2, flag_temp);

	elvdd_alarm_flag_buf_rcb = elvdd_alarm_flag_global;

	set_opmode(VBL_CHECK_INFO);
}

void rcb_elvdd_alarm_set_info_proc(char *c)
{
	if(strncmp(c, KEY_ESC, 2)==0)
	{
		if( (model_selection_end==ACK)&&(group_selection_end==ACK) )
		{
			rcb_ready_screen(ACK);
			set_opmode(READY);
		}
		else rcb_menu();
	}
	else if(strncmp(c, KEY_OK, 2)==0)
	{
		set_elvdd_alarm_flag_global(elvdd_alarm_flag_buf_rcb);

		if( (model_selection_end==ACK)&&(group_selection_end==ACK) )
		{
			rcb_ready_screen(ACK);
			set_opmode(READY);
		}
		else rcb_menu();
	}
	else if(strncmp(c, KEY_UP, 2)==0)
	{
		char flag_temp[16];

		elvdd_alarm_flag_buf_rcb--;

		if(elvdd_alarm_flag_buf_rcb < 0)
		{
			elvdd_alarm_flag_buf_rcb = 0;
		}

		memset(flag_temp, 0, 16);

		if(elvdd_alarm_flag_buf_rcb == 1)
		{
			strcpy(flag_temp, "ON      [OK?]");
		}
		else if(elvdd_alarm_flag_buf_rcb == 0)
		{
			strcpy(flag_temp, "OFF     [OK?]");
		}

		rcb_write(rcb_fd, RCB_LINE1, "ELVDD ALRAM SET");
		rcb_write(rcb_fd, RCB_LINE2, flag_temp);
	}
	else if(strncmp(c, KEY_DOWN, 2)==0)
	{
		char flag_temp[16];

		elvdd_alarm_flag_buf_rcb++;

		if(elvdd_alarm_flag_buf_rcb > 1)
		{
			elvdd_alarm_flag_buf_rcb = 1;
		}

		memset(flag_temp, 0, 16);

		if(elvdd_alarm_flag_buf_rcb == 1)
		{
			strcpy(flag_temp, "ON      [OK?]");
		}
		else if(elvdd_alarm_flag_buf_rcb == 0)
		{
			strcpy(flag_temp, "OFF     [OK?]");
		}

		rcb_write(rcb_fd, RCB_LINE1, "ELVDD ALRAM SET");
		rcb_write(rcb_fd, RCB_LINE2, flag_temp);
	}
	else if(strncmp(c, KEY_RCB_INIT, 2)==0)
	{
		char flag_temp[16];

		memset(flag_temp, 0, 16);

		if(elvdd_alarm_flag_buf_rcb == 1)
		{
			strcpy(flag_temp, "ON      [OK?]");
		}
		else if(elvdd_alarm_flag_buf_rcb == 0)
		{
			strcpy(flag_temp, "OFF     [OK?]");
		}

		rcb_write(rcb_fd, RCB_LINE1, "ELVDD ALRAM SET");
		rcb_write(rcb_fd, RCB_LINE2, flag_temp);

		rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
		rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_ONOFF);
	}
}
