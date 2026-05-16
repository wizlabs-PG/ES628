/*
 * model_data.c
 *
 *  Created on: Sep 19, 2017
 *      Author: root
 */

#include <model_data.h>
#include <pwr_control.h>
#include <rcb.h>
#include <fpga_spi.h>
#include <group_data.h>
#include <nvgstplayer.h>
#include <gxttf.h>
#include <util.h>
#include <pattern.h>
#include <pattern_control.h>
#include <i2c_sensing.h>
#include <i2c_dvi.h>
#include <gpio.h>

static char *file_pat_ext[]  	= { EXT_PATTERN, EXT_PATTERN2, EXT_PATTERN3, EXT_PATTERN4, EXT_PATTERN5, EXT_PATTERN6, EXT_PATTERN7, EXT_PATTERN8, NULL };
static char *file_bmp_ext[]  	= { EXT_BMP, EXT_BMP2, EXT_BMP3, EXT_BMP4, EXT_BMP5, EXT_BMP6, EXT_BMP7, EXT_BMP8, NULL };
static char *file_png_ext[]  	= { EXT_PNG, EXT_PNG2, EXT_PNG3, EXT_PNG4, EXT_PNG5, EXT_PNG6, EXT_PNG7, EXT_PNG8, NULL };
static char *file_jpg_ext[]  	= { EXT_JPG, EXT_JPG2, EXT_JPG3, EXT_JPG4, EXT_JPG5, EXT_JPG6, EXT_JPG7, EXT_JPG8, NULL };


int screen_init_check = 0;
int first_screen_init_check = 0;
//DRAW FAILED
void model_var_init(void)
{
	int i;

	dc_buffer 		= NULL;
	dc_buffer_quhd 	= NULL;
	for(i=0; i<MAX_MEMORY_PRELOAD_CNT; i++) dc_buffer_preload[i] = NULL;

	visInfo 		= NULL;
	xdisp 			= NULL;
}

void set_model_init_status(int status)
{
	model_init_status = status;
}

int get_model_init_status(void)
{
	return model_init_status;
}

static void model_default(void)
{
	int i;

	model_init_status				= NACK;
	model_idx 						= 0;

	memset(model_name, 0, MAX_MODEL_NAME);
	strcpy(model_name, "ERROR");

	memset(&model_data, 0, sizeof(model_data_t));

	model_data.ver					= 0;
	model_data.mode					= 0x124;	// Hexa, 10bit, Vesa
	model_data.freq					= 74250000;
	model_data.h_total				= 2200;
	model_data.h_active				= 1920;
	model_data.h_bpo				= 192;
	model_data.h_width				= 60;
	model_data.v_total				= 1125;
	model_data.v_active				= 1080;
	model_data.v_bpo				= 36;
	model_data.v_width				= 5;
	model_data.pol					= 0x06;

	model_data.sync					= 0;
	model_data.if_type				= IF_VBY1;
	model_data.rgb_set				= 0;
	model_data.disp_mode			= 0;

	model_data.vdd					= 1200;
	model_data.vdd_h				= 1300;
	model_data.vdd_l				= 0;
	model_data.idd_h				= 1000;
	model_data.idd_l				= 0;
	model_data.vbl					= 2400;
	model_data.vbl_h				= 2500;
	model_data.vbl_l				= 0;
	model_data.ibl_h				= 1500;
	model_data.ibl_l				= 0;

	model_data.seq					= 4;
	for(i=0; i<MAX_SEQ_CNT; i++)
	{
		model_data.on_seq[i] 		= i;
		model_data.off_seq[i] 		= MAX_SEQ_CNT-i;
		model_data.on_delay[i] 		= 50;
		model_data.off_delay[i] 	= 50;;
	}

	model_data.ins_type				= 0;
	for(i=0; i<MAX_INS_CNT; i++)
	{
		model_data.dim[i]			= 1;
		model_data.vbr[i]			= 33;
		model_data.pwm_freq[i]		= 120;
		model_data.pwm_duty[i]		= 9;
	}

	model_data.use_cycle			= 0;
	for(i=0; i<MAX_CYCLETEST_CNT; i++)
	{
		model_data.cycle_test[i].start_idx 	= 0;
		model_data.cycle_test[i].end_idx 	= 0;
		model_data.cycle_test[i].on_time 	= 0;
		model_data.cycle_test[i].off_time 	= 0;
		model_data.cycle_test[i].cycle_cnt 	= 0;
	}

	model_data.port					= 0;
	model_data.use_dvi				= 0;
}

static void print_model_data(void)
{
	uint64_t freq_high, freq64;
	freq_high 	= model_data.freq_high;
	freq64 		= (freq_high << 32) |  model_data.freq;

	printf("mode		: 0x%X \n", model_data.mode);
	printf("freq		: %ld \n", freq64);
	printf("h_total		: %d \n", model_data.h_total);
	printf("h_active	: %d \n", model_data.h_active);
	printf("h_bpo		: %d \n", model_data.h_bpo);
	printf("h_width		: %d \n", model_data.h_width);
	printf("v_total		: %d \n", model_data.v_total);
	printf("v_active	: %d \n", model_data.v_active);
	printf("v_bpo		: %d \n", model_data.v_bpo);
	printf("v_width		: %d \n", model_data.v_width);
	printf("pol		: %d \n", model_data.pol);
	printf("sync		: %d \n", model_data.sync);
	printf("if_type		: %d \n", model_data.if_type);
	printf("vdd		: %d \n", model_data.vdd);
	printf("idd_h		: %d \n", model_data.idd_h);
	printf("vbl		: %d \n", model_data.vbl);
	printf("ibl_h		: %d \n", model_data.ibl_h);
	printf("PwrSeq		: %d \n", model_data.seq);
}

static int model_list_load(void)
{
	DIR				*dir;
	struct dirent	*dir_entry;
	char			path[MAX_PATH], ext[MAX_EXT];
	int				cnt = 0;

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/", DIR_ROOT, DIR_MODEL);

	if( (dir=opendir(path)) == NULL )
	{
		return 0;
	}

	memset(model_list, 0, MAX_MODEL_CNT*MAX_MODEL_NAME);

	while( (dir_entry = readdir(dir)) )
	{
		if( !strcmp(dir_entry->d_name, ".") ) continue;
		if( !strcmp(dir_entry->d_name, "..") ) continue;
		if( !strchr(dir_entry->d_name, '.') ) continue;

		memset(ext, 0, sizeof(ext));
		strcpy(ext, strrchr(dir_entry->d_name, '.'));
		if( !strcmp(ext, EXT_MODEL) )
		{
			strcpy(model_list[cnt], strtok(dir_entry->d_name, "."));
			printf("model[%d] : %s\n", cnt, model_list[cnt]);

			if(cnt++ > MAX_MODEL_CNT) break;
		}
	}
	printf("\n");

	closedir(dir);

	return cnt;
}

void set_curmodel_idx(char *name)
{
	int i;
	for(i=0; i<MAX_MODEL_CNT; i++)
	{
		if(0==strcmp(model_list[i], name))
		{
			model_idx = i;
			break;
		}
	}
}

static unsigned char edid_checksum(unsigned char* edid)
{
	int 			i;
	unsigned int 	sum = 0;

	for(i=0; i<127; i++){
		sum += edid[i];
		sum &= 0xff;
	}

	return 256 - sum;
}

static int get_bit(int in, int begin, int end)
{
	int mask = (1<< (end-begin +1))-1;
	return (in >> begin) & mask;
}

static void hide_cursor()
{
	Pixmap 	blank;
	XColor 	dummy;
	char 	data[1] = {0};
	Cursor 	cursor;

	blank = XCreateBitmapFromData (xdisp, window, data, 1, 1);
	if(blank == None)
		fprintf(stderr, "error: out of memory.\n");

	cursor = XCreatePixmapCursor(xdisp, blank, blank, &dummy, &dummy, 0, 0);

	XFreePixmap(xdisp, blank);
	XDefineCursor(xdisp, window, cursor);
}

static void initialize_opengl(int screen)
{
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE,8,
		GLX_GREEN_SIZE,8,
		GLX_BLUE_SIZE,8,
		GLX_ALPHA_SIZE,8,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE,24,
		None,
	};

	visInfo = glXChooseVisual(xdisp, 0, attrib);

	if(!visInfo)
	{
		printf("Error: invalid visInfo\n");
		return;
	}

	XSetWindowAttributes wattr;

	wattr.background_pixel 	= 0;
	wattr.border_pixel 		= 0;
	wattr.colormap 			= XCreateColormap(xdisp, RootWindow(xdisp,screen), visInfo->visual, AllocNone);
	wattr.override_redirect = True;
	wattr.event_mask 		= StructureNotifyMask | ExposureMask | KeyPressMask;

	unsigned long mask 		= CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect; //CWColormap | CWEventMask |CWOverrideRedirect;//CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;
	window 					= XCreateWindow(xdisp, RootWindow(xdisp,screen), 0, 0, tegra_time.h_active, tegra_time.v_active, 0, visInfo->depth, InputOutput, visInfo->visual, mask, &wattr);
	glxContext 				= glXCreateContext(xdisp, visInfo, NULL, True);

	glXMakeCurrent(xdisp, window, glxContext);

	//printf("GL_RENDERER: %s\n",(char*)glGetString(GL_RENDERER));
	//printf("GL_VENDOR: %s\n",(char*)glGetString(GL_VENDOR));
	//printf("GL_VERSION: %s\n",(char*)glGetString(GL_VERSION));
}

static void set_old_pattern_list(int reset)
{
	int i;

	old_pattern_cnt = pattern_cnt;

	for(i=0; i<old_pattern_cnt; i++)
	{
		memset(&old_pattern_list[i], 0, MAX_PAT_NAME);
		if(!reset) memcpy(&old_pattern_list[i], &pattern_list[i], MAX_PAT_NAME);
		//printf("old_pattern[%d] : %s\n", i, old_pattern_list[i]);
	}
}
/*
static int comp_pattern_list(void)
{
	int i;

	if(old_pattern_cnt != pattern_cnt)
	{
		return ACK;
	}

	for(i=0; i<old_pattern_cnt; i++)
	{
		//printf("[%d] old : %s, new : %s\n", i, old_pattern_list[i], pattern_list[i]);
		if(strcmp(old_pattern_list[i], pattern_list[i]) != 0)
		{
			break;
		}
	}

	printf("i=%d, old_pattern_cnt=%d\n", i, old_pattern_cnt);
	if(i==old_pattern_cnt)	return NACK;
	else					return ACK;
}
*/

int get_divider(void)
{
	int ret = 1;

	switch(model_data.mode & 0xf)
	{
		case MODE_DUAL:			ret = 2; 	break;
		case MODE_QUAD:			ret = 4; 	break;
		case MODE_OCTA:			ret = 8; 	break;
		case MODE_HEXA:			ret = 16; 	break;
		case MODE_32LANE_8x4:	ret = 32; 	break;
		case MODE_64LANE: 		ret = 64; 	break;
		case MODE_4LANEx4:		ret = 16;	break;
		case MODE_32LANE_16x2:	ret = 32;	break;
		default: 				ret = 1; 	break;
	}

	return ret;
}

void set_portmap(int port)
{
	uint16_t 	port_map[]	= { 0x00E4, 0x008D, 0x00D8, 0x0072, 0x001B, 0x004E, 0x00B1 };
	int 		pt 			= port;

	if(pt<0 || pt>6) pt = 0;

	FPGA_Write(FPGA_PORT_MAP, port_map[pt]);

	printf("FPGA_PORT_MAP\t\t: 0x%X\n", FPGA_Read(FPGA_PORT_MAP));
}

