/*
============================================================================
 Name        : msg_comu.c
 Author      : jschoi
 Version     : 1.0
 Copyright   : copyright (c) 2017 ensis.co.,Ltd.
 Description : C, Ansi-style
 ============================================================================
 */

#include <msg_comu.h>
#include <model_data.h>
#include <group_data.h>
#include <pwr_control.h>
#include <pattern_control.h>
#include <i2c_sensing.h>
#include <rcb.h>
#include <global.h>
#include <pwr_tcpclient.h>

int msq_init(void)
{
	msq_id = msgget((key_t)MSQ_KEY, IPC_CREAT | 0666);
	if( (-1) == msq_id )
	{
		perror("msgget() error!");
	}
	else msq_clean();

	return msq_id;
}

int msq_send_comm(msg_data_t* pdata)
{
	int	ret = 0;

	pdata->target_type = DEV_COMU;
	pdata->source_type = DEV_MAIN;

	ret = msgsnd(msq_id, (void*)pdata, sizeof(msg_data_t)-sizeof(long int), 0);
	if( (-1) == ret )
	{
		perror("msgsnd() error!");
	}

	return ret;
}

int msq_send_pwrc(msg_data_t* pdata)
{
	int	ret = 0;

	pdata->target_type = DEV_PWRC;
	pdata->source_type = DEV_MAIN;

	ret = msgsnd(msq_id, (void*)pdata, sizeof(msg_data_t)-sizeof(long int), 0);
	if( (-1) == ret )
	{
		perror("msgsnd_pwrc() error!");
	}

	return ret;
}

int msq_recv(msg_data_t* pdata)
{
	int	ret = 0;

	ret = msgrcv(msq_id, (void*)pdata, sizeof(msg_data_t)-sizeof(long int), DEV_MAIN, IPC_NOWAIT);	// IPC_NOWAIT : non-blocking
	if( (-1) == ret )
	{
		//printf("%d\n",errno);
		//perror("msgrcv() error!");
	}

	return ret;
}

int msq_clean()
{
	int			ret = 0;
	msg_data_t 	data = {0,};

	while(1)
	{
		ret = msgrcv(msq_id, (void*)&data, sizeof(msg_data_t)-sizeof(long int), DEV_MAIN, IPC_NOWAIT);	// IPC_NOWAIT : non-blocking
		if( (-1) == ret ) 	break;
		else 				printf("ES628 : msq_clean\n");
	}

	return ret;
}

int msq_close()
{
	int ret = 0;

	ret = msgctl(msq_id, IPC_RMID, 0);
	if( (-1) == ret )
	{
		perror("msgctl() error!");
	}

	return ret;
}

int wait_ack_pwrc(msg_data_t *msg, int delay)
{
	int     	wait_cnt 	= 0;
	msg_data_t	rmsg 		= {0,};

	msq_clean();	// in case of previous msgrcv error, after cleaning, run next working.

	if( (-1) != msq_send_pwrc(msg) )
	{
		wait_cnt = 0;
		memset(&rmsg, 0, sizeof(msg_data_t));
		while(1)
		{
			if( (-1) != msq_recv(&rmsg) )
			{
//				printf("[PWRC_ACK] cmd_id=0x%X, data_len=%d\n", rmsg.header.cmd_id, rmsg.header.data_len);
				printf("[PWRC_ACK] cmd_id=0x%X, elapse_time=%dms\n", rmsg.header.cmd_id, wait_cnt);
				break;
			}
			else
			{
				if( wait_cnt++ > delay )	// delay*10 = msec
				{
//					fprintf(stderr, "COMMUNICATOR : cmd_id - 0x%X time out!\n", msg->header.cmd_id);

					printf("[PWRC_ACK] cmd_id=0x%X, %dms time out!\n", msg->header.cmd_id, wait_cnt);
					return NACK;
				}
				usleep(1000);
			}
		}
	}

	return ACK;
}

static void send_version(void)
{
	int			i;
	msg_data_t 	smsg;
	res_ver_t 	data_ack 		= {0,};

	memset(&data_ack, 0, sizeof(res_ver_t));
	data_ack.hdr.cmd_id			= RES_ID(SID_VER_REQ);
	data_ack.hdr.data_len		= sizeof(res_ver_t)-sizeof(res_head_t);
	data_ack.res				= ACK;
	data_ack.ver				= FW_VERSION;

	for(i=0; i<ENSIS_PWR_CH; i++)
	{
		data_ack.pwr_ver[i]		= htons(rsp_ver_data[i].fw_ver);
	}

	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &data_ack, sizeof(res_ver_t));
	msq_send_comm(&smsg);
}

void pwr_version_checking(void)
{
	if(version_start==1)
	{
		int i;
		if(get_pwr_vendor()==0)
		{
			for(i=0; i<ENSIS_PWR_CH; i++)
			{
				if(version_end[i]==0)
				{
					if(version_ack[i]==RES_ACK)
					{
						version_end[i] = 1;
						if(i<ENSIS_PWR_CH-1) pwr_ver_req(i+2);
					}
					else
					{
						if(version_ack_cnt++>100)	// timeout(100*POLL_LOOP_TIME=1000ms)
						{
							printf("[%d] timeout!\n", i);
							version_end[i] = 1;
							rsp_ver_data[i].fw_ver = 0xFFFF;
							if(i<ENSIS_PWR_CH-1) pwr_ver_req(i+2);
						}
					}
					break;
				}
			}

			if(version_end[ENSIS_PWR_CH-1]>0)
			{
				printf("version check end!\n");
				send_version();
				version_start = 0;
			}
		}
		else
		{
				if(version_osung_end==RES_NACK)
				{
					if(version_osung_ack==RES_ACK)
					{
						version_osung_end=RES_ACK;
						printf("power version: %d.%02d\n", rsp_ver_osung_data.fw_ver/100, rsp_ver_osung_data.fw_ver%100);
					}
					else
					{
						if(version_ack_cnt++>200)	// timeout(200*POLL_LOOP_TIME=2000ms)
						{
							printf("version check timeout!\n");
							version_osung_end = RES_ACK;
							rsp_ver_osung_data.fw_ver = 0xFFFF;
						}
					}
				}
				else
				{
					printf("version check end!\n");
					version_start = 0;
				}
		}
	}
}

static void send_pwr_offset(void)
{
	msg_data_t 		smsg;
	res_offset_t 	data_ack 		= {0,};

	memset(&data_ack, 0, sizeof(res_offset_t));
	data_ack.hdr.cmd_id			= RES_ID(SID_PWR_OFFSET_REQ);
	data_ack.hdr.data_len		= sizeof(res_offset_t)-sizeof(res_head_t);
	data_ack.res				= ACK;

	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &data_ack, sizeof(res_ver_t));
	msq_send_comm(&smsg);
}

void pwr_offset_checking(void)
{
	if(offset_start==1)
	{
		int i;
		for(i=0; i<ENSIS_PWR_CH; i++)
		{
			if(offset_end[i]==0)
			{
				if(offset_ack[i]==RES_ACK)
				{
					offset_end[i] = 1;
					if(i<ENSIS_PWR_CH-1) pwr_offset_req(i+2);
				}
				else
				{
					if(offset_ack_cnt++>50)	// timeout(50*POLL_LOOP_TIME=1000ms)
					{
						printf("[%d] timeout!\n", i);
						offset_end[i] = 1;
						if(i<ENSIS_PWR_CH-1) pwr_offset_req(i+2);
					}
				}
				break;
			}
		}

		if(offset_end[ENSIS_PWR_CH-1]>0)
		{
			printf("power offset check end!\n");
			send_pwr_offset();
			offset_start = 0;
		}
	}
}

void pwr_seq_check(void)
{
	msg_data_t 		smsg;
	res_pwr_seq_t 	data_ack 		= {0,};

	memset(&data_ack, 0, sizeof(res_pwr_seq_t));
	data_ack.hdr.cmd_id			= RES_ID(SID_PWR_SEQ_STATUS_REQ);
	data_ack.hdr.data_len		= sizeof(res_pwr_seq_t)-sizeof(res_head_t);
	data_ack.repeat				= pseq_rpcnt;
	data_ack.scenario			= scenario_cnt+1;

	for(int i=0; i<4; i++)
	{
//		data_ack.read_aux_data[i] = aux_data[i];
	}

	printf("###########Repeat Count : %d\n", data_ack.repeat);
	printf("###########Scenario Count : %d\n", data_ack.scenario);

	for(int i=0; i<4; i++)
	{
		printf("###########read_aux_data[%d] : 0x%04x\n", i, data_ack.read_aux_data[i]);
	}

	memset(&smsg, 0, sizeof(msg_data_t));
	memcpy(&smsg.header, &data_ack, sizeof(res_pwr_seq_t));
	msq_send_comm(&smsg);
}

char global_group_name[MAX_GROUP_NAME];

