/*
 * pattern_control.c
 *
 *  Created on: Nov 7, 2017
 *      Author: root
 */

#include <pattern_control.h>
#include <model_data.h>
#include <rcb.h>
#include <pattern.h>
#include <nvgstplayer.h>
#include <fpga_spi.h>
#include <i2c_sii9135.h>

#define COL(x) ((float)(1.0f/255))*(x)

static char *file_pat_ext[]  	= { EXT_PATTERN, EXT_PATTERN2, EXT_PATTERN3, EXT_PATTERN4, EXT_PATTERN5, EXT_PATTERN6, EXT_PATTERN7, EXT_PATTERN8, NULL };
static char *file_bmp_ext[]  	= { EXT_BMP, EXT_BMP2, EXT_BMP3, EXT_BMP4, EXT_BMP5, EXT_BMP6, EXT_BMP7, EXT_BMP8, NULL };
static char *file_png_ext[]  	= { EXT_PNG, EXT_PNG2, EXT_PNG3, EXT_PNG4, EXT_PNG5, EXT_PNG6, EXT_PNG7, EXT_PNG8, NULL };
static char *file_jpg_ext[]  	= { EXT_JPG, EXT_JPG2, EXT_JPG3, EXT_JPG4, EXT_JPG5, EXT_JPG6, EXT_JPG7, EXT_JPG8, NULL };

static void convertDeviceXYOpenGLXY(int x, int y, float* ox, float* oy)
/* x,y : ?�도??마우??좌표 변??, ox, oy : Open GL 좌표�?변?????�?�할 변??*/
{
	*ox =  (float)((double)x - ((double)model_data.h_active / 2.0))*(double)(1.0 / (double)(model_data.h_active / 2.0));
	*oy = -(float)((double)y - ((double)model_data.v_active / 2.0))*(double)(1.0 / (double)(model_data.v_active / 2.0));
	if(*ox<=-1.0) *ox=-0.999998;
	//printf("x=%.6f, y=%.6f\n", *ox, *oy);
}

void vdd_onoff_control(enum_onoff_t onoff)
{
	if(onoff == ENUM_ON) //2022.04.06 ksk 620QP pwr seq
	{
		if(model_data.if_type == IF_HDMI)
		{
			ERRCHECK(system("echo 4 > /sys/class/graphics/fb1/blank"));
			ERRCHECK(system("echo 0 > /sys/class/graphics/fb1/blank"));
		}

		i2c_gpio_set(GPIO_EX18, 0x00);

		if( get_schedule_flag() == 0 ) set_aging_start_time();		// aging start time
		set_sensing_start_time();	// sensing start time
		set_schedule_start_time();	// onoff start time
//		reset_schedule_func();		// comment 19-07-29
		set_detect_start_time();
	}
	else
	{
		player_stop();
		if(model_data.if_type == IF_HDMI)
		{
			glClearColor(0,0,0,1.0f);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glFinish();
			glXSwapBuffers(xdisp,window);
			glClearColor(0,0,0,1.0f);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glFinish();
			glXSwapBuffers(xdisp,window);
		}

		i2c_gpio_set(GPIO_EX18, 0x01);
	}
}
#if 0
static void if_onoff_control(enum_onoff_t onoff)
{
	if(model_data.if_type != IF_HDMI)
	{
		FPGA_Write(FPGA_IF_PWR_CTRL, (onoff==ENUM_ON) ? 0x0007 : 0x0000);
		printf("FPGA_IF_PWR_CTRL\t\t: %04d\n", FPGA_Read(FPGA_IF_PWR_CTRL));
	}
}
#endif
void onoff_by_power_seq(enum_onoff_t onoff)
{
	int i,j=0, seq_cnt=0;

	if(onoff==ENUM_ON) //2022.04.06 ksk EP620QP pwr seq
	{
		if(model_data.use_dvi) {
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0001);
		}

		for(i=0; i<MAX_SEQ_CNT; i++)
		{
			switch(model_data.on_seq[i])
			{
			case 0:	// vdd
				pwr_vdd_onoff_set(onoff);		//19-07-31 test
				vdd_onoff_control(onoff);		// pg vdd out onoff
				break;
			case 1:	// signal
				set_pattern_scroll(0,1,0);//test		//reset scroll
				FPGA_Write(FPGA_VX1_SIG_ONOFF, onoff);	// 2020.04.17
				pattern_change(get_pattern_index());
				break;
			case 2:	// vbl
				printf("VBL timeout = %d\n",model_data.VBL_EN_timeout);
//				if(model_data.VBL_EN_timeout>0){//2023.03.20 vbl en timeout added
					pwr_vbl_enable_timeout();
//				}
				pwr_vbl_onoff_set(onoff);	//19-07-31 test
				break;
			case 3:	// bl-on
				pwr_bl_onoff_set(onoff);	//19-07-31 test
				break;
			}
			usleep(model_data.on_delay[i]*1000);
		}
	}
	else
	{

		for(i=0; i<MAX_SEQ_CNT; i++)
		{
			switch(model_data.off_seq[i])
			{
			case 0:	// vdd
				vdd_onoff_control(onoff); // 2018.05.08 requested by kim tae-hee
				pwr_vdd_onoff_set(onoff);		//19-07-31 test
				break;
			case 1:	// signal
				FPGA_Write(FPGA_VX1_SIG_ONOFF, onoff);	// 2018.02.21 requested by kim tae-hee
				if(get_schedule_flag()==0) set_pattern_index(0);
				set_pattern_scroll(0,1,0);//test		//reset scroll
//				if( (fpga_time.h_active<3840) && (fpga_time.v_active<2160) )
//				{
//					ERRCHECK(system("killall gst-launch-1.0"));
//					FPGA_Write(FPGA_MOV_POS_Y, 0);
//				}
				break;
			case 2:	// vbl
				pwr_vbl_onoff_set(onoff);	//19-07-31 test
				break;
			case 3:	// bl-on
				pwr_bl_onoff_set(onoff);	//19-07-31 test
				break;
			}
			usleep(model_data.off_delay[i]*1000);
		}
	}
}

int gray_change_func(uint16_t r, uint16_t g, uint16_t b)
{
	if(DRAWCHECK(model_data.if_type))
	{
		FPGA_Write(FPGA_PAL0_R, r);
		FPGA_Write(FPGA_PAL0_G, g);
		FPGA_Write(FPGA_PAL0_B, b);
		printf("gray change = R:%d\tG:%d\tB:%d\tEN:0x%04X\n",FPGA_Read(FPGA_PAL0_R),FPGA_Read(FPGA_PAL0_G),FPGA_Read(FPGA_PAL0_B),FPGA_Read(FPGA_MEM_RD_CTRL));
	}
	else
	{
		glClearColor(COL(r>>2), COL(g>>2), COL(b>>2), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glXSwapBuffers(xdisp, window);
	}

	return ACK;
}

int indirect_gray_change_func(uint16_t pal, uint16_t r, uint16_t g, uint16_t b)
{
	if(DRAWCHECK(model_data.if_type))
	{
		if(pal==0)
		{
			FPGA_Write(FPGA_PAL0_R, r);
			FPGA_Write(FPGA_PAL0_G, g);
			FPGA_Write(FPGA_PAL0_B, b);
			printf("indirect gray change = Pal:%d\tR:%d\tG:%d\tB:%d\n",pal,FPGA_Read(FPGA_PAL0_R),FPGA_Read(FPGA_PAL0_G),FPGA_Read(FPGA_PAL0_B));
		}
		else if(pal==1)
		{
			FPGA_Write(FPGA_PAL1_R, r);
			FPGA_Write(FPGA_PAL1_G, g);
			FPGA_Write(FPGA_PAL1_B, b);
			printf("indirect gray change = Pal:%d\tR:%d\tG:%d\tB:%d\n",pal,FPGA_Read(FPGA_PAL1_R),FPGA_Read(FPGA_PAL1_G),FPGA_Read(FPGA_PAL1_B));
		}
		else if(pal==2)
		{
			FPGA_Write(FPGA_PAL2_R, r);
			FPGA_Write(FPGA_PAL2_G, g);
			FPGA_Write(FPGA_PAL2_B, b);
			printf("indirect gray change = Pal:%d\tR:%d\tG:%d\tB:%d\n",pal,FPGA_Read(FPGA_PAL2_R),FPGA_Read(FPGA_PAL2_G),FPGA_Read(FPGA_PAL2_B));
		}
		else
		{
		}
	}
	else
	{
	}

	return ACK;
}

int gray_level_change_func(void)
{
	if(get_pattern_index()<0 || get_pattern_index()>MAX_PAT_CNT)
	{
		fprintf(stderr, "pattern index range over!\n");
		return NACK;
	}

	if(group_data.pat[get_pattern_index()].type != TYPE_USER)
	{
		fprintf(stderr, "this is not user pattern!\n");
		return NACK;
	}

	char			path[MAX_PATH];
	pattern_head_t	ph = {0,};

	if(DRAWCHECK(model_data.if_type))
	{
		unsigned short cur_bank = FPGA_Read(FPGA_MEM_RD_BANK);

		FPGA_Write(FPGA_MEM_RD_CTRL	, 0x0000); // function reset

		FPGA_Write(FPGA_MEM_WR_BANK	, (cur_bank == FPGA_REALTIME_MEMORY0) ? (uint16_t)FPGA_REALTIME_MEMORY1 : (uint16_t)FPGA_REALTIME_MEMORY0);

		memset(path, 0, MAX_PATH);
		sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[get_pattern_index()].name, EXT_PATTERN);
		pattern_load(dc_buffer, path, &ph);

		FPGA_Write(FPGA_MEM_WR_BANK	, (cur_bank == FPGA_REALTIME_MEMORY0) ? (uint16_t)FPGA_REALTIME_MEMORY0 : (uint16_t)FPGA_REALTIME_MEMORY1);
		FPGA_Write(FPGA_MEM_RD_BANK	, (cur_bank == FPGA_REALTIME_MEMORY0) ? (uint16_t)FPGA_REALTIME_MEMORY1 : (uint16_t)FPGA_REALTIME_MEMORY0);
	}
	else
	{
		memset(path, 0, MAX_PATH);
		sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[get_pattern_index()].name, EXT_PATTERN);
		pattern_load(dc_buffer, path, &ph);

		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
		glFinish();
		glXSwapBuffers(xdisp,window);
	}

	return ACK;
}

// jschoi
static void set_cursor_by_disp_mode(pos_data_t *p)
{
	pos_t pos_limit;

	switch(model_data.rgb_set)
	{
		case RGB_HORZ:
		case BGR_HORZ:
			pos_limit.x = (model_data.h_active*3)-1;
			pos_limit.y = model_data.v_active-1;
			break;
		case RGB_VERT:
		case BGR_VERT:
			pos_limit.x = model_data.h_active-1;
			pos_limit.y = (model_data.v_active*3)-1;
			break;
		default:
			pos_limit.x = model_data.h_active-1;
			pos_limit.y = model_data.v_active-1;
			break;
	}

	switch(model_data.disp_mode)
	{
		case FLIP_HV:
			p->x = pos_limit.x - p->x;
			p->y = pos_limit.y - p->y;
			break;
		case FLIP_V:
			p->y = pos_limit.y - p->y;
			break;
		case FLIP_H:
			p->x = pos_limit.x - p->x;
			break;
		default:
			break;
	}
}

static void set_cursor_by_disp_mode_for_donga(pos_data_t *p)
{
	pos_t pos_limit;

	pos_limit.x = model_data.h_active-1;
	pos_limit.y = model_data.v_active-1;

	switch(model_data.disp_mode)
	{
		case FLIP_HV:
			p->x = pos_limit.x - p->x;
			p->y = pos_limit.y - p->y;
			break;
		case FLIP_V:
			p->y = pos_limit.y - p->y;
			break;
		case FLIP_H:
			p->x = pos_limit.x - p->x;
			break;
		default:
			break;
	}
}