void set_mode_by_twist(enum_twist_t twist)
{
	uint16_t mode = model_data.mode & 0xFCFF;

	if(twist==TWIST_VESA)	mode |= 0x0100;

	printf("MODE : 0x%X \n", mode);

	FPGA_Write(FPGA_MODE, mode);

	printf("FPGA_MODE\t\t: 0x%X\n", FPGA_Read(FPGA_MODE));
}

void set_mode_bit_type_qems(int bittype)
{
	uint16_t mode = model_data.mode&0xf00f;

	//QEMS bit type:normal 8bit(8),normal 10bit(10),jeida 8bit(12),jeida 10bit(14),Vesa 8bit(16),vesa 10bit(18)
	if((bittype==8)||(bittype==12)||(bittype==16))
	{
		mode|=0x10;//8bit
	}
	else
	{
		mode|=0x20;//10bit
	}

	if((bittype==12)||(bittype==14))
	{
		mode|=0x000;//Jeida
	}
	else
	{
		mode|=0x100;//vesa
	}

	printf("MODE : 0x%X \n", mode);

	FPGA_Write(FPGA_MODE, mode);

	printf("FPGA_MODE\t\t: 0x%X\n", FPGA_Read(FPGA_MODE));
}

void set_dvi(void)		// HDMI input in ES626, ES620
{
	printf("Set DVI TP \r\n"); //es620de test
	edid_data_t 	dvi_edid = {0,};
	double 			dvi_freq;

	i2c_gpio_set(GPIO_EX9, 0x01);		// HDMI input HPD disable

	delay_us(1000000);

	i2c_gpio_set(GPIO_EX8, 0x01);		// HDMI input reset disable
	i2c_gpio_set(GPIO_EX9, 0x00);		// HDMI input HPD enable
	i2c_gpio_set(GPIO_EX12, 0x01);		// HDMI input L_MODE3
	i2c_gpio_set(GPIO_EX13, 0x00);		// HDMI input L_MODE2
	i2c_gpio_set(GPIO_EX14, 0x01);		// LVEDGE
	i2c_gpio_set(GPIO_EX18, 0x01);	// vdd off(default)

	i2c_gpio_set(GPIO_EX10, 0x00);	// HDMI input L_MODE0
	i2c_gpio_set(GPIO_EX11, 0x01);	// HDMI input L_MODE1

	i2c_gpio_set(GPIO_EX16, 0x00);
	i2c_gpio_set(GPIO_EX17, 0x00);

	i2c_ddc_sel(2);

	switch(model_data.mode & 0xf)
	{
		case MODE_DUAL:
			dvi_freq = (double)(model_data.freq * 0.000001) / 2;
			break;
		case MODE_QUAD:
			dvi_freq = (double)(model_data.freq * 0.000001) / 4;
			break;
		case MODE_OCTA:
			if(model_data.freq>=150000000)	// if freq is upper than 150MHz, divide by 8 (by Ki seung-en)
				dvi_freq = (double)(model_data.freq * 0.000001) / 8;
			else
				dvi_freq = (double)(model_data.freq * 0.000001) / 4;
			break;
		case MODE_HEXA:
			dvi_freq = (double)(model_data.freq * 0.000001) / 8;
			break;
		default:	// MODE_SINGLE, MODE_DUAL
			dvi_freq = (double)(model_data.freq * 0.000001);
			break;
	}

	memset(&dvi_edid, 0, sizeof(edid_data_t));

	// Header
	dvi_edid.Header[0]							= 0x00;
	dvi_edid.Header[1]							= 0xFF;
	dvi_edid.Header[2]							= 0xFF;
	dvi_edid.Header[3]							= 0xFF;
	dvi_edid.Header[4]							= 0xFF;
	dvi_edid.Header[5]							= 0xFF;
	dvi_edid.Header[6]							= 0xFF;
	dvi_edid.Header[7]							= 0x00;
	// Manufact (GVV)
	dvi_edid.ManufactureName					= 0xD61E;
	// Product
	dvi_edid.ProductCode						= 0x085B;
	// Serial No.
	dvi_edid.SerialNumber						= 0x00001000;
	// Week/Year (2001, 01)
	dvi_edid.WeekOfManufacture 					= 0x0B;
	dvi_edid.YearOfManufacture 					= 0x1B;
	// Version
	dvi_edid.EdidVersion 						= 0x01;
	dvi_edid.EdidRevision						= 0x03;
	// Feature
	dvi_edid.VideoInputDefinition 				= 0x80;
	dvi_edid.MaxHorizontalImageSize 			= 0x33;
	dvi_edid.MaxVerticalImageSize 				= 0x12;
	dvi_edid.DisplayTransferCharacteristic 		= 0x78;
	dvi_edid.FeatureSupport 					= 0xEA;
	// Chromaticity
	dvi_edid.RedGreenLowBits 					= 0x1E;
	dvi_edid.BlueWhiteLowBits 					= 0xC5;
	dvi_edid.RedX 								= 0xAE;
	dvi_edid.RedY 								= 0x4F;
	dvi_edid.GreenX 							= 0x34;
	dvi_edid.GreenY 							= 0xB1;
	dvi_edid.BlueX 								= 0x26;
	dvi_edid.BlueY 								= 0x0E;
	dvi_edid.WhiteX 							= 0x50;
	dvi_edid.WhiteY 							= 0x54;
	// Established Timing
	dvi_edid.EstablishedTimings[0] 				= 0x00;
	dvi_edid.EstablishedTimings[1] 				= 0x00;
	dvi_edid.EstablishedTimings[2] 				= 0x00;
	// Standard Timing
	dvi_edid.StandardTimingIdentification[0] 	= 0x01;
	dvi_edid.StandardTimingIdentification[1] 	= 0x00;
	dvi_edid.StandardTimingIdentification[2] 	= 0x01;
	dvi_edid.StandardTimingIdentification[3] 	= 0x00;
	dvi_edid.StandardTimingIdentification[4] 	= 0x01;
	dvi_edid.StandardTimingIdentification[5] 	= 0x00;
	dvi_edid.StandardTimingIdentification[6] 	= 0x01;
	dvi_edid.StandardTimingIdentification[7] 	= 0x00;
	dvi_edid.StandardTimingIdentification[8] 	= 0x01;
	dvi_edid.StandardTimingIdentification[9] 	= 0x00;
	dvi_edid.StandardTimingIdentification[10] 	= 0x01;
	dvi_edid.StandardTimingIdentification[11] 	= 0x00;
	dvi_edid.StandardTimingIdentification[12] 	= 0x01;
	dvi_edid.StandardTimingIdentification[13] 	= 0x00;
	dvi_edid.StandardTimingIdentification[14] 	= 0x01;
	dvi_edid.StandardTimingIdentification[15] 	= 0x00;

	// Detailed Timing
	dvi_edid.DetailedTimingDescriptions[0] 		= (unsigned char)((int)((dvi_freq*100)+0.5) % 256);
	dvi_edid.DetailedTimingDescriptions[1] 		= (unsigned char)((int)((dvi_freq*100)+0.5) / 256);
	dvi_edid.DetailedTimingDescriptions[2] 		= (unsigned char)(model_data.h_active % 256);
	dvi_edid.DetailedTimingDescriptions[3] 		= (unsigned char)((model_data.h_total - model_data.h_active) % 256);
	dvi_edid.DetailedTimingDescriptions[4] 		= (unsigned char)((model_data.h_active / 256) * 16) + ((model_data.h_total - model_data.h_active) / 256);
	dvi_edid.DetailedTimingDescriptions[5] 		= (unsigned char)(model_data.v_active % 256);
	dvi_edid.DetailedTimingDescriptions[6] 		= (unsigned char)((model_data.v_total - model_data.v_active) % 256);
	dvi_edid.DetailedTimingDescriptions[7] 		= (unsigned char)(((model_data.v_active / 256)*16) + ((model_data.v_total - model_data.v_active) / 256));
	dvi_edid.DetailedTimingDescriptions[8] 		= (unsigned char)((model_data.h_total-(model_data.h_active+model_data.h_bpo+model_data.h_width)) % 256);
	dvi_edid.DetailedTimingDescriptions[9] 		= (unsigned char)(model_data.h_width % 256);
	dvi_edid.DetailedTimingDescriptions[10] 	= (unsigned char)((((model_data.v_total-(model_data.v_active+model_data.v_bpo+model_data.v_width)) % 16)*16) + (model_data.v_width % 16));
	dvi_edid.DetailedTimingDescriptions[11] 	= (unsigned char)((((model_data.h_total-(model_data.h_active+model_data.h_bpo+model_data.h_width))/256)*64) + ((model_data.h_width/256)*16) + (((model_data.v_total-(model_data.v_active+model_data.v_bpo+model_data.v_width))/16)*4) + (model_data.v_width/16));
	dvi_edid.DetailedTimingDescriptions[17] 	= 0x18|((unsigned char)(model_data.pol<<1)&0x06); //model.chk_sync ? 0x18 : 0x00;

	// extention
	dvi_edid.ExtensionFlag						= 0x00;
	// checksum
	dvi_edid.Checksum 							= edid_checksum((unsigned char*)&dvi_edid);

	write_edid_to_eeprom((uint8_t*)&dvi_edid);	// for DVI
	read_edid_to_eeprom();

	// HPD enable
	i2c_gpio_set(GPIO_EX9, 0x00);
}

#define HD_MODEL_TEST
#if defined(HD_MODEL_TEST)
void set_hd_to_uhd(void)
{
	printf("Set HD to UHD TP \r\n"); //es620de test
	if( (model_data.if_type==IF_VBY1) && (model_data.use_dvi) )
	{
		//for ES628 DP input
		uint64_t	vfreq=0, cal=0, fbuf=0;

		vfreq = fpga_time.freq/(fpga_time.h_total*fpga_time.v_total);
		cal = (fpga_time.freq*24)/4;		//(ht*vt*vsync*24bpp/4lane)

		while(1)
		{
			if(cal>4320000000)		// if(cal > (5.4GHz*0.8))	vsync/2
			{
				cal /= 2;
				vfreq /= 2;
			}
			else	break;
		}

		fbuf=(fpga_time.h_total*fpga_time.v_total)*vfreq;

		tegra_time.freq		= (unsigned int)fbuf;
		printf("DP input edid set: freq=%dHz,	Vs=%ld\n", tegra_time.freq, vfreq);

		tegra_time.h_total 	= fpga_time.h_total;
		tegra_time.h_active = fpga_time.h_active;
		tegra_time.h_bpo	= fpga_time.h_bpo;
		tegra_time.h_width	= fpga_time.h_width;
		tegra_time.v_total 	= fpga_time.v_total;
		tegra_time.v_active = fpga_time.v_active;
		tegra_time.v_bpo	= fpga_time.v_bpo;
		tegra_time.v_width	= fpga_time.v_width;
	}
	else
	{
		tegra_time.freq 	= 594000000;
		tegra_time.h_total 	= 4400;
		tegra_time.h_active = 3840;
		tegra_time.h_bpo	= 192;
		tegra_time.h_width	= 128;
		tegra_time.v_total 	= 2250;
		tegra_time.v_active = 2160;
		tegra_time.v_bpo	= 72;
		tegra_time.v_width	= 10;
	}
}
#endif