void msg_analyze(void)
{
	int i;
	msg_data_t	rmsg, smsg;

	memset(&rmsg, 0, sizeof(msg_data_t));

	if( (-1) != msq_recv(&rmsg) )
	{
		printf("ES620 : cmd_id - 0x%X\n", rmsg.header.cmd_id);		//test block 19-08-14

		switch(rmsg.header.cmd_id)
		{
		case SID_VER_REQ:
			{
				res_ver_t data_ack	={0,};

				for(i=0; i<ENSIS_PWR_CH; i++)	version_end[i] = 0;
												version_osung_end=0;
				pwr_ver_req(1);
				version_start = 1;

				memset(&data_ack, 0, sizeof(res_ver_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_VER_REQ);
				data_ack.hdr.data_len		= sizeof(res_ver_t)-sizeof(res_head_t);
				data_ack.res				= ACK;
				data_ack.ver				= FW_VERSION;


				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_ver_t));
				msq_send_comm(&smsg);

			}
			break;
		case SID_RUN_TYPE_REQ:
			{
				req_run_set_t data 		= {0,};
				res_run_set_t data_ack 	= {0,};

				memset(&data, 0, sizeof(req_run_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_run_set_t));
				printf("type : %s\n", (data.type==0) ? "auto" : "manu");

				if(data.type==0)
				{
					set_opmode(AUTO_RUN);
					rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
					rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
					rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
				}
				else
				{
					set_opmode(MANU_RUN);
					rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
					rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
				}

				memset(&data_ack, 0, sizeof(res_run_set_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_RUN_TYPE_REQ);
				data_ack.hdr.data_len		= sizeof(uint16_t);
				data_ack.res				= ACK;

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_run_set_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_MODEL_SEL_REQ:
			{
				printf("--------Model Select Requested--------\n");
				req_model_sel_t data 		= {0,};
				res_model_sel_t data_ack 	= {0,};

				memset(&data, 0, sizeof(req_model_sel_t));
				memcpy(&data, &rmsg.header, sizeof(req_model_sel_t));

				model_update();

				adim_change_flag = 0;

				memset(&data_ack, 0, sizeof(res_model_sel_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_MODEL_SEL_REQ);
				data_ack.hdr.data_len		= sizeof(uint16_t);
				data_ack.res				= model_select((char*)data.model_name, (int)data.use_hando);

				if(data_ack.res==ACK)
				{
					model_selection_end = ACK;
					group_selection_end = NACK;
					set_model_init_status(ACK);
					set_curmodel_idx((char*)data.model_name);

					rcb_write(rcb_fd, RCB_LINE2, "WAITING GROUP...");
				}
				else
				{
					model_selection_end = NACK;
					group_selection_end = NACK;
					set_model_init_status(NACK);

					memset(old_model_name,0,sizeof(old_model_name));
					model_init();
					rcb_model_change();
				}
				DP_EDID_change();
				model_variable_reset();
//				dp_port_sel(model_data.DP_Port_sel); dp edid

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_model_sel_t));
				msq_send_comm(&smsg);
			}
			break;

#ifdef GROUP_SELECT
		case SID_GROUP_SEL_REQ:
			{
				printf("--------Group Select Requested--------\n");
				req_group_sel_t data 		= {0,};
				res_group_sel_t data_ack 	= {0,};

				memset(&data, 0, sizeof(req_group_sel_t));
				memcpy(&data, &rmsg.header, sizeof(req_group_sel_t));

				group_list_load();

				memset(&data_ack, 0, sizeof(res_group_sel_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_GROUP_SEL_REQ);
				data_ack.hdr.data_len		= sizeof(uint16_t);

				if(model_selection_end==ACK)
				{
					set_model_init_status(ACK);
					data_ack.res = group_select(data.group_name);
					strcpy(global_group_name,data.group_name);
					printf("group name: %s\n",data.group_name);
					if(data_ack.res==ACK)
					{
						group_selection_end=ACK;
						set_curgroup_idx((char*)data.group_name);
					}
					else					group_selection_end=NACK;
				}
				else
				{
					set_model_init_status(NACK);
					set_model_init_status(NACK);
					memset(old_model_name,0,sizeof(old_model_name));
					model_init();
					rcb_model_change();
					data_ack.res				= NACK;
				}

//				if(data_ack.res==ACK)
//				{
//					group_selection_end = ACK;
//					set_curgroup_idx((char*)data.group_name);
//				}
//				else group_selection_end = NACK;


				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_group_sel_t));
				msq_send_comm(&smsg);
			}
			break;
#endif
		case SID_LCD_ONOFF_REQ:
			{
				req_lcd_onoff_t data 		= {0,};
				res_lcd_onoff_t data_ack 	= {0,};
				int		index_c=0;

				memset(&data, 0, sizeof(req_lcd_onoff_t));
				memcpy(&data, &rmsg.header, sizeof(req_lcd_onoff_t));

				memset(&data_ack, 0, sizeof(res_lcd_onoff_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_LCD_ONOFF_REQ);
				data_ack.hdr.data_len		= sizeof(res_lcd_onoff_t)-sizeof(res_head_t);

				adim_change_flag = 1;

				if(data.on_off==0)	// off
				{
					vde_cnt=0;
					onoff_by_power_seq(ENUM_OFF);
					//ES628
					{
						FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
//						if(get_schedule_flag()==0) set_pattern_index(0);
						set_pattern_scroll(0,1,0);

//						if(FPGA_Read(FPGA_MEM_WR_CTRL)&&0x0001)
//						{
////							if( (fpga_time.h_active<3840) && (fpga_time.v_active<2160) )
////							{
////								ERRCHECK(system("killall gst-launch-1.0"));
////								FPGA_Write(FPGA_MOV_POS_Y, 0);
////							}
//							FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0
							player_stop();
//						}
					}

					printf("lcd off\n");

					rcb_ready_screen(ACK);
					set_opmode(READY);
					set_onoff_flag(ENUM_OFF);
//					pwr_model_set(&model_data, 0, 0);
					data_ack.res 			= ACK;
				}
				else				// on
				{
					onoff_by_power_seq(ENUM_ON);

					//ES628
					{
						set_pattern_scroll(0,1,0);
//						FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
//						pattern_change(get_pattern_index());
					}

					reset_schedule_func();
					rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
					set_onoff_flag(ENUM_ON);

					if(data.type==0)	// auto
					{
						set_opmode(AUTO_RUN);
						rcb_write(rcb_fd, RCB_LINE1, TITLE_AUTO_MODE);
						rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
						rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_AUTOMANU);
						set_pattern_index(0);
						printf("Auto Run\n");
						data_ack.res 			= pattern_change(0);		//if auto case, pattern index=0
					}
					else				// manual
					{
						set_opmode(MANU_RUN);
						rcb_write(rcb_fd, RCB_LINE1, TITLE_MANU_MODE);
						rcb_write(rcb_fd, RCB_LINE2, get_pattern_name());
						rcb_write(rcb_fd, RCB_LEDONOFF, LED_OFF_AUTOMANU);
						printf("Manual Run\n");
						data_ack.res 			= pattern_change(data.pat_num);
					}

//					data_ack.res 			= pattern_change(data.pat_num);
					FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
				}


				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_lcd_onoff_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_PAT_DISP_REQ:
			{
				req_pat_disp_t data	 		= {0,};
				res_pat_disp_t data_ack 	= {0,};
				unsigned int		index_c=0;

				memset(&data, 0, sizeof(req_pat_disp_t));
				memcpy(&data, &rmsg.header, sizeof(req_pat_disp_t));

				memset(&data_ack, 0, sizeof(res_pat_disp_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_PAT_DISP_REQ);
				data_ack.hdr.data_len		= sizeof(res_pat_disp_t)-sizeof(res_head_t);

				if(get_onoff_flag()==ENUM_ON)
				{
					if( !((op_mode == MANU_RUN) || (op_mode == AUTO_RUN)) ){		//  SP Tech control problem (gray <-> pattern) 19-10-28 added
						FPGA_AND_SET(FPGA_MEM_RD_CTRL, 0x0008);
						set_opmode(MANU_RUN);
						color_done = 0;
					}

					if(data.type==0)		//index
					{
						index_c = get_pattern_index();
						if(index_c==data.pat_num)	data_ack.res = ACK;
						else						data_ack.res = pattern_change(data.pat_num);
//						data_ack.res = pattern_change(data.pat_num);
					}
					else					//just increase or decrease
					{
						if(data.pat_num==1) data_ack.res = pattern_inc();
						else				data_ack.res = pattern_dec();
					}

//					set_pattern_voltage(data.vdd);	// voltage for pattern		//no need to ES628
//					set_pattern_vsync(data.vsync);	// vsync for pattern		//no need to ES628
					if(data.type==0)	set_pattern_scroll(group_data.pat[data.pat_num].scroll_direction, 1, group_data.pat[data.pat_num].scroll_speed);
					else				set_pattern_scroll(group_data.pat[pattern_index].scroll_direction, 1, group_data.pat[pattern_index].scroll_speed);
				}
				else
				{
//					set_pattern_index(data.pat_num);
					data_ack.res=ACK;
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_pat_disp_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_STATUS_REQ:
			{
				req_status_t data	 		= {0,};
				res_status_t data_ack 		= {0,};

				memset(&data, 0, sizeof(req_status_t));
				memcpy(&data, &rmsg.header, sizeof(req_status_t));

				memset(&data_ack, 0, sizeof(res_status_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_STATUS_REQ);
				data_ack.hdr.data_len		= sizeof(res_status_t)-sizeof(res_head_t);
				data_ack.res				= ACK;

				if(get_opmode()==AUTO_RUN)
					data_ack.state = 2;
				else if( ((get_opmode()>=MANU_RUN) && (get_opmode()<=PWM_CHANGE)) || (get_opmode()==CURSOR_COLOR) )
					data_ack.state = 3;
				else
					data_ack.state = 1;

				if(get_model_init_status()==NACK)
				{
					sprintf((char*)data_ack.cur_model, "%s", ERR_STRING);
					sprintf((char*)data_ack.cur_pattern, "%s", ERR_STRING);
				}
				else
				{
					sprintf((char*)data_ack.cur_model, "%s", model_name);
					sprintf((char*)data_ack.cur_pattern, "%s", get_pattern_name());


					if(get_pwr_vendor()==0)
					{
						data_ack.vdd = htons(rsp_detect_data[0].det_val.vdd);	// voltage(only value of ch0 - requested by kim joo yong)
						data_ack.vbl = htons(rsp_detect_data[0].det_val.vbl);

						for(i=0; i<ENSIS_PWR_CH; i++)
						{
							data_ack.idd += htons(rsp_detect_data[i].det_val.idd);	// current(sum of ch0 to ch3 - requested by kim joo yong)
							data_ack.ibl += htons(rsp_detect_data[i].det_val.ibl);
						}

						printf("vdd:%d, vbl:%d, idd:%d, ibl:%d\n", data_ack.vdd, data_ack.vbl, data_ack.idd, data_ack.ibl);

						for(i=0; i<ENSIS_PWR_CH; i++)
						{
							if(rsp_detect_data[i].error!=0)
							{
								// 1st check(power)
								data_ack.limit 	= rsp_detect_data[i].error;
								data_ack.vdd	= htons(rsp_detect_data[i].det_val.vdd);
								data_ack.vbl	= htons(rsp_detect_data[i].det_val.vbl);
								data_ack.ocp	= (uint8_t)(i+1);	// power board id(1~4)
								printf("limit(power_ch%d) : 0x%02X \n", i, data_ack.limit);
								break;
							}
						}
					}
					else
					{
						data_ack.vdd = (rsp_detect_osung_data.det_val[0].vdd);
						data_ack.vbl = (rsp_detect_osung_data.det_val[0].vbl);
						data_ack.idd = (rsp_detect_osung_data.det_val[0].idd);
						data_ack.ibl = (rsp_detect_osung_data.det_val[0].ibl);

						printf("vdd:%d, vbl:%d, idd:%d, ibl:%d\n", data_ack.vdd, data_ack.vbl, data_ack.idd, data_ack.ibl);

						if(rsp_detect_osung_data.error[0]!=0)
						{
							data_ack.limit 	= (rsp_detect_osung_data.error[0]);
							data_ack.vdd	= (rsp_detect_osung_data.det_val[0].vdd);
							data_ack.vbl	= (rsp_detect_osung_data.det_val[0].vbl);
							data_ack.ocp 	= 1;
							printf("limit(power) : 0x%02X \n", data_ack.limit);
						}
					}


					// 2nd check(fw)
					if(data_ack.limit==0x00)
					{
//						if(data_ack.vdd<model_data.vdd_l) 	data_ack.limit |= ERR_VDD_MIN;
//						if(data_ack.vdd>model_data.vdd_h) 	data_ack.limit |= ERR_VDD_MAX;
//						if(data_ack.vbl<model_data.vbl_l) 	data_ack.limit |= ERR_VBL_MIN;
//						if(data_ack.vbl>model_data.vbl_h) 	data_ack.limit |= ERR_VBL_MAX;
						if(data_ack.idd<model_data.idd_l*10) 	data_ack.limit |= ERR_IDD_MIN;
						if(data_ack.idd>model_data.idd_h*10) 	data_ack.limit |= ERR_IDD_MAX;
						if(data_ack.ibl<model_data.ibl_l*10) 	data_ack.limit |= ERR_IBL_MIN;
						if(data_ack.ibl>model_data.ibl_h*10) 	data_ack.limit |= ERR_IBL_MAX;

						if(data_ack.limit!=0x00)
						{
							data_ack.ocp = 0x00;	// if it is not ep616p power..
							printf("limit(fw) : 0x%02X \n", data_ack.limit);
						}
					}

					if( (data_ack.limit!=0x00) && (get_onoff_flag()==ENUM_ON) )
					{
						onoff_by_power_seq(ENUM_OFF);
						set_opmode(READY);
						set_onoff_flag(ENUM_OFF);

						for(i=0; i<8; i++)
						{
							if( (data_ack.limit>>i)&0x01 )
							{
								rcb_ready_screen(i+11);
								break;
							}
						}
					}
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_status_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_FREQ_SET_REQ:
			{
				req_freq_set_t data	 	= {0,};

				memset(&data, 0, sizeof(req_freq_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_freq_set_t));
				printf("type=%d, freq=%ld\n", data.type, data.freq);


				freq_set_from_pc(&data);
				gp.freq=data.freq;
				gp.vsync = (ushort)(freq_to_vsync(var_data.freq)*10.0);
			}
			break;
		case SID_VSYNC_SET_REQ:
			{
				req_vsync_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_vsync_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_vsync_set_t));
				printf("vsync : %f\n", data.vsync/10.0);		// data.vsync *10 from communicator

				vsync_set_from_pc(&data);
				gp.vsync=data.vsync;
				gp.freq = var_data.freq;
			}
			break;
		case SID_COLOR_SET_REQ:// Gray change for RCB/SPtech
			{
				req_color_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_color_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_color_set_t));
				printf("color : %d, %d, %d\n", data.red, data.green, data.blue);

				color_set_from_pc(&data);
			}
			break;
		case SID_VDD_SET_REQ:
			{
				req_vdd_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_vdd_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_vdd_set_t));
				printf("vdd : %.2f\n", data.vdd/100.0);

				var_data.vdd = data.vdd;

				pwr_vdd_set(data.vdd, 0);
			}
			break;
		case SID_ADIM_SET_REQ:
			{
				req_adim_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_adim_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_adim_set_t));
				printf("adim : %.1f\n", data.adim/10.0);
				adim_change_flag = 1;
				pwr_adimm_control_set(data.adim, 0);		// -> gp.adim
				printf("adim set %d	in msg\n", data.adim);
//				gp.adim=data.adim;
			}
			break;
		case SID_PDIM_SET_REQ:
			{
				req_pdim_set_t 	data 	= {0,};
				pwm_data_t		pwm		= {0,};

				memset(&data, 0, sizeof(req_pdim_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_pdim_set_t));
				printf("duty : %d%%\n", data.duty);

				memset(&pwm, 0, sizeof(pwm_data_t));
				pwm.freq = model_data.pwm_freq[0];
				pwm.duty = (uint8_t)data.duty;

				pwr_pdimm_control_set(pwm,0,0);
			}
			break;
		case SID_POS_SET_REQ:
			{
				req_pos_set_t data 		= {0,};

				memset(&data, 0, sizeof(req_pos_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_pos_set_t));
				printf("position : (%d,%d), R%d G%d B%d\n", data.x, data.y, data.red, data.green, data.blue);

				pos_set_from_pc(&data);
			}
			break;
		case SID_REBOOT_REQ:
			{
				set_reboot(1);	// reboot
			}
			break;
		case SID_PWR_FWDN_REQ:
			{
				req_pwr_fw_t data	 	= {0,};
				res_pwr_fw_t data_ack 	= {0,};

				memset(&data, 0, sizeof(req_pwr_fw_t));
				memcpy(&data, &rmsg.header, sizeof(req_pwr_fw_t));

				memset(&data_ack, 0, sizeof(res_pwr_fw_t));
				data_ack.hdr.cmd_id		= RES_ID(SID_PWR_FWDN_REQ);
				data_ack.hdr.data_len	= sizeof(res_pwr_fw_t)-sizeof(res_head_t);

				pwr_fw_upload(pwr_fd);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_pwr_fw_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_PWR_OFFSET_REQ:
			{
				offset_start = 1;
				for(i=0; i<ENSIS_PWR_CH; i++) offset_end[i] = 0;
				pwr_offset_req(1);
			}
			break;
		case SID_CYCLE_REQ:
			{
				req_cycle_sel_t data 		= {0,};
				res_cycle_sel_t data_ack 	= {0,};

				memset(&data, 0, sizeof(req_cycle_sel_t));
				memcpy(&data, &rmsg.header, sizeof(req_cycle_sel_t));

				model_update();

				memset(&data_ack, 0, sizeof(res_cycle_sel_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_CYCLE_REQ);
				data_ack.hdr.data_len		= sizeof(uint16_t);
				data_ack.res				= 1;
				//model_select((char*)data.cycle_name, (int)data.use_hando);

				//if(data_ack.res==ACK) set_curmodel_idx((char*)data.model_name);
				//model_variable_reset();

				schedule_list_load();

				if( (data.cycle_name[0] == NULL) || (data.cycle_name[0] == 0) )
				{
					set_schedule_flag(0);
					uint64_t freq_high 	= model_data.freq_high;
					var_data.freq = (freq_high << 32) |  model_data.freq;
					freq_set_func(var_data.freq);

					memset(&schedule_data_new, 0, sizeof(schedule_data_t_new));
					pwr_model_set(&model_data, 0, 0);

				}
				else
				{
					for(i=0; i<MAX_SCHEDULE_CNT; i++)
					{
						if(strncmp(schedule_list[i],data.cycle_name,MAX_CYCLE_NAME)==0)		// schedule name
						{
							if(get_schedule_info(schedule_list[i]))
							{
								set_schedule_flag(1);
								rcb_write(rcb_fd, RCB_LINE1, get_schedule_flag() ? TITLE_SCHEDULE_O : TITLE_SCHEDULE_X);
								schedule_list_index=0;
								rcb_ready_screen(ACK);
								set_opmode(READY);
							}
							break;
						}
					}
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_cycle_sel_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_QEMS_REQ:
			{
//				req_qems_t	data	 		= {0,};
//				res_qems_t 	data_ack 		= {0,};
//				char data_ret[MAX_MSG_BUF] 	= {0,};
//				char alarm_str[128] 		= {0,};
//				uint16_t 	limit			= 0;
//				int32_t 	level			= 0;
//
//				memset(&data_ret, 0, sizeof(data_ret));
//				memset(&data, 0, sizeof(req_qems_t));
//				memcpy(&data, &rmsg.header, sizeof(req_qems_t));
//
//				memset(&data_ack, 0, sizeof(res_qems_t));
//				data_ack.hdr.cmd_id			= RES_ID(SID_QEMS_REQ);
//				data_ack.hdr.data_len		= sizeof(res_qems_t)-sizeof(res_head_t);
//
//				if(get_opmode()==AUTO_RUN)
//					data_ack.state = 0;
//				else if( ((get_opmode()>=MANU_RUN) && (get_opmode()<=PWM_CHANGE)) || (get_opmode()==CURSOR_COLOR) )
//					data_ack.state = 2;
//				else
//					data_ack.state = 1;
//
//				if(get_model_init_status()==NACK)
//				{
//					//sprintf(data_ack.model_name, "");
//					memset(data_ack.model_name,0,sizeof(data_ack.model_name));
//					data_ack.patternid=0;
//					data_ack.alarm=1;
//				}
//				else
//				{
//					sprintf(data_ack.model_name, "%s", model_name);
//					data_ack.patternid=(uint16_t)get_pattern_index();
//
//					data_ack.vdd_set_value=model_data.vdd;
//					data_ack.vbl_set_value=model_data.vbl;
//					if(pattern_change_error>0) data_ack.alarm=2;
//
//					data_ack.time=elapse_qems_aging_time/60;
//
//					if(get_pwr_vendor()==0)
//					{
//						data_ack.vdd_sensing_value = htons(rsp_detect_data[0].det_val.vdd);	// voltage(only value of ch0 - requested by kim joo yong)
//						data_ack.vbl_sensing_value = htons(rsp_detect_data[0].det_val.vbl);
//						for(i=0; i<ENSIS_PWR_CH; i++)
//						{
//							data_ack.idd_sensing_value += htons(rsp_detect_data[i].det_val.idd);	// current(sum of ch0 to ch3 - requested by kim joo yong)
//							data_ack.ibl_sensing_value += htons(rsp_detect_data[i].det_val.ibl);
//						}
//
//						for(i=0; i<ENSIS_PWR_CH; i++)
//						{
//							if(rsp_detect_data[i].error!=0)
//							{
//								// 1st check(power)
//								limit 	= rsp_detect_data[i].error;
//								data_ack.vdd_sensing_value	= htons(rsp_detect_data[i].det_val.vdd);
//								data_ack.vbl_sensing_value	= htons(rsp_detect_data[i].det_val.vbl);
//								printf("qems chk limit(power_ch%d) : 0x%02X \n", i, limit);
//
//								if(data_ack.idd_sensing_value<model_data.idd_l*10) 	data_ack.alarm=5;//ERR_IDD_MIN;
//								if(data_ack.idd_sensing_value>model_data.idd_h*10) 	data_ack.alarm=4;//ERR_IDD_MAX;
//								if(data_ack.ibl_sensing_value<model_data.ibl_l*10) 	data_ack.alarm=7;//ERR_IBL_MIN;
//								if(data_ack.ibl_sensing_value>model_data.ibl_h*10) 	data_ack.alarm=6;//ERR_IBL_MAX;
//								break;
//							}
//						}
//					}
//					else
//					{
//						data_ack.vdd_sensing_value = (rsp_detect_osung_data.det_val[0].vdd);
//						data_ack.vbl_sensing_value = (rsp_detect_osung_data.det_val[0].vbl);
//						data_ack.idd_sensing_value = (rsp_detect_osung_data.det_val[0].idd);
//						data_ack.ibl_sensing_value = (rsp_detect_osung_data.det_val[0].ibl);
//
//						limit 	= rsp_detect_osung_data.error[0];
//						data_ack.vdd_sensing_value	= (rsp_detect_osung_data.det_val[0].vdd);
//						data_ack.vbl_sensing_value	= (rsp_detect_osung_data.det_val[0].vbl);
//						printf("qems chk limit(power) : 0x%02X \n", limit);
//
//						if(data_ack.idd_sensing_value<model_data.idd_l*10) 	data_ack.alarm=5;//ERR_IDD_MIN;
//						if(data_ack.idd_sensing_value>model_data.idd_h*10) 	data_ack.alarm=4;//ERR_IDD_MAX;
//						if(data_ack.ibl_sensing_value<model_data.ibl_l*10) 	data_ack.alarm=7;//ERR_IBL_MIN;
//						if(data_ack.ibl_sensing_value>model_data.ibl_h*10) 	data_ack.alarm=6;//ERR_IBL_MAX;
//					}
//
//					if(data_ack.alarm>0)
//					{
//						switch(data_ack.alarm)
//						{
//							case 1:sprintf(alarm_str,"MODEL FAIL"); break;
//							case 2:sprintf(alarm_str,"PATTERN FAIL"); break;
//							case 3:sprintf(alarm_str,"IDD LOW LIMIT"); break;
//							case 4:sprintf(alarm_str,"IDD HIGH LIMIT"); break;
//							default:memset(alarm_str,0,sizeof(alarm_str)); break;
//						}
//					}
//
//
//
//					#if defined(USE_SDC)
////						data_ack.idd_sensing_value	= idd_value;
////						if(data_ack.idd_sensing_value<model_data.idd_l*10) 	data_ack.alarm=3;//ERR_IDD_MIN;
////						if(data_ack.idd_sensing_value>model_data.idd_h*10) 	data_ack.alarm=4;//ERR_IDD_MAX;
////
////						data_ack.vdd_sensing_value = (rsp_detect_osung_data.det_val[0].vdd);
////						data_ack.vbl_sensing_value = (rsp_detect_osung_data.det_val[0].vbl);
////						//data_ack.idd_sensing_value = (rsp_detect_osung_data.det_val[0].idd);
////						data_ack.ibl_sensing_value = (rsp_detect_osung_data.det_val[0].ibl);
////
////						if(data_ack.idd_sensing_value<model_data.idd_l*10) 	data_ack.alarm=5;//ERR_IDD_MIN;
////						if(data_ack.idd_sensing_value>model_data.idd_h*10) 	data_ack.alarm=4;//ERR_IDD_MAX;
////						if(data_ack.ibl_sensing_value<model_data.ibl_l*10) 	data_ack.alarm=7;//ERR_IBL_MIN;
////						if(data_ack.ibl_sensing_value>model_data.ibl_h*10) 	data_ack.alarm=6;//ERR_IBL_MAX;
//
//					#else
//
//						data_ack.vdd_sensing_value = htons(rsp_detect_data[0].det_val.vdd);	// voltage(only value of ch0 - requested by kim joo yong)
//						data_ack.vbl_sensing_value = htons(rsp_detect_data[0].det_val.vbl);
//
//						for(i=0; i<ENSIS_PWR_CH; i++)
//						{
//							data_ack.idd_sensing_value += htons(rsp_detect_data[i].det_val.idd);	// current(sum of ch0 to ch3 - requested by kim joo yong)
//							data_ack.ibl_sensing_value += htons(rsp_detect_data[i].det_val.ibl);
//						}
//
//						for(i=0; i<ENSIS_PWR_CH; i++)
//						{
//							if(rsp_detect_data[i].error!=0)
//							{
//								// 1st check(power)
//								limit 	= rsp_detect_data[i].error;
//								data_ack.vdd_sensing_value	= htons(rsp_detect_data[i].det_val.vdd);
//								data_ack.vbl_sensing_value	= htons(rsp_detect_data[i].det_val.vbl);
//								printf("limit(power_ch%d) : 0x%02X \n", i, limit);
//
//								if(data_ack.idd_sensing_value<model_data.idd_l*10) 	data_ack.alarm=5;//ERR_IDD_MIN;
//								if(data_ack.idd_sensing_value>model_data.idd_h*10) 	data_ack.alarm=4;//ERR_IDD_MAX;
//								if(data_ack.ibl_sensing_value<model_data.ibl_l*10) 	data_ack.alarm=7;//ERR_IBL_MIN;
//								if(data_ack.ibl_sensing_value>model_data.ibl_h*10) 	data_ack.alarm=6;//ERR_IBL_MAX;
//								break;
//							}
//						}
//					#endif
//
//
//				}
//
////				if(data_ack.alarm>0)
////				{
////					switch(data_ack.alarm)
////					{
////						case 1:sprintf(alarm_str,"MODEL FAIL"); break;
////						case 2:sprintf(alarm_str,"PATTERN FAIL"); break;
////						case 3:sprintf(alarm_str,"IDD LOW LIMIT"); break;
////						case 4:sprintf(alarm_str,"IDD HIGH LIMIT"); break;
////						default:memset(alarm_str,0,sizeof(alarm_str)); break;
////					}
////				}
//#if 0
//				if((model_data.mode&0x0030)==0x00)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//					color_bit=6;
//				else if((model_data.mode&0x0030)==0x10)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//					color_bit=8;
//				else if((model_data.mode&0x0030)==0x20)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//					color_bit=10;
//#endif
//
//				//20181030 Normal 8bit(8),Normal 10bit(10),jeida 8bit(12),jeida 10bit(14),vesa 8bit(16),vesa 10bit(18)
//#if 0
//				if((model_data.mode&0x0030)==0x00)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//				{
//					if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
//						color_bit=0; //6bit Jeida
//					else color_bit=0;//6bit vesa
//
//				}
//				else if((model_data.mode&0x0030)==0x10)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//				{
//					if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
//						color_bit=12; //8bit Jeida
//					else color_bit=16;//8bit vesa
//				}
//				else if((model_data.mode&0x0030)==0x20)// Bit[5:4]:0-6bit,1-8bit,2-10bit
//				{
//					if((model_data.mode&0x0100)==0x00)// Bit[9:8]:0-jeida,1-vesa
//						color_bit=14; //10bit Jeida
//					else color_bit=18;//10bit vesa
//				}
//#endif
//
///*
//				//20181030 fix
//				sprintf(data_ret,"eqpid=%s,20001=%d,30001=%s,40001=%d,40002=%s,50001=%d,60001=%.1f,70001=%.1f,80001=%.1f,90001=%.1f,100001=%d,110001=%d,120001=%ld,130001=%dx%d,140001=%d,150001=%d,600001=%.1f,220036=%.1f,220038=%d,230001=%.1f,230002=%.1f",
//						qems_eqpid,
//						data_ack.patternid+1,
//						data_ack.model_name,
//						data_ack.alarm,
//						alarm_str,
//						data_ack.state,
//						(float)(data_ack.vdd_set_value/100.0),
//						(float)(data_ack.vbl_set_value/100.0),
//						(float)(data_ack.vdd_sensing_value/100.0),
//						(float)(data_ack.vbl_sensing_value/100.0),
//						data_ack.idd_sensing_value,
//						data_ack.ibl_sensing_value,
//						data_ack.time,
//						model_data.h_active,model_data.v_active,
//						gp.bittype,
//						(int)((data_ack.state==1) ? 1 : 0),
//						(float)(FW_VERSION/10.0),
//						(float)(gp.adim/10.0),
//						gp.port,
//						(float)(gp.freq*0.000001),
//						(float)(gp.vsync/10.0));
//*/
//				int mode8k=get_quhd_enable();
////				enum_qid_t master=get_qid();
//				level=get_wifi_level_int();
//
//				sprintf(data_ret,"eqpid=%s,20001=%d,30001=%s,40001=%d,40002=%s,50001=%d,60001=%.1f,70001=%.1f,80001=%.1f,90001=%.1f,100001=%d,110001=%d,120001=%ld,130001=%dx%d,140001=%d,150001=%d,600001=%.1f,220036=%.1f,220038=%d,230001=%.1f,230002=%.1f,400001=%d,12345=%d,700001=%02X:%02X:%02X:%02X:%02X:%02X,160001=%s,600002=%s",
//					qems_eqpid,
//					data_ack.patternid+1,
//					data_ack.model_name,
//					data_ack.alarm,
//					alarm_str,
//					data_ack.state,
//					(float)(data_ack.vdd_set_value/100.0),
//					(float)(data_ack.vbl_set_value/100.0),
//					(float)(data_ack.vdd_sensing_value/100.0),
//					(float)(data_ack.vbl_sensing_value/100.0),
//					data_ack.idd_sensing_value,
//					data_ack.ibl_sensing_value,
//					data_ack.time,
//					model_data.h_active,model_data.v_active,
//					gp.bittype,
//					(int)((data_ack.state==1) ? 1 : 0),
//					(float)(FW_VERSION/10.0),
//					(float)(gp.adim/10.0),
//					gp.port,
//					(float)freq_to_mhz(gp.freq),
//					(float)(gp.vsync/10.0),
//					mode8k,
//					level,
//					gp.wifi_mac[0]&0xFF,
//					gp.wifi_mac[1]&0xFF,
//					gp.wifi_mac[2]&0xFF,
//					gp.wifi_mac[3]&0xFF,
//					gp.wifi_mac[4]&0xFF,
//					gp.wifi_mac[5]&0xFF,
//					group_name,
//					PG_NAME
//				);
//
//				if(gp.model_change==0)
//				{
//					for(i=0;i<group_data.hdr.count;i++)
//					{
//						strcat(data_ret,",");
//						if(i==0) strcat(data_ret,"\"800001=");
//						strcat(data_ret,(char *)group_data.pat[i].name);
//					}
//					strcat(data_ret,"\"");
//				}
//
//				int ret_size=strlen(data_ret);
//				data_ret[ret_size]=0x0A;
//				memset(&smsg, 0, sizeof(msg_data_t));
//				smsg.header.data_len=ret_size+1;
//				memcpy(&smsg.buf, &data_ret,ret_size+1);
//				msq_send_comm(&smsg);
			}
			break;

		case SID_CHANNEL_SHIFT_SET_REQ:
			{
				req_channel_shift_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_channel_shift_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_channel_shift_set_t));
				printf("channel_shift : %d\n", data.channel_shift);
				//set_mode_by_twist(var_data.twist);
				set_portmap(data.channel_shift);
				gp.port=data.channel_shift;
			}
			break;
		case SID_BIT_TYPE_SET_REQ:
			{
				req_bit_type_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_bit_type_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_bit_type_set_t));
				printf("bit type : %d\n", data.type);

				set_mode_bit_type_qems(data.type);
				gp.bittype=data.type;
			}
			break;
		case SID_VBL_SET_REQ:
			{
				req_vbl_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_vbl_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_vbl_set_t));
				printf("vbl : %.2f\n", data.vbl/100.0);

				pwr_vbl_set(data.vbl, 0);

			}
			break;

		case SID_CNT_I2C_SEL_REQ:
			{
				req_ext_i2c_sel_t	data 	= {0,};
				uint16_t	sel=0;

				memset(&data, 0, sizeof(req_ext_i2c_sel_t));
				memcpy(&data, &rmsg.header, sizeof(req_ext_i2c_sel_t));
				printf("ext i2c channel : %d\n", data.channel);
				sel = data.channel;

				if ( sel==0 ){
					i2c_gpio_set(GPIO_EX34, 0x00);
					i2c_gpio_set(GPIO_EX35, 0x00);
//					printf("ext i2c ch sel0\n");
				}
				else if( sel==1 ){
					i2c_gpio_set(GPIO_EX34, 0x01);
					i2c_gpio_set(GPIO_EX35, 0x00);
//					printf("ext i2c ch sel1\n");
				}
				else if( sel==2 ){
					i2c_gpio_set(GPIO_EX34, 0x00);
					i2c_gpio_set(GPIO_EX35, 0x01);
//					printf("ext i2c ch sel2\n");
				}
				else if( sel==3 ){
					i2c_gpio_set(GPIO_EX34, 0x01);
					i2c_gpio_set(GPIO_EX35, 0x01);
//					printf("ext i2c ch sel3\n");
				}
				else{
					i2c_gpio_set(GPIO_EX34, 0x00);
					i2c_gpio_set(GPIO_EX35, 0x00);
				}
			}
			break;
		case SID_CNT_VCC_CTRL_REQ:
			{
				req_onoff_ctrl_t data 	= {0,};

				memset(&data, 0, sizeof(req_onoff_ctrl_t));
				memcpy(&data, &rmsg.header, sizeof(req_onoff_ctrl_t));
				printf("pwr ctrl : %d, 0x%01x\n", data.on_off, data.type);

				if( data.on_off == ENUM_ON )		// on
				{
					if( (data.type&0x01)==0x01 ){	//vdd
						pwr_vdd_onoff_set(data.on_off);
						vdd_onoff_control(data.on_off);
						pwr_onoff_status |= 0x0001;
					}
					if( (data.type&0x02)==0x02 ){	//vbl
						pwr_vbl_onoff_set(data.on_off);
						pwr_onoff_status |= 0x0002;
					}
					if( (data.type&0x04)==0x04 ){	//sig
						pattern_change(get_pattern_index());
						FPGA_Write(FPGA_VX1_SIG_ONOFF, data.on_off);
						pwr_onoff_status |= 0x0004;
					}
					if( (data.type&0x08)==0x08 ){	//bl_on
						pwr_bl_onoff_set(data.on_off);
						pwr_onoff_status |= 0x0008;
					}


					if ( pwr_onoff_status>0 )
					{
						reset_schedule_func();
						rcb_write(rcb_fd, RCB_LEDONOFF, LED_ON_ONOFF);
						set_onoff_flag(ENUM_ON);
						set_opmode(MANU_RUN);
					}
				}
				else	//off
				{
					if( (data.type&0x01)==0x01 ){	//vdd
						pwr_vdd_onoff_set(data.on_off);
						vdd_onoff_control(data.on_off);
						pwr_onoff_status &= 0xfffe;
					}
					if( (data.type&0x02)==0x02 ){	//vbl
						pwr_vbl_onoff_set(data.on_off);
						pwr_onoff_status &= 0xfffd;
					}
					if( (data.type&0x04)==0x04 ){	//sig
						FPGA_Write(FPGA_VX1_SIG_ONOFF, data.on_off);
						if(get_schedule_flag()==0) set_pattern_index(0);
						pwr_onoff_status &= 0xfffb;
					}
					if( (data.type&0x08)==0x08 ){	//bl_on
						pwr_bl_onoff_set(data.on_off);
						pwr_onoff_status &= 0xfff7;
					}

					if( pwr_onoff_status==0 )
					{
						rcb_ready_screen(ACK);
						set_opmode(READY);
						set_onoff_flag(ENUM_OFF);
						pwr_model_set(&model_data, 0, 0);
					}
				}
			}
			break;
		case SID_IND_COLOR_SET_REQ: //Dong-A protocol
			{
				req_ind_color_set_t data 	= {0,};
				int index=0;

				memset(&data, 0, sizeof(req_ind_color_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_ind_color_set_t));

				index = get_pattern_index();

				if(preload_data[index].indirect.colormode > 0)
				{
					if(data.pal==0)
					{
						FPGA_Write(FPGA_PAL0_R, (data.red)>>2);
						FPGA_Write(FPGA_PAL0_G, (data.green)>>2);
						FPGA_Write(FPGA_PAL0_B, (data.blue)>>2);
					}
					else if(data.pal==1)
					{
						FPGA_Write(FPGA_PAL1_R, (data.red)>>2);
						FPGA_Write(FPGA_PAL1_G, (data.green)>>2);
						FPGA_Write(FPGA_PAL1_B, (data.blue)>>2);
					}
					else if(data.pal==2)
					{
						FPGA_Write(FPGA_PAL2_R, (data.red)>>2);
						FPGA_Write(FPGA_PAL2_G, (data.green)>>2);
						FPGA_Write(FPGA_PAL2_B, (data.blue)>>2);
					}
					else
					{
					}
//					switch(model_data.mode & 0xf){
//					case MODE_HEXA :
//						if(data.pal==0)
//						{
//							FPGA_Write(FPGA_PAL0_R, (data.red));
//							FPGA_Write(FPGA_PAL0_G, (data.green));
//							FPGA_Write(FPGA_PAL0_B, (data.blue));
//						}
//						else if(data.pal==1)
//						{
//							FPGA_Write(FPGA_PAL1_R, (data.red));
//							FPGA_Write(FPGA_PAL1_G, (data.green));
//							FPGA_Write(FPGA_PAL1_B, (data.blue));
//						}
//						else if(data.pal==2)
//						{
//							FPGA_Write(FPGA_PAL2_R, (data.red));
//							FPGA_Write(FPGA_PAL2_G, (data.green));
//							FPGA_Write(FPGA_PAL2_B, (data.blue));
//						}
//						else{
//						}
//						break;
//					default :
//						if(data.pal==0)
//						{
//							FPGA_Write(FPGA_PAL0_R, (data.red)>>2);
//							FPGA_Write(FPGA_PAL0_G, (data.green)>>2);
//							FPGA_Write(FPGA_PAL0_B, (data.blue)>>2);
//						}
//						else if(data.pal==1)
//						{
//							FPGA_Write(FPGA_PAL1_R, (data.red)>>2);
//							FPGA_Write(FPGA_PAL1_G, (data.green)>>2);
//							FPGA_Write(FPGA_PAL1_B, (data.blue)>>2);
//						}
//						else if(data.pal==2)
//						{
//							FPGA_Write(FPGA_PAL2_R, (data.red)>>2);
//							FPGA_Write(FPGA_PAL2_G, (data.green)>>2);
//							FPGA_Write(FPGA_PAL2_B, (data.blue)>>2);
//						}
//						else{
//						}
//						break;
//
//					} RESERVED
					printf("indirect palette : pal%d %d %d %d\n", data.pal, data.red, data.green, data.blue);
				}
				else
				{
				}
			}
			break;

		case SID_VRR_REQ:
			{
				req_vrr_t data 	= {0,};

				uint32_t	vtotal=0, v_ref=0;

				memset(&data, 0, sizeof(req_vrr_t));
				memcpy(&data, &rmsg.header, sizeof(req_vrr_t));

				vtotal = (data.v_total) & 0x0000ffff;

				v_ref = model_data.v_active + model_data.v_width + model_data.v_bpo;		// auto calculation v_fporch in fpga sync gen

				if(vtotal < v_ref)	var_data.v_total = v_ref;
				else				var_data.v_total = vtotal;

				vrr_set_for_donga(&var_data);
			}
			break;

		case SID_PRE_EMPHASIS_REQ:
			{
				req_pre_emphasis_t data 	= {0,};

				memset(&data, 0, sizeof(req_pre_emphasis_t));
				memcpy(&data, &rmsg.header, sizeof(req_pre_emphasis_t));

				printf("[PC] pre_emphasis : vod=%d, 1post=%d, 2post=%d, 1pre=%d, 2pre=%d\n", data.vod, data.post_1st, data.post_2nd, data.pre_1st, data.pre_2nd);

				pre_emphasis_set_from_pc(data.vod, data.post_1st, data.post_2nd, data.pre_1st, data.pre_2nd);
			}
			break;

		case SID_COLOR_CURSOR_REQ:
			{
				req_color_cursor_t data 	= {0,};

				memset(&data, 0, sizeof(req_color_cursor_t));
				memcpy(&data, &rmsg.header, sizeof(req_color_cursor_t));

				printf("[PC] color cursor : x=%d, y=%d, r=%d, g=%d, b=%d\n", data.x, data.y, data.red, data.green, data.blue);

				color_cursor_from_pc(&data);
			}
			break;

		case SID_RGB_DISABLE_REQ:
			{
				req_rgb_disable_set_t 	data 	= {0,};
				uint8_t					dis=0;

				memset(&data, 0, sizeof(req_rgb_disable_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_rgb_disable_set_t));

				dis = (unsigned char)((~data.disable)&0x00000007);

				printf("[PC] rgb disable : 0x%02x\n", dis);

				gray_color_set(dis);
			}
			break;
		case SID_SCROLL_SET_REQ:
			{
				req_pattern_scroll_set_t data 	= {0,};

				memset(&data, 0, sizeof(req_pattern_scroll_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_pattern_scroll_set_t));

				if( (data.speed<1) || (data.speed>99) )	data.speed = 1;

				if(data.type==1)	set_pattern_scroll(data.direction,1,data.speed);
				else				set_pattern_scroll(0,1,1);
			}
			break;