int cursor_func(pos_data_t *pt)
{
	float 		ox, oy;
	pos_data_t 	pos = {0,};

	memset(&pos, 0, sizeof(pos_data_t));
	memcpy(&pos, pt, sizeof(pos_data_t));

	set_cursor_by_disp_mode(&pos);

	if(DRAWCHECK(model_data.if_type))
	{
		if(pos.type == 0)	// cursor disable
		{
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0004);
		}
		else				// cursor enable
		{
			// cursor color
			switch(model_data.rgb_set)
			{
				case RGB_HORZ:
				case BGR_HORZ:
//					FPGA_Write(FPGA_POSION_R, (pos.x%3==0) ? 0x3FF : 0);
//					FPGA_Write(FPGA_POSION_G, (pos.x%3==1) ? 0x3FF : 0);
//					FPGA_Write(FPGA_POSION_B, (pos.x%3==2) ? 0x3FF : 0);
					pos.x /= 3;
					break;
				case RGB_VERT:
				case BGR_VERT:
//					FPGA_Write(FPGA_POSION_R, (pos.y%3==0) ? 0x3FF : 0);
//					FPGA_Write(FPGA_POSION_G, (pos.y%3==1) ? 0x3FF : 0);
//					FPGA_Write(FPGA_POSION_B, (pos.y%3==2) ? 0x3FF : 0);
					pos.y /= 3;
					break;
				default:
//					FPGA_Write(FPGA_POSION_R, 0x3FF);
//					FPGA_Write(FPGA_POSION_G, 0x3FF);
//					FPGA_Write(FPGA_POSION_B, 0x3FF);
					break;
			}

			FPGA_Write(FPGA_POSION_R, (pos.red)>>2);
			FPGA_Write(FPGA_POSION_G, (pos.green)>>2);
			FPGA_Write(FPGA_POSION_B, (pos.blue)>>2);
/*
			FPGA_Write(FPGA_POSION_R, (pos.red)); // 2022.04.06 ksk 11bit 2
			FPGA_Write(FPGA_POSION_G, (pos.green));
			FPGA_Write(FPGA_POSION_B, (pos.blue));
*/
			// cursor position
			FPGA_Write(FPGA_X_POSITION, pos.x);
			FPGA_Write(FPGA_Y_POSITION, pos.y);

			// cursor enable
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0004);
		}
	}
	else
	{
		if(preload_data[pattern_index].use>0)
			glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_preload[preload_data[pattern_index].index]->mapped);
		else
			glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);

		if(pos.type>0)
		{
			// jschoi
			switch(model_data.rgb_set)
			{
				case RGB_HORZ:
					pos.x += 3;	// x offset(1-base sub-pixel)
					convertDeviceXYOpenGLXY(pos.x/3, pos.y, &ox, &oy);
//					switch(pos.x % 3)
//					{
//						case 0: glColor3f(COL(255), COL(0)  , COL(0)  ); break;
//						case 1: glColor3f(COL(0)  , COL(255), COL(0)  ); break;
//						case 2: glColor3f(COL(0)  , COL(0)  , COL(255)); break;
//					}
					glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));
					break;
				case BGR_HORZ:
					pos.x += 3;	// x offset(1-base sub-pixel)
					convertDeviceXYOpenGLXY(pos.x/3, pos.y, &ox, &oy);
//					switch(pos.x % 3)
//					{
//						case 2: glColor3f(COL(255), COL(0)  , COL(0)  ); break;
//						case 1: glColor3f(COL(0)  , COL(255), COL(0)  ); break;
//						case 0: glColor3f(COL(0)  , COL(0)  , COL(255)); break;
//					}
					glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));
					break;
				case RGB_VERT:
					pos.y += 3;	// y offset(1-base sub-pixel)
					convertDeviceXYOpenGLXY(pos.x, pos.y/3, &ox, &oy);
//					switch(pos.y % 3)
//					{
//						case 0: glColor3f(COL(255), COL(0)  , COL(0)  ); break;
//						case 1: glColor3f(COL(0)  , COL(255), COL(0)  ); break;
//						case 2: glColor3f(COL(0)  , COL(0)  , COL(255)); break;
//					}
					glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));
					break;
				case BGR_VERT:
					pos.y += 3;	// y offset(1-base sub-pixel)
					convertDeviceXYOpenGLXY(pos.x, pos.y/3, &ox, &oy);
//					switch(pos.y % 3)
//					{
//						case 2: glColor3f(COL(255), COL(0)  , COL(0)  ); break;
//						case 1: glColor3f(COL(0)  , COL(255), COL(0)  ); break;
//						case 0: glColor3f(COL(0)  , COL(0)  , COL(255)); break;
//					}
					glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));
					break;
				default:
					convertDeviceXYOpenGLXY(pos.x, pos.y, &ox, &oy);
//					glColor3f(COL(255), COL(255), COL(255));
					glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));
					break;
			}

			glBegin(GL_LINES);
			glVertex2f(ox, 1);
			glVertex2f(ox,-1);
			glVertex2f( 1,oy);
			glVertex2f(-1,oy);
			glEnd();
		}
		glFinish();
		glXSwapBuffers(xdisp, window);
	}
	return ACK;
}

int cursor_func_for_donga(pos_data_t *pt)
{
	float 		ox, oy;
	pos_data_t 	pos = {0,};

	memset(&pos, 0, sizeof(pos_data_t));
	memcpy(&pos, pt, sizeof(pos_data_t));

	set_cursor_by_disp_mode_for_donga(&pos);

	if(DRAWCHECK(model_data.if_type))
	{
		if(pos.type == 0)	// cursor disable
		{
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0004);
		}
		else				// cursor enable
		{
			// cursor color
			FPGA_Write(FPGA_POSION_R, (pos.red)>>2);
			FPGA_Write(FPGA_POSION_G, (pos.green)>>2);
			FPGA_Write(FPGA_POSION_B, (pos.blue)>>2);
/*
			FPGA_Write(FPGA_POSION_R, (pos.red)); // 2022.04.06 ksk 11bit 3
			FPGA_Write(FPGA_POSION_G, (pos.green));
			FPGA_Write(FPGA_POSION_B, (pos.blue));
*/
			// cursor position
			FPGA_Write(FPGA_X_POSITION, pos.x);
			FPGA_Write(FPGA_Y_POSITION, pos.y);

			// cursor enable
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0004);
		}
	}
	else
	{
		if(preload_data[pattern_index].use>0)
			glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_preload[preload_data[pattern_index].index]->mapped);
		else
			glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);

		if(pos.type>0)
		{
			convertDeviceXYOpenGLXY(pos.x, pos.y, &ox, &oy);
			glColor3f(COL((pos.red)>>4), COL((pos.green)>>4), COL((pos.blue)>>4));

			glBegin(GL_LINES);
			glVertex2f(ox, 1);
			glVertex2f(ox,-1);
			glVertex2f( 1,oy);
			glVertex2f(-1,oy);
			glEnd();
		}
		glFinish();
		glXSwapBuffers(xdisp, window);
	}
	return ACK;
}

int vdd_set_func(uint16_t vdd)
{
	if(DRAWCHECK(model_data.if_type))
	{
		FPGA_Write(FPGA_VDD_VALUE	, vdd);
	}
	else
	{

	}

	return ACK;
}

int freq_set_func(uint64_t freq)
{
	unsigned int f_chk=0;

	printf("freq in=%ld\n", freq);
	if(DRAWCHECK(model_data.if_type))
	{
		uint32_t fpga_freq;
		if(model_data.if_type == IF_VBY1)
		{
			fpga_freq = (uint32_t)((freq / get_divider()) * 2);
			cdce913_d_freq_set(fpga_freq);
			printf("frequency_vby1 = %d		mode = %x\n", fpga_freq,model_data.mode);
		}
		else
		{
			fpga_freq = (uint32_t)(freq / get_divider());
			cdce913_d_freq_set(fpga_freq);
			printf("frequency = %d		mode = %x\n", fpga_freq,model_data.mode);
		}
		usleep(pll_interval_time*100);

		FPGA_Write(FPGA_VAR_TCLK_LOW	, fpga_freq&0xffff);
		FPGA_Write(FPGA_VAR_TCLK_HIGH	, (fpga_freq>>16)&0xffff);

		f_chk = FPGA_Read(FPGA_VAR_TCLK_HIGH);
		f_chk = (f_chk<<16) | FPGA_Read(FPGA_VAR_TCLK_LOW);

		if(fpga_freq!=f_chk)
		{
			FPGA_Write(FPGA_VAR_TCLK_LOW	, fpga_freq&0xffff);
			FPGA_Write(FPGA_VAR_TCLK_HIGH	, (fpga_freq>>16)&0xffff);
			printf("retry freq set\n");
		}

		usleep(10000);
//		printf("check FPGA_VAR_TCLK = %d\n",(((uint32_t)FPGA_Read(FPGA_VAR_TCLK_HIGH))<<16)+((uint32_t)FPGA_Read(FPGA_VAR_TCLK_LOW)));
	}
	else
	{
	}

	return ACK;
}

int bporch_set_func(int32_t flag, var_t *pvar)
{
	uint64_t	buf=0;

	if(DRAWCHECK(model_data.if_type))
	{
		if(flag == RCB_HTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;

			printf("RCB_HTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			{
				FPGA_Write(FPGA_H_TOTAL		, pvar->h_total);
				FPGA_Write(FPGA_H_BPORCH	, pvar->h_bpo);
			}

			printf("hbporch : %d\n", FPGA_Read(FPGA_H_BPORCH));
		}
		else if(flag == RCB_VTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;
			printf("RCB_VTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			FPGA_Write(FPGA_V_TOTAL		, pvar->v_total);
			FPGA_Write(FPGA_V_BPORCH	, pvar->v_bpo);

			printf("vbporch : %d\n", FPGA_Read(FPGA_V_BPORCH));
		}
		else
		{
			{
				FPGA_Write(FPGA_H_TOTAL		, model_data.h_total);
				FPGA_Write(FPGA_H_BPORCH	, model_data.h_bpo);
			}
			FPGA_Write(FPGA_V_TOTAL		, model_data.v_total);
			FPGA_Write(FPGA_V_BPORCH	, model_data.v_bpo);
		}
	}
	else
	{
	}

	return ACK;
}

static void pattern_display(char *name)
{
	printf("pattern display TP %s\n",name);
	if(dc_buffer==NULL)
	{
		printf("dc_buffer==NULL(pattern_display)\n");
		return;
	}

	char     		file_ext[4];
	pattern_head_t 	phead;

	memcpy( file_ext, name+strlen(name)-3, 3);
	file_ext[3] = '\0';

	FPGA_Write(FPGA_GRAY_LEVEL_R,0);
	FPGA_Write(FPGA_GRAY_LEVEL_G,0);
	FPGA_Write(FPGA_GRAY_LEVEL_B,0);

	pattern_load(dc_buffer, name, &phead);

	if(DRAWCHECK(model_data.if_type))
	{
		if ( 0 != strcasecmp( file_ext, "pat"))
		{
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0002); //ai disable
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0030);

			glDrawPixels(fpga_time.h_active, fpga_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
			glFinish();
			glXSwapBuffers(xdisp,window);
			player_stop();
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0008);
			fpga_draw_check(DRAW_PATTERN_CHECK);
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0008);
		}

		FPGA_Write(FPGA_MEM_RD_CTRL, 0x0000); // indirect off
	}
	else
	{
		glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer->mapped);
		glFinish();
		glXSwapBuffers(xdisp, window);
	}
}