void set_edid(edid_timing_t *et)
{
	printf("Set EDID TP \r\n"); //es620de test
	FILE			*fp;
	EDID_BLOCK_256 	edid;
	double 			tegra_hz;
//	double			fpga_hz;
	double			tegra_freq, fpga_freq;
	timing_data_t	t_time;
//	fpga_timing_data_t	f_time;

#if defined(HD_MODEL_TEST)
	set_hd_to_uhd();
#endif
	memcpy(&t_time,&tegra_time,sizeof(timing_data_t));
//	memcpy(&f_time,&fpga_time,sizeof(fpga_timing_data_t));

	tegra_hz = ((float)t_time.freq/(t_time.h_total*t_time.v_total));

//	fpga_hz =  ((float)f_time.freq/(f_time.h_total*f_time.v_total));

	printf("tegra_hz : %f, freq : %d, ht : %d, vt : %d\n", tegra_hz, t_time.freq, t_time.h_total, t_time.v_total);
//	printf("fpga_hz : %f, freq : %d, ht : %d, vt : %d\n", fpga_hz, f_time.freq, f_time.h_total, f_time.v_total);

//	if( tegra_hz > 60 )
//	{
//		tegra_freq = (double)(60.0 * t_time.h_total * t_time.v_total) * 0.000001;
//	}
//	else
//	{
//		tegra_freq	= (double)((t_time.freq) * 0.000001);
//	}
	if( (model_data.if_type==IF_VBY1) && (model_data.use_dvi) )
	{
		tegra_freq	= (double)((t_time.freq) * 0.000001);
	}
	else
	{
		if( tegra_hz > 60 )	// if Vfreq is bigger than 60Hz, set 30Hz - 19-07-03 30Hz->60Hz
		{
			tegra_freq = (double)(60.0 * t_time.h_total * t_time.v_total) * 0.000001;
		}
		else
		{
			tegra_freq	= (double)((t_time.freq) * 0.000001);
		}
	}

	memset(&edid, 0, sizeof(edid));

	// Header
	edid.data.Header[0]							= 0x00;
	edid.data.Header[1]							= 0xFF;
	edid.data.Header[2]							= 0xFF;
	edid.data.Header[3]							= 0xFF;
	edid.data.Header[4]							= 0xFF;
	edid.data.Header[5]							= 0xFF;
	edid.data.Header[6]							= 0xFF;
	edid.data.Header[7]							= 0x00;
	// Manufact (GVV)
	edid.data.ManufactureName					= 0x0DC4;
	// Product
	edid.data.ProductCode						= 0x0027;
	// Serial No.
	edid.data.SerialNumber						= 0x00000000;
	// Week/Year (2001, 01)
	edid.data.WeekOfManufacture 				= 0x17;
	edid.data.YearOfManufacture 				= 0x1B;

	// Version
	edid.data.EdidVersion 						= 0x01;
	edid.data.EdidRevision						= 0x04;
	// Feature
	edid.data.VideoInputDefinition 				= 0xA5;	// DP - DO NOT CHANGE!
	edid.data.MaxHorizontalImageSize 			= 0x3C;
	edid.data.MaxVerticalImageSize 				= 0x21;
	edid.data.DisplayTransferCharacteristic 	= 0x78;
	edid.data.FeatureSupport 					= 0x2B;

	// Chromaticity
	edid.data.RedGreenLowBits 					= 0xEE;
	edid.data.BlueWhiteLowBits 					= 0xD1;
	edid.data.RedX 								= 0xA5;
	edid.data.RedY 								= 0x55;
	edid.data.GreenX 							= 0x48;
	edid.data.GreenY 							= 0x9B;
	edid.data.BlueX 							= 0x26;
	edid.data.BlueY 							= 0x12;
	edid.data.WhiteX 							= 0x50;
	edid.data.WhiteY 							= 0x54;
	// Established Timing
	edid.data.EstablishedTimings[0] 			= 0x00;
	edid.data.EstablishedTimings[1] 			= 0x00;
	edid.data.EstablishedTimings[2] 			= 0x00;
	// Standard Timing
	edid.data.StandardTimingIdentification[0] 	= 0x01;
	edid.data.StandardTimingIdentification[1] 	= 0x00;
	edid.data.StandardTimingIdentification[2] 	= 0x01;
	edid.data.StandardTimingIdentification[3] 	= 0x00;
	edid.data.StandardTimingIdentification[4] 	= 0x01;
	edid.data.StandardTimingIdentification[5] 	= 0x00;
	edid.data.StandardTimingIdentification[6] 	= 0x01;
	edid.data.StandardTimingIdentification[7] 	= 0x00;
	edid.data.StandardTimingIdentification[8] 	= 0x01;
	edid.data.StandardTimingIdentification[9] 	= 0x00;
	edid.data.StandardTimingIdentification[10] 	= 0x01;
	edid.data.StandardTimingIdentification[11] 	= 0x00;
	edid.data.StandardTimingIdentification[12] 	= 0x01;
	edid.data.StandardTimingIdentification[13] 	= 0x00;
	edid.data.StandardTimingIdentification[14] 	= 0x01;
	edid.data.StandardTimingIdentification[15] 	= 0x00;

	// Detailed Timing
	edid.data.DetailedTimingDescriptions[0] 	= (unsigned char)((int)((tegra_freq*100)+0.5) % 256);
	edid.data.DetailedTimingDescriptions[1] 	= (unsigned char)((int)((tegra_freq*100)+0.5) / 256);
	edid.data.DetailedTimingDescriptions[2] 	= (unsigned char)(t_time.h_active % 256);
	edid.data.DetailedTimingDescriptions[3] 	= (unsigned char)((t_time.h_total - t_time.h_active) % 256);
	edid.data.DetailedTimingDescriptions[4] 	= (unsigned char)((t_time.h_active / 256) * 16) + ((t_time.h_total - t_time.h_active) / 256);
	edid.data.DetailedTimingDescriptions[5] 	= (unsigned char)(t_time.v_active % 256);
	edid.data.DetailedTimingDescriptions[6] 	= (unsigned char)((t_time.v_total - t_time.v_active) % 256);
	edid.data.DetailedTimingDescriptions[7] 	= (unsigned char)(((t_time.v_active / 256)*16) + ((t_time.v_total - t_time.v_active) / 256));
	edid.data.DetailedTimingDescriptions[8] 	= (unsigned char)((t_time.h_total-(t_time.h_active+t_time.h_bpo+t_time.h_width)) % 256);
	edid.data.DetailedTimingDescriptions[9] 	= (unsigned char)(t_time.h_width % 256);
	edid.data.DetailedTimingDescriptions[10] 	= (unsigned char)((((t_time.v_total - (t_time.v_active+t_time.v_bpo+t_time.v_width)) % 16)*16) + (t_time.v_width % 16));
	edid.data.DetailedTimingDescriptions[11] 	= (unsigned char)((((t_time.h_total-(t_time.h_active+t_time.h_bpo+t_time.h_width))/256)*64) + ((t_time.h_width/256)*16) + (((t_time.v_total - (t_time.v_active+t_time.v_bpo+t_time.v_width))/16)*4) + (t_time.v_width/16));
	//edid.data.DetailedTimingDescriptions[12] 	= 0x58;	//hv image size
	//edid.data.DetailedTimingDescriptions[13] 	= 0x4A;	//hv image size
	//edid.data.DetailedTimingDescriptions[14] 	= 0x21;	//hv image size
	//edid.data.DetailedTimingDescriptions[15] 	= 0x00;	//hv image size
	edid.data.DetailedTimingDescriptions[17] 	= 0x18|((unsigned char)(model_data.pol<<1)&0x06);//model.chk_sync ? 0x18 : 0x00;

	edid.data.ExtensionFlag						= 0x00;
	edid.data.Checksum 							= edid_checksum((unsigned char*)&edid.data);

	// for FPGA
	write_edid((unsigned char*)&edid.data);

	edid.data.ExtensionFlag						= 0x01;
	edid.data.Checksum 							= edid_checksum((unsigned char*)&edid.data);

	// for TEGRA
	fp = fopen("/root/edid.bin","w");
	fwrite(&edid, sizeof(edid), 1, fp);
	fclose(fp);
	sync();

	// for fbset
	et->t 		= ((edid.data.DetailedTimingDescriptions[1]<<8) | edid.data.DetailedTimingDescriptions[0]);
	et->ha 		= ((edid.data.DetailedTimingDescriptions[4] & 0xF0)<<4) + edid.data.DetailedTimingDescriptions[2];
	et->va 		= ((edid.data.DetailedTimingDescriptions[7] & 0xF0)<<4) + edid.data.DetailedTimingDescriptions[5];
	et->hblank 	= ((edid.data.DetailedTimingDescriptions[4] & 0x0F)<<8) + edid.data.DetailedTimingDescriptions[3];
	et->vblank 	= ((edid.data.DetailedTimingDescriptions[7] & 0x0F)<<8) + edid.data.DetailedTimingDescriptions[6];
	et->hfpo 	= edid.data.DetailedTimingDescriptions[8] | get_bit(edid.data.DetailedTimingDescriptions[11],6,7)<<8;
	et->vfpo 	= get_bit(edid.data.DetailedTimingDescriptions[10],4,7) | get_bit(edid.data.DetailedTimingDescriptions[11],2,3)<<4;
	et->hsync 	= edid.data.DetailedTimingDescriptions[9] | get_bit(edid.data.DetailedTimingDescriptions[11],4,5)<<8;
	et->vsync 	= get_bit(edid.data.DetailedTimingDescriptions[10],0,3) | get_bit(edid.data.DetailedTimingDescriptions[11],0,1)<<4;
}


int output_set_edp(void)
{
	printf("Output Set EDP TP \r\n"); //es620de test
	char 			str_cmd[MAX_PATH];
	edid_timing_t 	et = {0,};
	int 			ret = 1;
	uint32_t		fpga_freq;

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..30%");

	i2c_hdmi_hpd_set(low);	// this code must be applied.. if not, bitmap will be displayed gray color

	ERRCHECK(system("echo 4 > /sys/class/graphics/fb0/blank"));

	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0000);

	screen_close();

	FPGA_Write(FPGA_MODE				, model_data.mode);	// mode/bit/type

	FPGA_Write(FPGA_VDD_VALUE			, model_data.vdd);	// 0x2C

	// main clock for variables.. (if Vx1, divided freq * 2)
	if(model_data.if_type == IF_VBY1)
	{
		fpga_freq = (uint32_t)((fpga_time.freq / get_divider()) * 2);
		printf("fpga_freq in=%d\n", fpga_freq);
		cdce913_d_freq_set(fpga_freq);

		FPGA_Write(FPGA_VAR_TCLK_LOW	, (unsigned short)(fpga_freq&0xffff));
		FPGA_Write(FPGA_VAR_TCLK_HIGH	, (unsigned short)((fpga_freq>>16)&0xffff));
	}
	else
	{
		fpga_freq = (uint32_t)(fpga_time.freq / get_divider());
		printf("fpga_freq in=%d\n", fpga_freq);
		cdce913_d_freq_set(fpga_freq);

		FPGA_Write(FPGA_VAR_TCLK_LOW	, (unsigned short)(fpga_freq&0xffff));
		FPGA_Write(FPGA_VAR_TCLK_HIGH	, (unsigned short)((fpga_freq>>16)&0xffff));
	}
	printf("FPGA_VAR_TCLK = %d\n",(((uint32_t)FPGA_Read(FPGA_VAR_TCLK_HIGH))<<16)+((uint32_t)FPGA_Read(FPGA_VAR_TCLK_LOW)));


	set_portmap(model_data.port);

	// for DVI
	FPGA_Write(FPGA_DVI_ENABLE			, model_data.use_dvi);
	FPGA_Write(FPGA_IF_PWR_CTRL			, 0x0007);	// i/f power on