//		case SID_GAMMA_SET_REQ:
//			{
//				req_gamma_set_t data 	= {0,};
//
//				memset(&data, 0, sizeof(req_gamma_set_t));
//				memcpy(&data, &rmsg.header, sizeof(req_gamma_set_t));
////					printf("msg gamma data = %d %d %d\n", data.red, data.green, data.blue);
//
//				gamma_change(data.red, data.green, data.blue);
//			}
//			break;
		case SID_BPORCH_SET_REQ:
			{
				req_bpo_set_t data={0,};
				uint32_t	flag=0;
				uint16_t	pre_data=0;

				memset(&data, 0, sizeof(req_bpo_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_bpo_set_t));

				printf("[PC] bporch set data = %d\n", data.bpo);

				flag=(data.type & 0x0001);
				if(flag>0)
				{
					pre_data=var_data.v_total - var_data.v_width - var_data.v_active - var_data.v_fpo;
					if (data.bpo > pre_data)	var_data.v_total += (data.bpo - pre_data);
					else						var_data.v_total -= (pre_data - data.bpo);

					var_data.v_bpo = data.bpo;
				}
				else
				{
					pre_data=var_data.h_total - var_data.h_width - var_data.h_active - var_data.h_fpo;
					if (data.bpo > pre_data)	var_data.h_total += (data.bpo - pre_data);
					else						var_data.h_total -= (pre_data - data.bpo);

					var_data.h_bpo = data.bpo;
				}

				bporch_set_func(flag, &var_data);
			}
			break;
		case SID_FPORCH_SET_REQ:
			{
				req_fpo_set_t data={0,};
				uint32_t	flag=0;
				uint16_t	pre_data=0;

				memset(&data, 0, sizeof(req_fpo_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_fpo_set_t));

				printf("[PC] fporch set data = %d\n", data.fpo);

				flag=(data.type & 0x0001);
				if(flag>0)
				{
					pre_data=var_data.v_total - var_data.v_width - var_data.v_bpo - var_data.v_active;
					if (data.fpo > pre_data)	var_data.v_total += (data.fpo - pre_data);
					else						var_data.v_total -= (pre_data - data.fpo);

					var_data.v_fpo = data.fpo;
				}
				else
				{
					pre_data=var_data.h_total - var_data.h_width - var_data.h_bpo - var_data.h_active;
					if (data.fpo > pre_data)	var_data.h_total += (data.fpo - pre_data);
					else						var_data.h_total -= (pre_data - data.fpo);

					var_data.h_fpo = data.fpo;
				}

				fporch_set_func(flag, &var_data);

			}
			break;
		case SID_WIDTH_SET_REQ:
			{
				req_width_set_t data={0,};
				uint32_t	flag=0;
				uint16_t	pre_data=0;

				memset(&data, 0, sizeof(req_width_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_width_set_t));

				printf("[PC] width set data = %d\n", data.width);

				flag=(data.type & 0x0001);
				if(flag>0)
				{
					pre_data=var_data.v_total - var_data.v_bpo - var_data.v_active - var_data.v_fpo;
					if (data.width > pre_data)	var_data.v_total += (data.width - pre_data);
					else						var_data.v_total -= (pre_data - data.width);

					var_data.v_width = data.width;
				}
				else
				{
					pre_data=var_data.h_total - var_data.h_bpo - var_data.h_active - var_data.h_fpo;
					if (data.width > pre_data)	var_data.h_total += (data.width - pre_data);
					else						var_data.h_total -= (pre_data - data.width);

					var_data.h_width = data.width;
				}

				width_set_func(flag, &var_data);
			}
			break;
		case SID_STD_ACTIVE_SET_REQ:
			{
				req_active_set_t data={0,};
				uint32_t	flag=0;
				uint16_t	pre_data=0;

				memset(&data, 0, sizeof(req_active_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_active_set_t));

				printf("[PC] active set data = %d\n", data.active);

				flag=(data.type & 0x0001);
				if(flag>0)
				{
					if(data.active > 1)	var_data.v_active = data.active;
					else				var_data.v_active = 1;

					var_data.v_total = var_data.v_width + var_data.v_bpo + var_data.v_active + var_data.v_fpo;
				}
				else
				{
					if(data.active > 1)	var_data.h_active = data.active;
					else				var_data.h_active = 1;

					var_data.h_total = var_data.h_width + var_data.h_bpo + var_data.h_active + var_data.h_fpo;
				}

				active_set_func(flag, &var_data);
			}
			break;
		case SID_INST_IND_PAT_ONOFF_REQ:
			{
				req_inst_ind_pat_onoff_t	data={0,};

				memcpy(&data, &rmsg.header, sizeof(req_inst_ind_pat_onoff_t));

				if( (FPGA_Read(FPGA_MEM_WR_CTRL)&0x0001)==0 )	//Check MOV write flag
				{
					printf("Instant indirect pattern draw: start-h=%d, start-v=%d, width=%d, height=%d\n", data.start_h, data.start_v, data.width, data.height);
					inst_ind_pat_draw(&data);					//Dangerous when playing video !!
				}
				else printf("Instant indirect pattern draw: Failed. Video Playing\n");
			}
			break;
		case SID_INST_IND_PAT_COLOR_SET_REQ: //Gray change for DAVINCI
			{
				req_inst_ind_pat_color_t	data={0,};

				memcpy(&data, &rmsg.header, sizeof(req_inst_ind_pat_color_t));

				inst_ind_pat_color(&data);//11bit 2023.03.14 test ksk

//				req_color_set_t data 	= {0,};
//
//				memset(&data, 0, sizeof(req_color_set_t));
//				memcpy(&data, &rmsg.header, sizeof(req_color_set_t));
//				printf("color : %d, %d, %d\n", data.red, data.green, data.blue);
//
//				color_set_from_pc(&data);
			}
			break;
		case SID_VRR_TEST_REQ:
			{
				req_vrr_test_t data		= {0,};
				uint16_t	scnt=2, i=0;
				memset(&data, 0, sizeof(req_vrr_test_t));
				memcpy(&data, &rmsg.header, sizeof(req_vrr_test_t));

				if(data.step<2)	scnt=2;
				else if(data.step>200)	scnt=200;
				else	scnt=data.step;

				if(data.onoff==1)
				{
					FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0008); // Gray 30 step issue 24.02.21
					printf("VRR step: %d\n",data.step);
					FPGA_VRR_DATA_Write(FPAG_VRR_GRAY_SEL, 0); //Gray select: off
					if(FPGA_Read(FPGA_MEM_WR_CTRL) & 0x4000)
					{
						printf("FPGA_MEM_WR_CTRL 3 : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
						FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x4000);
					}
					for(i=0;i<scnt;i++)
					{
						if(i < 10) {
							FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal[i]);
							FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num[i]);
							printf("%d VRR Total: %d\n",i ,FPGA_VRR_DATA_Read(1+i));
							printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read(101+i));
						}
						else if(i>=10 && i<100) {
							FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal_ex[i-10]);
							FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num_ex[i-10]);
							printf("%d VRR Total: %d\n",i , FPGA_VRR_DATA_Read(1+i));
							printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read(101+i));
						}
						else // 100~199
						{
							FPGA_VRR_DATA_Write_EX(FPGA_VRR_V_TOTAL_EX+i-100, data.vtotal_ex[i-10]);
							FPGA_VRR_DATA_Write_EX(FPGA_VRR_FRAME_EX+i-100, data.frame_num_ex[i-10]);
							printf("%d VRR Total: %d\n",i , FPGA_VRR_DATA_Read_EX(i-100));
							printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read_EX(i));
						}
					}
					FPGA_VRR_DATA_Write(FPGA_VRR_STEP, scnt);

					FPGA_ANDOR_SET(FPGA_MEM_WR_CTRL, 0x000a, 0x4000);// VRR gray issue 24.02.23