static void video_display(char *name)
{
	char str_buf[256];
	uint16_t	i;

	FPGA_AND_SET(FPGA_MEM_RD_CTRL,0x0002); //ai disable

	glClearColor(0,0,0,1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glFinish();
	glXSwapBuffers(xdisp,window);
	glClearColor(0,0,0,1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glFinish();
	glXSwapBuffers(xdisp,window);

	for(i=0;i<4;i++)
	{
		FPGA_Write(FPGA_MEM_WR_BANK, i);
		inst_black_pat_draw();
	}

	FPGA_Write(FPGA_MEM_WR_BANK, 0);
	FPGA_Write(FPGA_MEM_RD_BANK, 0);

	if(is_4k_8k_hdmi_or_not())
	{
//		FPGA_Write(FPGA_MOV_POS_Y, 52);
		FPGA_Write(FPGA_MOV_POS_Y, 0);

		memset(under_video_path,0,sizeof(under_video_path));
		memset(str_buf,0,sizeof(str_buf));
		strcpy (str_buf, name);

//		if((is_over_4k_less_8k()) || ((model_data.mode & 0xf) == 8))
		if(is_over_4k_less_8k())
		{
			sprintf(under_video_path, "gst-launch-1.0 playbin uri=file:\"%s\" video-sink=\"nvoverlaysink overlay-x=0 overlay-y=0 overlay-w=%d overlay-h=%d display-id=1\" &", str_buf, fpga_time.h_active/2, fpga_time.v_active/2);
		}
		else
		{
			sprintf(under_video_path, "gst-launch-1.0 playbin uri=file:\"%s\" video-sink=\"nvoverlaysink overlay-x=0 overlay-y=0 overlay-w=%d overlay-h=%d display-id=1\" &", str_buf, fpga_time.h_active, fpga_time.v_active);
		}

		ERRCHECK(system(under_video_path));

		video_start=1;

		usleep(300000);
	}
	else		//HDMI or 4k or 8k
	{
		FPGA_Write(FPGA_MOV_POS_Y, 0);

		memset(under_video_path,0,sizeof(under_video_path));
		memset(str_buf,0,sizeof(str_buf));
		strcpy (str_buf, name);

		if(((model_data.mode & 0xf) == 8) && ((model_data.h_active > 3840) || (model_data.v_active > 2160)))
//		if((model_data.mode & 0xf) == 8)
		{
			sprintf(under_video_path, "gst-launch-1.0 playbin uri=file:\"%s\" video-sink=\"nvoverlaysink overlay-x=0 overlay-y=0 overlay-w=%d overlay-h=%d display-id=1\" &", str_buf, fpga_time.h_active/2, fpga_time.v_active/2);
		}
		else
		{
			sprintf(under_video_path, "gst-launch-1.0 playbin uri=file:\"%s\" video-sink=\"nvoverlaysink overlay-x=0 overlay-y=0 overlay-w=%d overlay-h=%d display-id=1\" &", str_buf, fpga_time.h_active, fpga_time.v_active);
		}

		ERRCHECK(system(under_video_path));

		video_start=1;

		usleep(300000);



//		FPGA_Write(FPGA_MOV_POS_Y, 0);
//
//
//		player_play(name);
	}

//	if( (DRAWCHECK(model_data.if_type)) && (fpga_time.h_active<3840) && (fpga_time.v_active<2160) )
//	{
//
//		FPGA_Write(FPGA_MOV_POS_Y, 52);
//
//		memset(under_video_path,0,sizeof(under_video_path));
//		memset(str_buf,0,sizeof(str_buf));
//		strcpy (str_buf, name);
//
//		sprintf(under_video_path, "gst-launch-1.0 playbin uri=file://%s video-sink=\"nvoverlaysink overlay-x=0 overlay-y=52 overlay-w=%d overlay-h=%d\" -e &", str_buf, fpga_time.h_active, fpga_time.v_active);
//		ERRCHECK(system(under_video_path));
//
//		video_start=1;
//
//		usleep(300000);
//	}
//	else
//	{
//		FPGA_Write(FPGA_MOV_POS_Y, 0);
//		player_play(name);
//	}
	FPGA_OR_SET(FPGA_MEM_WR_CTRL,0x0001);//MOV write flag
}


int vdd_changed = 0;
void set_pattern_voltage(uint16_t vdd)
{
	if(vdd==0)
	{
		if( (pattern_index>=0) && (pattern_index<pattern_cnt) )
		{
//			if(group_data.pat[pattern_index].vdd != 0)
//			{
//				pwr_vdd_set(group_data.pat[pattern_index].vdd, 0);
//				vdd_changed = 1;
//			}
//			else
//			{
//				if(vdd_changed==1)
//				{
//					pwr_vdd_set(model_data.vdd, 0);
//					vdd_changed = 0;
//				}
//			}
			pwr_vdd_set(var_data.vdd, 0);
		}
	}
	else
	{
//		var_data.vdd = vdd;

		pwr_vdd_set(vdd, 0);
		vdd_changed = 1;
	}
}

int vsync_changed = 0;
void set_pattern_vsync(uint16_t vsync)
{
	if(vsync==0)
	{
		if( (pattern_index>=0) && (pattern_index<pattern_cnt) )
		{
//			if(group_data.pat[pattern_index].vsync != 0)
//			{
//				freq_set_func(vsync_to_freq(group_data.pat[pattern_index].vsync *10));
//				vsync_changed = 1;
//			}
//			else
//			{
//				if(vsync_changed==1)
//				{
//					uint64_t	freq_high, freq64;
//					freq_high 	= model_data.freq_high;
//					freq64 		= (freq_high << 32) |  model_data.freq;
//					freq_set_func(freq64);
//					vsync_changed = 0;
//				}
//			}

			freq_set_func(var_data.freq);
		}
	}
	else
	{

//		var_data.freq=vsync_to_freq(vsync *10);

		freq_set_func(vsync_to_freq(vsync *10));
//		freq_set_func(var_data.freq);//test
		vsync_changed = 1;
	}
}

void set_pattern_scroll(uint16_t scroll_direction, uint16_t model_mode, uint16_t scroll_speed)
{
//	uint16_t mode=model_mode&0x7;
	printf("group scroll= dir%x		frame%d		speed%d\n", scroll_direction,scroll_frame_num,scroll_speed);

	FPGA_Write(FPGA_SCROLL_CTRL			, scroll_direction);
//	FPGA_Write(FPGA_SCROLL_FRAME		, 1);
	FPGA_Write(FPGA_SCROLL_FRAME		, scroll_frame_num);
	FPGA_Write(FPGA_SCROLL_PIXEL		, scroll_speed);
//	FPGA_OR_SET(FPGA_PARAM_LATCH_EN, 0x0002);
//	FPGA_AND_SET(FPGA_PARAM_LATCH_EN, 0x0002);
}

int pattern_change(int index)
{
	char str_cmd[256];
	int	k,m;

	FPGA_Write(FPGA_PARAM_LATCH_EN, 0x0001);

	if(model_data.use_dvi)
	{
		rcb_write(rcb_fd, RCB_LINE2, "DP INPUT...");
//		return NACK;
		return ACK;
	}

//	if(video_start==1)
//	{
//		ERRCHECK(system("killall gst-launch-1.0"));
//		usleep(500000);
//		FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0 : off
//	}
//	video_start=0;

	if(get_schedule_flag()==0)
	{
//		set_pattern_voltage(group_data.pat[index].vdd);	// voltage for pattern		//no need to 628
//		printf("check group vsync : %d\n",group_data.pat[index].vsync);			//test 19-09-24
//		set_pattern_vsync(group_data.pat[index].vsync);	// vsync for pattern		//no need to 628
	}
//	set_pattern_scroll(group_data.pat[index].scroll_direction, model_data.mode, group_data.pat[index].scroll_speed);//test


	set_pattern_start_time();
	//time_check_start();
	if(index < pattern_cnt)
	{
		gp.gx_ai_enable=0;
		if(preload_data[index].use>0)
		{
//			if( (fpga_time.h_active<3840) && (fpga_time.v_active<2160) )
//			{
//				FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0 : off
//			}
//			else
//			{
//				player_stop();
//				FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0
//			}
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0

			if(DRAWCHECK(model_data.if_type))
			{
				vde_check();		//check vde rising edge

				FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0002);
				FPGA_Write(FPGA_MEM_RD_BANK, preload_data[index].index);
				set_pattern_scroll(group_data.pat[index].scroll_direction, model_data.mode, group_data.pat[index].scroll_speed);

				if(preload_data[index].indirect.colormode > 0)
				{
/*
					FPGA_Write(FPGA_PAL0_R, preload_data[index].indirect.color[0][0]>>2);
					FPGA_Write(FPGA_PAL0_G, preload_data[index].indirect.color[0][1]>>2);
					FPGA_Write(FPGA_PAL0_B, preload_data[index].indirect.color[0][2]>>2);
					FPGA_Write(FPGA_PAL1_R, preload_data[index].indirect.color[1][0]>>2);
					FPGA_Write(FPGA_PAL1_G, preload_data[index].indirect.color[1][1]>>2);
					FPGA_Write(FPGA_PAL1_B, preload_data[index].indirect.color[1][2]>>2);
					FPGA_Write(FPGA_PAL2_R, preload_data[index].indirect.color[2][0]>>2);
					FPGA_Write(FPGA_PAL2_G, preload_data[index].indirect.color[2][1]>>2);
					FPGA_Write(FPGA_PAL2_B, preload_data[index].indirect.color[2][2]>>2);
*/
					switch(model_data.mode & 0xf)
					{
						case MODE_HEXA: //for 11bit
							FPGA_Write(FPGA_PAL0_R, preload_data[index].indirect.color[0][0]); // 2022.05.03 ksk 11bit 4
							FPGA_Write(FPGA_PAL0_G, preload_data[index].indirect.color[0][1]);
							FPGA_Write(FPGA_PAL0_B, preload_data[index].indirect.color[0][2]);
							FPGA_Write(FPGA_PAL1_R, preload_data[index].indirect.color[1][0]);
							FPGA_Write(FPGA_PAL1_G, preload_data[index].indirect.color[1][1]);
							FPGA_Write(FPGA_PAL1_B, preload_data[index].indirect.color[1][2]);
							FPGA_Write(FPGA_PAL2_R, preload_data[index].indirect.color[2][0]);
							FPGA_Write(FPGA_PAL2_G, preload_data[index].indirect.color[2][1]);
							FPGA_Write(FPGA_PAL2_B, preload_data[index].indirect.color[2][2]);
							printf("11bit 4 hexa\n");
							break;
						/* 2022.07.20 ksk dqhd V gray issue */
						default:
							FPGA_Write(FPGA_PAL0_R, preload_data[index].indirect.color[0][0]>>2);
							FPGA_Write(FPGA_PAL0_G, preload_data[index].indirect.color[0][1]>>2);
							FPGA_Write(FPGA_PAL0_B, preload_data[index].indirect.color[0][2]>>2);
							FPGA_Write(FPGA_PAL1_R, preload_data[index].indirect.color[1][0]>>2);
							FPGA_Write(FPGA_PAL1_G, preload_data[index].indirect.color[1][1]>>2);
							FPGA_Write(FPGA_PAL1_B, preload_data[index].indirect.color[1][2]>>2);
							FPGA_Write(FPGA_PAL2_R, preload_data[index].indirect.color[2][0]>>2);
							FPGA_Write(FPGA_PAL2_G, preload_data[index].indirect.color[2][1]>>2);
							FPGA_Write(FPGA_PAL2_B, preload_data[index].indirect.color[2][2]>>2);
							printf("11bit 4 else\n");
							break;
					}


					if(preload_data[index].indirect.ai >0)
					{
						FPGA_Write(AI_FRAME_COUNT,preload_data[index].indirect.ai_frame_cnt);
						FPGA_Write(FPGA_MEM_RD_CTRL, 0x0002);
						gp.gx_ai_enable=1;
//						printf("indirect ai pattern\n");
					}
					else
					{
						FPGA_Write(AI_FRAME_COUNT, 0x0001);
						FPGA_Write(FPGA_MEM_RD_CTRL, 0x0001);			//indirect sel	enable
					}
				}
				else
				{
					if(preload_data[index].indirect.ai >0)
					{
						FPGA_Write(AI_FRAME_COUNT,preload_data[index].indirect.ai_frame_cnt);
						FPGA_Write(FPGA_MEM_RD_CTRL, 0x0002);
						gp.gx_ai_enable=1;
	//						printf("direct ai pattern\n");
					}
					else
					{
						FPGA_Write(AI_FRAME_COUNT, 0x0001);
						FPGA_Write(FPGA_MEM_RD_CTRL, 0x0000);			//direct
					}
				}
			}
			else
			{
				glDrawPixels(tegra_time.h_active, tegra_time.v_active, GL_BGRA, GL_UNSIGNED_BYTE, dc_buffer_preload[preload_data[index].index]->mapped);
				glFinish();
				glXSwapBuffers(xdisp, window);
			}
			player_stop();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag

			printf("preload_pattern_change = %d(%d)\n", index, FPGA_Read(FPGA_MEM_RD_BANK));
		}
		else
		{
//			printf("direct_pattern_change = %d\n", index);
			if(group_data.pat[index].type == TYPE_USER)
			{
//				sprintf(str_cmd, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[index].name, EXT_PATTERN);
				for(k=0;k<8;k++)
				{
					sprintf(str_cmd, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[index].name, file_pat_ext[k]);
					if( 0 == access(str_cmd, F_OK) ) break;
				}
			}
			else if(group_data.pat[index].type == TYPE_BMP)
			{
//				sprintf(str_cmd, "%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[index].name, EXT_BMP);
				for(k=0;k<8;k++)
				{
					sprintf(str_cmd, "%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[index].name, file_bmp_ext[k]);
					if( 0 == access(str_cmd, F_OK) ) break;
				}
			}
			else if(group_data.pat[index].type == TYPE_IMAGE)
			{
//				sprintf(str_cmd,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[index].name, EXT_PNG);
//				if( 0 != access(str_cmd, F_OK) ){
//					sprintf(str_cmd,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_BMP, model_data.h_active, model_data.v_active, group_data.pat[index].name, EXT_PNG);
//					if( 0 != access(str_cmd, F_OK) ){
//						sprintf(str_cmd,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[index].name, EXT_JPG);
//					}
//				}
				for(k=0;k<8;k++)
				{
					sprintf(str_cmd,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_PNG, model_data.h_active, model_data.v_active, group_data.pat[index].name, file_png_ext[k]);
					if( 0 == access(str_cmd, F_OK) ) break;
					else
					{
						if(k==7)
						{
							for(m=0;m<8;m++)
							{
								sprintf(str_cmd,"%s%s/%d_%d/%s%s", DIR_ROOT, DIR_JPG, model_data.h_active, model_data.v_active, group_data.pat[index].name, file_jpg_ext[m]);
								if( 0 == access(str_cmd, F_OK) ) break;
							}
						}
					}
				}
			}
			else if(group_data.pat[index].type == TYPE_MOV)
			{
				sprintf(str_cmd, "%s%s/%s", DIR_ROOT, DIR_MOV, group_data.pat[index].name);
			}

			if(group_data.pat[index].type == TYPE_MOV)
			{
				FPGA_AND_SET(FPGA_MEM_WR_CTRL	, 0x0030);	// bitmap index set : 0  - 19-06-28 test add
/*
#ifdef USE_NEW_WR_BANK
				FPGA_Write(FPGA_MEM_WR_BANK, 0);
				FPGA_Write(FPGA_MEM_RD_BANK, 0);
#else
				FPGA_Write(FPGA_MEM_WR_BANK, FPGA_REALTIME_MEMORY0);
				FPGA_Write(FPGA_MEM_RD_BANK, FPGA_REALTIME_MEMORY0);
#endif
*/
				player_stop();

//				ERRCHECK(system("xrandr --output DP-0 --rotate inverted"));
//				ERRCHECK(system("xrandr --output DP-0 --rotate normal"));
				usleep(100000);
				video_display(str_cmd);
			}
			else
			{
//				ERRCHECK(system("xrandr --output DP-0 --rotate normal"));
				pattern_display(str_cmd);
//				if( (DRAWCHECK(model_data.if_type)) && (fpga_time.h_active<3840) && (fpga_time.v_active<2160) )
//				{
//					ERRCHECK(system("killall gst-launch-1.0"));
//				}
//				else
//				{
					player_stop();
//				}
				FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag
			}

			printf("direct_pattern_change = %d(%d)\n", index, FPGA_Read(FPGA_MEM_RD_BANK));
		}
		pattern_index = index;

//		if(get_schedule_flag()==0)	rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		if(schedule_move_menu==0) 	rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
		pattern_change_error=0;
	}
	else pattern_change_error=1;

	return ACK;
}

char *get_pattern_name(void)
{
	if(pattern_cnt>pattern_index)
	{
		if(pattern_index<group_data.hdr.count)
		{
			return (char *)group_data.pat[pattern_index].name;
		}
		else
		{
			if(use_hando>0)
			{
				char	*ptr;
				int		hando_idx = pattern_index-group_data.hdr.count;

				if(hando_idx>=0 && hando_idx<MAX_HANDO_CNT)
				{
					ptr = strrchr(hando_list[hando_idx].name, '/');
					return ptr;
				}
			}
		}
	}

	return ERR_STRING;
}

int get_pattern_index(void)
{
	return pattern_index;
}

void set_pattern_index(int index)
{
	pattern_index = index;
}

int pattern_dec(void)
{
	if(model_data.use_dvi)
	{
		rcb_write(rcb_fd, RCB_LINE2, "DP INPUT...");
		return NACK;
	}

	if(get_schedule_flag())
	{
		uint16_t start_index = schedule_data_new.schedule_info[schedule_index].start_pattern_index;
		uint16_t end_index = schedule_data_new.schedule_info[schedule_index].end_pattern_index;

		if(start_index < end_index)		// end index is bigger than start index or equal
		{
			if((end_index > (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(pattern_cnt > pattern_index)
				{
					if(pattern_index>0) pattern_index--;
					else 				pattern_index = pattern_cnt-1;
				}
			}
			else if((end_index > (pattern_cnt-1)) && (start_index <= (pattern_cnt-1)))
			{
				if(pattern_cnt > pattern_index)
				{
					if(pattern_index>start_index)	pattern_index--;
					else 							pattern_index = pattern_cnt-1;
				}
			}
			else
			{
				if(end_index > pattern_index)
				{
					if(pattern_index>start_index)	pattern_index--;
					else 							pattern_index = end_index-1;
				}
			}
		}
		else		//start_index >= end_index
		{
			if((end_index > (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(pattern_cnt > pattern_index)
				{
					if(pattern_index>0) pattern_index--;
					else 				pattern_index = pattern_cnt-1;
				}
			}
			else if((end_index <= (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(end_index > pattern_index)
				{
					if(pattern_index>0) pattern_index--;
					else 				pattern_index = end_index-1;
				}
			}
			else
			{

				if(end_index > pattern_index)
				{
					if(pattern_index>0) pattern_index--;
					else 				pattern_index = end_index-1;
				}
			}
		}
	}
	else
	{
		if(pattern_cnt > pattern_index)
		{
			if(pattern_index>0) pattern_index--;
			else 				pattern_index = pattern_cnt-1;
		}
	}
	pattern_change(pattern_index);
	return ACK;
}

int pattern_inc(void)
{
	if(model_data.use_dvi)
	{
		rcb_write(rcb_fd, RCB_LINE2, "DP INPUT...");
		return NACK;
	}

	if(get_schedule_flag())
	{
		uint16_t start_index = schedule_data_new.schedule_info[schedule_index].start_pattern_index;
		uint16_t end_index = schedule_data_new.schedule_info[schedule_index].end_pattern_index;

		if(start_index < end_index)		// end index is bigger than start index or equal
		{
			if((end_index > (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(pattern_index>=(pattern_cnt-1)) 	pattern_index=0;
				else 								pattern_index++;
			}
			else if((end_index > (pattern_cnt-1)) && (start_index <= (pattern_cnt-1)))
			{
				if(pattern_index>=(pattern_cnt-1)) 	pattern_index=start_index;
				else 								pattern_index++;
			}
			else
			{
				if(pattern_index>=end_index) 	pattern_index=start_index;
				else 							pattern_index++;
			}
		}
		else		//start_index >= end_index
		{
			if((end_index > (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(pattern_index>=(pattern_cnt-1)) 	pattern_index=0;
				else 								pattern_index++;
			}
			else if((end_index <= (pattern_cnt-1)) && (start_index > (pattern_cnt-1)))
			{
				if(pattern_index>=end_index) 		pattern_index=0;
				else 								pattern_index++;
			}
			else
			{
				if(pattern_index>=end_index) 	pattern_index=0;
				else 							pattern_index++;
			}
		}
	}
	else
	{
		if(pattern_index>=(pattern_cnt-1)) 	pattern_index=0;
		else 								pattern_index++;
	}

	pattern_change(pattern_index);
	return ACK;
}

void set_pattern_time_offset(uint64_t offset)
{
	pattern_time_offset = offset;
}

uint64_t get_pattern_time_offset(void)
{
	return pattern_time_offset;
}

void pattern_time_offset_inc(void)
{
	if(pattern_time_offset>=MAX_PATTERN_TIME_OFFSET) 							pattern_time_offset = MAX_PATTERN_TIME_OFFSET;
	else																		pattern_time_offset += 100;
}

void pattern_time_offset_dec(void)
{
	if(pattern_time_offset<=0 || pattern_time_offset>MAX_PATTERN_TIME_OFFSET) 	pattern_time_offset = 0;
	else																		pattern_time_offset -= 100;
}


void pattern_roll(void)
{
	if(model_data.use_dvi)
	{
		return;
	}

//	if( (get_onoff_flag()==ENUM_ON) && ((get_opmode()==AUTO_RUN)&&(get_quhd_enable()<=0)) )
	if( (get_onoff_flag()==ENUM_ON) && (get_opmode()==AUTO_RUN) )
	{
		if( (pattern_index>=0) && (pattern_index<pattern_cnt) )
		{
//			printf("Roll Frame: %d\n",group_data.pat[pattern_index].roll_frame);
			if(group_data.pat[pattern_index].roll_frame&0x8000)
//			if(pat_roll_test == 1)
			{
				set_inc_start_time();
				uint64_t elapse_time, pattern_time;
				elapse_time 	= get_pattern_elapse_time();
//				pattern_time 	= (group_data.pat[pattern_index].time*1000/(model_data.freq/(model_data.h_total*model_data.v_total)));
//				printf("pattern time: %ld\n",pattern_time);
				pattern_time 	= ((group_data.pat[pattern_index].roll_frame&0x7fff)*1000/(model_data.freq/(model_data.h_total*model_data.v_total)));
				if(elapse_time>=pattern_time)
				{
//					printf("Roll Frame: %d\n",group_data.pat[pattern_index].roll_frame&0x7fff);
					pattern_inc();
//					printf("Pattern Elapse time: %ld\n",elapse_time);
//					printf("Increase Elapse time: %ld\n",get_inc_elapse_time());
				}
			}
			else
			{
				uint64_t elapse_time, pattern_time;
				elapse_time 	= get_pattern_elapse_time();
				pattern_time 	= (group_data.pat[pattern_index].time*TIME_MULTI_VALUE) + get_pattern_time_offset();
//				printf("pattern time: %ld\n",pattern_time);
				if(pattern_time==0) pattern_time=1000;
				if(elapse_time>=pattern_time)
				{
					pattern_inc();
				}
			}
		}
	}
}


/*void pattern_roll(void)
{
	if(model_data.use_dvi)
	{
		return;
	}

//	if( (get_onoff_flag()==ENUM_ON) && ((get_opmode()==AUTO_RUN)&&(get_quhd_enable()<=0)) )
	if( (get_onoff_flag()==ENUM_ON) && (get_opmode()==AUTO_RUN) )
	{
		if( (pattern_index>=0) && (pattern_index<pattern_cnt) )
		{
			uint64_t elapse_time, pattern_time;
			elapse_time 	= get_pattern_elapse_time();
			pattern_time 	= (group_data.pat[pattern_index].time*TIME_MULTI_VALUE) + get_pattern_time_offset();
			if(pattern_time==0) pattern_time=1000;
			if(elapse_time>=pattern_time)
			{
				pattern_inc();
			}
		}
	}
}*/

int schedule_list_load(void)
{
	DIR				*dir;
	struct dirent	*dir_entry;
	char			path[MAX_PATH], ext[MAX_EXT];
	int				cnt = 0;

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/", DIR_ROOT, DIR_SCHEDULE);

	if( (dir=opendir(path)) == NULL )	return 0;

	memset(schedule_list, 0, MAX_SCHEDULE_CNT*MAX_SCHEDULE_NAME);

	while( (dir_entry = readdir(dir)) )
	{
		if( !strcmp(dir_entry->d_name, ".") ) continue;
		if( !strcmp(dir_entry->d_name, "..") ) continue;
		if( !strchr(dir_entry->d_name, '.') ) continue;

		memset(ext, 0, sizeof(ext));
		strcpy(ext, strrchr(dir_entry->d_name, '.'));
		if( !strcmp(ext, EXT_SCHEDULE) )
		{
			strcpy(schedule_list[cnt], strtok(dir_entry->d_name, "."));
			printf("schedule[%d] : %s\n", cnt, schedule_list[cnt]);

			if(cnt++ > MAX_SCHEDULE_CNT) break;
		}
	}
	printf("\n");

	closedir(dir);

	schedule_list_count = cnt;

	return cnt;
}

int get_schedule_info(char *name)
{
	FILE 	*fp;
	char	path[MAX_PATH];
	uint32_t	i;
	uint64_t	hour=0, min=0, sec=0, total_second=0;
	uint64_t	total_hour=0, total_min=0, total_sec=0;

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_SCHEDULE, name, EXT_SCHEDULE);
	fp = fopen(path, "r");
	if(fp)
	{
		memset(&schedule_data_new, 0, sizeof(schedule_data_t_new));
		if(0==fread(&schedule_data_new, sizeof(schedule_data_t_new), 1, fp)){}

		fclose(fp);

		for(i=0;i<schedule_data_new.total_schedule_count;i++)
		{
			hour	+= (schedule_data_new.schedule_info[i].on_hour + schedule_data_new.schedule_info[i].off_hour) * schedule_data_new.schedule_info[i].cycle_count;
			min		+= (schedule_data_new.schedule_info[i].on_min + schedule_data_new.schedule_info[i].off_min) * schedule_data_new.schedule_info[i].cycle_count;
			sec		+= (schedule_data_new.schedule_info[i].on_sec + schedule_data_new.schedule_info[i].off_sec) * schedule_data_new.schedule_info[i].cycle_count;
		}
		one_step_time = (uint64_t)((hour*3600) + (min*60) + sec);
		total_second = (uint64_t)(((hour*3600) + (min*60) + sec))*schedule_data_new.step_count;
		total_hour		= total_second / 3600;
		total_min		= (total_second / 60) % 60;
		total_sec		= total_second % 60;
		schedule_data_new.total_time_hour	= total_hour;
		schedule_data_new.total_time_min	= total_min;
		schedule_data_new.total_time_sec	= total_sec;
/*
		printf("total sche count : %d\n", schedule_data_new.total_schedule_count);
		printf("step count 		: %d\n", schedule_data_new.step_count);
		printf("total time hour : %d\n", schedule_data_new.total_time_hour);
		printf("total time min	: %d\n", schedule_data_new.total_time_min);
		printf("total time sec	: %d\n", schedule_data_new.total_time_sec);

		printf("index\tp_start\tp_end\ton_h\ton_m\ton_s\toff_h\ton_m\ton_s\tc_count\tvdd\tvbl\tclk\tadim\tpdim_f\tpdim_d\n");
		for(i=0;i<schedule_data_new.total_schedule_count;i++)
		{
			printf("%d\t",schedule_data_new.schedule_info[i].index);
			printf("%d\t",schedule_data_new.schedule_info[i].start_pattern_index);
			printf("%d\t",schedule_data_new.schedule_info[i].end_pattern_index);
			printf("%d\t",schedule_data_new.schedule_info[i].on_hour);
			printf("%d\t",schedule_data_new.schedule_info[i].on_min);
			printf("%d\t",schedule_data_new.schedule_info[i].on_sec);
			printf("%d\t",schedule_data_new.schedule_info[i].off_hour);
			printf("%d\t",schedule_data_new.schedule_info[i].off_min);
			printf("%d\t",schedule_data_new.schedule_info[i].off_sec);
			printf("%d\t",schedule_data_new.schedule_info[i].cycle_count);
			printf("%d\t",schedule_data_new.schedule_info[i].vdd_mode);
			printf("%d\t",schedule_data_new.schedule_info[i].vbl_mode);
			printf("%d\t",schedule_data_new.schedule_info[i].clock_mode);
			printf("%d\t",schedule_data_new.schedule_info[i].adim_mode);
			printf("%d\t",schedule_data_new.schedule_info[i].pdim_freq_mode);
			printf("%d\n",schedule_data_new.schedule_info[i].pdim_duty_mode);
		}
*/
		return 1;
	}
	else
	{
		return 0;
	}
}

uint64_t get_second(int index)
{
	uint8_t hour 	= schedule_data.schedule_info[index].hour;
	uint8_t min 	= schedule_data.schedule_info[index].min;
	uint8_t sec 	= schedule_data.schedule_info[index].sec;

	return (uint64_t)((hour*3600) + (min*60) + sec);
}

uint64_t get_second_new(int onoff, int index)
{
	uint32_t hour=0, min=0, sec=0;
	if(onoff>0)
	{
		hour 	= schedule_data_new.schedule_info[index].off_hour;
		min 	= schedule_data_new.schedule_info[index].off_min;
		sec 	= schedule_data_new.schedule_info[index].off_sec;
	}
	else
	{
		hour 	= schedule_data_new.schedule_info[index].on_hour;
		min 	= schedule_data_new.schedule_info[index].on_min;
		sec 	= schedule_data_new.schedule_info[index].on_sec;
	}

	return (uint64_t)((hour*3600) + (min*60) + sec);
}

void reset_schedule_func(void)
{
	schedule_count 	= 0;
	schedule_index 	= 0;
	schedule_first 	= 0;
	schedule_step_count = 0;
	schedule_cycle_count = 0;
	schedule_offset_time = 0;
	schedule_on_off_t = 0;
	schedule_start = 0;
}

void set_schedule_flag(int flag)
{
	schedule_flag 	= flag;
	if(flag==1)	printf("Schedule mode ON\n");
	else		printf("Schedule mode OFF\n");
}

int get_schedule_flag(void)
{
	return schedule_flag;
}

int get_schedule_index(void)
{
	return schedule_index;
}

void set_schedule_index(int index)
{
	schedule_index = index;
}

void set_schedule_step(int step)
{
	schedule_step_count = step;
}

uint64_t get_schedule_offset_time(int step, int index)
{
	uint64_t hour=0, min=0, sec=0;
	uint32_t i=0;

	for(i=0;i<(index);i++)
	{
		hour	+= (schedule_data_new.schedule_info[i].on_hour + schedule_data_new.schedule_info[i].off_hour) * schedule_data_new.schedule_info[i].cycle_count;
		min		+= (schedule_data_new.schedule_info[i].on_min + schedule_data_new.schedule_info[i].off_min) * schedule_data_new.schedule_info[i].cycle_count;
		sec		+= (schedule_data_new.schedule_info[i].on_sec + schedule_data_new.schedule_info[i].off_sec) * schedule_data_new.schedule_info[i].cycle_count;
	}

	if(step>0)
	{
		return	(uint64_t)( ((hour*3600) + (min*60) + sec) + (one_step_time*step) );
	}
	else	return (uint64_t)((hour*3600) + (min*60) + sec);
}

void run_schedule_func(int index)
{
	pwm_data_t		pwm		= {0,};
	uint64_t sche_freq=0;

	memset(&pwm, 0, sizeof(pwm_data_t));
	pwm.freq = model_data.pwm_freq[0];	// fix
	pwm.duty = model_data.pwm_duty[0]*10;

	//vdd
	if(get_pwr_vendor()==0)
	{
//		if(schedule_data_new.schedule_info[index].vdd_mode==0)	pwr_vdd_set(var_data.vdd, schedule_data_new.schedule_info[index].vdd_mode);
//		else													pwr_vdd_set(model_data.vdd, schedule_data_new.schedule_info[index].vdd_mode);
		pwr_vdd_set(var_data.vdd, schedule_data_new.schedule_info[index].vdd_mode);
		pwr_vbl_set(model_data.vbl, schedule_data_new.schedule_info[index].vbl_mode);
		pwr_adimm_control_set(model_data.vbr[0],schedule_data_new.schedule_info[index].adim_mode);
		pwr_pdimm_control_set(pwm, schedule_data_new.schedule_info[index].pdim_freq_mode, schedule_data_new.schedule_info[index].pdim_duty_mode);
	}
	else	pwr_model_set(&model_data, 0, 1);

	//clock
//	sche_freq = (model_data.h_total * model_data.v_total);
//	sche_freq = ( sche_freq *  schedule_data_new.schedule_info[index].clock_mode );
//	freq_set_func(sche_freq);

	set_schedule_vsync(schedule_data_new.schedule_info[index].clock_mode);

//	set_schedule_start_time();
}

uint64_t	old_elapse_time;
void pwr_schedule_test(void)
{
	int			on_hour, on_min, on_sec, off_hour, off_min, off_sec,el_hour, el_min, el_sec;
	uint64_t	elapse_time, sche_on_time=0, sche_off_time=0;

	if( get_schedule_flag() == 0 ) return;

	if( (get_onoff_flag()==ENUM_ON) /*&& (get_opmode()==AUTO_RUN)*/ )
	{
		sche_on_time 	= get_second_new(ON_CYCLE, schedule_index);
		sche_off_time 	= get_second_new(OFF_CYCLE, schedule_index);

		on_hour		= sche_on_time / 3600;
		on_min		= (sche_on_time / 60) % 60;
		on_sec		= sche_on_time % 60;
		off_hour	= sche_off_time / 3600;
		off_min		= (sche_off_time / 60) % 60;
		off_sec		= sche_off_time % 60;

		if(schedule_start==0)
		{
			schedule_start=1;
			set_aging_start_time();		// aging start time
		}

		if(schedule_step_count < schedule_data_new.step_count)
		{
			if (schedule_index < schedule_data_new.total_schedule_count)
			{
				if(schedule_cycle_count < schedule_data_new.schedule_info[schedule_index].cycle_count)
				{
					if(schedule_first==0)
					{
						schedule_first = 1;
						run_schedule_func(schedule_index);
						set_schedule_start_time();

						printf("Step[%d] Index[%d], on %02d:%02d:%02d, off %02d:%02d:%02d\n", schedule_step_count, schedule_index, on_hour,on_min,on_sec,off_hour,off_min,off_sec);
					}

					elapse_time = get_schedule_elapse_time();
					el_hour		= elapse_time / 3600;
					el_min		= (elapse_time / 60) % 60;
					el_sec		= elapse_time % 60;

					if( (sche_on_time > 0) && (elapse_time < sche_on_time) )	// 0 <= elapse_time < sche_on_time
					{
						//on time
						if(schedule_on_off_t==0)	// if (initial state)
						{
							schedule_on_off_t=1;	// change to on state
							onoff_by_power_seq(ENUM_ON);
							printf("on\n");
						}
					}
					else
					{
						if( (sche_off_time > 0) && (elapse_time < (sche_on_time + sche_off_time)) )	// 0 <= elapse_time < (sche_on_time+sche_off_time)
						{
							if(schedule_on_off_t<2) // (sche_on_time=0) & (sche_on_time > 0)
							{
								schedule_on_off_t=3;	// change to off state
								onoff_by_power_seq(ENUM_OFF);
								printf("%02d:%02d:%02d\n",el_hour,el_min,el_sec);		//debugging
								printf("off\n");
							}
						}
						else
						{
							printf("%02d:%02d:%02d\n",el_hour,el_min,el_sec);		//debugging
							printf("Cycle[%d] done\n",schedule_cycle_count);
							schedule_on_off_t=0;
							schedule_cycle_count++;
							set_schedule_start_time();
						}
					}
				}

				if(schedule_cycle_count >= schedule_data_new.schedule_info[schedule_index].cycle_count)
				{
					//next schedule_index
					schedule_first=0;
					schedule_on_off_t=0;
					schedule_cycle_count=0;
					schedule_index++;
				}
			}

			if(schedule_index >= schedule_data_new.total_schedule_count)
			{
				schedule_index=0;
				schedule_step_count++;
			}
		}
		else
		{
			onoff_by_power_seq(ENUM_OFF);
			set_opmode(READY);
			set_onoff_flag(ENUM_OFF);
			rcb_write(rcb_fd, RCB_LINE2, "SCHEDULE DONE..");
			// over step count = finish
//			printf("finish\n");
		}
//-======================================================================================----
	}
	else
	{
		reset_schedule_func();
	}
}


void gray_change_init(void)
{
	FPGA_Write(FPGA_MEM_RD_CTRL, 0x0008);		//RED, gray_change enable
//	FPGA_Write(FPGA_PAL0_R, 0x03ff); 20220616 ksk indirect rgb
	switch(model_data.mode & 0xf){
		case MODE_HEXA:
			FPGA_Write(FPGA_PAL0_R, 0x0fff);
			break;
		default :
			FPGA_Write(FPGA_PAL0_R, 0x03ff);
			break;
	}
	FPGA_Write(FPGA_PAL0_G, 0x0000);
	FPGA_Write(FPGA_PAL0_B, 0x0000);
	printf("gray change init");
}

void gray_color_set(uint8_t color_mode)
{
	switch (color_mode)
	{
		case COLOR_MODE_RED :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0010);
			break;
		}
		case COLOR_MODE_GREEN :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0020);
			break;
		}
		case COLOR_MODE_BLUE :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0040);
			break;
		}
		case COLOR_MODE_RED_GREEN :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0030);
			break;
		}
		case COLOR_MODE_GREEN_BLUE :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0060);
			break;
		}
		case COLOR_MODE_BLUE_RED :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0050);
			break;
		}
		case COLOR_MODE_WHITE :{
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0070);
			break;
		}
		default :{
			FPGA_OR_SET(FPGA_MEM_RD_CTRL, 0x0070);
			FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0010);
			break;
		}
	}

	printf("gray color reg = %04x\n", FPGA_Read(FPGA_MEM_RD_CTRL));
}