//	if(model_data.use_dvi) set_dvi();
//	else	i2c_gpio_set(GPIO_EX9, 0x01);

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..31%");
	set_edid(&et);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..32%");
	delay_us(500000);

	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0001);
	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..33%");
	delay_us(500000);
	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0002);
	printf("FPGA_DP_RX_CTRL\t\t: %04d\n", FPGA_Read(FPGA_DP_RX_CTRL));
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..34%");
	printf("Wait DP RX HPD Operation...\n");
	printf("3\n");
	sleep(1);
	printf("2\n");
	sleep(1);
	printf("1\n");
	sleep(1);
	printf("Done\n");
	/*/ important! (some delay)
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..33%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..34%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..35%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..36%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..37%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..38%");	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..40%");
	/*/


	sprintf(str_cmd, "xrandr --output DP-0 --primary --mode %dx%d --pos 0x0", et.ha, et.va);
	ERRCHECK(system(str_cmd));
	ERRCHECK(system("echo 0 > /sys/class/graphics/fb0/blank"));
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..35%");

	sprintf(str_cmd, "fbset -fb /dev/fb0 -g %d %d %d %d 32 -t %d %d %d %d %d %d %d", et.ha, et.va, et.ha, et.va, (unsigned int)(100000000.0/et.t), et.hblank-et.hfpo-et.hsync, et.hfpo, et.vblank-et.vfpo-et.vsync, et.vfpo, et.hsync, et.vsync);
	ERRCHECK(system(str_cmd));
	printf("%s\n", str_cmd);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..36%");
	ERRCHECK(system("xrandr"));
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..37%");
	sprintf(str_cmd, "xrandr --output DP-0 --primary --mode %dx%d --pos 0x0", et.ha, et.va);


	if(!model_data.use_dvi)		// Not use DP input
	{
		if(0!=WEXITSTATUS(system(str_cmd)))
		{
			rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..38%");

			//
			ERRCHECK(system("xrandr --output DP-0 --off"));
			sleep(2);
			ERRCHECK(system("xrandr --output DP-0 --auto"));
			//


	//		ERRCHECK(system("xrandr"));
			if(0!=WEXITSTATUS(system(str_cmd)))
			{
				rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..39%");
				fprintf(stderr, "RETRY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				ret = 0;
			}
		}
	}



	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..45%");

	ERRCHECK(system("echo 5 > /sys/class/graphics/fb0/blank"));	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..46%");
	ERRCHECK(system("echo 2 > /sys/class/graphics/fb0/blank"));	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..47%");
	ERRCHECK(system("echo 3 > /sys/class/graphics/fb0/blank"));	delay_us(300000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..48%");
	ERRCHECK(system("echo 0 > /sys/class/graphics/fb0/blank"));

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..50%");

	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0003);
//	screen_init(0);									// for Tegra X1
	screen_init(1);									// for Jetson nano
	FPGA_Write(FPGA_DP_RX_CTRL			, 0x0002);

	printf("fpga_time_H:%d \n", fpga_time.h_active);
	printf("fpga_time_V:%d \n", fpga_time.v_active);
	printf("tegra_time_H:%d \n", tegra_time.h_active);
	printf("tegra_time_V:%d \n", tegra_time.v_active);

	FPGA_Write(FPGA_H_TOTAL				, fpga_time.h_total);
	FPGA_Write(FPGA_H_ACTIVE			, fpga_time.h_active);
	FPGA_Write(FPGA_H_BPORCH			, fpga_time.h_bpo);
	FPGA_Write(FPGA_H_WIDTH				, fpga_time.h_width);
	FPGA_Write(FPGA_V_TOTAL				, fpga_time.v_total);
	FPGA_Write(FPGA_V_ACTIVE			, fpga_time.v_active);
	FPGA_Write(FPGA_V_BPORCH			, fpga_time.v_bpo);
	FPGA_Write(FPGA_V_WIDTH				, fpga_time.v_width);
	FPGA_Write(FPGA_REVERSE				, model_data.pol);				// 2018.01.15 requested by ki seung-en
	FPGA_Write(FPGA_8K_HG2D_ENABLE		, model_data.use_8k_hg2d);		// 2018.05.21 requested by ki seung-en

	return ret;
}