//					FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x4000);
					printf("FPGA_MEM_WR_CTRL 4 : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
					printf("vrr test on\n");
					/*
					for(i=0;i<scnt;i++) {
						printf("%3rd : total : %5d, frame : %5d\n", i, (unsigned short)FPGA_VRR_DATA_Read(FPGA_VRR_V_TOTAL+i), (unsigned short)FPGA_VRR_DATA_Read(FPGA_VRR_FRAME+i));
					}
					*/
				}
				else if(data.onoff==2) // vrr gray pattern 23.12.04 ksk
				{
					printf("VRR Gray step: %d\n",data.step);
					FPGA_VRR_DATA_Write(FPAG_VRR_GRAY_SEL, 1);
					if(FPGA_Read(FPGA_MEM_WR_CTRL) & 0x4000)
					{
						printf("FPGA_MEM_WR_CTRL 0 : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
						FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x4000); //VRR En Off
					}
					for(i=0; i<scnt; i++) {
//						FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0008); // Gray 30 step issue 24.02.21
						if((model_data.mode & 0x4) && (model_data.use_dvi == 0)) {//hexa vx1 model only
							if(i<10){
//								printf("TP1\n");
								FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal[i]);
								FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num[i]);
								printf("%d VRR Total: %d\n",i ,FPGA_VRR_DATA_Read(1+i));
								printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read(101+i));
							}
							else if(i<100){
//								printf("TP2\n");
								FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal_ex[i-10]);
								FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num_ex[i-10]);
								printf("%d VRR Total: %d\n",i , FPGA_VRR_DATA_Read(1+i));
								printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read(101+i));
							}
							else{
//								printf("TP3\n");
								FPGA_VRR_DATA_Write_EX(FPGA_VRR_V_TOTAL_EX+i-100, data.vtotal_ex[i-10]);
								FPGA_VRR_DATA_Write_EX(FPGA_VRR_FRAME_EX+i-100, data.frame_num_ex[i-10]);
								printf("%d VRR Total: %d\n",i , FPGA_VRR_DATA_Read_EX(i-100));
								printf("%d VRR Frame Number: %d\n",i , FPGA_VRR_DATA_Read_EX(i));
							}
							FPGA_VRR_GRAY_R_Write(i,data.red[i]);
							FPGA_VRR_GRAY_G_Write(i,data.green[i]);
							FPGA_VRR_GRAY_B_Write(i,data.blue[i]);
						}
						else{
							if(i<10){
//								printf("TP4\n");
								FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal[i]);
								FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num[i]);
							}
							else if(i<100){
//								printf("TP5\n");
								FPGA_VRR_DATA_Write(FPGA_VRR_V_TOTAL+i, data.vtotal_ex[i-10]);
								FPGA_VRR_DATA_Write(FPGA_VRR_FRAME+i, data.frame_num_ex[i-10]);
							}
							else{
//								printf("TP6\n");
								FPGA_VRR_DATA_Write_EX(FPGA_VRR_V_TOTAL_EX+i-100, data.vtotal_ex[i-10]);
								FPGA_VRR_DATA_Write_EX(FPGA_VRR_FRAME_EX+i-100, data.frame_num_ex[i-10]);
							}
							FPGA_VRR_GRAY_R_Write(i,data.red[i]>>2);
							FPGA_VRR_GRAY_G_Write(i,data.green[i]>>2);
							FPGA_VRR_GRAY_B_Write(i,data.blue[i]>>2);
						}
					}

					FPGA_VRR_DATA_Write(FPGA_VRR_STEP, scnt);
					FPGA_ANDOR_SET(FPGA_MEM_WR_CTRL, 0x0a0f, 0x45c0); // VRR gray issue 24.02.23
//					FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x4000);// VRR En On
					printf("FPGA_MEM_WR_CTRL 1 : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
					printf("vrr gray test on\n");
				}
				else
				{
					FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x4000); //VRR En Off
					FPGA_VRR_DATA_Write(FPAG_VRR_GRAY_SEL, 0);
					printf("FPGA_MEM_WR_CTRL 2 : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
					printf("vrr test off\n");
				}
			}
			break;

		case SID_STD_IP_PWR_ONOFF_REQ:
			{
				req_ip_pwr_onoff_t data		= {0,};

				memset(&data, 0, sizeof(req_ip_pwr_onoff_t));
				memcpy(&data, &rmsg.header, sizeof(req_ip_pwr_onoff_t));


#ifdef EXTERNAL_POWER_CTRL
//				uint16_t	pwr_id, pwr_ch, volt, curr, onoff;
				pwr_ctrl_vol_cur(data.pwr_id, data.pwr_ch, data.voltage, data.current);
				pwr_ctrl_onoff(data.pwr_id, data.pwr_ch, data.onoff);
#else
				std_power_volt_curr_set(&data);
				std_power_onoff_set(data.pwr_id, data.pwr_ch, data.onoff);
#endif

			}
			break;
		case SID_STD_IP_PWR_ADDR_SET_REQ:
			{
				req_ip_pwr_addr_set_t data	= {0,};
				req_pwrc_ip_addr_set_t pwrc_data ={0,};

				memset(&data, 0, sizeof(req_ip_pwr_addr_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_ip_pwr_addr_set_t));

				memset(power1_ip_info, 0, MAX_IP_ADDR);
				memset(power2_ip_info, 0, MAX_IP_ADDR);
				sprintf(power1_ip_info, "%s", data.pwr1_addr);
				sprintf(power2_ip_info, "%s", data.pwr2_addr);
				set_power_ip(power1_ip_info, power2_ip_info);

#ifdef EXTERNAL_POWER_CTRL
				pwr_ip_changed=1;
#else
				//to PWRC
				memset(&pwrc_data, 0, sizeof(req_pwrc_ip_addr_set_t));
				pwrc_data.hdr.cmd_id = SID_PWR_CTRL_IP_SET_REQ;
				pwrc_data.hdr.data_len = sizeof(req_pwrc_ip_addr_set_t)-sizeof(res_head_t);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &pwrc_data, sizeof(req_pwrc_ip_addr_set_t));
				msq_send_pwrc(&smsg);
#endif
			}
			break;
		case SID_STD_VBY1_CHARA_SET_REQ:
			{
				req_std_vby1_chara_set_t data		= {0,};
				uint16_t	buf=0;

				memset(&data, 0, sizeof(req_std_vby1_chara_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_vby1_chara_set_t));

				printf("vby1 chara: ch=0x%X type=0x%X value=0x%X\n", data.ch, data.type, data.value);

				if(data.type==0) {	//VOD
					buf =((data.ch<<8)&0xff00);
					buf |= (unsigned short)((data.value/100)&0x001f);
					printf("vod=0x%04X\n", buf);
					FPGA_Write(FPGA_XCVR_VOD, buf);
				}
				else if(data.type==1) {		//Pre-emp (1st Post tap)
					buf=((data.ch<<8)&0xff00);
					if((data.value)&0x80000000){
						buf |= (unsigned short)((((~data.value)+1)/100)&0x001f);
						buf |= 0x0040;	//negative polar bit
					}
					else {
						buf |= (unsigned short)((data.value/100)&0x001f);
						buf &= 0xffbf;
					}
//					printf("pre-emp=0x%04X\n", buf);
					FPGA_Write(FPGA_XCVR_1POST_TAP, buf);
				}
				else if(data.type==2) {		//skew (bitsliip)
					buf = ((data.ch<<8)&0xff00);
					buf |= (unsigned short)((data.value/100)&0x001f);
//					printf("skew=0x%04X\n", buf);
					FPGA_Write(FPGA_BIT_SLIP, buf);
				}
			}
			break;
		case SID_STD_SSCG_SET_REQ:
			{
				req_std_sscg_test_set_t data		= {0,};
				uint16_t	spr=0, rate=0;
				memset(&data, 0, sizeof(req_std_sscg_test_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_sscg_test_set_t));

//				if(data.ratio < 1)		spr=1;
//				else if(data.ratio>250)	spr=250;
//				else					spr=data.ratio;		//1~250 = 0.01% ~ 2.50%
				spr=data.ratio;		//0~250 = 0, 0.01% ~ 2.50%	(value 1 = 0.01%)

//				if(data.mf<30000)		rate=30000;
//				else if(data.mf>31500)	rate=31500;
//				else					rate=data.mf;		//30KHz~31.5KHz = 30000 ~ 31500
				rate=data.mf;		//30KHz~31.5KHz = 30000 ~ 31500

				FPGA_Write(FPGA_SSC_AMPLITUDE, spr);
				FPGA_Write(FPGA_SSC_RATE, rate);

				printf("sscg set: %d, %d\n", spr, rate);
			}
			break;
		case SID_STD_PWR_SEQ_SET_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C
				req_std_pwr_seq_test_t data		= {0,};

				memset(&data, 0, sizeof(req_std_pwr_seq_test_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_pwr_seq_test_t));//Pseq start
				/* 2022.08.25 ksk i2c 2 pwr seq */
				if ((data.onoff& 0x01)>0)	flag_i2c_1_on_en=1;
				else flag_i2c_1_on_en=0;
				if ((data.onoff & 0x02)>0)	flag_i2c_1_off_en=1;
				else flag_i2c_1_off_en=0;
				if ((data.onoff & 0x04)>0)	flag_i2c_2_on_en=1;
				else flag_i2c_2_on_en=0;
				if ((data.onoff & 0x08)>0)	flag_i2c_2_off_en=1;
				else flag_i2c_2_off_en=0;
				printf("flag 1 on en: %d\n", 	flag_i2c_1_on_en);
				printf("flag 1 off en: %d\n", 	flag_i2c_1_off_en);
				printf("flag 2 on en: %d\n", 	flag_i2c_2_on_en);
				printf("flag 2 off en: %d\n", 	flag_i2c_2_off_en);
				if(std_power_seq_run==0)
				{
					std_power_seq_i2c_set(&data);
					std_power_sequence(&data);
					set_std_pseq_power_start_time();
				}
				else printf("Pseq set Req - running already\n");
			}
			break;
		case SID_STD_ROLLING_SET_REQ:
			{
				req_std_rolling_set_set_t data		= {0,};

				memset(&data, 0, sizeof(req_std_rolling_set_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_rolling_set_set_t));

				STD_Rolling_func(&data);
			}
			break;
		case SID_STD_I2C_WR_TEST_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C
				req_std_i2c_test_wr_t data		= {0,};
				memset(&data, 0, sizeof(req_std_i2c_test_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_i2c_test_wr_t));

				std_i2c_test_fpga_set(data, 0);		//write
			}
			break;

		case SID_STD_ACDET_SET_REQ:
			{
				req_std_acdet_set_t data		= {0,};
				memset(&data, 0, sizeof(req_std_acdet_set_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_acdet_set_t));

				printf("acdet set=%d\n", data.onoff);
				if(data.onoff==1)	ext_cnt_vdd_sel(1);	//0=gnd, 1=3.3V
				else				ext_cnt_vdd_sel(0);	//0=gnd, 1=3.3V
			}
			break;

		case SID_STD_PWR_SEQ_ONOFF_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C
				req_std_pseq_onoff_t data		= {0,};
				memset(&data, 0, sizeof(req_std_pseq_onoff_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_pseq_onoff_t));

//				printf("pseq onoff=%s\n", (data.onoff==1) ? "on" : "off");

				if(data.onoff==1)
				{
					std_power_paused=0;
//					scenario_cnt=scenario_cnt_temp;
					if(std_power_seq_run==0)
					{
						if(pseq_set_scnt>0)
						{
							std_power_seq_run=1;//on
							scenario_cnt=1;
							printf("Pseq onoff Req - on\n");
						}
						else	printf("Pseq onoff Req - on failed, sequence not selected\n");
					}
				}
				else if(data.onoff==2)//pause
				{
					if((std_power_seq_run==1)||(std_power_seq_run==3))
					{
						std_power_paused = 1;
						set_std_pseq_power_pause_start_time(); //set standard time of pause
//						elapse_pause_time=get_std_pseq_power_pause_elapse_time() + old_elapse_pause_time; //elapse time of pause
						printf("Pseq Paused\n");
//						std_power_seq_run=0;
					}
					else printf("An Error occurred with Power Sequence Pause \n");
				}
				else
				{
					std_power_paused=0;
					pause_time=0;
					old_pause_time=0;
					sig_pause_time=0;
					old_sig_pause_time=0;
					elapse_time=0;
					if((std_power_seq_run==1) || (std_power_seq_run==3))
					{

						std_pseq_forced_shut_down();

						printf("Pseq onoff Req - off\n");
//						pseq_rpcnt=0;
//						scenario_cnt=0;//2023.11.02. reset not needed for cycle counter
					}
				}
			}
			break;

		case SID_STD_I2C_RD_TEST_REQ:
