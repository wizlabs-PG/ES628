/*
 * i2c_sensing.c
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#include <i2c_sensing.h>
#include <rcb.h>
#include <fpga_spi.h>
#include <pwr_control.h>
#include <global.h>
#include <pattern_control.h>
#include <model_data.h>
#include <i2c.h>

static int i2c_idd_open(uint8_t addr)
{
//	int 		mem_fd;
//	void		*map_base;
	char		str_cmd[128];
/*
	mem_fd = open(DEV_MEM, O_RDWR|O_SYNC);
	if(mem_fd == (-1))
	{
		fprintf(stderr, "[I2C IDD] mem open() error!\n");
		return 0;
	}

	map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, I2C_ADDRESS);
	if(map_base == MAP_FAILED)
	{
		fprintf(stderr, "[I2C IDD] map_base error!\n");
		return 0;
	}
	NVIDIA_I2C_DIV = (((10200/I2C_IDD_FREQ)<<16)&0xFFFF0000)|0x00000002;
	munmap(map_base, MAP_SIZE);
	close(mem_fd);
*/
	////////////////////////////////////////////////////////////////

	sprintf(str_cmd,"echo %d > /sys/bus/i2c/devices/i2c-%d/bus_clk_rate",I2C_IDD_FREQ*1000,2);
	system(str_cmd);
	sync();

	i2c_idd_fd = open(DEV_I2C, O_RDWR);
	if (i2c_idd_fd < 0)
	{
		fprintf(stderr, "[I2C IDD] i2c_idd_fd open() error!\n");
		return 0;
	}

	if (ioctl(i2c_idd_fd, I2C_SLAVE, addr) < 0)
	{
		fprintf(stderr, "[I2C IDD] i2c_idd_fd ioctl() error!\n");
		return 0;
	}

	return 1;
}

int i2c_idd_limit_set(uint32_t mili_amp)
{
	if(i2c_idd_open(I2C_LIMIT_SET_ADDR)==0)
	{
		return 0;
	}

	uint32_t	v_cir=3300, r_cir=6500, r_var=0;
	uint32_t	v_max=0, v_lim;
	uint32_t	spec_range=20000;	//20A,  mili_amp = 0~spec_range
	uint32_t	d_val=0;



	v_max = (v_cir * 10000) / (r_cir + 10000);		// reference voltage setting max value

	if(mili_amp > spec_range)	v_lim = v_max;
	else						v_lim = (v_max * (mili_amp - IDD_SET_OFFSET)) / spec_range;

	r_var = (r_cir * v_lim) / (v_cir - v_lim);

	d_val = (255 * r_var) / 10000;

//	unsigned char limit_val = (unsigned char)(((mili_amp - IDD_SET_OFFSET) / 40) & 0xff);
	unsigned char limit_val = (unsigned char)(d_val & 0xff);

	if(0 > i2c_smbus_write_i2c_block_data(i2c_idd_fd, IDD_SET_CMD, 1, &limit_val))
	{
		fprintf(stderr, "[I2C IDD] i2c_idd_limit_set() error!\n");
		return 0;
	}

//	printf("idd limit set error pass\n");	//19-07-03 test print
	printf("idd limit set value = %d mA\t%d ohm\t%02x = %d\n", mili_amp, r_var, limit_val, limit_val);

	close(i2c_idd_fd);

	i2c_read_idd();	//test 19-07-12
	return 1;
}

uint32_t i2c_read_idd(void)
{
	if(i2c_idd_open(I2C_READ_IDD_ADDR)==0)
	{
		return 0;
	}

	uint8_t 	read_val[5];
	uint16_t	tmp 	= 0;
	uint32_t	ret_val = 0;

	memset(read_val, 0, sizeof(read_val));
	if(0 > i2c_smbus_read_i2c_block_data(i2c_idd_fd, IDD_READ_CMD, 4, read_val))
	{
		fprintf(stderr, "[I2C IDD] i2c_read_idd() error!\n");
		close(i2c_idd_fd);
		return 0;
	}

	tmp = ((read_val[0]&0x0f)<<12) + (read_val[1]<<4) + ((read_val[2]&0xf0)>>4);

//	printf("idd tmp=0x%04x\t read_val[0]=%02x read_val[1]=%02x read_val[2]=%02x read_val[3]=%02x read_val[4]=%02x\n", tmp, read_val[0],read_val[1], read_val[2], read_val[3], read_val[4]);

	if (tmp > IDD_READ_OFFSET) 	tmp -= IDD_READ_OFFSET;
	else 						tmp  = 0;

	if (tmp < 0x130)  // 304
	{
		ret_val = ((130 * tmp * 2048) / 32768) / 10;
	}
	else
	{
		ret_val = ((100 * tmp * 2048) / 32768) / 10; 	//+a
	}

	close(i2c_idd_fd);

	return ret_val;
}