int output_set_hdmi(void)
{
	printf("Output Set HDMI TP \r\n"); //es620de test
	FILE			*fp;
	int 			h_fpo, v_fpo, ret=1;
	//char			buf[MAX_PATH];
	char 			str_cmd[MAX_PATH];
	EDID_BLOCK_256 	edid;
	//uint32_t		fpga_freq;
	double			tegra_freq;

	/*
	unsigned short 	ha			= 0;
	unsigned short 	va			= 0;
	unsigned int 	t			= 0;
	unsigned short 	hblank		= 0;
	unsigned short 	vblank		= 0;
	*/

	i2c_hdmi_hpd_set(low);

	//ERRCHECK(system("echo \"0\" > /sys/kernel/debug/tegradc.1/hotplug"));	// off
//	ERRCHECK(system("xrandr --output DP-0 --off"));
	delay_us(500000);

	FPGA_Write(FPGA_DP_RX_CTRL		, 0x0000);

	FPGA_Write(FPGA_MODE			, model_data.mode);	// mode/bit/type
	FPGA_Write(FPGA_VDD_VALUE		, model_data.vdd);	// 0x2C

/*	// main clock for DP_RX (single freq)
	FPGA_Write(FPGA_DCLK_LOW		, (fpga_time.freq)&0xffff);
	FPGA_Write(FPGA_DCLK_HIGH		, ((fpga_time.freq)>>16)&0xffff);

	// main clock for variables.. (if Vx1, divided freq * 2)
	fpga_freq = (uint32_t)(fpga_time.freq / get_divider());
	FPGA_Write(FPGA_VAR_TCLK_LOW	, fpga_freq&0xffff);
	FPGA_Write(FPGA_VAR_TCLK_HIGH	, (fpga_freq>>16)&0xffff);
*/
	set_portmap(model_data.port);

	rcb_write(rcb_fd, RCB_LINE2, "MODEL INIT...10%");

	// Detailed Timing
	tegra_freq = (double)(tegra_time.freq * 0.000001);
	{
		memset(&edid, 0, sizeof(edid));
		// Header
		edid.data.Header[0]							= 0x00;
		edid.data.Header[1]							= 0xFF;
		edid.data.Header[2]							= 0xFF;
		edid.data.Header[3]							= 0xFF;
		edid.data.Header[4]							= 0xFF;
		edid.data.Header[5]							= 0xFF;
		edid.data.Header[6]							= 0xFF;
		edid.data.Header[7]							= 0x00;
		// Manufact (GVV)
		edid.data.ManufactureName					= 0xD61E;
		// Product
		edid.data.ProductCode						= 0x085B;
		// Serial No.
		edid.data.SerialNumber						= 0x00001000;
		// Week/Year (2001, 01)
		edid.data.WeekOfManufacture 				= 0x0B;
		edid.data.YearOfManufacture 				= 0x1B;

		// Version
		edid.data.EdidVersion 						= 0x01;
		edid.data.EdidRevision						= 0x04;
		// Feature
		edid.data.VideoInputDefinition 				= 0xA2;	// HDMI - DO NOT CHANGE!!!
		edid.data.MaxHorizontalImageSize 			= 0x3C;
		edid.data.MaxVerticalImageSize 				= 0x21;
		edid.data.DisplayTransferCharacteristic 	= 0x78;
		edid.data.FeatureSupport 					= 0xFA;


		// Chromaticity
		edid.data.RedGreenLowBits 					= 0x1E;
		edid.data.BlueWhiteLowBits 					= 0xC5;
		edid.data.RedX 								= 0xAE;
		edid.data.RedY 								= 0x4F;
		edid.data.GreenX 							= 0x34;
		edid.data.GreenY 							= 0xB1;
		edid.data.BlueX 							= 0x26;
		edid.data.BlueY 							= 0x0E;
		edid.data.WhiteX 							= 0x50;
		edid.data.WhiteY 							= 0x54;
		// Established Timing
		edid.data.EstablishedTimings[0] 			= 0x00;
		edid.data.EstablishedTimings[1] 			= 0x00;
		edid.data.EstablishedTimings[2] 			= 0x00;
		// Standard Timing
		edid.data.StandardTimingIdentification[0] 	= 0x01;
		edid.data.StandardTimingIdentification[1] 	= 0x00;
		edid.data.StandardTimingIdentification[2] 	= 0x01;
		edid.data.StandardTimingIdentification[3] 	= 0x00;
		edid.data.StandardTimingIdentification[4] 	= 0x01;
		edid.data.StandardTimingIdentification[5] 	= 0x00;
		edid.data.StandardTimingIdentification[6] 	= 0x01;
		edid.data.StandardTimingIdentification[7] 	= 0x00;
		edid.data.StandardTimingIdentification[8] 	= 0x01;
		edid.data.StandardTimingIdentification[9] 	= 0x00;
		edid.data.StandardTimingIdentification[10] 	= 0x01;
		edid.data.StandardTimingIdentification[11] 	= 0x00;
		edid.data.StandardTimingIdentification[12] 	= 0x01;
		edid.data.StandardTimingIdentification[13] 	= 0x00;
		edid.data.StandardTimingIdentification[14] 	= 0x01;
		edid.data.StandardTimingIdentification[15] 	= 0x00;

		// Detailed Timing
		h_fpo = tegra_time.h_total - (tegra_time.h_active+tegra_time.h_bpo+tegra_time.h_width);
		v_fpo = tegra_time.v_total - (tegra_time.v_active+tegra_time.v_bpo+tegra_time.v_width);

		edid.data.DetailedTimingDescriptions[0] 	= (unsigned char)((int)((tegra_freq*100)+0.5) % 256);
		edid.data.DetailedTimingDescriptions[1] 	= (unsigned char)((int)((tegra_freq*100)+0.5) / 256);
		edid.data.DetailedTimingDescriptions[2] 	= (unsigned char)(tegra_time.h_active % 256);
		edid.data.DetailedTimingDescriptions[3] 	= (unsigned char)((tegra_time.h_total - tegra_time.h_active) % 256);
		edid.data.DetailedTimingDescriptions[4] 	= (unsigned char)((tegra_time.h_active / 256) * 16) + ((tegra_time.h_total - tegra_time.h_active) / 256);
		edid.data.DetailedTimingDescriptions[5] 	= (unsigned char)(tegra_time.v_active % 256);
		edid.data.DetailedTimingDescriptions[6] 	= (unsigned char)((tegra_time.v_total - tegra_time.v_active) % 256);
		edid.data.DetailedTimingDescriptions[7] 	= (unsigned char)(((tegra_time.v_active / 256)*16) + ((tegra_time.v_total - tegra_time.v_active) / 256));
		edid.data.DetailedTimingDescriptions[8] 	= (unsigned char)(h_fpo % 256);
		edid.data.DetailedTimingDescriptions[9] 	= (unsigned char)(tegra_time.h_width % 256);
		edid.data.DetailedTimingDescriptions[10] 	= (unsigned char)(((v_fpo % 16)*16) + (tegra_time.v_width % 16));
		edid.data.DetailedTimingDescriptions[11] 	= (unsigned char)(((h_fpo/256)*64) + ((tegra_time.h_width/256)*16) + ((v_fpo/16)*4) + (tegra_time.v_width/16));
		edid.data.DetailedTimingDescriptions[12] 	= 0x58;	//hv image size
		edid.data.DetailedTimingDescriptions[13] 	= 0x54;	//hv image size
		edid.data.DetailedTimingDescriptions[14] 	= 0x21;	//hv image size
		edid.data.DetailedTimingDescriptions[15] 	= 0x00;	//hv image size
		edid.data.DetailedTimingDescriptions[17] 	= 0x18|((unsigned char)(model_data.pol<<1)&0x06);

		//EXTENSION
		edid.data.ExtensionFlag						= 0x01;
		//CHECKSUM
		edid.data.Checksum 							= edid_checksum((unsigned char*)&edid.data);

		edid.ext[0]									= 0x02;	//Extension tag
		edid.ext[1]									= 0x03;	//version 3
		edid.ext[2]									= 0x1A;	//byte number
		edid.ext[3]									= 0x70;	//Basic Audio,YCbCr 4:4:4,YCbCr 4:2:2
		edid.ext[4]									= 0x41;	//start of data block collection
		if((gp.module_hz>30)&&(tegra_time.h_active>1920))
		{
			if((tegra_time.h_active==4096)&&(tegra_time.v_active==2160))
			{
				//256:135
				if(gp.module_hz>55)	edid.ext[5]		= 102;	//4096x2160 60hz 594Mhz
				else if(gp.module_hz>45) edid.ext[5]= 101;	//4096x2160 50hz 594Mhz
				else if(gp.module_hz>26) edid.ext[5]= 100;	//4096x2160 30hz 297Mhz
				else if(gp.module_hz>24) edid.ext[5]= 99;	//4096x2160 25hz 297Mhz
				else if(gp.module_hz>20) edid.ext[5]= 98;	//4096x2160 24hz 297Mhz
			}
			else if((tegra_time.h_active==3840)&&(tegra_time.v_active==2160))
			{
				//16:9
				if(gp.module_hz>55)		 edid.ext[5]= 97;	//3840x2160 60hz 297Mhz
				else if(gp.module_hz>45) edid.ext[5]= 96;	//3840x2160 50hz 297Mhz
				else if(gp.module_hz>26) edid.ext[5]= 95;	//3840x2160 30hz 297Mhz
				else if(gp.module_hz>24) edid.ext[5]= 94;	//3840x2160 25hz 594Mhz
				else if(gp.module_hz>20) edid.ext[5]= 93;	//3840x2160 24hz 594Mhz
			}
			else if((tegra_time.h_active==2560)&&(tegra_time.v_active==1080))
			{
				//64:27
				if(gp.module_hz>100)	 edid.ext[5]= 92;	//2560x1080 120hz 495Mhz
				else if(gp.module_hz>90) edid.ext[5]= 91;	//2560x1080 100hz 371.25Mhz
				else if(gp.module_hz>55) edid.ext[5]= 90;	//2560x1080 60hz 198Mhz
				else if(gp.module_hz>45) edid.ext[5]= 89;	//2560x1080 50hz 185.625 Mhz
				else if(gp.module_hz>26) edid.ext[5]= 88;	//2560x1080 30hz 118.8Mhz
				else if(gp.module_hz>24) edid.ext[5]= 87;	//2560x1080 25hz 90Mhz
				else if(gp.module_hz>20) edid.ext[5]= 86;	//2560x1080 24hz 99Mhz
			}
			else 						 edid.ext[5]= 97;
		}
		edid.ext[6]									= 0x23;
		edid.ext[7]									= 0x08;
		edid.ext[8]									= 0x07;
		edid.ext[9]									= 0x07;
		edid.ext[10]								= 0x67;
		edid.ext[11]								= 0x03;
		edid.ext[12]								= 0x0C;
		edid.ext[13]								= 0x00;
		edid.ext[14]								= 0x20;
		edid.ext[15]								= 0x00;
		edid.ext[16]								= 0xB8;
		edid.ext[17]								= 0x3C;
		edid.ext[18]								= 0x67;
		edid.ext[19]								= 0xD8;
		edid.ext[20]								= 0x5D;
		edid.ext[21]								= 0xC4;
		edid.ext[22]								= 0x01;
		edid.ext[23]								= 0x78;
		edid.ext[24]								= 0x80;
		edid.ext[25]								= 0x03;

		edid.ext[127]								= edid_checksum((unsigned char*)&edid.ext);
		//edid.ext[127]								= 0xF4;
	}

	fp = fopen("/root/edid.bin","w");
	fwrite(&edid, sizeof(edid), 1, fp);
	fclose(fp);
	sync();

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..20%");
	i2c_hdmi_hpd_set(high);

	screen_close();
	delay_us(500000);
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..30%");

	sprintf(str_cmd, "xrandr --output HDMI-0 --primary --mode %dx%d --pos 0x0", model_data.h_active, model_data.v_active);
	printf("%s\n", str_cmd);
	if(0!=WEXITSTATUS(system(str_cmd)))
	{
		rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..38%");

		/*/
		ERRCHECK(system("xrandr --output HDMI-0 --off"));
		sleep(2);
		ERRCHECK(system("xrandr --output HDMI-0 --auto"));
		/*/
		ERRCHECK(system("xrandr"));

		if(0!=WEXITSTATUS(system(str_cmd)))
		{
			if(0!=WEXITSTATUS(system(str_cmd)))
			{
				fprintf(stderr, "RETRY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				ret = 0;
			}
		}
	}
	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..40%");

//	screen_init(1);
	screen_init(0);

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..50%");

	glClearColor(0,0,0,1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glFinish();
	glXSwapBuffers(xdisp,window);

	glClearColor(0,0,0,1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glFinish();
	glXSwapBuffers(xdisp,window);

//	ERRCHECK(system("xrandr --output DP-0 --off"));

	DEBUG_PRINT("HDMI OUTPUT SET\n");

	return ret;
}

void model_init(void)
{
	FILE 	*fp;
	int		i;
	char	path[MAX_PATH], config_dir[MAX_PATH], local_name[MAX_MODEL_NAME];

	model_cnt = model_list_load();
//	printf("model count: %d\n", model_cnt);
	if(model_cnt==0)
	{
		fprintf(stderr, "model_list_load() error!\n");
		rcb_write(rcb_fd, RCB_LINE2, "NO MODEL FILE!");
		model_default();		//19-07-24 test
		return;
	}

//	model_default();		//19-07-24 test

	memset(path, 0, MAX_PATH);
	memset(local_name, 0, MAX_MODEL_NAME);

	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, CURRENT_MODEL_NAME);
	if( 0 == access(path, F_OK) )
	{
		fp = fopen(path, "r");
		if(NULL==fgets(local_name, MAX_MODEL_NAME-1, fp)){}
		fclose(fp);
	}
	else
	{
		memset(config_dir, 0, MAX_PATH);
		sprintf(config_dir, "%s%s", DIR_ROOT, DIR_CONFIG);
		if( 0 != access(config_dir, F_OK) ) mkdir(config_dir, 0776);	// if config directory does not exist, create directory

		fprintf(stderr, "%s file not exist. creating file...\n", CURRENT_MODEL_NAME);
		strcpy(local_name, model_list[0]);
		fp = fopen(path, "w");
		fputs(local_name, fp);
		fclose(fp);
		sync();
	}

	memset(model_name, 0, MAX_MODEL_NAME);
	sprintf(model_name, "%s", local_name);

	for(i=0; i<model_cnt; i++)
	{
		if(!strcmp(model_name, model_list[i]))
		{
			model_idx = i;
			break;
		}
	}
}

void model_update(void)
{
	model_cnt = model_list_load();
}

int is_5k_lvds_quad(void)
{
	if( (model_data.if_type==IF_LVDS) && ((model_data.mode&0xf)==MODE_QUAD) && (model_data.h_active>3840) )		// jschoi 2018.05.02 (requested by kim tae-hee)
	{
		//printf("5K LVDS QUAD 2DIV ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		return 1;
	}
	else if( (model_data.if_type==IF_DP) && (model_data.h_active>3840) )
	{
		return 1;
	}
	else if( (model_data.if_type==IF_VBY1) && (model_data.h_active>3840) )// 2022.04.01 ksk 5K
	{
//		printf("###################### 5K #############################\n");
		return 1;

	}

	return 0;
}

int is_4k_8k_hdmi_or_not(void)
{
	//0: just 4k or 8k or HDMI
	//1: not

	if(DRAWCHECK(model_data.if_type))
	{
		if ( ((fpga_time.h_active==3840) && (fpga_time.v_active==2160)) ||  ((fpga_time.h_active==7680) && (fpga_time.v_active==4320)))
		{
			return 0;	//4k or 8k
		}
		else
		{
			return 1;	//not
		}
//		else if( ((fpga_time.h_active>3840) || (fpga_time.v_active>2160)) )		//	h_active or v_active over 4k		seki 2020.11.03
//		{
//			if( (fpga_time.h_active<=7680) || (fpga_time.v_active<=4320) )	// less 8k
//			{
//				return 2;
//			}
//		}
//		else if( (fpga_time.h_active<=3840) && (fpga_time.v_active<=2160) )
//		{
//			return 1;
//		}
	}

	return 0;	//HDMI
}

int is_over_4k_less_8k(void)
{
//	if( (DRAWCHECK(model_data.if_type)) && ((fpga_time.h_active>3840) || (fpga_time.v_active>2160)) )		//	h_active or v_active over 4k		segi 2020.11.03
	if( (fpga_time.h_active>3840) || (fpga_time.v_active>2160) )		//	h_active or v_active over 4k		segi 2020.11.03
	{
		if( (fpga_time.h_active<7680) && (fpga_time.v_active<4320) )	// less 8k
		{
			return 1;
		}
	}

	if((fpga_time.h_active == 7680) && (fpga_time.v_active == 2160))	// test ensis_display.
	{
		return 1;
	}

	return 0;
}

int quhd_memory_preload(void)
{
	int ret = ACK;
	unsigned int wait_u=200000;

	if(dc_buffer==NULL || dc_buffer_quhd==NULL)
		return NACK;

	quhd_copy_mode = 1;	// important

	FPGA_Write(FPGA_MEM_RD_CTRL		, 0x0000);	// function reset

	if(is_5k_lvds_quad())
	{
		if(model_data.v_active<2160) // for DQHD
		{
			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active, dc_buffer, 0, 0, fpga_time.h_active/2, fpga_time.v_active);	// 5K | O | X |
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);

			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;


			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active, dc_buffer, fpga_time.h_active/2, 0, fpga_time.h_active/2, fpga_time.v_active);	// 5K | X | O |
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0010);	// bitmap index : 1
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;
		}
		else // for 5K(5120x2880) 2023.06.13 KSK
		{
//			void  gx_bitblt_bmp( dc_t *dc_dest, int dest_x, int dest_y, dc_t *dc_sour, int sour_x, int sour_y, int sour_w, int sour_h)
			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, 0, fpga_time.v_active/2, fpga_time.h_active/2, fpga_time.v_active/2);
//			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, 0, 0, fpga_time.h_active/2, fpga_time.v_active/2);
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;

			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, fpga_time.h_active/2, fpga_time.v_active/2, fpga_time.h_active/2, fpga_time.v_active/2);
//			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, fpga_time.h_active/2, 0, fpga_time.h_active/2, fpga_time.v_active/2);
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0010);	// bitmap index : 1
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;

			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, 0, 0, fpga_time.h_active/2, fpga_time.v_active/2);
//			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, 0, fpga_time.v_active/2, fpga_time.h_active/2, fpga_time.v_active/2);
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0020);	// bitmap index : 2
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;


			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, fpga_time.h_active/2, 0, fpga_time.h_active/2, fpga_time.v_active/2);
//			gx_bitblt_bmp(dc_buffer_quhd, 0, MAX_TEGRA_VACTIVE-fpga_time.v_active/2, dc_buffer, fpga_time.h_active/2, fpga_time.v_active/2, fpga_time.h_active/2, fpga_time.v_active/2);
			glDrawPixels(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

			glFinish();
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index : 3
			glXSwapBuffers(xdisp, window);
			glFinish();
			delay_us(wait_u);
			FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
			ret = fpga_draw_check(DRAW_BMP_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
			if(ret!=ACK) return ret;
		}
	}
	else { // QUHD
		gx_bitblt_bmp(dc_buffer_quhd, 0, 0, dc_buffer, 0, 0, tegra_time.h_active, tegra_time.v_active);
		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

		glFinish();
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0
		glXSwapBuffers(xdisp, window);
		glFinish();
		delay_us(wait_u);
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
		ret = fpga_draw_check(DRAW_BMP_CHECK);
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
		if(ret!=ACK) return ret;

		gx_bitblt_bmp(dc_buffer_quhd, 0, 0, dc_buffer, tegra_time.h_active, 0, tegra_time.h_active, tegra_time.v_active);
		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

		glFinish();
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0010);	// bitmap index : 1
		glXSwapBuffers(xdisp, window);
		glFinish();
		delay_us(wait_u);
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
		ret = fpga_draw_check(DRAW_BMP_CHECK);
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
		if(ret!=ACK) return ret;

		gx_bitblt_bmp(dc_buffer_quhd, 0, 0, dc_buffer, 0, tegra_time.v_active, tegra_time.h_active, tegra_time.v_active);
		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

		glFinish();
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0020);	// bitmap index : 2
		glXSwapBuffers(xdisp, window);
		glFinish();
		delay_us(wait_u);
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
		ret = fpga_draw_check(DRAW_BMP_CHECK);
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
		if(ret!=ACK) return ret;

		gx_bitblt_bmp(dc_buffer_quhd, 0, 0, dc_buffer, tegra_time.h_active, tegra_time.v_active, tegra_time.h_active, tegra_time.v_active);
		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_quhd->mapped);

		glFinish();
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index : 3
		glXSwapBuffers(xdisp, window);
		glFinish();
		delay_us(wait_u);
		FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
		ret = fpga_draw_check(DRAW_BMP_CHECK);
		FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
		if(ret!=ACK) return ret;
	}

	return ret;
}