#if 1
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C
				req_std_i2c_test_wr_t data		= {0,};
				res_std_i2c_test_rd_t data_ack	= {0,};

				uint16_t	i=0, cnt=0;
				memset(&data, 0, sizeof(req_std_i2c_test_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_i2c_test_wr_t));

				std_i2c_test_fpga_set(data, 1);		//read

				sleep(1);

				memset(&data_ack, 0, sizeof(res_std_i2c_test_rd_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_STD_I2C_RD_TEST_REQ);
				data_ack.hdr.data_len		= sizeof(res_std_i2c_test_rd_t)-sizeof(res_head_t);
				data_ack.res=ACK;

				cnt=data.i2c_info.byte_num;
				if(cnt>200)	cnt=200;

				printf("i2c test read data\n");
				for(i=0;i<cnt;i++)
				{
					data_ack.reg_data[i] = 	FPGA_I2C_RD_DATA_Read(i);
					printf("[%d] 0x%04x\n", i, data_ack.reg_data[i]);
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_i2c_test_rd_t));
				msq_send_comm(&smsg);
			}
#else
			{
				req_std_edp_chara_set_read_t data = {0,};
				unsigned short i2c_ack;
				unsigned short rd_data;

				printf("eDP Signal Characteristics Read Req!\n");

				FPGA_Write(EDP_SIGNAL_READ_REQ, 0);
				FPGA_Write(EDP_SIGNAL_READ_REQ, 1);

				usleep(500000);

				if(FPGA_Read(EDP_SIGNAL_I2C_ACK))
				{
					/*
					{
						uint16_t				ch;
						uint16_t				type[10];		//0:vod, 1:pre-emphasis, 2:eq-mode, 3:rx-gain, 4:eq dc-gain, 5:eq-setting, 6:tx-gain
						uint32_t				value[10];
					} __PACKED__ edp_chara_set_t;
					*/
					i2c_ack = ACK;

					for(i=0; i<16; i++)
					{
						data.edp_sig_chara[i].type[0] = 0;
						data.edp_sig_chara[i].type[1] = 1;
						data.edp_sig_chara[i].type[2] = 2;
						data.edp_sig_chara[i].type[3] = 3;
						data.edp_sig_chara[i].type[4] = 4;
						data.edp_sig_chara[i].type[5] = 5;
						data.edp_sig_chara[i].type[6] = 6;
					}

					rd_data = FPGA_Read(EDP_TX1_AC_PARAM_READ_WRITE);
					data.edp_sig_chara[0].value[0] = rd_data&0x0003;			//DP PORT1 Lane0 Vod
					data.edp_sig_chara[0].value[1] = (rd_data&0x000c)>>2;		//DP PORT1 Lane0 pre-emphasis
					data.edp_sig_chara[1].value[0] = (rd_data&0x0030)>>4;		//DP PORT1 Lane1 Vod
					data.edp_sig_chara[1].value[1] = (rd_data&0x00c0)>>6;		//DP PORT1 Lane1 pre-emphasis
					data.edp_sig_chara[2].value[0] = (rd_data&0x0300)>>8;		//DP PORT1 Lane2 Vod
					data.edp_sig_chara[2].value[1] = (rd_data&0x0c00)>>10;		//DP PORT1 Lane2 pre-emphasis
					data.edp_sig_chara[3].value[0] = (rd_data&0x3000)>>12;		//DP PORT1 Lane3 Vod
					data.edp_sig_chara[3].value[1] = (rd_data&0xc000)>>14;		//DP PORT1 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_AC_PARAM_READ_WRITE);
					data.edp_sig_chara[4].value[0] = rd_data&0x0003;			//DP PORT2 Lane0 Vod
					data.edp_sig_chara[4].value[1] = (rd_data&0x000c)>>2;		//DP PORT2 Lane0 pre-emphasis
					data.edp_sig_chara[5].value[0] = (rd_data&0x0030)>>4;		//DP PORT2 Lane1 Vod
					data.edp_sig_chara[5].value[1] = (rd_data&0x00c0)>>6;		//DP PORT2 Lane1 pre-emphasis
					data.edp_sig_chara[6].value[0] = (rd_data&0x0300)>>8;		//DP PORT2 Lane2 Vod
					data.edp_sig_chara[6].value[1] = (rd_data&0x0c00)>>10;		//DP PORT2 Lane2 pre-emphasis
					data.edp_sig_chara[7].value[0] = (rd_data&0x3000)>>12;		//DP PORT2 Lane3 Vod
					data.edp_sig_chara[7].value[1] = (rd_data&0xc000)>>14;		//DP PORT2 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_AC_PARAM_READ_WRITE);
					data.edp_sig_chara[8].value[0] = rd_data&0x0003;			//DP PORT3 Lane0 Vod
					data.edp_sig_chara[8].value[1] = (rd_data&0x000c)>>2;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[9].value[0] = (rd_data&0x0030)>>4;		//DP PORT3 Lane1 Vod
					data.edp_sig_chara[9].value[1] = (rd_data&0x00c0)>>6;		//DP PORT3 Lane1 pre-emphasis
					data.edp_sig_chara[10].value[0] = (rd_data&0x0300)>>8;		//DP PORT3 Lane2 Vod
					data.edp_sig_chara[10].value[1] = (rd_data&0x0c00)>>10;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[11].value[0] = (rd_data&0x3000)>>12;		//DP PORT3 Lane3 Vod
					data.edp_sig_chara[11].value[1] = (rd_data&0xc000)>>14;		//DP PORT3 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_AC_PARAM_READ_WRITE);
					data.edp_sig_chara[12].value[0] = rd_data&0x0003;			//DP PORT3 Lane0 Vod
					data.edp_sig_chara[12].value[1] = (rd_data&0x000c)>>2;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[13].value[0] = (rd_data&0x0030)>>4;		//DP PORT3 Lane1 Vod
					data.edp_sig_chara[13].value[1] = (rd_data&0x00c0)>>6;		//DP PORT3 Lane1 pre-emphasis
					data.edp_sig_chara[14].value[0] = (rd_data&0x0300)>>8;		//DP PORT3 Lane2 Vod
					data.edp_sig_chara[14].value[1] = (rd_data&0x0c00)>>10;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[15].value[0] = (rd_data&0x3000)>>12;		//DP PORT3 Lane3 Vod
					data.edp_sig_chara[15].value[1] = (rd_data&0xc000)>>14;		//DP PORT3 Lane3 pre-emphasis

					rd_data = FPGA_Read(EDP_TX1_EQ_PARAM_READ_WRITE1);
					data.edp_sig_chara[0].value[3] = rd_data&0x0001;			//DP PORT1 Lane0 rx-gain
					data.edp_sig_chara[0].value[4] = (rd_data&0x0004)>>2;		//DP PORT1 Lane0 eq dc-gain
					data.edp_sig_chara[0].value[6] = (rd_data&0x0008)>>3;		//DP PORT1 Lane0 tx-gain
					data.edp_sig_chara[0].value[5] = (rd_data&0x0070)>>4;		//DP PORT1 Lane0 pre-emphasis
					data.edp_sig_chara[1].value[3] = (rd_data&0x0100)>>8;		//DP PORT1 Lane1 rx-gain
					data.edp_sig_chara[1].value[4] = (rd_data&0x0400)>>10;		//DP PORT1 Lane1 eq dc-gain
					data.edp_sig_chara[1].value[6] = (rd_data&0x0800)>>11;		//DP PORT1 Lane1 tx-gain
					data.edp_sig_chara[1].value[5] = (rd_data&0x7000)>>12;		//DP PORT1 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX1_EQ_PARAM_READ_WRITE2);
					data.edp_sig_chara[2].value[3] = rd_data&0x0001;			//DP PORT1 Lane2 rx-gain
					data.edp_sig_chara[2].value[4] = (rd_data&0x0004)>>2;		//DP PORT1 Lane2 eq dc-gain
					data.edp_sig_chara[2].value[6] = (rd_data&0x0008)>>3;		//DP PORT1 Lane2 tx-gain
					data.edp_sig_chara[2].value[5] = (rd_data&0x0070)>>4;		//DP PORT1 Lane2 pre-emphasis
					data.edp_sig_chara[3].value[3] = (rd_data&0x0100)>>8;		//DP PORT1 Lane3 rx-gain
					data.edp_sig_chara[3].value[4] = (rd_data&0x0400)>>10;		//DP PORT1 Lane3 eq dc-gain
					data.edp_sig_chara[3].value[6] = (rd_data&0x0800)>>11;		//DP PORT1 Lane3 tx-gain
					data.edp_sig_chara[3].value[5] = (rd_data&0x7000)>>12;		//DP PORT1 Lane3 pre-emphasis

					data.edp_sig_chara[0].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane0 eq-mode
					data.edp_sig_chara[1].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane1 eq-mode
					data.edp_sig_chara[2].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane2 eq-mode
					data.edp_sig_chara[3].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane3 eq-mode

					rd_data = FPGA_Read(EDP_TX2_EQ_PARAM_READ_WRITE1);
					data.edp_sig_chara[4].value[3] = rd_data&0x0001;			//DP PORT2 Lane0 rx-gain
					data.edp_sig_chara[4].value[4] = (rd_data&0x0004)>>2;		//DP PORT2 Lane0 eq dc-gain
					data.edp_sig_chara[4].value[6] = (rd_data&0x0008)>>3;		//DP PORT2 Lane0 tx-gain
					data.edp_sig_chara[4].value[5] = (rd_data&0x0070)>>4;		//DP PORT2 Lane0 pre-emphasis
					data.edp_sig_chara[5].value[3] = (rd_data&0x0100)>>8;		//DP PORT2 Lane1 rx-gain
					data.edp_sig_chara[5].value[4] = (rd_data&0x0400)>>10;		//DP PORT2 Lane1 eq dc-gain
					data.edp_sig_chara[5].value[6] = (rd_data&0x0800)>>11;		//DP PORT2 Lane1 tx-gain
					data.edp_sig_chara[5].value[5] = (rd_data&0x7000)>>12;		//DP PORT2 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_EQ_PARAM_READ_WRITE2);
					data.edp_sig_chara[6].value[3] = rd_data&0x0001;			//DP PORT2 Lane2 rx-gain
					data.edp_sig_chara[6].value[4] = (rd_data&0x0004)>>2;		//DP PORT2 Lane2 eq dc-gain
					data.edp_sig_chara[6].value[6] = (rd_data&0x0008)>>3;		//DP PORT2 Lane2 tx-gain
					data.edp_sig_chara[6].value[5] = (rd_data&0x0070)>>4;		//DP PORT2 Lane2 pre-emphasis
					data.edp_sig_chara[7].value[3] = (rd_data&0x0100)>>8;		//DP PORT2 Lane3 rx-gain
					data.edp_sig_chara[7].value[4] = (rd_data&0x0400)>>10;		//DP PORT2 Lane3 eq dc-gain
					data.edp_sig_chara[7].value[6] = (rd_data&0x0800)>>11;		//DP PORT2 Lane3 tx-gain
					data.edp_sig_chara[7].value[5] = (rd_data&0x7000)>>12;		//DP PORT2 Lane3 pre-emphasis

					data.edp_sig_chara[4].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane0 eq-mode
					data.edp_sig_chara[5].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane1 eq-mode
					data.edp_sig_chara[6].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane2 eq-mode
					data.edp_sig_chara[7].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane3 eq-mode

					rd_data = FPGA_Read(EDP_TX3_EQ_PARAM_READ_WRITE1);
					data.edp_sig_chara[8].value[3] = rd_data&0x0001;			//DP PORT3 Lane0 rx-gain
					data.edp_sig_chara[8].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane0 eq dc-gain
					data.edp_sig_chara[8].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane0 tx-gain
					data.edp_sig_chara[8].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[9].value[3] = (rd_data&0x0100)>>8;		//DP PORT3 Lane1 rx-gain
					data.edp_sig_chara[9].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane1 eq dc-gain
					data.edp_sig_chara[9].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane1 tx-gain
					data.edp_sig_chara[9].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX3_EQ_PARAM_READ_WRITE2);
					data.edp_sig_chara[10].value[3] = rd_data&0x0001;			//DP PORT3 Lane2 rx-gain
					data.edp_sig_chara[10].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane2 eq dc-gain
					data.edp_sig_chara[10].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane2 tx-gain
					data.edp_sig_chara[10].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[11].value[3] = (rd_data&0x0100)>>8;		//DP PORT3 Lane3 rx-gain
					data.edp_sig_chara[11].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane3 eq dc-gain
					data.edp_sig_chara[11].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane3 tx-gain
					data.edp_sig_chara[11].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane3 pre-emphasis

					data.edp_sig_chara[8].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane0 eq-mode
					data.edp_sig_chara[9].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane1 eq-mode
					data.edp_sig_chara[10].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane2 eq-mode
					data.edp_sig_chara[11].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane3 eq-mode

					rd_data = FPGA_Read(EDP_TX4_EQ_PARAM_READ_WRITE1);
					data.edp_sig_chara[12].value[3] = rd_data&0x0001;			//DP PORT3 Lane0 rx-gain
					data.edp_sig_chara[12].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane0 eq dc-gain
					data.edp_sig_chara[12].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane0 tx-gain
					data.edp_sig_chara[12].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[13].value[3] = (rd_data&0x0100)>>8;		//DP PORT3 Lane1 rx-gain
					data.edp_sig_chara[13].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane1 eq dc-gain
					data.edp_sig_chara[13].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane1 tx-gain
					data.edp_sig_chara[13].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX4_EQ_PARAM_READ_WRITE2);
					data.edp_sig_chara[14].value[3] = rd_data&0x0001;			//DP PORT3 Lane2 rx-gain
					data.edp_sig_chara[14].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane2 eq dc-gain
					data.edp_sig_chara[14].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane2 tx-gain
					data.edp_sig_chara[14].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[15].value[3] = (rd_data&0x0100)>>8;		//DP PORT3 Lane3 rx-gain
					data.edp_sig_chara[15].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane3 eq dc-gain
					data.edp_sig_chara[15].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane3 tx-gain
					data.edp_sig_chara[15].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane3 pre-emphasis

					data.edp_sig_chara[12].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane0 eq-mode
					data.edp_sig_chara[13].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane1 eq-mode
					data.edp_sig_chara[14].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane2 eq-mode
					data.edp_sig_chara[15].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane3 eq-mode
				}
				else
				{
					i2c_ack = NACK;
				}

				memset(&data, 0, sizeof(req_std_edp_chara_set_read_t));

				data.hdr.cmd_id			= RES_ID(SID_STD_I2C_RD_TEST_REQ);
				data.hdr.data_len		= sizeof(req_std_edp_chara_set_read_t)-sizeof(res_head_t);
				data.res				= i2c_ack;

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data, sizeof(req_std_edp_chara_set_read_t));
				msq_send_comm(&smsg);
			}
#endif
			break;
		case SID_CTL_WRITE_REQ: //ksk
			{
				req_std_ctl_wr_t data		= {0,};
				memset(&data, 0, sizeof(req_std_ctl_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ctl_wr_t));

				std_ctl_fpga_set(data, 0);		//write
			}
			break;