void i2c_idd_sensing(void)
{
	if( get_onoff_flag()==ENUM_OFF ){
		rcb_display_roll=0;
		schedule_offset_time=0;
		return;
	}
	if( (get_opmode()!=AUTO_RUN) && (get_opmode()!=MANU_RUN) ) return;

	uint64_t 	elapse_time, elapse_aging_time, freq=0;
	int			hour, min, sec;
	int i;
	u16 j=0;
	uint32_t	vs=0;

	res_status_t data_status 		= {0,};
	memset(&data_status, 0, sizeof(res_status_t));

	elapse_qems_aging_time = elapse_aging_time 	= get_aging_elapse_time();

	if(get_schedule_flag()==1)	elapse_aging_time += schedule_offset_time;

	min					= elapse_aging_time / 60;
	hour				= min / 60;
	sec					= elapse_aging_time % 60;
	min					= min % 60;

	elapse_time = get_sensing_elapse_time();
	if(elapse_time >= I2C_IDD_SENSING_TIME)
	{
		char str[MAX_TEXT_BUF];

//		idd_value = i2c_read_idd();		//<- change to pwr idd value

		vdd_value=0;
		idd_value=0;
		vbl_value=0;
		ibl_value=0;

		//19-07-02 	display vdd, idd, vbl, ibl to RCB
		if(get_pwr_vendor()==0)
		{
			if ((rsp_detect_data[0].hdr.cmd==0) && (rsp_detect_data[0].hdr.board_id==0)) //no external power
			{
//				for(j=0;j<20;j++) i2c_read_idd();		//test 19-07-12
				idd_value = i2c_read_idd();
			}
			else 	//power connected
			{
				for(i=0; i<ENSIS_PWR_CH; i++)
				{
					vdd_value += htons(rsp_detect_data[i].det_val.vdd);
					idd_value += htons(rsp_detect_data[i].det_val.idd);
					vbl_value += htons(rsp_detect_data[i].det_val.vbl);
					ibl_value += htons(rsp_detect_data[i].det_val.ibl);
				}
				vdd_value /= ENSIS_PWR_CH;
				idd_value /= ENSIS_PWR_CH;
				vbl_value /= ENSIS_PWR_CH;
				ibl_value /= ENSIS_PWR_CH;
			}
		}
		else
		{
			if ((rsp_detect_osung_data.hdr.cmd==0) && (rsp_detect_osung_data.hdr.board_id==0)) //no external power
			{
//				for(j=0;j<20;j++) i2c_read_idd();		//test 19-07-12
				idd_value = i2c_read_idd();
			}
			else 	//power connected
			{
				vdd_value = (rsp_detect_osung_data.det_val[0].vdd);
				idd_value = (rsp_detect_osung_data.det_val[0].idd);
				vbl_value = (rsp_detect_osung_data.det_val[0].vbl);
				ibl_value = (rsp_detect_osung_data.det_val[0].ibl);
			}
		}

		memset(str, 0, sizeof(str));
		if (((rsp_detect_data[0].hdr.cmd==0) & (rsp_detect_data[0].hdr.board_id==0)) &&  ((rsp_detect_osung_data.hdr.cmd==0) & (rsp_detect_osung_data.hdr.board_id==0)))
		{
			if(get_schedule_flag()==1)	sprintf(str, "%03d:%02d:%02d/%03d:%02d", hour, min, sec, schedule_data_new.total_time_hour, schedule_data_new.total_time_min);
			else						sprintf(str, "%02d:%02d:%02d  %.2fA", hour, min, sec, idd_value/1000.0);
		}
		else
		{
			if(get_schedule_flag()==1)	sprintf(str, "%03d:%02d:%02d/%03d:%02d", hour, min, sec, schedule_data_new.total_time_hour, schedule_data_new.total_time_min);
			else
			{
				switch(rcb_display_roll)
				{
					case MODEL_TURN	: sprintf(str, "%s", model_name);	break;
					case TIME_TURN	: sprintf(str, "TIME %02d:%02d:%02d", hour, min, sec);	break;
					case VDD_TURN	: sprintf(str, "VDD  %.2fV", vdd_value/100.0);			break;
					case IDD_TURN	: sprintf(str, "IDD  %.2fA", idd_value/1000.0);			break;
					case VBL_TURN	: sprintf(str, "VBL  %.2fV", vbl_value/100.0);			break;
					case IBL_TURN	: sprintf(str, "IBL  %.2fA", ibl_value/1000.0);			break;
					case DIM_TURN	:
						{
//							printf("[%02d:%02d:%02d] var data adim: %d\n", hour, min, sec,	var_data.adim);		// test print 19-08-14 adim problem
							sprintf(str, "A-DIM  %.1fV", var_data.adim/10.0);		break;
						}
					case VSYNC_TURN	:
					{
						freq = FPGA_Read(FPGA_VAR_TCLK_HIGH);
						freq = (freq<<16) | FPGA_Read(FPGA_VAR_TCLK_LOW);
						if(model_data.if_type == IF_VBY1)	vs = ((freq / 2)* get_divider()) / (model_data.h_total * model_data.v_total);
						else								vs = (freq * get_divider()) / (model_data.h_total * model_data.v_total);
						sprintf(str, "VSYNC  %dHz", vs);	break;
					}
					default 		: sprintf(str, "%02d:%02d:%02d  %.2fA", hour, min, sec, idd_value/1000.0); break;
				}
			}
		}
		rcb_write(rcb_fd, RCB_LINE1, str);
		if (rcb_display_roll<MAX_ROLL)	rcb_display_roll++;
		else							rcb_display_roll=0;

//		memset(str, 0, sizeof(str));
//		sprintf(str, "%02d:%02d:%02d  %.2fA", hour, min, sec, idd_value/1000.0);
//		rcb_write(rcb_fd, RCB_LINE1, str);


		if(FPGA_ocp_detect() == ACK)	// high limit
		{

			rcb_write(rcb_fd, RCB_LINE2, "IDD HIGH LIMIT!");
			delay_us(3000000);
			rcb_auto_run_proc(KEY_ESC);
		}
		else
		{
			if (((rsp_detect_data[0].hdr.cmd==0) & (rsp_detect_data[0].hdr.board_id==0)) &&  ((rsp_detect_osung_data.hdr.cmd==0) & (rsp_detect_osung_data.hdr.board_id==0)))
			{
				// check limit
				if(idd_value < model_data.idd_l*10)	// low limit
				{
					rcb_write(rcb_fd, RCB_LINE2, "IDD LOW LIMIT!");
					delay_us(3000000);
					rcb_auto_run_proc(KEY_ESC);
				}
			}
			else
			{
				if(get_pwr_vendor()==0)
				{
					for(i=0; i<ENSIS_PWR_CH; i++)
					{
						if(rsp_detect_data[i].error!=0)
						{
	//						// 1st check(power)
							data_status.limit 	= rsp_detect_data[i].error;
//							data_status.vdd	= htons(rsp_detect_data[i].det_val.vdd);
	//						data_status.vbl	= htons(rsp_detect_data[i].det_val.vbl);
							data_status.ocp	= (uint8_t)(i+1);	// power board id(1~4)
							printf("limit(power_ch%d) : 0x%02X\t%d \n", i, data_status.limit, data_status.limit);
							break;
						}
					}
				}
				else
				{
					if(rsp_detect_osung_data.error[0]!=0)
					{
						data_status.limit 	= rsp_detect_osung_data.error[0];
						data_status.ocp 	= 1;
						printf("limit(power) : 0x%02X \n", data_status.limit);
					}
				}
			}
		}


		// 2nd check(fw)
		if(data_status.limit==0x00)
		{
			if(get_schedule_flag()==1)
			{
				if(vdd_value<(model_data.vdd_l)) 		data_status.limit |= ERR_VDD_MIN;
				if(vdd_value>(model_data.vdd_h*1.2)) 	data_status.limit |= ERR_VDD_MAX;
				if(vbl_value<(model_data.vbl_l))		data_status.limit |= ERR_VBL_MIN;
				if(vbl_value>(model_data.vbl_h*1.2)) 	data_status.limit |= ERR_VBL_MAX;
			}
			else
			{
				if(vdd_value<model_data.vdd_l) 		data_status.limit |= ERR_VDD_MIN;
				if(vdd_value>model_data.vdd_h) 		data_status.limit |= ERR_VDD_MAX;
				if(vbl_value<model_data.vbl_l) 		data_status.limit |= ERR_VBL_MIN;
				if(vbl_value>model_data.vbl_h) 		data_status.limit |= ERR_VBL_MAX;

			}
				if(idd_value<model_data.idd_l*10) 	data_status.limit |= ERR_IDD_MIN;
				if(idd_value>model_data.idd_h*10) 	data_status.limit |= ERR_IDD_MAX;
				if(ibl_value<model_data.ibl_l*10) 	data_status.limit |= ERR_IBL_MIN;
				if(ibl_value>model_data.ibl_h*10) 	data_status.limit |= ERR_IBL_MAX;



			if(data_status.limit!=0x00)
			{
				data_status.ocp = 0x00;	// if it is not ep616p power..
				printf("limit(fw) : 0x%02X \n", data_status.limit);
//				printf("============================================\n");
//				if (data_status.limit == ERR_VDD_MIN) printf("vdd value: %d\tmodel vdd low limit: %d\n", vdd_value,model_data.vdd_l);
//				else if (data_status.limit == ERR_VDD_MAX) printf("vdd value: %d\tmodel vdd high limit: %d\n", vdd_value,model_data.vdd_h);
//				else if (data_status.limit == ERR_VBL_MIN) printf("vbl value: %d\tmodel vbl low limit: %d\n", vbl_value,model_data.vbl_l);
//				else if (data_status.limit == ERR_VBL_MAX) printf("vbl value: %d\tmodel vbl high limit: %d\n", vbl_value,model_data.vbl_h);
//				else if (data_status.limit == ERR_IDD_MIN) printf("idd value: %d\tmodel idd low limit: %d\n", idd_value,model_data.idd_l);
//				else if (data_status.limit == ERR_IDD_MAX) printf("idd value: %d\tmodel idd high limit: %d\n", idd_value,model_data.idd_h);
//				else if (data_status.limit == ERR_IBL_MIN) printf("ibl value: %d\tmodel ibl low limit: %d\n", ibl_value,model_data.ibl_l);
//				else if (data_status.limit == ERR_IBL_MAX) printf("ibl value: %d\tmodel ibl high limit: %d\n", ibl_value,model_data.ibl_h);
//				else {
//					printf("vdd value: %d\tvbl value: %d\tidd value: %d\tibl value: %d\n",vdd_value,vbl_value,idd_value,ibl_value);
//					printf("vdd limit: %d\tvbl limit: %d\tidd limit: %d\tibl limit: %d\n",model_data.vdd_h,model_data.vbl_h,model_data.idd_h,model_data.ibl_h);
//				}

			}
		}


		if( (data_status.limit!=0x00) && (get_onoff_flag()==ENUM_ON) )
		{
			onoff_by_power_seq(ENUM_OFF);
			set_opmode(READY);
			set_onoff_flag(ENUM_OFF);

			for(i=0; i<8; i++)
			{
				if( (data_status.limit>>i)&0x01 )
				{
					if((var_spc_en>0) && (i==7))		// When enabled SPC mode and ibl limit status
					{
//						var_spc_en=0;
//						pwr_spc_onoff_set(var_spc_en);
//						pwr_model_set(&model_data, 0, 0);
						printf("limit screen num %d\n", i+11);
						rcb_ready_screen(i+11);
					}
					else
					{
						printf("limit screen num %d\n", i+10);
						rcb_ready_screen(i+10);
					}
					break;
				}
			}
		}

		set_sensing_start_time();
	}
}