int get_hando_list(void)
{
	FILE	*fp;
	char 	path[MAX_PATH], origin[MAX_PATH], temp[MAX_PATH], *ptr;
	int		i = 0;

	memset(hando_list, 0, sizeof(hando_list_t)*MAX_HANDO_CNT);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_HANDO, model_list[hando_model_idx], EXT_HANDO);	// .hnd
	fp = fopen(path, "r");
	if(fp == NULL)
	{
		memset(path, 0, MAX_PATH);
		sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_HANDO, model_list[hando_model_idx], EXT_BIG_HANDO);	// .HND
		fp = fopen(path, "r");
		if(fp == NULL)
		{
			fprintf(stderr, "%s file not found\n", path);
			return NACK;
		}
	}

	while(!feof(fp))
	{
		if(i>MAX_HANDO_CNT-1) break;

		memset(origin, 0, MAX_PATH);
		if(0==fgets(origin, MAX_PATH, fp)) {}

		memset(temp, 0, MAX_PATH);
		replace(origin, temp, "\r\n", "\0", 0, 0);

		if(strlen(temp)==0) break;

		ptr = strstr(temp, STR_HANDO0);
		if(ptr==NULL) ptr = strstr(temp, STR_HANDO1);
		if(ptr==NULL) ptr = strstr(temp, STR_HANDO2);

		memset(origin, 0, MAX_PATH);
		sprintf(origin, "%s", ptr);

		memset(hando_list[i].name, 0, MAX_PATH);
		replace(origin, hando_list[i].name, "\\", "/", 0, 0);
		printf("hando_name[%d] : %s\n", i, hando_list[i].name);

		hando_list[i].useage = 1;
		i++;
	}
	fclose(fp);

	return ACK;
}

static void dc_copy(dc_t *src, dc_t *dest)
{
	int 	i=0;
	char 	*pdest, *psrc;

	psrc  = (char*)src->mapped;
	pdest = (char*)dest->mapped + dest->width * (dest->height - src->height);

	for(i=0; i<src->height; i++){
		memcpy(pdest, psrc, src->bytes_per_line);
		pdest += dest->bytes_per_line;
		psrc += src->bytes_per_line;
	}
}