/*		case SID_CTL_READ_REQ:
			{
				req_std_ctl_wr_t data		= {0,};
				req_std_ctl_rd_t data_ack	= {0,};

				uint16_t	i=0;
				memset(&data, 0, sizeof(req_std_ctl_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ctl_wr_t));

				std_ctl_fpga_set(data, 1);		//read

				sleep(1);

				memset(&data_ack, 0, sizeof(req_std_ctl_rd_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_CTL_READ_REQ);
				data_ack.hdr.data_len		= sizeof(req_std_ctl_rd_t)-sizeof(res_head_t);
				data_ack.res=ACK;


				printf("ctl test read data\n");
				for(i=0;i<256;i++)
				{
					data_ack.reg_data[i] = 	FPGA_CTL_DATA_READ(i);
					printf("[%d] 0x%04x\n", i, data_ack.reg_data[i]);
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(req_std_ctl_rd_t));
				msq_send_comm(&smsg);
			}
			break;
*/		case SID_CTL_ENABLE_REQ:
			{
				req_std_ctl_enable_t data		= {0,};
				memset(&data, 0, sizeof(req_std_ctl_enable_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ctl_enable_t));
				printf("CTL Enable set=%d\n", data.onoff);
				if(data.onoff==0)
					{
						FPGA_Write(FPGA_CTL_EN, ENUM_OFF); 	//OFF
						printf("CTL Disable\r\n");
					}
				else
					{
						FPGA_Write(FPGA_CTL_EN, ENUM_ON);	//ON
						printf("CTL Enable\r\n");
					}
			}
			break;

		case SID_STD_OCMD_WR_TEST_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0002);	//FPGA_I2C_SEL = 1 : OCMD
				req_std_ocmd_test_wr_t data		= {0,};
				memset(&data, 0, sizeof(req_std_ocmd_test_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ocmd_test_wr_t));

				std_ocmd_test_fpga_set(data, 0);		//write
			}
			break;

		case SID_STD_OCMD_RD_TEST_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0002);	//FPGA_I2C_SEL = 1 : OCMD
				req_std_ocmd_test_wr_t data		= {0,};
				res_std_ocmd_test_rd_t data_ack	= {0,};

				uint16_t	i=0, cnt=0;
				memset(&data, 0, sizeof(req_std_ocmd_test_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ocmd_test_wr_t));

				std_ocmd_test_fpga_set(data, 1);		//read
				sleep(1);

				memset(&data_ack, 0, sizeof(res_std_ocmd_test_rd_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_STD_OCMD_RD_TEST_REQ);
				data_ack.hdr.data_len		= sizeof(res_std_ocmd_test_rd_t)-sizeof(res_head_t);
				data_ack.res=ACK;

				cnt=data.ocmd_info.byte_num;
				if(cnt>200)	cnt=200;

				printf("OCMD test read data\n");
				for(i=0;i<cnt;i++)
				{
//					data_ack.reg_data_h[i] = FPGA_OCMD_1_ON_REG_DATA_H_Read(i);
					data_ack.reg_data_h[i] = FPGA_OCMD_H_DATA_Read(i);
//					data_ack.reg_data_l[i] = FPGA_OCMD_1_ON_REG_DATA_L_Read(i);
					data_ack.reg_data_l[i] = FPGA_OCMD_L_DATA_Read(i);
					printf("[%d] 0x%04x\n", i, (data_ack.reg_data_h[i]<<8) + data_ack.reg_data_l[i]);
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_ocmd_test_rd_t));
				msq_send_comm(&smsg);
			}
			break;

		case SID_STD_OCMD_PWR_SEQ_SET_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0002);	//FPGA_I2C_SEL = 1 : OCMD
				req_std_ocmd_pwr_seq_test_t data		= {0,};

				memset(&data, 0, sizeof(req_std_ocmd_pwr_seq_test_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_ocmd_pwr_seq_test_t));//Pseq start
				if ((data.onoff& 0x01)>0)	flag_ocmd_1_on_en=1;
				else flag_ocmd_1_on_en=0;
				if ((data.onoff & 0x02)>0)	flag_ocmd_1_off_en=1;
				else flag_ocmd_1_off_en=0;
				if ((data.onoff & 0x04)>0)	flag_ocmd_2_on_en=1;
				else flag_ocmd_2_on_en=0;
				if ((data.onoff & 0x08)>0)	flag_ocmd_2_off_en=1;
				else flag_ocmd_2_off_en=0;
				printf("OCDM Pwr Seq flag 1 on en: %d\n", 	flag_ocmd_1_on_en);
				printf("OCDM Pwr Seq flag 1 off en: %d\n", 	flag_ocmd_1_off_en);
				printf("OCDM Pwr Seq flag 2 on en: %d\n", 	flag_ocmd_2_on_en);
				printf("OCDM Pwr Seq flag 2 off en: %d\n", 	flag_ocmd_2_off_en);
				if(std_power_seq_run==0)
				{
					std_power_seq_ocmd_set(&data);
					std_ocmd_power_sequence(&data);
					set_std_pseq_power_start_time();
				}
				else printf("Pseq set Req - running already\n");
			}
			break;
		case SID_STD_OCMD_PWR_SEQ_ONOFF_REQ:
			{
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0002);	//FPGA_I2C_SEL = 1 : OCMD
				req_std_pseq_onoff_t data		= {0,};
				memset(&data, 0, sizeof(req_std_pseq_onoff_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_pseq_onoff_t));

				if(data.onoff==1)
				{
					std_power_paused=0;
//					scenario_cnt=scenario_cnt_temp;
					if(std_power_seq_run==0)
					{
						if(pseq_set_scnt>0)
						{
							std_power_seq_run=3;//on
							scenario_cnt=1;
							printf("Pseq onoff Req - on\n");
						}
						else	printf("Pseq OCMD onoff Req - on failed, sequence not selected\n");
					}
				}
				else if(data.onoff==2)//pause
				{
					if((std_power_seq_run==1)||(std_power_seq_run==3))
					{
						std_power_paused = 1;
						set_std_pseq_power_pause_start_time(); //set standard time of pause
//						elapse_pause_time=get_std_pseq_power_pause_elapse_time() + old_elapse_pause_time; //elapse time of pause
						printf("Pseq Paused\n");
//						std_power_seq_run=0;
					}
					else printf("An Error occurred with Power Sequence Pause \n");
				}
				else
				{
					std_power_paused=0;
					pause_time=0;
					old_pause_time=0;
					sig_pause_time=0;
					old_sig_pause_time=0;
					elapse_time=0;
//					set_std_pseq_power_pause_start_time(); //set standard time of pause
					if((std_power_seq_run==3) || (std_power_seq_run==1))
					{
						std_pseq_forced_shut_down();
						printf("Pseq onoff Req - off\n");
					}
				}
			}
			break;
		case SID_PWR_SEQ_STATUS_REQ:
			{
				res_pwr_seq_t data_ack	= {0, };

				memset(&data_ack, 0, sizeof(res_pwr_seq_t));
				data_ack.hdr.cmd_id			= RES_ID(SID_PWR_SEQ_STATUS_REQ);
				data_ack.hdr.data_len		= sizeof(res_pwr_seq_t)-sizeof(res_head_t);
				data_ack.scenario			= scenario_cnt;
				data_ack.repeat				= pseq_rpcnt;

				for(int i=0; i<4; i++)
				{
					data_ack.read_aux_data[i] = pwr_seq_aux_addr_data.reg_data[i];
				}

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_pwr_seq_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_PCC_LOG_DOWN_REQ:
			{

			}
			break;
			case SID_STD_AUX_I2C_PWR_SEQ_TEST_REQ:	//0x104f
			{
				req_std_aux_i2c_pwr_seq_test_t data = {0, };
				res_std_aux_i2c_pwr_seq_test_t data_ack = {0, };

				std_aux_pwr_seq_cmd_t aux_data = {0, };

//				std_aux_timing_t aux_data_timing = {0, };

//				unsigned short	data_array_size_temp	= 0;
//				unsigned short	data_array_temp[65535]	= {0, };

				memset(&data, 0, sizeof(req_std_aux_i2c_pwr_seq_test_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_aux_i2c_pwr_seq_test_t));

				memset(&aux_data.reg_addr[0], 0, sizeof(std_aux_pwr_seq_cmd_t));
				memcpy(&aux_data.reg_addr[0], &data.aux_pwr_seq_cmd.reg_addr[0], sizeof(std_aux_pwr_seq_cmd_t));

				printf("SID_STD_AUX_I2C_PWR_SEQ_TEST_REQ(0x104F) Start\n");
				printf("data.sel : %d\n", data.sel);

				for(int i=0; i<7; i++)
				{
					printf("data.pwr_seq_timing[%d].t1 : %d\n", i, data.pwr_seq_timing[i].t1);
					printf("data.pwr_seq_timing[%d].t2 : %d\n", i, data.pwr_seq_timing[i].t2);
					printf("data.pwr_seq_timing[%d].t3 : %d\n", i, data.pwr_seq_timing[i].t3);
					printf("data.pwr_seq_timing[%d].t3_1 : %d\n", i, data.pwr_seq_timing[i].t3_1);
					printf("data.pwr_seq_timing[%d].t3_2 : %d\n", i, data.pwr_seq_timing[i].t3_2);
					printf("data.pwr_seq_timing[%d].t3_3 : %d\n", i, data.pwr_seq_timing[i].t3_3);
					printf("data.pwr_seq_timing[%d].t3_4 : %d\n", i, data.pwr_seq_timing[i].t3_4);
					printf("data.pwr_seq_timing[%d].t4 : %d\n", i, data.pwr_seq_timing[i].t4);
					printf("data.pwr_seq_timing[%d].t5 : %d\n", i, data.pwr_seq_timing[i].t5);
					printf("data.pwr_seq_timing[%d].ton : %d\n", i, data.pwr_seq_timing[i].ton);
					printf("data.pwr_seq_timing[%d].acdet_on : %d\n", i, data.pwr_seq_timing[i].acdet_on);
					printf("data.pwr_seq_timing[%d].acdet_off : %d\n", i, data.pwr_seq_timing[i].acdet_off);
					printf("data.pwr_seq_timing[%d].t_i2c_on_1 : %d\n", i, data.pwr_seq_timing[i].t_i2c_on_1);
					printf("data.pwr_seq_timing[%d].t_i2c_off_1 : %d\n", i, data.pwr_seq_timing[i].t_i2c_off_1);
					printf("data.pwr_seq_timing[%d].t_i2c_on_2 : %d\n", i, data.pwr_seq_timing[i].t_i2c_on_2);
					printf("data.pwr_seq_timing[%d].t_i2c_off_2 : %d\n", i, data.pwr_seq_timing[i].t_i2c_off_2);
					printf("data.pwr_seq_timing[%d].repeat : %d\n", i, data.pwr_seq_timing[i].repeat);
				}

				for(int i=0; i<4; i++)
				{
					printf("data.pwr_seq_i2c[%d].i2c_clock_sel : %d\n", i, data.pwr_seq_i2c[i].i2c_clock_sel);
					printf("data.pwr_seq_i2c[%d].reg_addr_size : %d\n", i, data.pwr_seq_i2c[i].reg_addr_size);
					printf("data.pwr_seq_i2c[%d].data_size : %d\n", i, data.pwr_seq_i2c[i].data_size);
					printf("data.pwr_seq_i2c[%d].dev_addr : %d\n", i, data.pwr_seq_i2c[i].dev_addr);
					printf("data.pwr_seq_i2c[%d].byte_num : %d\n", i, data.pwr_seq_i2c[i].byte_num);

					for(int j=0; j<10; j++)
					{
						printf("data.pwr_seq_i2c[%d].reg_addr[%d] : %d\n", i, j, data.pwr_seq_i2c[i].reg_addr[j]);
						printf("data.pwr_seq_i2c[%d].reg_data[%d] : %d\n", i, j, data.pwr_seq_i2c[i].reg_data[j]);
					}
				}
				printf("data.onoff : %d\n", data.onoff);
				printf("data.scenario : %d\n", data.scenario);
				printf("data.rpcnt : %d\n", data.rpcnt);

				for(int i=0; i<4; i++)
				{
					printf("data.aux_pwr_seq_cmd.reg_addr[%d] : %d\n", i, data.aux_pwr_seq_cmd.reg_addr[i]);
					printf("data.aux_pwr_seq_cmd.reg_data[%d] : %d\n", i, data.aux_pwr_seq_cmd.reg_data[i]);
				}
				printf("SID_STD_AUX_I2C_PWR_SEQ_TEST_REQ(0x104F) End\n");

				for(int i=0; i<4; i++)
				{
					printf("aux_data.reg_addr[%d] : %d\n", i, aux_data.reg_addr[i]);
					printf("aux_data.reg_data[%d] : %d\n", i, aux_data.reg_data[i]);
				}

//				data_array_size_temp = (unsigned short)(sizeof(std_aux_pwr_seq_cmd_t)/2);
//
//				memset(data_array_temp, 0, sizeof(data_array_temp));
//				memcpy(data_array_temp, &aux_data.reg_addr[0], (data_array_size_temp * 2));
//
//				// SPI Transfer to FPGA
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_Write(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_RST));
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_RST));
//
//				FPGA_Write(FPGA_SEND_ARRAY_SIZE, data_array_size_temp);
//
//				// Writing aux data
//				for(int i=0; i<=(int)data_array_size_temp; i++)
//				{
//					FPGA_Write(FPGA_SEND_ARRAY_CNT, (unsigned short)i);
//					FPGA_Write(FPGA_SEND_ARRAY_DATA, data_array_temp[i]);
//
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_OR_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_REQ));
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_REQ));
//				}

//				data_array_size_temp = (unsigned short)((sizeof(req_std_aux_i2c_pwr_seq_test_t) - sizeof(req_head_t))/2);
//
//				memset(data_array_temp, 0, sizeof(data_array_temp));
//				memcpy(data_array_temp, &data.sel, (data_array_size_temp * 2));
//
//				// SPI Transfer to FPGA
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_Write(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_I2C_RST));
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_I2C_RST));
//
//				FPGA_Write(FPGA_SEND_ARRAY_SIZE, data_array_size_temp);
//
//				// Writing pwr_seq_i2c_data
//				for(int i=0; i<=(int)data_array_size_temp; i++)
//				{
//					FPGA_Write(FPGA_SEND_ARRAY_CNT, (unsigned short)i);
//					FPGA_Write(FPGA_SEND_ARRAY_DATA, data_array_temp[i]);
//
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_OR_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_I2C_REQ));
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_I2C_REQ));
//				}

				// reset pwr_seq_aux_addr_data
				memset(&pwr_seq_aux_addr_data.reg_addr[0], 0, sizeof(pwr_seq_aux_addr_data));
				memcpy(&pwr_seq_aux_addr_data.reg_addr[0], &aux_data.reg_addr[0], sizeof(pwr_seq_aux_addr_data.reg_addr));

				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C

				/* 2022.08.25 ksk i2c 2 pwr seq */
				if ((data.onoff& 0x01)>0)	flag_i2c_1_on_en=1;
				else						flag_i2c_1_on_en=0;

				if ((data.onoff & 0x02)>0)	flag_i2c_1_off_en=1;
				else						flag_i2c_1_off_en=0;

				if ((data.onoff & 0x04)>0)	flag_i2c_2_on_en=1;
				else						flag_i2c_2_on_en=0;

				if ((data.onoff & 0x08)>0)	flag_i2c_2_off_en=1;
				else						flag_i2c_2_off_en=0;

				printf("flag 1 on en: %d\n", 	flag_i2c_1_on_en);
				printf("flag 1 off en: %d\n", 	flag_i2c_1_off_en);
				printf("flag 2 on en: %d\n", 	flag_i2c_2_on_en);
				printf("flag 2 off en: %d\n", 	flag_i2c_2_off_en);

				if(std_power_seq_run==0)
				{
					std_edp_power_seq_i2c_set(&data);
					std_edp_power_sequence(&data);

					pwr_seq_aux_state = 0;

					set_std_pseq_power_start_time();
				}
				else printf("Pseq set Req - running already\n");

				memset(&data_ack, 0, sizeof(res_std_aux_i2c_pwr_seq_test_t));
				data_ack.res = ACK;

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_aux_i2c_pwr_seq_test_t));
				msq_send_comm(&smsg);
			}
			break;
			case SID_STD_AUX_OCMD_PWR_SEQ_TEST_REQ:	//0x1050
			{
				req_std_aux_ocmd_pwr_seq_test_t data = {0, };
				res_std_aux_ocmd_pwr_seq_test_t data_ack = {0, };

				std_aux_pwr_seq_cmd_t aux_data = {0, };

//				unsigned short	data_array_size_temp	= 0;
//				unsigned short	data_array_temp[65535]	= {0, };

				memset(&data, 0, sizeof(req_std_aux_ocmd_pwr_seq_test_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_aux_ocmd_pwr_seq_test_t));

				printf("SID_STD_AUX_OCMD_PWR_SEQ_TEST_REQ(0x1050) Start\n");
				printf("data.sel : %d\n", data.sel);
				for(int i=0; i<7; i++)
				{
					printf("data.pwr_seq_timing[%d].t1 : %d\n", i, data.pwr_seq_timing[i].t1);
					printf("data.pwr_seq_timing[%d].t2 : %d\n", i, data.pwr_seq_timing[i].t2);
					printf("data.pwr_seq_timing[%d].t3 : %d\n", i, data.pwr_seq_timing[i].t3);
					printf("data.pwr_seq_timing[%d].t3_1 : %d\n", i, data.pwr_seq_timing[i].t3_1);
					printf("data.pwr_seq_timing[%d].t3_2 : %d\n", i, data.pwr_seq_timing[i].t3_2);
					printf("data.pwr_seq_timing[%d].t3_3 : %d\n", i, data.pwr_seq_timing[i].t3_3);
					printf("data.pwr_seq_timing[%d].t3_4 : %d\n", i, data.pwr_seq_timing[i].t3_4);
					printf("data.pwr_seq_timing[%d].t4 : %d\n", i, data.pwr_seq_timing[i].t4);
					printf("data.pwr_seq_timing[%d].t5 : %d\n", i, data.pwr_seq_timing[i].t5);
					printf("data.pwr_seq_timing[%d].ton : %d\n", i, data.pwr_seq_timing[i].ton);
					printf("data.pwr_seq_timing[%d].acdet_on : %d\n", i, data.pwr_seq_timing[i].acdet_on);
					printf("data.pwr_seq_timing[%d].acdet_off : %d\n", i, data.pwr_seq_timing[i].acdet_off);
					printf("data.pwr_seq_timing[%d].t_i2c_on_1 : %d\n", i, data.pwr_seq_timing[i].t_i2c_on_1);
					printf("data.pwr_seq_timing[%d].t_i2c_off_1 : %d\n", i, data.pwr_seq_timing[i].t_i2c_off_1);
					printf("data.pwr_seq_timing[%d].t_i2c_on_2 : %d\n", i, data.pwr_seq_timing[i].t_i2c_on_2);
					printf("data.pwr_seq_timing[%d].t_i2c_off_2 : %d\n", i, data.pwr_seq_timing[i].t_i2c_off_2);
					printf("data.pwr_seq_timing[%d].repeat : %d\n", i, data.pwr_seq_timing[i].repeat);
				}
				for(int i=0; i<4; i++)
				{
					printf("data.pwr_seq_ocmd[%d].ocmd_clock_sel : %d\n", i, data.pwr_seq_ocmd[i].ocmd_clock_sel);
					printf("data.pwr_seq_ocmd[%d].data_size : %d\n", i, data.pwr_seq_ocmd[i].data_size);
					printf("data.pwr_seq_ocmd[%d].dev_addr : %d\n", i, data.pwr_seq_ocmd[i].dev_addr);
					printf("data.pwr_seq_ocmd[%d].byte_num : %d\n", i, data.pwr_seq_ocmd[i].byte_num);
					for(int j=0; j<10; j++)
					{
						printf("data.pwr_seq_ocmd[%d].sub_addr[%d] : %d\n", i, j, data.pwr_seq_ocmd[i].sub_addr[j]);
						printf("data.pwr_seq_ocmd[%d].reg_data_h[%d] : %d\n", i, j, data.pwr_seq_ocmd[i].reg_data_h[j]);
						printf("data.pwr_seq_ocmd[%d].reg_data_l[%d] : %d\n", i, j, data.pwr_seq_ocmd[i].reg_data_l[j]);
						printf("data.pwr_seq_ocmd[%d].table_data_h[%d] : %d\n", i, j, data.pwr_seq_ocmd[i].table_data_h[j]);
						printf("data.pwr_seq_ocmd[%d].table_data_l[%d] : %d\n", i, j, data.pwr_seq_ocmd[i].table_data_l[j]);
					}
				}
				printf("data.onoff : %d\n", data.onoff);
				printf("data.scenario : %d\n", data.scenario);
				printf("data.rpcnt : %d\n", data.rpcnt);
				for(int i=0; i<4; i++)
				{
					printf("data.aux_pwr_seq_cmd.reg_addr[%d] : %d\n", i, data.aux_pwr_seq_cmd.reg_addr[i]);
					printf("data.aux_pwr_seq_cmd.reg_data[%d] : %d\n", i, data.aux_pwr_seq_cmd.reg_data[i]);
				}
				printf("SID_STD_AUX_OCMD_PWR_SEQ_TEST_REQ(0x1050) End\n");

				memset(&aux_data.reg_addr[0], 0, sizeof(std_aux_pwr_seq_cmd_t));
				memcpy(&aux_data.reg_addr[0], &data.aux_pwr_seq_cmd.reg_addr[0], sizeof(std_aux_pwr_seq_cmd_t));

				for(int i=0; i<4; i++)
				{
					printf("aux_data.reg_addr[%d] : %d\n", i, aux_data.reg_addr[i]);
					printf("aux_data.reg_data[%d] : %d\n", i, aux_data.reg_data[i]);
				}

//				data_array_size_temp = (unsigned short)(sizeof(std_aux_pwr_seq_cmd_t)/2);
//
//				memset(data_array_temp, 0, sizeof(data_array_temp));
//				memcpy(data_array_temp, &aux_data.reg_addr[0], (data_array_size_temp * 2));
//
//				// SPI Transfer to FPGA
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_Write(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_RST));
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_RST));
//
//				FPGA_Write(FPGA_SEND_ARRAY_SIZE, data_array_size_temp);
//
//				// Writing aux data
//				for(int i=0; i<=(int)data_array_size_temp; i++)
//				{
//					FPGA_Write(FPGA_SEND_ARRAY_CNT, (unsigned short)i);
//					FPGA_Write(FPGA_SEND_ARRAY_DATA, data_array_temp[i]);
//
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_OR_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_REQ));
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_AUX_REQ));
//				}