void schedule_menu_step_dec(void)
{
	char 		str[MAX_TEXT_BUF];

	if(schedule_move_menu==0)
	{
		pre_sche_step	= schedule_step_count;
		pre_sche_index	= schedule_index;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
		schedule_move_menu=1;
	}
	else
	{
		if (schedule_data_new.step_count==0)
		{
			pre_sche_step=0;
		}
		else
		{
			if(pre_sche_step < 1)	pre_sche_step=(schedule_data_new.step_count-1);
			else			pre_sche_step--;
		}
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
}


void schedule_menu_step_inc(void)
{
	char 		str[MAX_TEXT_BUF];

	if(schedule_move_menu==0)
	{
		pre_sche_step	= schedule_step_count;
		pre_sche_index	= schedule_index;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
		schedule_move_menu=1;
	}
	else
	{
		if (schedule_data_new.step_count<1)
		{
			pre_sche_step=0;
		}
		else
		{
			if(pre_sche_step >= (schedule_data_new.step_count-1))	pre_sche_step=0;
			else							pre_sche_step++;
		}
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
}

void schedule_menu_ok(void)
{
	char 		str[MAX_TEXT_BUF];

	if(schedule_move_menu)
	{
		if(schedule_move_check)
		{
			schedule_first=0;
			schedule_cycle_count = 0;
			schedule_start=0;
			schedule_on_off_t=0;
			set_schedule_step(pre_sche_step);
			set_schedule_index(pre_sche_index);
			schedule_offset_time = get_schedule_offset_time(pre_sche_step, pre_sche_index);
			printf("sche_offset_time: %ld\n",schedule_offset_time);
			rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
			schedule_move_menu=0;
			schedule_move_check=0;
		}
		else
		{
			if( (pre_sche_step != schedule_step_count) || (pre_sche_index != schedule_index) )
			{
				schedule_move_check=1;
				memset(str, 0, sizeof(str));
				sprintf(str, "OK to move sche.");
				rcb_write(rcb_fd, RCB_LINE2, str);
			}
			else
			{
				schedule_move_menu=0;
				schedule_move_check=0;
				pre_sche_step=0;
				pre_sche_index=0;
				schedule_move_menu=0;
				schedule_move_check=0;
			}
		}
	}
}

void schedule_menu_index_inc(void)
{
	char 		str[MAX_TEXT_BUF];

	if(schedule_move_menu)
	{
		if(pre_sche_index >= (schedule_data_new.total_schedule_count-1))	pre_sche_index=0;
		else									pre_sche_index++;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else
	{
		pre_sche_step	= schedule_step_count;
		pre_sche_index	= schedule_index;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
		schedule_move_menu=1;
	}
}

void schedule_menu_index_dec(void)
{
	char 		str[MAX_TEXT_BUF];

	if(schedule_move_menu)
	{
		if(pre_sche_index <= 0)	pre_sche_index = (schedule_data_new.total_schedule_count-1);
		else					pre_sche_index--;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
	}
	else
	{
		pre_sche_step	= schedule_step_count;
		pre_sche_index	= schedule_index;
		memset(str, 0, sizeof(str));
		sprintf(str, "Stp:%03d Indx:%03d", pre_sche_step, pre_sche_index);
		rcb_write(rcb_fd, RCB_LINE2, str);
		schedule_move_menu=1;
	}
}


void schedule_rcb_init(void)
{
	rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
	rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
	rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
	rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
	schedule_move_menu=0;
	schedule_move_check=0;
	set_opmode(AUTO_RUN);
	rcb_quhd_mode_select(get_quhd_enable());// 20180420 seochihong
}

void schedule_menu_reset(void)
{
	pre_sche_step=0;
	pre_sche_index=0;
	schedule_move_menu=0;
	schedule_move_check=0;
	rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
}

void set_schedule_vsync(uint16_t vsync)
{
	if(vsync==0)
	{
//		uint64_t	freq_high, freq64;
//		freq_high 	= model_data.freq_high;
//		freq64 		= (freq_high << 32) |  model_data.freq;
//		freq_set_func(freq64);

		freq_set_func(var_data.freq);
	}
	else
	{
		freq_set_func(vsync_to_freq(vsync*10));

//		var_data.freq = vsync_to_freq(vsync*10);
//		freq_set_func(var_data.freq);
	}
}

uint32_t cdce913_d_freq_set(unsigned int f)
{
	unsigned int m, n;
	unsigned int p, q, n_exp, r, PDiv;
	unsigned int value;
	unsigned int fvco,freq;
	unsigned char matching;
	double temp;
	unsigned int m_chk=0, n_chk=0;

	freq = f/1000;
	matching = 0;

	for(n=1; n<4096; n++){
		value = 27000 * n;
		for(m=1; m<512; m++){
			if(m > n) break;
			else{
				fvco = value / m;
				if(fvco >= 80000 && fvco <= 230000) {
//					if(fvco%freq == 0){	//share=n, remain=0
					if(fvco==freq){		//share=1, remain=0
						matching = 1;
						break;
					}
				}
			}
		}
		if(matching) break;
	}

	if(matching==0) {
		for(n=1; n<4096; n++){
			value = 27000 * n;
			for(m=1; m<512; m++){
				if(m > n) break;
				else{
					fvco = value / m;
					if(fvco >= 80000 && fvco <= 230000) {
						if((fvco%freq < 10) || (fvco%freq > (freq - 10))){ //allow +-10khz range
//						if((fvco%freq < 1) || (fvco%freq > (freq - 1))){ //allow +-1khz range
							matching = 1;
							break;
						}
					}
				}
			}
			if(matching){
				printf("freq set - approximative value\n");
				break;
			}
			if(n==4095){
				printf("freq set - no matched value\n");
			}
		}
	}
	//	N=4096, M=512 when no matched
	if(m>511)	m=511;		//prevent overflow when no matched
	if(n>4095)	n=4095;

	if(fvco%freq){
		if((fvco%freq) < 10) PDiv = (fvco/freq);
		else PDiv = (fvco/freq)+1;
	}
	else	PDiv = fvco/freq;

	temp = n/m;
	p = 4 - (int)log2(temp);
	if(p < 0) p = 0;
	if(p==0) n_exp = n * 1;
	else if(p==1) n_exp = n * 2;
	else if(p==2) n_exp = n * 4;
	else if(p==3) n_exp = n * 8;
	else if(p==4) n_exp = n * 16;

	q = n_exp/m;		//16<=q<=63
	r = n_exp - (m*q);	//0<=r<=511

	printf(">dclk n=%d, m=%d, p=%d, q=%d, r= %d, PDiv = %d, fvco=%d, freq_vco=%d, freq=%d\n",n,m,p,q,r,PDiv,fvco*1000, fvco / PDiv * 1000, f);
	FPGA_Write(FPGA_TCLK_PLL_M_LOW, (unsigned short)(m&0xffff));
	FPGA_Write(FPGA_TCLK_PLL_M_HIGH, (unsigned short)((m>>16)&0xffff));
	FPGA_Write(FPGA_TCLK_PLL_N_LOW, (unsigned short)(n&0xffff));
	FPGA_Write(FPGA_TCLK_PLL_N_HIGH, (unsigned short)((n>>16)&0xffff));

	m_chk = FPGA_Read(FPGA_TCLK_PLL_M_HIGH);
	m_chk = (m_chk<<16) | FPGA_Read(FPGA_TCLK_PLL_M_LOW);
	n_chk = FPGA_Read(FPGA_TCLK_PLL_N_HIGH);
	n_chk = (n_chk<<16) | FPGA_Read(FPGA_TCLK_PLL_N_LOW);

	if(m!=m_chk)
	{
		printf("m%d retry pll m set\n", m_chk);
		FPGA_Write(FPGA_TCLK_PLL_M_LOW, (unsigned short)(m&0xffff));
		FPGA_Write(FPGA_TCLK_PLL_M_HIGH, (unsigned short)((m>>16)&0xffff));
	}
	if(n!=n_chk)
	{
		printf("n%d retry pll n set\n", n_chk);
		FPGA_Write(FPGA_TCLK_PLL_N_LOW, (unsigned short)(n&0xffff));
		FPGA_Write(FPGA_TCLK_PLL_N_HIGH, (unsigned short)((n>>16)&0xffff));
	}


	return 0;
}

int vrr_set_for_donga(var_t *pvar)
{
	if(DRAWCHECK(model_data.if_type))
	{
		FPGA_Write(FPGA_V_TOTAL		, pvar->v_total);

		printf("VRR : v_total=%d\tvsync=%d\n", FPGA_Read(FPGA_V_TOTAL), (pvar->freq)/FPGA_Read(FPGA_H_TOTAL)/FPGA_Read(FPGA_V_TOTAL));
	}
	else
	{
	}

	return ACK;
}

int vde_check(void)
{
	u16 vde=1, rde=0;
	u32 cnt;

	vde = FPGA_Read(FPGA_VDE_READ);
	if(!vde){
//		printf("vde check case0 start\n");
		for(cnt=0;cnt<10000;cnt++)
		{
			rde = FPGA_Read(FPGA_VDE_READ);
			if(rde==1) {
				Aprintf("vde ok\n");
				vde_cnt+=1;
				break;
			}
//			usleep(50);
		}
//		printf("vde check case0 end\n");
		return 0;
	}
	else
	{
//		printf("vde check case1 start\n");
		for(cnt=0;cnt<10000;cnt++)
		{
			rde = FPGA_Read(FPGA_VDE_READ);
			if(rde==0) {
				Aprintf("vde high to low\n");
				break;
			}
//			usleep(50);
		}
	}
	return 0;
}
/*int vde_check(void)
{
	uint16_t 	vde=1;
	uint16_t	rde=0;

	vde = FPGA_Read(FPGA_VDE_READ);
	if(!vde){
		rde = FPGA_Read(FPGA_VDE_READ);
		if(rde==1) {
			vde_cnt += 1;
			Aprintf("VDE Count: %d\r\n",vde_cnt);
		}
		return 0;
	}
	return 0;
}*/

void gamma_change(signed short red, signed short green, signed short blue)		//-1023 ~ 1023
{
	signed short	max_level;
	signed short	level_r, level_g, level_b;

	switch(model_data.mode&0x30)
	{
		case 0x00: //6bit mode
			{
//				printf("gamma set 6bit\n");
				max_level = 64;
				if(red>=max_level)							level_r=(max_level<<4)-1;
				else if( (red<max_level) && (red>0) )		level_r=(red<<4)-1;
				else if(red<=(-max_level))					level_r=(-(max_level<<4))+1;
				else if( (red>(-max_level)) && (red<0) )	level_r=(-((-red)<<4))+1;
				else										level_r=0;

				if(green>=max_level)							level_g=(max_level<<4)-1;
				else if( (green<max_level) && (green>0) )		level_g=(green<<4)-1;
				else if(green<=(-max_level))					level_g=(-(max_level<<4))+1;
				else if( (green>(-max_level)) && (green<0) )	level_g=(-((-green)<<4))+1;
				else											level_g=0;

				if(blue>=max_level)							level_b=(max_level<<4)-1;
				else if( (blue<max_level) && (blue>0) )		level_b=(blue<<4)-1;
				else if(blue<=(-max_level))					level_b=(-(max_level<<4))+1;
				else if( (blue>(-max_level)) && (blue<0) )	level_b=(-((-blue)<<4))+1;
				else										level_b=0;

//				printf("value= %d\t0x%04x\n",level_r, level_r);
				break;	// 6bit
			}
		case 0x10:	//8bit mode
			{
//				printf("gamma set 8bit\n");
				max_level = 256;
				if(red>=max_level)							level_r=(max_level<<2)-1;
				else if( (red<max_level) && (red>0) )		level_r=(red<<2)-1;
				else if(red<=(-max_level))					level_r=(-(max_level<<2))+1;
				else if( (red>(-max_level)) && (red<0) )	level_r=(-((-red)<<2))+1;
				else										level_r=0;

				if(green>=max_level)							level_g=(max_level<<2)-1;
				else if( (green<max_level) && (green>0) )		level_g=(green<<2)-1;
				else if(green<=(-max_level))					level_g=(-(max_level<<2))+1;
				else if( (green>(-max_level)) && (green<0) )	level_g=(-((-green)<<2))+1;
				else											level_g=0;

				if(blue>=max_level)							level_b=(max_level<<2)-1;
				else if( (blue<max_level) && (blue>0) )		level_b=(blue<<2)-1;
				else if(blue<=(-max_level))					level_b=(-(max_level<<2))+1;
				else if( (blue>(-max_level)) && (blue<0) )	level_b=(-((-blue)<<2))+1;
				else										level_b=0;

//				printf("value= %d\t0x%04x\n",level_r, level_r);
				break;	// 8bit
			}
		case 0x20:	//10bit mode
			{
				max_level = 1024;
//				printf("gamma set 10bit\n");
				if(red>=max_level)							level_r=(max_level)-1;
				else if( (red<max_level) && (red>0) )		level_r=(red)-1;
				else if(red<=(-max_level))					level_r=(-(max_level))+1;
				else if( (red>(-max_level)) && (red<0) )	level_r=(-((-red)))+1;
				else										level_r=0;

				if(green>=max_level)							level_g=(max_level)-1;
				else if( (green<max_level) && (green>0) )		level_g=(green)-1;
				else if(green<=(-max_level))					level_g=(-(max_level))+1;
				else if( (green>(-max_level)) && (green<0) )	level_g=(-((-green)))+1;
				else											level_g=0;

				if(blue>=max_level)							level_b=(max_level)-1;
				else if( (blue<max_level) && (blue>0) )		level_b=(blue)-1;
				else if(blue<=(-max_level))					level_b=(-(max_level))+1;
				else if( (blue>(-max_level)) && (blue<0) )	level_b=(-((-blue)))+1;
				else										level_b=0;

//				printf("value= %d\t0x%04x\n",level_r, level_r);
				break;	// 10bit
			}
/* 2022.05.11 ksk Scroll 11bit */
//		case 0x30:	//12bit mode (not using in ES628)
//			{
//				max_level = 4096;
//				printf("gamma set 12bit\n");
//				if(red>=max_level)							level_r=(max_level)-1;
//				else if( (red<max_level) && (red>0) )		level_r=(red)-1;
//				else if(red<=(-max_level))					level_r=(-(max_level))+1;
//				else if( (red>(-max_level)) && (red<0) )	level_r=(-((-red)))+1;
//				else										level_r=0;
//
//				if(green>=max_level)							level_g=(max_level)-1;
//				else if( (green<max_level) && (green>0) )		level_g=(green)-1;
//				else if(green<=(-max_level))					level_g=(-(max_level))+1;
//				else if( (green>(-max_level)) && (green<0) )	level_g=(-((-green)))+1;
//				else											level_g=0;
//
//				if(blue>=max_level)							level_b=(max_level)-1;
//				else if( (blue<max_level) && (blue>0) )		level_b=(blue)-1;
//				else if(blue<=(-max_level))					level_b=(-(max_level))+1;
//				else if( (blue>(-max_level)) && (blue<0) )	level_b=(-((-blue)))+1;
//				else										level_b=0;
//
//				printf("value= %d\t0x%04x\n",level_r, level_r);
//				break;	// 12bit
//			}
/*							*/

		default:
			{
				level_r=0;
				level_g=0;
				level_b=0;
				break;
			}
	}

	printf("gamma level set= %d %d %d\n",level_r, level_g, level_b);

	FPGA_Write_signed(FPGA_GRAY_LEVEL_R, level_r);
	FPGA_Write_signed(FPGA_GRAY_LEVEL_G, level_g);
	FPGA_Write_signed(FPGA_GRAY_LEVEL_B, level_b);
}

int width_set_func(int32_t flag, var_t *pvar)
{
	uint64_t	buf=0;

	if(DRAWCHECK(model_data.if_type))
	{
		if(flag == RCB_HTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;
			printf("RCB_HTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			{
				FPGA_Write(FPGA_H_TOTAL		, pvar->h_total);
				FPGA_Write(FPGA_H_WIDTH		, pvar->h_width);
			}

			printf("hwidth : %d\n", FPGA_Read(FPGA_H_WIDTH));
		}
		else if(flag == RCB_VTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;
			printf("RCB_VTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			FPGA_Write(FPGA_V_TOTAL		, pvar->v_total);
			FPGA_Write(FPGA_V_WIDTH		, pvar->v_width);

			printf("vwidth : %d\n", FPGA_Read(FPGA_V_WIDTH));
		}
		else
		{
			{
				FPGA_Write(FPGA_H_TOTAL		, model_data.h_total);
				FPGA_Write(FPGA_H_WIDTH		, model_data.h_width);
			}
			FPGA_Write(FPGA_V_TOTAL		, model_data.v_total);
			FPGA_Write(FPGA_V_WIDTH		, model_data.v_width);
		}
	}
	else
	{
	}

	return ACK;
}

int fporch_set_func(int32_t flag, var_t *pvar)
{
	uint64_t	buf=0;

	if(DRAWCHECK(model_data.if_type))
	{
		if(flag == RCB_HTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;

			printf("RCB_HTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			{
				FPGA_Write(FPGA_H_TOTAL		, pvar->h_total);
			}

			printf("hfporch : %d\n", FPGA_Read(FPGA_H_TOTAL) - FPGA_Read(FPGA_H_WIDTH) - FPGA_Read(FPGA_H_BPORCH) - FPGA_Read(FPGA_H_ACTIVE));
		}
		else if(flag == RCB_VTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;

			printf("RCB_VTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			FPGA_Write(FPGA_V_TOTAL		, pvar->v_total);

			printf("vfporch : %d\n", FPGA_Read(FPGA_V_TOTAL) - FPGA_Read(FPGA_V_WIDTH) - FPGA_Read(FPGA_V_BPORCH) - FPGA_Read(FPGA_V_ACTIVE));
		}
		else
		{
			{
				FPGA_Write(FPGA_H_TOTAL		, model_data.h_total);
			}
			FPGA_Write(FPGA_V_TOTAL		, model_data.v_total);
		}
	}
	else
	{
	}

	return ACK;
}

int active_set_func(int32_t flag, var_t *pvar)
{
	uint64_t	buf=0;

	if(DRAWCHECK(model_data.if_type))
	{
		if(flag == RCB_HTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;
			printf("RCB_HTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			{
				FPGA_Write(FPGA_H_TOTAL		, pvar->h_total);
				FPGA_Write(FPGA_H_ACTIVE	, pvar->h_active);
			}

			printf("hactive : %d\n", FPGA_Read(FPGA_H_ACTIVE));
		}
		else if(flag == RCB_VTIME)
		{
			buf = pvar->freq / (pvar->h_total * pvar->v_total);
			pvar->v_freq = (uint32_t)buf;
			printf("RCB_VTIME - ht:%d, vt:%d, vsync:%d, freq:%lu \n", pvar->h_total, pvar->v_total, pvar->v_freq, pvar->freq);

			FPGA_Write(FPGA_V_TOTAL		, pvar->v_total);
			FPGA_Write(FPGA_V_ACTIVE	, pvar->v_active);

			printf("vactive : %d\n", FPGA_Read(FPGA_V_ACTIVE));
		}
		else
		{
			{
				FPGA_Write(FPGA_H_TOTAL		, model_data.h_total);
				FPGA_Write(FPGA_H_ACTIVE	, model_data.h_active);
			}
			FPGA_Write(FPGA_V_TOTAL		, model_data.v_total);
			FPGA_Write(FPGA_V_ACTIVE	, model_data.v_active);
		}
	}
	else
	{
	}

	return ACK;
}

void inst_ind_pat_draw(req_inst_ind_pat_onoff_t *istnp)
{
	int ret=ACK;
	uint16_t	val_w=0,val_h=0, val_sh=0, val_sv=0;

	if(istnp->on_off>0)
	{
		{
			gp.inst_ind_pat_enable=1;

			//draw instant indirect box pattern
			FPGA_Write(FPGA_MEM_WR_BANK, FPGA_REALTIME_MEMORY0);
/*			if(DRAWCHECK(dc_buffer->output_display) )
			{
				if((get_quhd_enable()==0))	FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag on
				else
				{
					if(is_5k_lvds_quad()) FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag on
				}
			}
*/
			/* 2021.12.29 :*/
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag on
			/*  */
			/*	2022.05.31 ksk scroll issue  */
			gx_pen_color(dc_buffer, gx_color(0, 0, 0, 255));//BG
			gx_brush_color(dc_buffer, gx_color(0, 0, 0, 255));
			gx_rectangle(dc_buffer, 0, 0,fpga_time.h_active,fpga_time.v_active);
			if( ((istnp->height)>0) && ((istnp->width)>0) && ((istnp->start_h)<(fpga_time.h_active)) && ((istnp->start_v)<(fpga_time.v_active)) )
			{
				//prevent drawing oversize
				if( (istnp->width)>(fpga_time.h_active - istnp->start_h) )	val_w=(fpga_time.h_active - istnp->start_h);
				else														val_w=istnp->width;
				if( (istnp->height)>(fpga_time.v_active - istnp->start_v) )	val_h=(fpga_time.v_active - istnp->start_v);
				else														val_h=istnp->height;

				int sx = istnp->start_h;
				int sy = istnp->start_v;
				int ex = (istnp->start_h + val_w)-1;
				int ey = (istnp->start_v + val_h)-1;
				/* 	2022.05.31 ksk scroll issue */
				switch(model_data.mode & 0xf)
				{
					case MODE_HEXA: //for 11bit
						if(model_data.if_type == IF_VBY1)
						{
							gx_pen_color(dc_buffer, gx_color(4, 4, 4, 255));
							gx_brush_color(dc_buffer, gx_color(4, 4, 4, 255));
							gx_rectangle(dc_buffer, sx, sy, ex, ey);
						}
						else
						{
							gx_pen_color(dc_buffer, gx_color(1, 1, 1, 255));
							gx_brush_color(dc_buffer, gx_color(1, 1, 1, 255));
							gx_rectangle(dc_buffer, sx, sy, ex, ey);
						}
						break;
					/* 2022.07.20 ksk dqhd V gray issue */
//					case MODE_32LANE_16x2:
//						gx_pen_color(dc_buffer, gx_color(4, 4, 4, 255));
//						gx_brush_color(dc_buffer, gx_color(4, 4, 4, 255));
//						gx_rectangle(dc_buffer, sx, sy, ex, ey);
//						break;
					default:
						gx_pen_color(dc_buffer, gx_color(1, 1, 1, 255));
						gx_brush_color(dc_buffer, gx_color(1, 1, 1, 255));
						gx_rectangle(dc_buffer, sx, sy, ex, ey);
						break;
				}
			}

			if(DRAWCHECK(dc_buffer->output_display) )
			{
				if(get_quhd_enable()==0)
				{
					printf("tp1 \n");
					FPGA_SendEnd();
					transfer();
					FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);	// fill box write flag off
					fpga_draw_check(DRAW_PATTERN_CHECK);
				}
				else
				{
					printf("tp2 \n");
					if(is_5k_lvds_quad())
					{
						FPGA_SendEnd();
						transfer();
						FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);	// fill box write flag off
						fpga_draw_check(DRAW_PATTERN_CHECK);
					}
					else
					{
					}
				}
			}

			if(get_quhd_enable() && (is_5k_lvds_quad()==0))
			{
				ret = quhd_memory_preload();
				if(ret!=ACK) printf("quhd instant indirect pattern draw failed\n");
			}

			FPGA_Write(FPGA_MEM_RD_BANK, FPGA_REALTIME_MEMORY0);
			FPGA_Write(FPGA_MEM_RD_CTRL, 0x0001);					//indirect sel	enable
		}
	}
	else
	{
		gp.inst_ind_pat_enable=0;
		FPGA_Write(FPGA_MEM_RD_CTRL, 0x0000);			//indirect sel	enable

		pattern_change(get_pattern_index());
	}
}

void inst_ind_pat_color(req_inst_ind_pat_color_t *istnc) //for DAVINCI color test
{
	if((istnc->pal)==1)			//background
	{
//		FPGA_Write(FPGA_PAL0_R, (istnc->red)>>2);
//		FPGA_Write(FPGA_PAL0_G, (istnc->green)>>2);
//		FPGA_Write(FPGA_PAL0_B, (istnc->blue)>>2);

		switch(model_data.mode & 0xf){
			case MODE_HEXA :
				if(model_data.if_type == IF_VBY1)
				{
					FPGA_Write(FPGA_PAL0_R, (istnc->red)); //2022.05.11 ksk 11bit 5
					FPGA_Write(FPGA_PAL0_G, (istnc->green));
					FPGA_Write(FPGA_PAL0_B, (istnc->blue));
					printf("11bit 5 hexa\n");
					printf("PAL0 R: %d\n",istnc->red);
					printf("PAL0 G: %d\n",istnc->green);
					printf("PAL0 B: %d\n",istnc->blue);
				}
				else
				{
					FPGA_Write(FPGA_PAL0_R, (istnc->red)>>2);
					FPGA_Write(FPGA_PAL0_G, (istnc->green)>>2);
					FPGA_Write(FPGA_PAL0_B, (istnc->blue)>>2);
					printf("11bit 5 else\n");
				}
				break;
			default:
				FPGA_Write(FPGA_PAL0_R, (istnc->red)>>2);
				FPGA_Write(FPGA_PAL0_G, (istnc->green)>>2);
				FPGA_Write(FPGA_PAL0_B, (istnc->blue)>>2);
				printf("11bit 5 else\n");
				break;
		}

	}
	else if((istnc->pal)==0)	//foreground
	{
//		FPGA_Write(FPGA_PAL1_R, (istnc->red)>>2);
//		FPGA_Write(FPGA_PAL1_G, (istnc->green)>>2);
//		FPGA_Write(FPGA_PAL1_B, (istnc->blue)>>2);

		switch(model_data.mode & 0xf){
			case MODE_HEXA:
				if(model_data.if_type == IF_VBY1)
				{
					FPGA_Write(FPGA_PAL1_R, (istnc->red)); //2022.05.11 ksk 11bit 6
					FPGA_Write(FPGA_PAL1_G, (istnc->green));
					FPGA_Write(FPGA_PAL1_B, (istnc->blue));
					printf("11bit 6 hexa\n");
				}
				else
				{
					FPGA_Write(FPGA_PAL1_R, (istnc->red)>>2);
					FPGA_Write(FPGA_PAL1_G, (istnc->green)>>2);
					FPGA_Write(FPGA_PAL1_B, (istnc->blue)>>2);
					printf("11bit 6 else\n");
				}
				break;
			default :
				FPGA_Write(FPGA_PAL1_R, (istnc->red)>>2);
				FPGA_Write(FPGA_PAL1_G, (istnc->green)>>2);
				FPGA_Write(FPGA_PAL1_B, (istnc->blue)>>2);
				printf("11bit 6 else\n");
				break;
		}
	}
	else
	{
		FPGA_Write(FPGA_PAL0_R, 0);
		FPGA_Write(FPGA_PAL0_G, 0);
		FPGA_Write(FPGA_PAL0_B, 0);
		FPGA_Write(FPGA_PAL1_R, 0);
		FPGA_Write(FPGA_PAL1_G, 0);
		FPGA_Write(FPGA_PAL1_B, 0);
//		FPGA_Write(FPGA_PAL2_R, 0);
//		FPGA_Write(FPGA_PAL2_G, 0);
//		FPGA_Write(FPGA_PAL2_B, 0);
	}
}

void inst_black_pat_draw(void)
{
//	int ret=ACK;
	uint16_t	val_w=0,val_h=0;

	uint16_t	start_h=0, start_v=0, width=0, height=0;

	width = fpga_time.h_active;
	height = fpga_time.v_active;

	{
		if(DRAWCHECK(dc_buffer->output_display) )
		{
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag on
		}

		gx_pen_color(dc_buffer, gx_color(0, 0, 0, 255));
		gx_brush_color(dc_buffer, gx_color(0, 0, 0, 255));
		gx_rectangle(dc_buffer, 0, 0,fpga_time.h_active,fpga_time.v_active);
		if( (height>0) && (width>0) && (start_h<fpga_time.h_active) && (start_v<fpga_time.v_active) )
		{
			//prevent drawing oversize
			if( width>(fpga_time.h_active - width) )	val_w=(fpga_time.h_active - start_h);
			else										val_w=width;
			if( height>(fpga_time.v_active - start_v) )	val_h=(fpga_time.v_active - start_v);
			else										val_h=height;

			int sx = start_h;
			int sy = start_v;
			int ex = (start_h + val_w)-1;
			int ey = (start_v + val_h)-1;

			//gx_pen_color(dc_buffer, gx_color(0, 0, 0, 255));
			gx_brush_color(dc_buffer, gx_color(0, 0, 0, 255));
			gx_rectangle(dc_buffer, sx, sy, ex, ey);
			//printf("%d x %d\n", ex, ey);
		}

		if(DRAWCHECK(dc_buffer->output_display) )
		{
			FPGA_SendEnd();
			transfer();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);	// fill box write flag off
			fpga_draw_check(DRAW_PATTERN_CHECK);
		}

//		if(get_quhd_enable() && (is_5k_lvds_quad()==0))
//		{
//			ret = quhd_memory_preload();
//			if(ret!=ACK) printf("quhd instant indirect pattern draw failed\n");
//		}

//		printf("draw instant black pattern\n");
	}
}

void STD_Rolling_func(req_std_rolling_set_set_t *rdata)
{
	uint16_t	pre_data=0;
	uint32_t	flag=0;

//	printf("Rolling test: type=%d, value=%d\n", rdata->type, rdata->value);


	switch(rdata->type)
	{
		// 0:pixel_clock, 1:h_freq, 2:h_fporch, 3:h_width, 4:h_bporch, 5:v_freq, 6:v_fporch, 7:v_width, 8:v_bporch
		case 0:	//pixel freq
			{
				req_freq_set_t	fdata	= {0,};

				fdata.type = 1;
				fdata.freq = (unsigned long long int)(rdata->value);	//ex)118800
				fdata.freq *= 10000;		//ex)118800-> 1188000000

				printf("type=%d, freq=%ld\n", fdata.type, fdata.freq);
				freq_set_from_pc(&fdata);
				gp.freq=fdata.freq;
				gp.vsync = (ushort)(freq_to_vsync(var_data.freq)*10.0);
			} break;
		case 1:	//h freq
			{

			} break;
		case 2:	//h fporch
			{
				uint32_t	hfporch=0;

				hfporch=(rdata->value)/100;
				pre_data=var_data.h_total - var_data.h_width - var_data.h_bpo - var_data.h_active;
				if (hfporch > pre_data)	var_data.h_total += (hfporch - pre_data);
				else					var_data.h_total -= (pre_data - hfporch);
				flag = RCB_HTIME;
				var_data.h_fpo = hfporch;
				fporch_set_func(flag, &var_data);
				printf("Rolling h-fporch=%d\n", hfporch);
			} break;
		case 3:	//h width
			{
				uint32_t	hwidth=0;

				hwidth=(rdata->value)/100;
				pre_data=var_data.h_total - var_data.h_bpo - var_data.h_active - var_data.h_fpo;
				if (hwidth > pre_data)	var_data.h_total += (hwidth - pre_data);
				else					var_data.h_total -= (pre_data - hwidth);
				flag = RCB_HTIME;
				var_data.h_width = hwidth;
				width_set_func(flag, &var_data);
				printf("Rolling h-width=%d\n", hwidth);
			} break;
		case 4:	//h bporch
			{
				uint32_t	hbporch=0;

				hbporch=(rdata->value)/100;
				pre_data=var_data.h_total - var_data.h_width - var_data.h_active - var_data.h_fpo;
				if (hbporch > pre_data)	var_data.h_total += (hbporch - pre_data);
				else					var_data.h_total -= (pre_data - hbporch);
				flag = RCB_HTIME;
				var_data.h_bpo = hbporch;
				bporch_set_func(flag, &var_data);
				printf("Rolling h-bporch=%d\n", hbporch);
			} break;
		case 5:	//v freq
			{
//				req_vsync_set_t vfdata 	= {0,};
//				uint16_t		vsync=0;
//
//				vsync=(unsigned short)(rdata->value & 0x0000ffff);	//ex)60.00 -> 6000
//				vsync/=10;	//ex)6000 -> 600
//
//				printf("vsync : %f\n", vsync/10.0);		// data.vsync *10 from communicator
//
////				vfdata = 1;
////				vsync_set_from_pc(&vfdata);
////				gp.vsync=vfdata.vsync;
////				gp.freq = var_data.freq;
			} break;
		case 6:	//v fporch
			{
				uint32_t	vfporch=0;

				vfporch=(rdata->value)/100;
				pre_data=var_data.v_total - var_data.v_width - var_data.v_bpo - var_data.v_active;
				if (vfporch > pre_data)	var_data.v_total += (vfporch - pre_data);
				else					var_data.v_total -= (pre_data - vfporch);
				flag = RCB_VTIME;
				var_data.v_fpo = vfporch;
				fporch_set_func(flag, &var_data);
				printf("Rolling v-fporch=%d\n", vfporch);
			} break;
		case 7:	//v width
			{
				uint32_t	vwidth=0;

				vwidth=(rdata->value)/100;
				pre_data=var_data.v_total - var_data.v_bpo - var_data.v_active - var_data.v_fpo;
				if (vwidth > pre_data)	var_data.v_total += (vwidth - pre_data);
				else					var_data.v_total -= (pre_data - vwidth);
				flag = RCB_VTIME;
				var_data.v_width = vwidth;
				width_set_func(flag, &var_data);
				printf("Rolling v-width=%d\n", vwidth);
			} break;
		case 8:	//v bporch
			{
				uint32_t	vbporch=0;

				vbporch=(rdata->value)/100;
				pre_data=var_data.v_total - var_data.v_width - var_data.v_active - var_data.v_fpo;
				if (vbporch > pre_data )	var_data.v_total += (vbporch - pre_data);
				else								var_data.v_total -= (pre_data - vbporch);
				flag = RCB_VTIME;
				var_data.v_bpo = vbporch;
				bporch_set_func(flag, &var_data);
				printf("Rolling v-bporch=%d\n", vbporch);
			} break;
		default: break;

	}

}

void std_i2c_test_fpga_set(req_std_i2c_test_wr_t data, uint16_t flag)
{
	uint16_t	i=0, cnt=0;

	if(flag==0)	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data.i2c_info.dev_addr)&0x00fe);				//write rd
	else		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, ((data.i2c_info.dev_addr)&0x00ff)|0x0001);	//read

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc9, (data.i2c_info.reg_addr_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data.i2c_info.data_size)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data.i2c_info.byte_num)&0x00ff);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data.i2c_info.i2c_clock_sel)&0x00ff);

	printf("i2c test reg check\n");
	printf("i2c clock sel 0x%04x\t->\t0x%04x\n", data.i2c_info.i2c_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));
	printf("i2c reg size 0x%04x\t->\t0x%04x\n", data.i2c_info.reg_addr_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc9));
	printf("i2c data size 0x%04x\t->\t0x%04x\n", data.i2c_info.data_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
	printf("i2c device addr 0x%04x\t->\t0x%04x\n", data.i2c_info.dev_addr, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
	printf("i2c btye num 0x%04x\t->\t0x%04x\n", data.i2c_info.byte_num, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));

	cnt=data.i2c_info.byte_num;
	if(cnt>200)	cnt=200;

	for(i=0;i<cnt;i++)
	{
		FPGA_I2C_1_ON_ADDR_CTRL_Write(FPGA_I2C_1_ON_REG_ADDR+i,(data.i2c_info.reg_addr[i]));
		if(flag==0)	FPGA_I2C_1_ON_DATA_Write(FPGA_I2C_1_ON_REG_DATA+i,(data.i2c_info.reg_data[i]));		//write case only

		printf("[%d] 0x%04x - 0x%04x\t->\t0x%04x - 0x%04x\n", i, data.i2c_info.reg_addr[i], data.i2c_info.reg_data[i], FPGA_I2C_1_ON_ADDR_CTRL_Read(i), FPGA_I2C_1_ON_DATA_Read(i));
	}
	printf("\n");

	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0001);
	usleep(100);
	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0000);
}