//static int memory_preload(void)
int memory_preload(void)
{
	char 	path[MAX_PATH];
	int		ret				= ACK;
	int 	i				= 0;
	int 	j				= 0;
	int 	k,l				= 0;
	int 	m,n				= 0;
	int 	buf_cnt			= 0;
	int 	progress		= 0;
	int 	progress_start	= 50;

	switch(model_data.if_type)
	{
		case IF_HDMI:
			memset(&preload_data, 0, sizeof(preload_data));
			for(i=0; i<group_data.hdr.count; i++)
			{
				if(buf_cnt<MAX_MEMORY_PRELOAD_CNT)
				{
					if(group_data.pat[i].type == TYPE_USER)
					{
//						sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[i].name, EXT_PATTERN);
						for(k=0;k<8;k++)
						{
							sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[i].name, file_pat_ext[k]);
							if( 0 == access(path, F_OK) ) break;
						}

						preload_data[i].use		= 1;
						preload_data[i].index	= buf_cnt;
						preload_data[i].type	= TYPE_USER;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						ret = pattern_load(dc_buffer_preload[buf_cnt], path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;
					}
					else if(group_data.pat[i].type == TYPE_BMP)
					{
//						sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_BMP);
						for(k=0;k<8;k++)
						{
							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_bmp_ext[k]);
							if( 0 == access(path, F_OK) ) break;
						}

						preload_data[i].use		= 1;
						preload_data[i].index	= buf_cnt;
						preload_data[i].type	= TYPE_BMP;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						ret = pattern_load(dc_buffer_preload[buf_cnt], path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;
					}
					else if(group_data.pat[i].type == TYPE_IMAGE)
					{
//						sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_PNG);
//						if( 0 != access(path, F_OK) ){
//							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_PNG);
//							if( 0 != access(path, F_OK) ){
//								sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_JPG);
//								if( 0 != access(path, F_OK) ){
//									sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_JPG);
//								}
//							}
//						}
						for(k=0;k<8;k++)
						{
							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[k]);
							if( 0 == access(path, F_OK) ) break;
							else
							{
								if(k==7)
								{
									for(l=0;l<8;l++)
									{
										sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[l]);
										if( 0 == access(path, F_OK) ) break;
										else
										{
											if(l==7)
											{
												for(m=0;m<8;m++)
												{
													sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_jpg_ext[m]);
													if( 0 == access(path, F_OK) ) break;
													else
													{
														if(m==7)
														{
															for(n=0;n<8;n++)
															{
																sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_jpg_ext[n]);
																if( 0 == access(path, F_OK) ) break;
																else
																{
																	if(n==7)	sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[0]);
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}

						preload_data[i].use		= 1;
						preload_data[i].index	= buf_cnt;
						preload_data[i].type	= TYPE_IMAGE;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						ret = pattern_load(dc_buffer_preload[buf_cnt], path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;
					}
				}
				progress = (buf_cnt*progress_start)/group_data.hdr.count + progress_start;
				set_progress_count(progress);
				sprintf(path, "PATTERN LOAD.%02d%%", progress);
				rcb_write(rcb_fd, RCB_LINE2, path);
			}
			break;

		default:	// FPGA PRELOADING
			/*
			if( (comp_pattern_list()==NACK) && (model_comp_res==0) )	// if model_data and pattern_data were not changed
			{
				rcb_write(rcb_fd, RCB_LINE2, "SAME PATTERN.50%");
				printf("preload skip\n");
				delay_us(1000000);
				return ACK;
			}
			else
			{
			*/
				rcb_write(rcb_fd, RCB_LINE2, "PATTERN LOAD.50%");
			//}

			memset(&preload_data, 0, sizeof(preload_data));
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag

			FPGA_Write(FPGA_MEM_RD_BANK, FPGA_REALTIME_MEMORY0);


			for(i=0; i<group_data.hdr.count; i++)
			{
				if(buf_cnt<MAX_FPGA_FRELOAD_CNT)
				{
					if(group_data.pat[i].type == TYPE_USER)
					{
#ifdef USE_NEW_WR_BANK
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt+FPGA_MEMORY_BANK_OFFSET);
#else
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt);
#endif

//						sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[i].name, EXT_PATTERN);
						for(k=0;k<8;k++)
						{
							sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[i].name, file_pat_ext[k]);
							if( 0 == access(path, F_OK) ) break;
						}

						preload_data[i].use		= 1;

#ifdef USE_NEW_WR_BANK
						preload_data[i].index	= buf_cnt+FPGA_MEMORY_BANK_OFFSET;
#else
						preload_data[i].index	= buf_cnt;
#endif

						preload_data[i].type	= TYPE_USER;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						printf("tp type user 2 \n");
						ret = pattern_load(dc_buffer, path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;

#ifndef USE_8K_LOGIC_PATTERN
						if(get_quhd_enable())
						{
							ret = quhd_memory_preload();
							if(ret!=ACK) break;
						}
#endif
					}
					else if(group_data.pat[i].type == TYPE_BMP)
					{
#ifdef USE_NEW_WR_BANK
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt+FPGA_MEMORY_BANK_OFFSET);
#else
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt);
#endif

//						sprintf(path, "%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_BMP);
						for(k=0;k<8;k++)
						{
							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_bmp_ext[k]);
							if( 0 == access(path, F_OK) ) break;
						}


						preload_data[i].use		= 1;

#ifdef USE_NEW_WR_BANK
						preload_data[i].index	= buf_cnt+FPGA_MEMORY_BANK_OFFSET;
#else
						preload_data[i].index	= buf_cnt;
#endif

						preload_data[i].type	= TYPE_BMP;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						ret = pattern_load(dc_buffer, path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;

						if(get_quhd_enable())
						{
							ret = quhd_memory_preload();
							if(ret!=ACK) break;
						}
						else
						{
							FPGA_Write(FPGA_MEM_RD_CTRL		, 0x0000);	// function reset
							delay_us(10);
							FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : low
							delay_us(10);

							dc_copy(dc_buffer,dc_output);
//							glDrawPixels(fpga_time.h_active, fpga_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
							glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_output->mapped);
							glFinish();
							glXSwapBuffers(xdisp, window);
							delay_us(30000);
							FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
							ret = fpga_draw_check(DRAW_BMP_CHECK);
							FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
							if(ret!=ACK) break;
						}
					}
					else if(group_data.pat[i].type == TYPE_IMAGE)
					{

#ifdef USE_NEW_WR_BANK
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt+FPGA_MEMORY_BANK_OFFSET);
#else
						FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt);
#endif

//						sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_PNG);
//						if( 0 != access(path, F_OK) ){
//							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_PNG);
//							if( 0 != access(path, F_OK) ){
//								sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_JPG);
//								if( 0 != access(path, F_OK) ){
//									sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[i].name, EXT_JPG);
//								}
//							}
//						}
						for(k=0;k<8;k++)
						{
							sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[k]);
							if( 0 == access(path, F_OK) ) break;
							else
							{
								if(k==7)
								{
									for(l=0;l<8;l++)
									{
										sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[l]);
										if( 0 == access(path, F_OK) ) break;
										else
										{
											if(l==7)
											{
												for(m=0;m<8;m++)
												{
													sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_jpg_ext[m]);
													if( 0 == access(path, F_OK) ) break;
													else
													{
														if(m==7)
														{
															for(n=0;n<8;n++)
															{
																sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_jpg_ext[n]);
																if( 0 == access(path, F_OK) ) break;
																else
																{
																	if(n==7)	sprintf(path,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[i].name, file_png_ext[0]);
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}

						preload_data[i].use		= 1;

#ifdef USE_NEW_WR_BANK
						preload_data[i].index	= buf_cnt+FPGA_MEMORY_BANK_OFFSET;
#else
						preload_data[i].index	= buf_cnt;
#endif

						preload_data[i].type	= TYPE_IMAGE;
						printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
						ret = pattern_load(dc_buffer, path, &preload_data[i].indirect);
						if(ret!=ACK) break;
						buf_cnt++;

						if(get_quhd_enable())
						{
							ret = quhd_memory_preload();
							if(ret!=ACK) break;
						}
						else
						{
							FPGA_Write(FPGA_MEM_RD_CTRL		, 0x0000);	// function reset
							delay_us(10);
							FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : low
							delay_us(10);

							dc_copy(dc_buffer,dc_output);
							//glDrawPixels(fpga_time.h_active, fpga_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
							glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_output->mapped);
							glFinish();
							glXSwapBuffers(xdisp, window);
							delay_us(30000);

							FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
							ret = fpga_draw_check(DRAW_BMP_CHECK);
							FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
							if(ret!=ACK) break;
						}
					}
				}
				progress = (buf_cnt*progress_start)/group_data.hdr.count + progress_start;
				set_progress_count(progress);
				if(progress==100)	sprintf(path, "PATTERN LOAD%02d%%", progress);
				else				sprintf(path, "PATTERN LOAD.%02d%%", progress);
				rcb_write(rcb_fd, RCB_LINE2, path);
			}

			// hando file ---------------------------------------------------------------------------------------------------
			if((ret==ACK) && (use_hando>0))
			{
				if(get_hando_list() == ACK)
				{
					for(j=0;;j++)
					{
						if(buf_cnt<MAX_FPGA_FRELOAD_CNT)
						{
							if(hando_list[j].useage==0) break;

							FPGA_Write(FPGA_MEM_WR_BANK		, buf_cnt);

							sprintf(path, "%s%s/%s", DIR_ROOT, DIR_BMP, hando_list[j].name);
							preload_data[i].use		= 1;
							preload_data[i].index	= buf_cnt;
							preload_data[i].type	= TYPE_BMP;
							printf("preload:%s, buf_cnt=%d\n", path, buf_cnt);
							ret = pattern_load(dc_buffer, path, &preload_data[i].indirect);
							if(ret!=ACK) break;
							buf_cnt++;

							if(get_quhd_enable())
							{
								ret = quhd_memory_preload();
								if(ret!=ACK) break;
							}
							else
							{
								FPGA_Write(FPGA_MEM_RD_CTRL		, 0x0000);	// function reset
								FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : low

								dc_copy(dc_buffer,dc_output);
								//glDrawPixels(fpga_time.h_active, fpga_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
								glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_output->mapped);
								glFinish();
								glXSwapBuffers(xdisp, window);

								FPGA_OR_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : high
								ret = fpga_draw_check(DRAW_BMP_CHECK);
								FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0008);	// bitmap write flag : low
								if(ret!=ACK) break;
							}
							i++;	// important
						}
					}

					pattern_cnt = group_data.hdr.count + j;
					printf("pattern_cnt(+hando) = %d\n", pattern_cnt);
				}
			}
			// hando file ---------------------------------------------------------------------------------------------------

			break;
	}

	if(ret == ACK)
	{
		printf("PATTERN LOAD.99%%\n");
		rcb_write(rcb_fd, RCB_LINE2, "PATTERN LOAD.99%");
		set_old_pattern_list(0);
	}
	else if(ret == NACK)
	{
		printf("DRAW FAILED!\n");
		rcb_write(rcb_fd, RCB_LINE2, "DRAW FAILED!");
		set_old_pattern_list(1);	// reset old pattern
		ret = 8;
	}
	else
	{
		char temp[MAX_PATH], *ptr;

		printf("%s FILE NOT FOUND!\n", path);

		memset(temp, 0, MAX_PATH);
		sprintf(temp, "%s", path);

		ptr = strrchr(temp, '/');
		rcb_write(rcb_fd, RCB_LINE2, (ptr!=NULL) ? (ptr+1) : "FILE NOT FOUND!");

		set_old_pattern_list(1);	// reset old pattern
		ret = 9;
//		ret = 10;
	}

//	shm_send_text(NONE_DATA, NONE_DATA, ret, NONE_DATA);		// send error to shared memory

	return ret;
}

static int fpga_upload(void)
{
	if(model_data.if_type != IF_HDMI)
	{
		rcb_write(rcb_fd, RCB_LINE2, "FPGA LOAD....10%");
		FPGA_Close();	// Important!!
		printf("IF Type: %d\n",model_data.if_type); // To see which type is coming through davinci 240531 KSK
		if(0==FPGA_update(model_data.mode, model_data.if_type))
		{
			FPGA_Open(FPGA_SPI_MODE, FPGA_SPI_BITS, FPGA_SPI_SPEED, FPGA_SPI_DELAY);
			rcb_ready_screen(4);
			return 4;
		}
		FPGA_Open(FPGA_SPI_MODE, FPGA_SPI_BITS, FPGA_SPI_SPEED, FPGA_SPI_DELAY);
		FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0500);	// [11:8] set bitmap capture frame counter
	}

	return ACK;
}

#if defined(DRAW_FAIL_CHECK)
//static int draw_fail_check(void)
int draw_fail_check(void)
{
	int check_time 	= 0;
	int retry_cnt	= 0;

	printf("FPGA_DPRX_LOCK_CHECK start!\n");
	while(1)
	{
		if(0x00 == FPGA_Read(FPGA_DPRX_LOCK_CHECK))
		{
			check_time++;
		}
		else break;

		if(check_time>0x1000)
		{
			if(retry_cnt++>4)
			{
				rcb_ready_screen(8);
				return 8;
				break;
			}
			printf("FPGA_DPRX_LOCK_CHECK failed!!!---------------------------------------\n");
			fpga_upload();
			output_set_edp();
			check_time = 0;
		}
	}
	printf("FPGA_DPRX_LOCK_CHECK end!\n");

	return ACK;
}
#endif

static int model_set(void)
{
	int			ret = ACK;
	//char 		str_cmd[MAX_PATH];
	uint64_t	freq_high, freq64;

	rcb_write(rcb_fd, RCB_LINE2, "SCREEN INIT..30%");

	set_quhd_enable(0);

	player_stop();

	memset(&tegra_time, 0, sizeof(timing_data_t));
//	memset(&fpga_time, 0, sizeof(timing_data_t));
	memset(&fpga_time, 0, sizeof(fpga_timing_data_t));	// 2019-06-13 fpga_time.freq uint64_t test

	memcpy(&tegra_time, &model_data.freq, sizeof(timing_data_t));
//	memcpy(&fpga_time, &tegra_time, sizeof(timing_data_t));
	memcpy(&fpga_time.h_total, &model_data.h_total, sizeof(timing_data_t)-sizeof(uint32_t));	// 2019-06-13 fpga_time.freq uint64_t test


	// 2018.02.14 high frequency(for 8K)
	freq_high 	= model_data.freq_high;
	freq64 		= (freq_high << 32) |  model_data.freq;

	fpga_time.freq = freq64;	// 2019-06-13 fpga_time.freq uint64_t test

	if((model_data.h_active>0)&&(model_data.v_active>0))
	{
		// comment start
		if(model_data.h_active>3840 && model_data.v_active>2160) // 8k?
		{
			tegra_time.freq 	 = (uint32_t)(freq64/4);
			tegra_time.h_active /= 2;
			tegra_time.h_bpo 	/= 2;
			tegra_time.h_width 	/= 2;
			tegra_time.h_total 	/= 2;
			tegra_time.v_active /= 2;
			tegra_time.v_bpo 	/= 2;
			tegra_time.v_width 	/= 2;
			tegra_time.v_total 	/= 2;

			set_quhd_enable(1);
		}
		else if(model_data.h_active>3840 && model_data.v_active<=2160)
		{
			if( is_5k_lvds_quad() )
			{
				tegra_time.freq 	 = (uint32_t)(freq64/TEGRA_5K_DIVIDER);
				tegra_time.h_active /= TEGRA_5K_DIVIDER;
				tegra_time.h_bpo 	/= TEGRA_5K_DIVIDER;
				tegra_time.h_width 	/= TEGRA_5K_DIVIDER;
				tegra_time.h_total 	/= TEGRA_5K_DIVIDER;
			}
			else
			{
				tegra_time.freq 	 = (uint32_t)(freq64/2);
				tegra_time.h_active /= 2;
				tegra_time.h_bpo 	/= 2;
				tegra_time.h_width 	/= 2;
				tegra_time.h_total 	/= 2;
			}

			set_quhd_enable(1);
		}
		else if(model_data.h_active<=3840 && model_data.v_active>2160)
		{
			tegra_time.freq 	 = (uint32_t)(freq64/2);
			tegra_time.v_active /= 2;
			tegra_time.v_bpo 	/= 2;
			tegra_time.v_width 	/= 2;
			tegra_time.v_total 	/= 2;

			set_quhd_enable(1);
		}

		// comment end

		FPGA_Write(FPGA_PARAM_LATCH_EN, 0x0001);

#if defined(HD_MODEL_TEST)
		// 2019.11.27 requested by kim tae-hee
		if(model_data.h_active%16 > 0)
		{
//			tegra_time.h_active = ((model_data.h_active/16)+1)*16;
//			tegra_time.h_total	= model_data.h_total + (tegra_time.h_active-model_data.h_active);
			fpga_time.h_active	= ((model_data.h_active/16)+1)*16;
			fpga_time.h_total	= model_data.h_total + (tegra_time.h_active-model_data.h_active);
		}
#endif


		gp.module_hz = ((float)tegra_time.freq/(tegra_time.h_total*tegra_time.v_total));

		if(model_data.if_type == IF_HDMI)
		{
			if(0==output_set_hdmi())
			{
				if(0==output_set_hdmi())	// if failed, retry
				{
					rcb_ready_screen(7);
					return 7;
				}
			}
		}
		else
		{
			/* 20180829
			 *
			 */
			if(0==output_set_edp())
			{
				if(0==output_set_edp())		// if failed, retry
				{
					rcb_ready_screen(7);
					return 7;
				}
			}

#if defined(DRAW_FAIL_CHECK)
//			draw_fail_check();				// draw fail check
#endif
		}

#ifndef GROUP_SELECT
		set_progress_count(50);
		rcb_write(rcb_fd, RCB_LINE2, "MODEL INIT...50%");
#else
		set_progress_count(49);
		rcb_write(rcb_fd, RCB_LINE2, "MODEL INIT...49%");
#endif

		if(model_data.use_dvi)
		{
			FPGA_Write(FPGA_MEM_WR_BANK, 0);
			FPGA_Write(FPGA_MEM_RD_BANK, 0);
		}
		else
		{
#ifndef GROUP_SELECT
			ret = memory_preload();
			if(ACK != ret)
			{
				rcb_ready_screen(ret);
				return ret;
			}
#endif
		}
	}
	else{
		tegra_time.h_active = 800;
		tegra_time.v_active = 600;
		fpga_time.h_active 	= 800;
		fpga_time.v_active 	= 600;
	}




#if defined(USE_SDC)
	// OCP setting(mili-ampare)

	if(0==i2c_idd_limit_set(model_data.idd_h*10))
	{
		rcb_ready_screen(6);
		return 6;
	}

#endif

	if(ret == ACK)
	{
#ifndef GROUP_SELECT
		set_progress_count(99);
		rcb_write(rcb_fd, RCB_LINE2, "POWER INIT...99%");
#else
		set_progress_count(50);
		rcb_write(rcb_fd, RCB_LINE2, "POWER INIT...50%");
#endif

		adim_change_flag = 0;										// 20-03-30		request by sdc



		model_variable_reset();	//test 20-05-21




		if( RES_ACK != pwr_model_set(&model_data, 0, 0) )
		{
			fprintf(stderr, "pwr_model_set() failed!\n");
			rcb_ready_screen(5);
			return NACK;
		}

		// 2018.05.08 requested by kim tae-hee
		if(model_data.if_type== IF_LVDS)
		{
			if(get_pwr_vendor()==0) pwr_vdd_onoff_set(ENUM_ON);
		}
		else
		{
			if(get_pwr_vendor()==0) pwr_vdd_onoff_set(ENUM_OFF);
		}

		var_spc_en = model_data.use_spc;
		printf("spc model set test : %d\n",model_data.use_spc);
		pwr_spc_onoff_set(var_spc_en);								// 20-04-28		request by sdc		for SPC model


//		rcb_ready_screen(ACK);
#ifndef GROUP_SELECT
		set_opmode(READY);
		set_model_init_status(ret);
		delay_us(1000000);
		rcb_ready_screen(ACK);
#else
		set_model_init_status(ret);
		delay_us(1000000);
#endif
	}
	rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
//	FPGA_pre_emphasis_default();			//191023
	set_pre_emphasis_default();
	return ret;
}

// name : except for extension
int model_select(char* name, int flag)
{
	FILE 	*fp;
	int		i;
	int		ret = ACK;
	char	path[MAX_PATH], local_name[MAX_MODEL_NAME];

	gp.model_change=1;
	scroll_frame_num=1;		//19-08-20 test

	use_hando = flag;

	if (use_hando)
	{
		for(i=0; i<MAX_MODEL_CNT; i++)
		{
			if(0==strcmp(model_list[i], name))
			{
				hando_model_idx = i;
				break;
			}
		}
	}
	
	memset(local_name, 0, MAX_MODEL_NAME);
	sprintf(local_name, "%s", name);

	rcb_write(rcb_fd, RCB_LINE1, local_name);
	rcb_write(rcb_fd, RCB_LINE2, "MODEL INIT....0%");

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_MODEL, local_name, EXT_MODEL);
	fp = fopen(path, "r");
	if(fp)
	{
		// model loading
		memset(&model_data, 0, sizeof(model_data_t));
		if(0==fread(&model_data, sizeof(model_data_t), 1, fp)){}

		// compare old_model_data with model_data
		model_comp_res = memcmp(&old_model_data, &model_data, sizeof(model_data_t));
		printf("model compare result : %d\n", model_comp_res);

		// old model set
		memcpy(&old_model_data, &model_data, sizeof(model_data_t));

#ifndef GROUP_SELECT
		// group loading
		if(0==fread(&group_data.hdr, sizeof(group_head_t), 1, fp)){}

		printf("pattern count = %d\n", group_data.hdr.count);

		pattern_cnt = group_data.hdr.count;
		for(i=0; i<pattern_cnt; i++)
		{
			if(0==fread(&group_data.pat[i], sizeof(pattern_data_t), 1, fp)){}
			memset(&pattern_list[i], 0, MAX_PAT_NAME);
			memcpy(&pattern_list[i], group_data.pat[i].name, MAX_PAT_NAME);
			printf("pattern[%d] : %s\n", i, pattern_list[i]);
		}
#else
//		group_select(group_name);		//19-08-13 group test
#endif

		//20181030
		gp.adim=model_data.vbr[0];
		gp.freq = ((uint64_t)model_data.freq_high << 32);
		gp.freq |=(uint64_t) model_data.freq;
		gp.vsync=(ushort)(freq_to_vsync(gp.freq)*10.0);

		//20181030 Normal 8bit(8),Normal 10bit(10),jeida 8bit(12),jeida 10bit(14),vesa 8bit(16),vesa 10bit(18)
		if((model_data.mode&0x0030)==0x00)// Bit[5:4]:0-6bit,1-8bit,2-10bit
		{
			if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
				gp.bittype=0; //6bit Jeida
			else gp.bittype=0;//6bit vesa

		}
		else if((model_data.mode&0x0030)==0x10)// Bit[5:4]:0-6bit,1-8bit,2-10bit
		{
			if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
				gp.bittype=12; //8bit Jeida
			else gp.bittype=16;//8bit vesa
		}
		else if((model_data.mode&0x0030)==0x20)// Bit[5:4]:0-6bit,1-8bit,2-10bit
		{
			if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
				gp.bittype=14; //10bit Jeida
			else gp.bittype=18;//10bit vesa
		}

		model_data.mode &= 0xbfff;
		model_data.mode |= model_data.Corrention_Type << 14;
		printf("Tcon Type = %dChip\n", model_data.Corrention_Type + 1);

		fclose(fp);
	}
	else
	{
		fprintf(stderr, "[%s] model select failed!\n", path);
		rcb_ready_screen(2);
		return 2;
	}

	// reset curmodel.txt
	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, CURRENT_MODEL_NAME);
	fp = fopen(path, "w");
	if(fp)
	{
		if(strcmp(model_name, local_name))	set_old_pattern_list(1);

		printf("current model : %s\n", local_name);
		memset(model_name, 0, MAX_MODEL_NAME);
		sprintf(model_name, "%s", local_name);
		fputs(model_name, fp);
		fclose(fp);
		sync();
	}
	else
	{
		fprintf(stderr, "[%s] %s failed!\n", path, CURRENT_MODEL_NAME);
		rcb_ready_screen(3);
		return 3;
	}

	// create bitmap directory
	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%d_%d", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	print_model_data();	// test

	ret = fpga_upload();
	if( ACK == ret )
	{
		memset(old_model_name,0,sizeof(old_model_name));
		memcpy(old_model_name, model_name, sizeof(model_name));
		return model_set();
	}
	else
	{
		usleep(2000000);
	}

	return ret;
}