//				data_array_size_temp = (unsigned short)((sizeof(req_std_aux_ocmd_pwr_seq_test_t) - sizeof(req_head_t))/2);
//
//				memset(data_array_temp, 0, sizeof(data_array_temp));
//				memcpy(data_array_temp, &data.sel, (data_array_size_temp * 2));
//
//				// SPI Transfer to FPGA
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_Write(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_OCMD_RST));
//				usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//				FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_OCMD_RST));
//
//				FPGA_Write(FPGA_SEND_ARRAY_SIZE, data_array_size_temp);
//
//				// Writing pwr_seq_ocmd_data
//				for(int i=0; i<=(int)data_array_size_temp; i++)
//				{
//					FPGA_Write(FPGA_SEND_ARRAY_CNT, (unsigned short)i);
//					FPGA_Write(FPGA_SEND_ARRAY_DATA, data_array_temp[i]);
//
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_OR_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_OCMD_REQ));
//					usleep(2000);	// This is essential to reconize FPGA_SEND_ARRAY_CTRL_REG
//					FPGA_AND_SET(FPGA_SEND_ARRAY_CTRL_REG, (1 << FPGA_LSHIFT_BIT_PWR_SEQ_OCMD_REQ));
//				}

				// reset pwr_seq_aux_addr_data
				memset(&pwr_seq_aux_addr_data.reg_addr[0], 0, sizeof(pwr_seq_aux_addr_data));
				memcpy(&pwr_seq_aux_addr_data.reg_addr[0], &aux_data.reg_addr[0], sizeof(pwr_seq_aux_addr_data.reg_addr));

				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0002);	//FPGA_I2C_SEL = 1 : OCMD

				if ((data.onoff& 0x01)>0)	flag_ocmd_1_on_en=1;
				else flag_ocmd_1_on_en=0;
				if ((data.onoff & 0x02)>0)	flag_ocmd_1_off_en=1;
				else flag_ocmd_1_off_en=0;
				if ((data.onoff & 0x04)>0)	flag_ocmd_2_on_en=1;
				else flag_ocmd_2_on_en=0;
				if ((data.onoff & 0x08)>0)	flag_ocmd_2_off_en=1;
				else flag_ocmd_2_off_en=0;
				printf("OCDM Pwr Seq flag 1 on en: %d\n", 	flag_ocmd_1_on_en);
				printf("OCDM Pwr Seq flag 1 off en: %d\n", 	flag_ocmd_1_off_en);
				printf("OCDM Pwr Seq flag 2 on en: %d\n", 	flag_ocmd_2_on_en);
				printf("OCDM Pwr Seq flag 2 off en: %d\n", 	flag_ocmd_2_off_en);
				if(std_power_seq_run==0)
				{
					std_edp_power_seq_ocmd_set(&data);
					std_edp_ocmd_power_sequence(&data);

					pwr_seq_aux_state = 0;

					set_std_pseq_power_start_time();
				}
				else printf("Pseq set Req - running already\n");

				memset(&data_ack, 0, sizeof(res_std_aux_ocmd_pwr_seq_test_t));
				data_ack.res = ACK;

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_aux_ocmd_pwr_seq_test_t));
				msq_send_comm(&smsg);
			}
			break;
			case SID_STD_AUX_TEST_WR_REQ:			//0x1051
			{
				req_std_aux_test_wr_t data = {0, };
				res_std_aux_test_wr_t data_ack = {0, };

//				unsigned short aux_return[4] = {0, };
				unsigned short aux_return_value = 0;

				memset(&data, 0, sizeof(req_std_aux_test_wr_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_aux_test_wr_t));

				printf("SID_STD_AUX_TEST_WR_REQ(0x1051) Start\n");
				printf("data.aux_test.aux_clock_sel : %d\n", data.aux_test.aux_clock_sel);
				printf("data.aux_test.reg_addr_size : %d\n", data.aux_test.reg_addr_size);
				printf("data.aux_test.data_size : %d\n", data.aux_test.data_size);
				printf("data.aux_test.dev_addr : %d\n", data.aux_test.dev_addr);
				printf("data.aux_test.byte_num : %d\n", data.aux_test.byte_num);
				for(int i=0; i<data.aux_test.byte_num; i++)
				{
					printf("data.aux_test.reg_addr[%d] : 0x%04x\n", i, data.aux_test.reg_addr[i]);
					printf("data.aux_test.reg_data[%d] : 0x%04x\n", i, data.aux_test.reg_data[i]);
				}
				printf("SID_STD_AUX_TEST_WR_REQ(0x1051) End\n");

				// SPI Transfer to FPGA
				usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
				FPGA_Write(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));
				usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
				FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));

				// Writing data_size	// 1:1byte, 2:2byte
				if(data.aux_test.data_size == 1)
				{
					FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_DATA_SIZE));
				}
				else if(data.aux_test.data_size == 2)
				{
					FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_DATA_SIZE));
				}

				memset(&data_ack, 0, sizeof(res_std_aux_test_wr_t));

				// Writing AUX Data
				for(int i=0; i<(int)data.aux_test.byte_num; i++)
				{
					FPGA_Write(FPGA_AUX_BYTE_NUM, ((data.aux_test.byte_num << FPGA_LSHIFT_BIT_AUX_BYTE_NUM) | ((unsigned short)i << FPGA_LSHIFT_BIT_AUX_CNT)));
					FPGA_Write(FPGA_AUX_ADDR, data.aux_test.reg_addr[i]);
					FPGA_Write(FPGA_AUX_WR_DATA, data.aux_test.reg_data[i]);

//					for(int j=0; j<4; j++)
					{
//						FPGA_AND_SET(FPGA_AUX_CTRL_REG, (3 << FPGA_LSHIFT_BIT_AUX_TX_IDX));
//						FPGA_OR_SET(FPGA_AUX_CTRL_REG, (j << FPGA_LSHIFT_BIT_AUX_TX_IDX));

						usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
						FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_WR_REQ));
						usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
						FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_WR_REQ));

//						aux_return[j] = FPGA_Read(FPGA_AUX_RD_REG);
						aux_return_value = FPGA_Read(FPGA_AUX_RD_REG);

//						printf("aux_return(%d)[%d] : 0x%04x\n", i, j, aux_return[j]);
						printf("aux_return_value(%d) : 0x%04x\n", i, aux_return_value);
					}

//					if((aux_return[0] == 0) || (aux_return[1] == 0) || (aux_return[2] == 0) || (aux_return[3] == 0))
					if(aux_return_value == 0)
					{
						data_ack.res = ACK;
					}
					else
					{
						data_ack.res = NACK;
					}
				}

				data_ack.hdr.cmd_id = RES_ID(SID_STD_AUX_TEST_WR_REQ);
				data_ack.hdr.data_len = sizeof(res_std_aux_test_wr_t)-sizeof(res_head_t);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_aux_test_wr_t));
				msq_send_comm(&smsg);
			}
			break;
			case SID_STD_AUX_TEST_RD_REQ:			//0x1052
			{
				req_std_aux_test_rd_t data = {0, };
				res_std_aux_test_rd_t data_ack = {0, };

				unsigned short aux_rd_data[200]	= {0, };
//				unsigned short aux_return[4]	= {0, };
				unsigned short aux_return_value = 0;

				memset(&data, 0, sizeof(req_std_aux_test_rd_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_aux_test_rd_t));

				printf("SID_STD_AUX_TEST_RD_REQ(0x1052) Start\n");
				printf("data.aux_test.aux_clock_sel : %d\n", data.aux_test.aux_clock_sel);
				printf("data.aux_test.reg_addr_size : %d\n", data.aux_test.reg_addr_size);
				printf("data.aux_test.data_size : %d\n", data.aux_test.data_size);
				printf("data.aux_test.dev_addr : %d\n", data.aux_test.dev_addr);
				printf("data.aux_test.byte_num : %d\n", data.aux_test.byte_num);
				for(int i=0; i<data.aux_test.byte_num; i++)
				{
					printf("data.aux_test.reg_addr[%d] : 0x%04x\n", i, data.aux_test.reg_addr[i]);
					printf("data.aux_test.reg_data[%d] : 0x%04x\n", i, data.aux_test.reg_data[i]);
				}
				printf("SID_STD_AUX_TEST_RD_REQ(0x1052) End\n");

				memset(aux_rd_data, 0, sizeof(aux_rd_data));

				// SPI Transfer to FPGA
				usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
				FPGA_Write(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));
				usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
				FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));

				// Writing data_size	// 1:1byte, 2:2byte
				if(data.aux_test.data_size == 1)
				{
					FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_DATA_SIZE));
				}
				else if(data.aux_test.data_size == 2)
				{
					FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_DATA_SIZE));
				}

				memset(&data_ack, 0, sizeof(res_std_aux_test_rd_t));

				// Reading AUX Data
				for(int i=0; i<(int)data.aux_test.byte_num; i++)
				{
					FPGA_Write(FPGA_AUX_BYTE_NUM, ((data.aux_test.byte_num << FPGA_LSHIFT_BIT_AUX_BYTE_NUM) | ((unsigned short)i << FPGA_LSHIFT_BIT_AUX_CNT)));
					FPGA_Write(FPGA_AUX_ADDR, data.aux_test.reg_addr[i]);

//					for(int j=0; j<4; j++)
					{
//						FPGA_AND_SET(FPGA_AUX_CTRL_REG, (3 << FPGA_LSHIFT_BIT_AUX_TX_IDX));
//						FPGA_OR_SET(FPGA_AUX_CTRL_REG, (j << FPGA_LSHIFT_BIT_AUX_TX_IDX));

						usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
						FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_RD_REQ));
						usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
						FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_RD_REQ));

//						aux_return[j] = FPGA_Read(FPGA_AUX_RD_REG);
						aux_return_value = FPGA_Read(FPGA_AUX_RD_REG);

//						printf("aux_return[%d](%d) : 0x%04x\n", i, j, aux_return[j]);
						printf("aux_return_value[%d] : 0x%04x\n", i, aux_return_value);

//						if(aux_return[j] == 0)
						if(aux_return_value == 0)
						{
							aux_rd_data[i] = FPGA_Read(FPGA_AUX_RD_DATA);
//							printf("aux_rd_data(%d)[%d] : 0x%04x\n", j, i, aux_rd_data[i]);
							printf("aux_rd_data[%d] : 0x%04x\n", i, aux_rd_data[i]);
						}
					}