void std_ocmd_test_fpga_set(req_std_ocmd_test_wr_t data, uint16_t flag)
{
	uint16_t	i=0, cnt=0;

		if(flag==0)	FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, (data.ocmd_info.dev_addr)&0x00fe);				//write rd
		else		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xc8, ((data.ocmd_info.dev_addr)&0x00ff)|0x0001);		//read

		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xca, (data.ocmd_info.data_size)&0x00ff);
		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcb, (data.ocmd_info.byte_num)&0x00ff);
		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcc, (data.ocmd_info.ocmd_clock_sel)&0x00ff);

		printf("OCMD test reg check\n");
		printf("OCMD clock sel 0x%04x\t->\t0x%04x\n", data.ocmd_info.ocmd_clock_sel, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcc));
		printf("OCMD data size 0x%04x\t->\t0x%04x\n", data.ocmd_info.data_size, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xca));
		printf("OCMD device addr 0x%04x\t->\t0x%04x\n", data.ocmd_info.dev_addr, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xc8));
		printf("OCMD byte num 0x%04x\t->\t0x%04x\n", data.ocmd_info.byte_num, FPGA_I2C_1_ON_ADDR_CTRL_Read(0xcb));

		cnt=data.ocmd_info.byte_num;
		if(cnt>200)	cnt=200;
		for(i=0;i<cnt;i++)
		{
			FPGA_I2C_1_ON_ADDR_CTRL_Write(i,(data.ocmd_info.sub_addr[i]));
			if(flag==0) {
				FPGA_OCMD_1_ON_REG_DATA_H_Write(i,data.ocmd_info.reg_data_h[i]);
				FPGA_OCMD_1_ON_REG_DATA_L_Write(i,data.ocmd_info.reg_data_l[i]);
				FPGA_OCMD_1_ON_TABLE_DATA_H_Write(i,data.ocmd_info.table_data_h[i]);
				FPGA_OCMD_1_ON_TABLE_DATA_L_Write(i,data.ocmd_info.table_data_l[i]);
			}

			printf("[%d] 0x%04x:\t 0x%08x\t - \t0x%08x\n", i, FPGA_I2C_1_ON_ADDR_CTRL_Read(i),(FPGA_OCMD_1_ON_TABLE_DATA_H_Read(i)<<16) + (FPGA_OCMD_1_ON_TABLE_DATA_L_Read(i)),(FPGA_OCMD_1_ON_REG_DATA_H_Read(i)<<16) + (FPGA_OCMD_1_ON_REG_DATA_L_Read(i)));

		}
		printf("\n");
		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0001);
		usleep(100);
		FPGA_I2C_1_ON_ADDR_CTRL_Write(0xcd,0x0000);
}

void std_ctl_fpga_set(req_std_ctl_wr_t data, uint16_t flag) //ksk ctl fpga set
{
	uint16_t	i=0;

	printf("ctl reg check\n");
	for(i=0;i<256;i++)
	{
//		FPGA_CTL_DATA_WRITE(FPGA_CTL_REG_ADDR+i,(data.ctl_info.reg_addr[i]));
		if(flag==0)	FPGA_CTL_DATA_WRITE(FPGA_CTL_ON_REG_DATA+i,(data.ctl_info.reg_data[i]));		//write case only

		printf("[%d] 0x%04x - 0x%04x\t\n", i, data.ctl_info.reg_addr[i], data.ctl_info.reg_data[i]);
	}
	printf("\n");
//	usleep(100);

}