int screen_init(int screen_sel)
{
	int i;

	// comment start
	xdisp 							= XOpenDisplay(NULL);

	if(xdisp==NULL)
	{
		fprintf(stderr, "xdisp == NULL\n");
		return -1;
	}

	int screen 						= DefaultScreen(xdisp);

	initialize_opengl(screen);

	XSetWindowBackgroundPixmap (xdisp, window, None);
	Atom wmDeleteMessage 			= XInternAtom (xdisp, "WM_DELETE_WINDOW", False);
	if (wmDeleteMessage != None)
		XSetWMProtocols (xdisp, window, &wmDeleteMessage, 1);
	XSync (xdisp, 1);        //discard the events for now
	//XSelectInput(xdisp,window,KeyPressMask|ExposureMask);
	XSelectInput(xdisp, window, KeyPressMask|ButtonPressMask|UnmapNotify);

	// fullscreen
	{
		Atom wm_state 				= XInternAtom(xdisp, "_NET_WM_STATE", False);
		Atom fullscreen 			= XInternAtom(xdisp, "_NET_WM_STATE_FULLSCREEN", False);

		XEvent xev;
		memset(&xev, 0, sizeof(xev));
		xev.type 					= ClientMessage;
		xev.xclient.window 			= window;
		xev.xclient.message_type 	= wm_state;
		xev.xclient.format 			= 32;
		xev.xclient.data.l[0] 		= 1;
		xev.xclient.data.l[1] 		= fullscreen;
		xev.xclient.data.l[2] 		= 0;

		XSendEvent(xdisp, DefaultRootWindow(xdisp), False, SubstructureRedirectMask | SubstructureNotifyMask,&xev);
	}

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	//glEnable(GL_TEXTURE_2D);
	glLineWidth(1.0);

	glClear(GL_COLOR_BUFFER_BIT);
	XMapWindow(xdisp, window);
	hide_cursor();
	XFlush(xdisp);

	glRasterPos2f(-1.f,-1.f);
	// comment end

	printf("player_init	width=%d,	height=%d\n", fpga_time.h_active, fpga_time.v_active);
	if(first_screen_init_check==0)
	{
		player_init(screen_sel,fpga_time.h_active,fpga_time.v_active); // screen_sel 0:DP-0, 1:HDMI-0
		first_screen_init_check =1;
	}


	dc_output = gx_get_buffer_dc(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE);
	dc_output->output_display = model_data.if_type;
	// for 8K/5K
	if(get_quhd_enable())
	{
		//20180424 seochihong 5k mode 3840x2160 fix
		if(is_5k_lvds_quad())
		{
			gx_init(model_data.h_active, model_data.v_active, 24);
			dc_buffer = gx_get_buffer_dc(model_data.h_active, model_data.v_active);
			dc_buffer->output_display = model_data.if_type;

			dc_buffer_quhd = gx_get_buffer_dc(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE);
			dc_buffer_quhd->output_display = model_data.if_type;
		}
		else
		{
			gx_init(model_data.h_active, model_data.v_active, 24);
			dc_buffer = gx_get_buffer_dc(model_data.h_active, model_data.v_active);
			dc_buffer->output_display = model_data.if_type;

			dc_buffer_quhd = gx_get_buffer_dc(tegra_time.h_active, tegra_time.v_active);
			dc_buffer_quhd->output_display = model_data.if_type;
		}
	}
	else
	{
		/*
		if( (model_data.mode&0xf) == MODE_SINGLE )
		{
			gx_init(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE, 24);
			dc_buffer = gx_get_buffer_dc(MAX_TEGRA_HACTIVE, MAX_TEGRA_VACTIVE);
			dc_buffer->output_display = model_data.if_type;
		}
		else*/
		{
			gx_init(tegra_time.h_active, tegra_time.v_active, 24);
			dc_buffer = gx_get_buffer_dc(tegra_time.h_active, tegra_time.v_active);
			dc_buffer->output_display = model_data.if_type;
		}
	}

	if(model_data.if_type==IF_HDMI)
	{
		for(i=0; i<MAX_MEMORY_PRELOAD_CNT; i++)
		{
			dc_buffer_preload[i] = gx_get_buffer_dc(tegra_time.h_active, tegra_time.v_active);
			dc_buffer_preload[i]->output_display = model_data.if_type;
		}
	}

	dc_buffer->font_color	= gx_color(0, 0, 0, 255);
	glRasterPos2f(-1.f, -1.f);

	return 1;
}

int screen_close()
{
	int i=0;

	if(screen_init_check != 0)
	{
		//font_dispos();

		for(i=0;i<MAX_MEMORY_PRELOAD_CNT;i++)
		{
			if(dc_buffer_preload[i]!=NULL)
			{
				gx_release_dc(dc_buffer_preload[i]);
				dc_buffer_preload[i] = NULL;
			}
		}

		//if(dc_ai1_buffer!=NULL) 	gx_release_dc(dc_ai1_buffer);
		//if(dc_ai2_buffer!=NULL) 	gx_release_dc(dc_ai2_buffer);
		if(dc_output!=NULL)
		{
			gx_release_dc(dc_output);
			dc_output = NULL;
		}
		if(dc_buffer!=NULL)
		{
			gx_release_dc(dc_buffer);
			dc_buffer = NULL;
		}

		if(dc_buffer_quhd!=NULL)
		{
			gx_release_dc(dc_buffer_quhd);
			dc_buffer_quhd = NULL;
		}

		if(xdisp != NULL)
		{
			glXMakeCurrent(xdisp, None, NULL);
			glXDestroyContext(xdisp, glxContext);
			XDestroyWindow(xdisp, window);
			XCloseDisplay(xdisp);

			//player_destroy(1);
			xdisp = NULL;
		}
	}

	screen_init_check = 1;

	return 1;
}

void ext_cnt_vdd_sel(uint16_t sel)
{

//	if( sel==0 ){
//		i2c_gpio_set(GPIO_EX32, 0x01);
//		i2c_gpio_set(GPIO_EX33, 0x01);
//		printf("ext i2c vdd sel0 = open\n");
//	}
	if( sel==0x01 ){
		i2c_gpio_set(GPIO_EX32, 0x01);
		i2c_gpio_set(GPIO_EX33, 0x00);
//		printf("ext i2c vdd sel1 = 3.3V\n");
	}
	else if( sel==0x02 ){
		i2c_gpio_set(GPIO_EX32, 0x00);
		i2c_gpio_set(GPIO_EX33, 0x01);
//		printf("ext i2c vdd sel2 = 5V\n");
	}
	else if( sel==0x00 ){
		i2c_gpio_set(GPIO_EX32, 0x00);
		i2c_gpio_set(GPIO_EX33, 0x00);
//		printf("ext i2c vdd sel0 = gnd\n");
	}
	else
	{

	}
}

//void dp_port_sel(int flag)
//{
//	model_data.DP_Port_sel=0;
//	onoff_flag = flag;
//
//	if(onoff_flag==ENUM_ON)
//	{
//		FPGA_Write(FPGA_MODE,0x1<<10);
//	}
//	else FPGA_Write(FPGA_MODE,0x0<<10);
//
//	FPGA_Write(FPGA_DP_HZ,((model_data.freq/model_data.h_total)/model_data.v_total));
//	printf("FPGA_DP_HZ = %d\n",(model_data.freq/model_data.h_total)/model_data.v_total);
//}