//					if((aux_return[0] == 0) || (aux_return[1] == 0) || (aux_return[2] == 0) || (aux_return[3] == 0))
					if(aux_return_value == 0)
					{
						data_ack.res = ACK;
					}
					else
					{
						data_ack.res = NACK;
					}
				}

				data_ack.hdr.cmd_id = RES_ID(SID_STD_AUX_TEST_RD_REQ);
				data_ack.hdr.data_len = sizeof(res_std_aux_test_rd_t)-sizeof(res_head_t);

				memcpy(&data_ack.read_data[0], aux_rd_data, (sizeof(unsigned short) * data.aux_test.byte_num));		// modify code about reading data

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_aux_test_rd_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_STD_EDP_CHARA_WR_REQ:			//0x1053
			{
				unsigned short wr_data;
				req_std_edp_chara_set_write_t data = {0,};
				res_std_edp_chara_set_write_t data_ack = {0,};
				printf("eDP Signal Characteristics write!\n");

				memset(&data, 0, sizeof(req_std_edp_chara_set_write_t));
				memcpy(&data, &rmsg.header, sizeof(req_std_edp_chara_set_write_t));

				wr_data = 0;
				wr_data |= data.edp_sig_chara[0].value[0];
				wr_data |= data.edp_sig_chara[0].value[1]<<2;
				wr_data |= data.edp_sig_chara[1].value[0]<<4;
				wr_data |= data.edp_sig_chara[1].value[1]<<6;
				wr_data |= data.edp_sig_chara[2].value[0]<<8;
				wr_data |= data.edp_sig_chara[2].value[1]<<10;
				wr_data |= data.edp_sig_chara[3].value[0]<<12;
				wr_data |= data.edp_sig_chara[3].value[1]<<14;
				FPGA_Write(EDP_TX1_AC_PARAM_READ_WRITE, wr_data);

				wr_data = 0;
				wr_data = data.edp_sig_chara[4].value[0];
				wr_data |= data.edp_sig_chara[4].value[1]<<2;
				wr_data |= data.edp_sig_chara[5].value[0]<<4;
				wr_data |= data.edp_sig_chara[5].value[1]<<6;
				wr_data |= data.edp_sig_chara[6].value[0]<<8;
				wr_data |= data.edp_sig_chara[6].value[1]<<10;
				wr_data |= data.edp_sig_chara[7].value[0]<<12;
				wr_data |= data.edp_sig_chara[7].value[1]<<14;
				FPGA_Write(EDP_TX2_AC_PARAM_READ_WRITE, wr_data);

				wr_data = 0;
				wr_data = data.edp_sig_chara[8].value[0];
				wr_data |= data.edp_sig_chara[8].value[1]<<2;
				wr_data |= data.edp_sig_chara[9].value[0]<<4;
				wr_data |= data.edp_sig_chara[9].value[1]<<6;
				wr_data |= data.edp_sig_chara[10].value[0]<<8;
				wr_data |= data.edp_sig_chara[10].value[1]<<10;
				wr_data |= data.edp_sig_chara[11].value[0]<<12;
				wr_data |= data.edp_sig_chara[11].value[1]<<14;
				FPGA_Write(EDP_TX3_AC_PARAM_READ_WRITE, wr_data);

				wr_data = 0;
				wr_data = data.edp_sig_chara[12].value[0];
				wr_data |= data.edp_sig_chara[12].value[1]<<2;
				wr_data |= data.edp_sig_chara[13].value[0]<<4;
				wr_data |= data.edp_sig_chara[13].value[1]<<6;
				wr_data |= data.edp_sig_chara[14].value[0]<<8;
				wr_data |= data.edp_sig_chara[14].value[1]<<10;
				wr_data |= data.edp_sig_chara[15].value[0]<<12;
				wr_data |= data.edp_sig_chara[15].value[1]<<14;
				FPGA_Write(EDP_TX4_AC_PARAM_READ_WRITE, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[0].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[0].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[0].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[0].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[1].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[1].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[1].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[1].value[5] << 12;   // Lane1 eq adj set
				FPGA_Write(EDP_TX1_EQ_PARAM_READ_WRITE1, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[2].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[2].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[2].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[2].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[3].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[3].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[3].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[3].value[5] << 12;   // Lane1 eq adj set
				if(data.edp_sig_chara[0].value[2])	FPGA_Write(EDP_TX1_EQ_PARAM_READ_WRITE2, wr_data|0x8000);
				else								FPGA_Write(EDP_TX1_EQ_PARAM_READ_WRITE2, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[4].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[4].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[4].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[4].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[5].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[5].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[5].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[5].value[5] << 12;   // Lane1 eq adj set
				FPGA_Write(EDP_TX2_EQ_PARAM_READ_WRITE1, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[6].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[6].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[6].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[6].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[7].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[7].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[7].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[7].value[5] << 12;   // Lane1 eq adj set
				if(data.edp_sig_chara[4].value[2])	FPGA_Write(EDP_TX2_EQ_PARAM_READ_WRITE2, wr_data|0x8000);
				else								FPGA_Write(EDP_TX2_EQ_PARAM_READ_WRITE2, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[8].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[8].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[8].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[8].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[9].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[9].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[9].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[9].value[5] << 12;   // Lane1 eq adj set
				FPGA_Write(EDP_TX3_EQ_PARAM_READ_WRITE1, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[10].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[10].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[10].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[10].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[11].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[11].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[11].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[11].value[5] << 12;   // Lane1 eq adj set
				if(data.edp_sig_chara[8].value[2])	FPGA_Write(EDP_TX3_EQ_PARAM_READ_WRITE2, wr_data|0x8000);
				else								FPGA_Write(EDP_TX3_EQ_PARAM_READ_WRITE2, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[12].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[12].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[12].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[12].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[13].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[13].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[13].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[13].value[5] << 12;   // Lane1 eq adj set
				FPGA_Write(EDP_TX4_EQ_PARAM_READ_WRITE1, wr_data);

				wr_data = 0;
				wr_data |= data.edp_sig_chara[14].value[3];         // Lane0 rx-gain
				wr_data |= data.edp_sig_chara[14].value[4] << 2;    // Lane0 eq dc-gain
				wr_data |= data.edp_sig_chara[14].value[6] << 3;    // Lane0 tx-gain
				wr_data |= data.edp_sig_chara[14].value[5] << 4;    // Lane0 eq adj Set
				wr_data |= data.edp_sig_chara[15].value[3] << 8;    // Lane1 rx-gain
				wr_data |= data.edp_sig_chara[15].value[4] << 10;   // Lane1 eq dc-gain
				wr_data |= data.edp_sig_chara[15].value[6] << 11;   // Lane1 tx-gain
				wr_data |= data.edp_sig_chara[15].value[5] << 12;   // Lane1 eq adj set
				if(data.edp_sig_chara[12].value[2])	FPGA_Write(EDP_TX4_EQ_PARAM_READ_WRITE2, wr_data|0x8000);
				else								FPGA_Write(EDP_TX4_EQ_PARAM_READ_WRITE2, wr_data);

				memset(&data_ack, 0, sizeof(data_ack));

				data_ack.hdr.cmd_id			= RES_ID(SID_STD_EDP_CHARA_WR_REQ);
				data_ack.hdr.data_len		= sizeof(res_std_edp_chara_set_write_t)-sizeof(res_head_t);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data_ack, sizeof(res_std_edp_chara_set_write_t));
				msq_send_comm(&smsg);
			}
			break;
		case SID_STD_EDP_CHARA_RD_REQ:			//0x1054
			{
				req_std_edp_chara_set_read_t data = {0,};
				unsigned short i2c_ack;
				unsigned short rd_data;

				printf("eDP Signal Characteristics Read Req!\n");

				FPGA_Write(EDP_SIGNAL_READ_REQ, 0x0001);
				memset(&data, 0, sizeof(req_std_edp_chara_set_read_t));

				usleep(100000);

				if(FPGA_Read(EDP_SIGNAL_I2C_ACK))
				{
					/*
					{
						uint16_t				ch;
						uint16_t				type[10];		//0:vod, 1:pre-emphasis, 2:eq-mode, 3:rx-gain, 4:eq dc-gain, 5:eq-setting, 6:tx-gain
						uint32_t				value[10];
					} __PACKED__ edp_chara_set_t;
					*/
					printf("fpga signal i2c read ack\n");
					i2c_ack = ACK;

					for(i=0; i<16; i++)
					{
						data.edp_sig_chara[i].type[0] = 0;
						data.edp_sig_chara[i].type[1] = 1;
						data.edp_sig_chara[i].type[2] = 2;
						data.edp_sig_chara[i].type[3] = 3;
						data.edp_sig_chara[i].type[4] = 4;
						data.edp_sig_chara[i].type[5] = 5;
						data.edp_sig_chara[i].type[6] = 6;
					}

					rd_data = FPGA_Read(EDP_TX1_AC_PARAM_READ_WRITE);
					FPGA_Write(EDP_TX1_AC_PARAM_READ_WRITE, rd_data);
					data.edp_sig_chara[0].value[0] = rd_data&0x0003;			//DP PORT1 Lane0 Vod
					data.edp_sig_chara[0].value[1] = (rd_data&0x000c)>>2;		//DP PORT1 Lane0 pre-emphasis
					data.edp_sig_chara[1].value[0] = (rd_data&0x0030)>>4;		//DP PORT1 Lane1 Vod
					data.edp_sig_chara[1].value[1] = (rd_data&0x00c0)>>6;		//DP PORT1 Lane1 pre-emphasis
					data.edp_sig_chara[2].value[0] = (rd_data&0x0300)>>8;		//DP PORT1 Lane2 Vod
					data.edp_sig_chara[2].value[1] = (rd_data&0x0c00)>>10;		//DP PORT1 Lane2 pre-emphasis
					data.edp_sig_chara[3].value[0] = (rd_data&0x3000)>>12;		//DP PORT1 Lane3 Vod
					data.edp_sig_chara[3].value[1] = (rd_data&0xc000)>>14;		//DP PORT1 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_AC_PARAM_READ_WRITE);
					FPGA_Write(EDP_TX2_AC_PARAM_READ_WRITE, rd_data);
					data.edp_sig_chara[4].value[0] = rd_data&0x0003;			//DP PORT2 Lane0 Vod
					data.edp_sig_chara[4].value[1] = (rd_data&0x000c)>>2;		//DP PORT2 Lane0 pre-emphasis
					data.edp_sig_chara[5].value[0] = (rd_data&0x0030)>>4;		//DP PORT2 Lane1 Vod
					data.edp_sig_chara[5].value[1] = (rd_data&0x00c0)>>6;		//DP PORT2 Lane1 pre-emphasis
					data.edp_sig_chara[6].value[0] = (rd_data&0x0300)>>8;		//DP PORT2 Lane2 Vod
					data.edp_sig_chara[6].value[1] = (rd_data&0x0c00)>>10;		//DP PORT2 Lane2 pre-emphasis
					data.edp_sig_chara[7].value[0] = (rd_data&0x3000)>>12;		//DP PORT2 Lane3 Vod
					data.edp_sig_chara[7].value[1] = (rd_data&0xc000)>>14;		//DP PORT2 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX3_AC_PARAM_READ_WRITE);
					FPGA_Write(EDP_TX3_AC_PARAM_READ_WRITE, rd_data);
					data.edp_sig_chara[8].value[0] = rd_data&0x0003;			//DP PORT3 Lane0 Vod
					data.edp_sig_chara[8].value[1] = (rd_data&0x000c)>>2;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[9].value[0] = (rd_data&0x0030)>>4;		//DP PORT3 Lane1 Vod
					data.edp_sig_chara[9].value[1] = (rd_data&0x00c0)>>6;		//DP PORT3 Lane1 pre-emphasis
					data.edp_sig_chara[10].value[0] = (rd_data&0x0300)>>8;		//DP PORT3 Lane2 Vod
					data.edp_sig_chara[10].value[1] = (rd_data&0x0c00)>>10;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[11].value[0] = (rd_data&0x3000)>>12;		//DP PORT3 Lane3 Vod
					data.edp_sig_chara[11].value[1] = (rd_data&0xc000)>>14;		//DP PORT3 Lane3 pre-emphasis
					rd_data = FPGA_Read(EDP_TX4_AC_PARAM_READ_WRITE);
					FPGA_Write(EDP_TX4_AC_PARAM_READ_WRITE, rd_data);
					data.edp_sig_chara[12].value[0] = rd_data&0x0003;			//DP PORT4 Lane0 Vod
					data.edp_sig_chara[12].value[1] = (rd_data&0x000c)>>2;		//DP PORT4 Lane0 pre-emphasis
					data.edp_sig_chara[13].value[0] = (rd_data&0x0030)>>4;		//DP PORT4 Lane1 Vod
					data.edp_sig_chara[13].value[1] = (rd_data&0x00c0)>>6;		//DP PORT4 Lane1 pre-emphasis
					data.edp_sig_chara[14].value[0] = (rd_data&0x0300)>>8;		//DP PORT4 Lane2 Vod
					data.edp_sig_chara[14].value[1] = (rd_data&0x0c00)>>10;		//DP PORT4 Lane2 pre-emphasis
					data.edp_sig_chara[15].value[0] = (rd_data&0x3000)>>12;		//DP PORT4 Lane3 Vod
					data.edp_sig_chara[15].value[1] = (rd_data&0xc000)>>14;		//DP PORT4 Lane3 pre-emphasis

					rd_data = FPGA_Read(EDP_TX1_EQ_PARAM_READ_WRITE1);
					FPGA_Write(EDP_TX1_EQ_PARAM_READ_WRITE1, rd_data);
					data.edp_sig_chara[0].value[3] = rd_data&0x0003;			//DP PORT1 Lane0 rx-gain
					data.edp_sig_chara[0].value[4] = (rd_data&0x0004)>>2;		//DP PORT1 Lane0 eq dc-gain
					data.edp_sig_chara[0].value[6] = (rd_data&0x0008)>>3;		//DP PORT1 Lane0 tx-gain
					data.edp_sig_chara[0].value[5] = (rd_data&0x0070)>>4;		//DP PORT1 Lane0 pre-emphasis
					data.edp_sig_chara[1].value[3] = (rd_data&0x0300)>>8;		//DP PORT1 Lane1 rx-gain
					data.edp_sig_chara[1].value[4] = (rd_data&0x0400)>>10;		//DP PORT1 Lane1 eq dc-gain
					data.edp_sig_chara[1].value[6] = (rd_data&0x0800)>>11;		//DP PORT1 Lane1 tx-gain
					data.edp_sig_chara[1].value[5] = (rd_data&0x7000)>>12;		//DP PORT1 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX1_EQ_PARAM_READ_WRITE2);
					FPGA_Write(EDP_TX1_EQ_PARAM_READ_WRITE2, rd_data);
					data.edp_sig_chara[2].value[3] = rd_data&0x0003;			//DP PORT1 Lane2 rx-gain
					data.edp_sig_chara[2].value[4] = (rd_data&0x0004)>>2;		//DP PORT1 Lane2 eq dc-gain
					data.edp_sig_chara[2].value[6] = (rd_data&0x0008)>>3;		//DP PORT1 Lane2 tx-gain
					data.edp_sig_chara[2].value[5] = (rd_data&0x0070)>>4;		//DP PORT1 Lane2 pre-emphasis
					data.edp_sig_chara[3].value[3] = (rd_data&0x0300)>>8;		//DP PORT1 Lane3 rx-gain
					data.edp_sig_chara[3].value[4] = (rd_data&0x0400)>>10;		//DP PORT1 Lane3 eq dc-gain
					data.edp_sig_chara[3].value[6] = (rd_data&0x0800)>>11;		//DP PORT1 Lane3 tx-gain
					data.edp_sig_chara[3].value[5] = (rd_data&0x7000)>>12;		//DP PORT1 Lane3 pre-emphasis

					data.edp_sig_chara[0].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane0 eq-mode
					data.edp_sig_chara[1].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane1 eq-mode
					data.edp_sig_chara[2].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane2 eq-mode
					data.edp_sig_chara[3].value[2] = (rd_data&0x8000)>>15;		//DP PORT1 Lane3 eq-mode
					printf("EQ-MODE = %d\n", data.edp_sig_chara[0].value[2]);

					rd_data = FPGA_Read(EDP_TX2_EQ_PARAM_READ_WRITE1);
					FPGA_Write(EDP_TX2_EQ_PARAM_READ_WRITE1, rd_data);
					data.edp_sig_chara[4].value[3] = rd_data&0x0003;			//DP PORT2 Lane0 rx-gain
					data.edp_sig_chara[4].value[4] = (rd_data&0x0004)>>2;		//DP PORT2 Lane0 eq dc-gain
					data.edp_sig_chara[4].value[6] = (rd_data&0x0008)>>3;		//DP PORT2 Lane0 tx-gain
					data.edp_sig_chara[4].value[5] = (rd_data&0x0070)>>4;		//DP PORT2 Lane0 pre-emphasis
					data.edp_sig_chara[5].value[3] = (rd_data&0x0300)>>8;		//DP PORT2 Lane1 rx-gain
					data.edp_sig_chara[5].value[4] = (rd_data&0x0400)>>10;		//DP PORT2 Lane1 eq dc-gain
					data.edp_sig_chara[5].value[6] = (rd_data&0x0800)>>11;		//DP PORT2 Lane1 tx-gain
					data.edp_sig_chara[5].value[5] = (rd_data&0x7000)>>12;		//DP PORT2 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX2_EQ_PARAM_READ_WRITE2);
					FPGA_Write(EDP_TX2_EQ_PARAM_READ_WRITE2, rd_data);
					data.edp_sig_chara[6].value[3] = rd_data&0x0003;			//DP PORT2 Lane2 rx-gain
					data.edp_sig_chara[6].value[4] = (rd_data&0x0004)>>2;		//DP PORT2 Lane2 eq dc-gain
					data.edp_sig_chara[6].value[6] = (rd_data&0x0008)>>3;		//DP PORT2 Lane2 tx-gain
					data.edp_sig_chara[6].value[5] = (rd_data&0x0070)>>4;		//DP PORT2 Lane2 pre-emphasis
					data.edp_sig_chara[7].value[3] = (rd_data&0x0300)>>8;		//DP PORT2 Lane3 rx-gain
					data.edp_sig_chara[7].value[4] = (rd_data&0x0400)>>10;		//DP PORT2 Lane3 eq dc-gain
					data.edp_sig_chara[7].value[6] = (rd_data&0x0800)>>11;		//DP PORT2 Lane3 tx-gain
					data.edp_sig_chara[7].value[5] = (rd_data&0x7000)>>12;		//DP PORT2 Lane3 pre-emphasis

					data.edp_sig_chara[4].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane0 eq-mode
					data.edp_sig_chara[5].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane1 eq-mode
					data.edp_sig_chara[6].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane2 eq-mode
					data.edp_sig_chara[7].value[2] = (rd_data&0x8000)>>15;		//DP PORT2 Lane3 eq-mode
					printf("EQ-MODE = %d\n", data.edp_sig_chara[4].value[2]);

					rd_data = FPGA_Read(EDP_TX3_EQ_PARAM_READ_WRITE1);
					FPGA_Write(EDP_TX3_EQ_PARAM_READ_WRITE1, rd_data);
					data.edp_sig_chara[8].value[3] = rd_data&0x0003;			//DP PORT3 Lane0 rx-gain
					data.edp_sig_chara[8].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane0 eq dc-gain
					data.edp_sig_chara[8].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane0 tx-gain
					data.edp_sig_chara[8].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane0 pre-emphasis
					data.edp_sig_chara[9].value[3] = (rd_data&0x0300)>>8;		//DP PORT3 Lane1 rx-gain
					data.edp_sig_chara[9].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane1 eq dc-gain
					data.edp_sig_chara[9].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane1 tx-gain
					data.edp_sig_chara[9].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX3_EQ_PARAM_READ_WRITE2);
					FPGA_Write(EDP_TX3_EQ_PARAM_READ_WRITE2, rd_data);
					data.edp_sig_chara[10].value[3] = rd_data&0x0003;			//DP PORT3 Lane2 rx-gain
					data.edp_sig_chara[10].value[4] = (rd_data&0x0004)>>2;		//DP PORT3 Lane2 eq dc-gain
					data.edp_sig_chara[10].value[6] = (rd_data&0x0008)>>3;		//DP PORT3 Lane2 tx-gain
					data.edp_sig_chara[10].value[5] = (rd_data&0x0070)>>4;		//DP PORT3 Lane2 pre-emphasis
					data.edp_sig_chara[11].value[3] = (rd_data&0x0300)>>8;		//DP PORT3 Lane3 rx-gain
					data.edp_sig_chara[11].value[4] = (rd_data&0x0400)>>10;		//DP PORT3 Lane3 eq dc-gain
					data.edp_sig_chara[11].value[6] = (rd_data&0x0800)>>11;		//DP PORT3 Lane3 tx-gain
					data.edp_sig_chara[11].value[5] = (rd_data&0x7000)>>12;		//DP PORT3 Lane3 pre-emphasis

					data.edp_sig_chara[8].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane0 eq-mode
					data.edp_sig_chara[9].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane1 eq-mode
					data.edp_sig_chara[10].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane2 eq-mode
					data.edp_sig_chara[11].value[2] = (rd_data&0x8000)>>15;		//DP PORT3 Lane3 eq-mode
					printf("EQ-MODE = %d\n", data.edp_sig_chara[8].value[2]);

					rd_data = FPGA_Read(EDP_TX4_EQ_PARAM_READ_WRITE1);
					FPGA_Write(EDP_TX4_EQ_PARAM_READ_WRITE1, rd_data);
					data.edp_sig_chara[12].value[3] = rd_data&0x0003;			//DP PORT4 Lane0 rx-gain
					data.edp_sig_chara[12].value[4] = (rd_data&0x0004)>>2;		//DP PORT4 Lane0 eq dc-gain
					data.edp_sig_chara[12].value[6] = (rd_data&0x0008)>>3;		//DP PORT4 Lane0 tx-gain
					data.edp_sig_chara[12].value[5] = (rd_data&0x0070)>>4;		//DP PORT4 Lane0 pre-emphasis
					data.edp_sig_chara[13].value[3] = (rd_data&0x0300)>>8;		//DP PORT4 Lane1 rx-gain
					data.edp_sig_chara[13].value[4] = (rd_data&0x0400)>>10;		//DP PORT4 Lane1 eq dc-gain
					data.edp_sig_chara[13].value[6] = (rd_data&0x0800)>>11;		//DP PORT4 Lane1 tx-gain
					data.edp_sig_chara[13].value[5] = (rd_data&0x7000)>>12;		//DP PORT4 Lane1 pre-emphasis
					rd_data = FPGA_Read(EDP_TX4_EQ_PARAM_READ_WRITE2);
					FPGA_Write(EDP_TX4_EQ_PARAM_READ_WRITE2, rd_data);
					data.edp_sig_chara[14].value[3] = rd_data&0x0003;			//DP PORT4 Lane2 rx-gain
					data.edp_sig_chara[14].value[4] = (rd_data&0x0004)>>2;		//DP PORT4 Lane2 eq dc-gain
					data.edp_sig_chara[14].value[6] = (rd_data&0x0008)>>3;		//DP PORT4 Lane2 tx-gain
					data.edp_sig_chara[14].value[5] = (rd_data&0x0070)>>4;		//DP PORT4 Lane2 pre-emphasis
					data.edp_sig_chara[15].value[3] = (rd_data&0x0300)>>8;		//DP PORT4 Lane3 rx-gain
					data.edp_sig_chara[15].value[4] = (rd_data&0x0400)>>10;		//DP PORT4 Lane3 eq dc-gain
					data.edp_sig_chara[15].value[6] = (rd_data&0x0800)>>11;		//DP PORT4 Lane3 tx-gain
					data.edp_sig_chara[15].value[5] = (rd_data&0x7000)>>12;		//DP PORT4 Lane3 pre-emphasis

					data.edp_sig_chara[12].value[2] = (rd_data&0x8000)>>15;		//DP PORT4 Lane0 eq-mode
					data.edp_sig_chara[13].value[2] = (rd_data&0x8000)>>15;		//DP PORT4 Lane1 eq-mode
					data.edp_sig_chara[14].value[2] = (rd_data&0x8000)>>15;		//DP PORT4 Lane2 eq-mode
					data.edp_sig_chara[15].value[2] = (rd_data&0x8000)>>15;		//DP PORT4 Lane3 eq-mode
					printf("EQ-MODE = %d\n", data.edp_sig_chara[12].value[2]);

					printf("fpga signal i2c read complete\n");
				}
				else
				{
					printf("fpga signal read nack\n");
					i2c_ack = NACK;
				}
				FPGA_Write(EDP_SIGNAL_READ_REQ, 0x0000);

				data.hdr.cmd_id			= RES_ID(SID_STD_EDP_CHARA_RD_REQ);
				data.hdr.data_len		= sizeof(req_std_edp_chara_set_read_t)-sizeof(res_head_t);
				data.res				= i2c_ack;

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &data, sizeof(req_std_edp_chara_set_read_t));
				msq_send_comm(&smsg);
			}
			break;
		default:
			fprintf(stderr, "[0x%X]unknown tcp command id!\n", rmsg.header.cmd_id);
			break;
		}
	}
}
