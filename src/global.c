/*
 ============================================================================
 Name        : global.c
 Author      : jschoi
 Version     : 1.0
 Copyright   : copyright (c) 2017 ensis.co.,Ltd.
 Description : C, Ansi-style
 ============================================================================
 */

#include "global.h"
#include <i2c_dvi.h>
#include <i2c_gpio.h>
#include <fpga_spi.h>
#include <rcb.h>
#include <gpio.h>
#include <i2c_sii9135.h>
#include <pattern_control.h>
#include <pattern.h>

#include <model_data.h>
#include <group_data.h>
#include <msg_comu.h>
#include <gx.h>

struct route_info
{
	struct in_addr dstAddr;
	struct in_addr srcAddr;
	struct in_addr gateWay;
	char ifName[IF_NAMESIZE];
};

void create_dir(void)
{
	char	path[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_CONFIG);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_MODEL);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

#ifdef GROUP_SELECT
	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_GROUP);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);
#endif

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_PATTERN);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_BMP);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_PNG);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_MOV);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_FW);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_FPGA);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_FONT);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_SCHEDULE);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_HANDO);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s", DIR_ROOT, DIR_ETC);
	if( 0 != access(path, F_OK) ) mkdir(path, 0776);

}

int task_thread(int priority, void *taskfunc)
{
	int ret_val, scope;
	pthread_attr_t attr;
	struct sched_param param;

	if(task_cnt>=MAX_TASK_CNT)
		DEBUG_PRINT("Task count over!!!(Max:%d)", MAX_TASK_CNT);

	(void)pthread_attr_init(&attr);

	ret_val = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	ret_val |= pthread_attr_getscope(&attr, &scope);
	if (scope != PTHREAD_SCOPE_SYSTEM) {
		scope = PTHREAD_SCOPE_SYSTEM;
		ret_val |= pthread_attr_setscope(&attr, scope);
	}
	ret_val |= pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = sched_get_priority_min(SCHED_RR) + priority;
	ret_val |= pthread_attr_setschedparam(&attr, &param);
	ret_val |= pthread_create(&task_id[task_cnt], /*&attr*/NULL, taskfunc, (void *)NULL);

	task_cnt++;
	return(ret_val);
}

void do_aprf(void)
{
	if(APRF) {printf("Aprt Off!\r\n"); APRF = 0;}
	else {printf("Aprf On\r\n"); APRF = 1;}
}
void command_system(char* cmd)
{
	if(system(cmd)<0)
	{
		printf("system command fail\n");
	}
}

void *shell_cmd_thread(void *arg)
{
	char 	cmd[MAX_PATH], prompt[20];
	int		size = 0;

	char	data[64]={0,};
	signed short	in_data[20]={0,};
	int i=0;
	//net_info_t net[10];
	//int cnt=0;

	memset(prompt, 0, sizeof(prompt));
	sprintf(prompt, "[%s_%.1f]", PG_NAME, FW_VERSION/10.0);

	while(!exit_flag)
	{
		usleep(50000);	// jschoi 2018.01.06 cpu load 100%
		memset(cmd, 0, MAX_PATH);
		if( NULL != fgets(cmd, sizeof(cmd), stdin) )
		{
			size = strlen(cmd);
			if(size>0) cmd[size-1] = '\0';

			if(size==0) {}
			else if(!strcmp(cmd, "exit")) exit_flag = 1;
			else if(!strcmp(cmd, "ediw"))
			{
				unsigned char ucedid[128];

				memset(ucedid, 0, sizeof(ucedid));
				write_edid_to_eeprom(ucedid);
			}
			else if(!strcmp(cmd, "edir"))	read_edid_to_eeprom();
			else if(!strcmp(cmd, "ifon")) 	FPGA_Write(FPGA_IF_PWR_CTRL, 0x0007);	// i/f power on
			else if(!strcmp(cmd, "ifoff")) 	FPGA_Write(FPGA_IF_PWR_CTRL, 0x0000);	// i/f power off
			else if(!strcmp(cmd, "ddcon"))	i2c_ddc_sel(HDMI_I2C_EN);
			else if(!strcmp(cmd, "ddcoff"))	i2c_ddc_sel(2);
			else if(!strcmp(cmd, "hpdon"))	i2c_hdmi_hpd_set(1);
			else if(!strcmp(cmd, "hpdoff"))	i2c_hdmi_hpd_set(0);
			else if(!strcmp(cmd, "hpon"))	FPGA_Write(FPGA_DP_RX_CTRL, 0x0003);
			else if(!strcmp(cmd, "hpoff"))	FPGA_Write(FPGA_DP_RX_CTRL, 0x0000);
			else if(!strcmp(cmd, "setedp"))	output_set_edp();
			else if(!strcmp(cmd, "aprf"))	do_aprf();
//			else if(!strcmp(cmd, "test"))
//			{
//				unsigned short n=0,m=0,match=0;
//
//				for(n=1;n<16;n++){
//					for(m=1;m<5;m++){
//						if( (n==10) && (m==3) ){
//							match=1;
//							printf("match n=%d, m=%d\n", n,m);
//							break;
//						}
//					}
//					if (match) break;
//				}
//				printf("end n=%d, end m=%d\n", n,m);
//			}
//			else if(!strcmp(cmd, "test1"))
//			{
//				unsigned short n=0,m=0,match=0;
//
//				for(n=1;n<16;n++){
//					for(m=1;m<5;m++){
//					}
//				}
//				printf("end n=%d, end m=%d\n", n,m);
//			}
//			else if(!strcmp(cmd, "test2"))
//			{
//				unsigned short n=0,m=0,match=0;
//
//				for(n=1;n<16;n++){
//					for(m=1;m<5;m++){
//					}
//					if(n==15) break;
//				}
//				printf("end n=%d, end m=%d\n", n,m);
//			}

			else if(!strcmp(cmd, "sii0")) sii9135_init();

			else if(!strcmp(cmd, "vacu0")) re_driver_mux_ACnU_set(0);
			else if(!strcmp(cmd, "vacu1")) re_driver_mux_ACnU_set(1);
			else if(!strcmp(cmd, "vacu2")) re_driver_mux_ACnU_set(2);
			else if(!strcmp(cmd, "vacu3")) re_driver_mux_ACnU_set(3);
			else if(!strcmp(cmd, "vacul0")) re_driver_mux_ACnL_set(0);
			else if(!strcmp(cmd, "vacul1")) re_driver_mux_ACnL_set(1);
			else if(!strcmp(cmd, "vacul2")) re_driver_mux_ACnL_set(2);
			else if(!strcmp(cmd, "vacul3")) re_driver_mux_ACnL_set(3);
			else if(!strcmp(cmd, "dcn0")) re_driver_mux_DCn_set(0);
			else if(!strcmp(cmd, "dcn1")) re_driver_mux_DCn_set(1);
			else if(!strcmp(cmd, "dcn2")) re_driver_mux_DCn_set(2);
			else if(!strcmp(cmd, "dcn3")) re_driver_mux_DCn_set(3);
			else if(!strcmp(cmd, "i2cm")) {
				printf("I2C MASTER CLK SEL		 = 0x%04x\n", 0x0001);
				printf("I2C MASTER REG ADDR SIZE = 0x%04x\n", 0x0002);
				printf("I2C MASTER REG DATA SIZE = 0x%04x\n", 0x0002);
				printf("I2C MASTER DEV ADDR		 = 0x%04x\n", 0x0093);
				printf("I2C MASTER REG ADDR	  	 = 0x%04x\n", 0x356a);
				printf("I2C MASTER REG DATA	 	 = 0x%04x\n", 0x28c9);
				printf("I2C MASTER EN			 = low->high\n");

				i2c_gpio_set(GPIO_EX16, 0x01);

				FPGA_Write(FPGA_I2C_MASTER_CLK_SEL, 0x0001);
				FPGA_Write(FPGA_I2C_MASTER_REG_ADDR_SIZE, 0x0002);
				FPGA_Write(FPGA_I2C_MASTER_REG_DATA_SIZE, 0x0002);
				FPGA_Write(FPGA_I2C_MASTER_DEV_ADDR, 0x0093);
				FPGA_Write(FPGA_I2C_MASTER_REG_ADDR, 0x356a);
				FPGA_Write(FPGA_I2C_MASTER_REG_DATA, 0x28c9);
				printf("ack : %d\n", FPGA_Read(FPGA_I2C_MASTER_ACK));
				usleep(100);
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);
				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0001);
				usleep(100);

				i2c_gpio_set(GPIO_EX16, 0x00);
			}
			else if(!strcmp(cmd, "i2cs")) {

				printf("I2C SLAVE CLK SEL		= 0x%04x\n", 0x0001);
				printf("I2C SLAVE REG ADDR SIZE = 0x%04x\n", 0x0002);
				printf("I2C SLAVE REG DATA SIZE = 0x%04x\n", 0x0002);
				printf("I2C SLAVE DEV ADDR		= 0x%04x\n", 0x0094);
				printf("I2C SLAVE REG ADDR		= 0x%04x\n", 0x356a);
				printf("I2C SLAVE REG DATA		= 0x%04x\n", 0x28c9);
				printf("I2C SLAVE EN			= low->high\n");

				i2c_gpio_set(GPIO_EX17, 0x01);

				FPGA_Write(FPGA_I2C_SLAVE_CLK_SEL, 0x0001);
				FPGA_Write(FPGA_I2C_SLAVE_REG_ADDR_SIZE, 0x0002);
				FPGA_Write(FPGA_I2C_SLAVE_REG_DATA_SIZE, 0x0002);
				FPGA_Write(FPGA_I2C_SLAVE_DEV_ADDR, 0x0094);
				FPGA_Write(FPGA_I2C_SLAVE_REG_ADDR, 0x356a);
				FPGA_Write(FPGA_I2C_SLAVE_REG_DATA, 0x28c9);
				printf("ack : %d\n", FPGA_Read(FPGA_I2C_SLAVE_ACK));
				usleep(100);
				FPGA_Write(FPGA_I2C_SLAVE_EN, 0x0000);
				FPGA_Write(FPGA_I2C_SLAVE_EN, 0x0001);
				usleep(100);

				i2c_gpio_set(GPIO_EX17, 0x00);
			}

			else if(!strcmp(cmd, "eg0")) i2c_gpio_set(GPIO_EX14, 0x00);
			else if(!strcmp(cmd, "eg1")) i2c_gpio_set(GPIO_EX14, 0x01);

			else if(!strcmp(cmd, "mode")) i2c_gpio_set(GPIO_EX10, 0x00);
			else if(!strcmp(cmd, "eg1")) i2c_gpio_set(GPIO_EX10, 0x01);

			else if(!strcmp(cmd, "eg0")) i2c_gpio_set(GPIO_EX11, 0x00);
			else if(!strcmp(cmd, "eg1")) i2c_gpio_set(GPIO_EX11, 0x01);

			else if(!strcmp(cmd, "eg0")) i2c_gpio_set(GPIO_EX12, 0x00);
			else if(!strcmp(cmd, "eg1")) i2c_gpio_set(GPIO_EX12, 0x01);

			else if(!strcmp(cmd, "eg0")) i2c_gpio_set(GPIO_EX13, 0x00);
			else if(!strcmp(cmd, "eg1")) i2c_gpio_set(GPIO_EX13, 0x01);

			else if(!strcmp(cmd, "hrst")) {
				i2c_gpio_set(GPIO_EX8, 0x00);
				sleep(1);
				i2c_gpio_set(GPIO_EX8, 0x01);
				printf("reset 0\n");
			}
			else if(!strcmp(cmd, "srd"))	sii9135_video_in_reg_read();
			else if(!strcmp(cmd, "hpd1")){
				i2c_gpio_set(GPIO_EX9, 0x01);
				printf("hpd 1\n");
			}
			else if(!strcmp(cmd, "hpd0")) {
				i2c_gpio_set(GPIO_EX9, 0x00);
				printf("hpd 0\n");
			}
			else if(!strcmp(cmd, "preq")) {
				pwr_ver_req(1);
				version_osung_end=RES_NACK;
				version_start=1;
			}
			else if(!strncmp(cmd, "scf", 3)) {
				if		( (cmd[3]-'0') < 1) scroll_frame_num=1;
				else if ( (cmd[3]-'0') > 9) scroll_frame_num=9;
				else						scroll_frame_num=cmd[3]-'0';

				printf("scroll frame set: %d\n", scroll_frame_num);
			}
			else if(!strncmp(cmd, "rdclk", 5)) {

				printf("Read FPGA_VAR_TCLK = %d\n",(((uint32_t)FPGA_Read(FPGA_VAR_TCLK_HIGH))<<16)+((uint32_t)FPGA_Read(FPGA_VAR_TCLK_LOW)));
			}
			else if(!strncmp(cmd, "sigon", 5)) {
				FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_ON);
			}
			else if(!strncmp(cmd, "sigoff", 6)) {
				FPGA_Write(FPGA_VX1_SIG_ONOFF, ENUM_OFF);
			}
			else if(!strncmp(cmd, "scd", 3)) {
				if(cmd[3]=='0')		set_pattern_scroll(3, 1, group_data.pat[pattern_index].scroll_speed);
				else if(cmd[3]=='1')	set_pattern_scroll(4, 1, group_data.pat[pattern_index].scroll_speed);
				else				set_pattern_scroll(group_data.pat[pattern_index].scroll_direction, 1, group_data.pat[pattern_index].scroll_speed);
			}
			else if(!strncmp(cmd, "idx", 3)) {
				gp.gx_ai_enable=0;
				if(preload_data[pattern_index].use>0)
				{
					player_stop();
					FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag set to 0

					printf("preload_pattern_change = %d\n", pattern_index);
					if(DRAWCHECK(model_data.if_type))
					{
						FPGA_Write(FPGA_MEM_RD_BANK, preload_data[pattern_index].index);
					}
					player_stop();
					FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001); // MOV write flag
				}
			}
//			else if(!strncmp(cmd, "mpck", 4)) {
//				unsigned short tmp=0;
//
//				tmp=FPGA_Read(FPGA_MODE);
//
////				printf("%s %s %s %s\n", g_type_name[(tmp>>4)&0x3], g_out_name[tmp&0x7], g_twist_name[(tmp>>8)&0x3], g_divide_name[(tmp>>12)&0x3]);
//
//			}
			else if(!strncmp(cmd, "wbc", 3)) {
				unsigned short tmp=0;

				if( (cmd[3]>=0x30) && (cmd[3] < 0x3a) )	tmp += (cmd[3]-'0')*100;
				if( (cmd[4]>=0x30) && (cmd[4] < 0x3a) )	tmp += (cmd[4]-'0')*10;
				if( (cmd[5]>=0x30) && (cmd[5] < 0x3a) )	tmp += (cmd[5]-'0');

				printf("change write bank = %d\n", tmp);
				FPGA_Write(FPGA_MEM_WR_BANK,tmp);
			}
			else if(!strncmp(cmd, "rbc", 3)) {
				unsigned short tmp=0;

				if( (cmd[3]>=0x30) && (cmd[3] < 0x3a) )	tmp += (cmd[3]-'0')*100;
				if( (cmd[4]>=0x30) && (cmd[4] < 0x3a) )	tmp += (cmd[4]-'0')*10;
				if( (cmd[5]>=0x30) && (cmd[5] < 0x3a) )	tmp += (cmd[5]-'0');

				printf("change read bank = %d\n", tmp);
				FPGA_Write(FPGA_MEM_RD_BANK,tmp);
			}
			else if(!strncmp(cmd, "pint", 4)) {
				unsigned short tmp=0;

				if( (cmd[4]>=0x30) && (cmd[4] < 0x3a) )	tmp += (cmd[4]-'0')*100;
				if( (cmd[5]>=0x30) && (cmd[5] < 0x3a) )	tmp += (cmd[5]-'0')*10;
				if( (cmd[6]>=0x30) && (cmd[6] < 0x3a) )	tmp += (cmd[6]-'0');

				printf("pll interval = %d ms\n", tmp);
				pll_interval_time = tmp;
			}
/*
			else if(!strncmp(cmd, "vod", 3)) {
				unsigned short tmp=0;

				if( (cmd[3]>=0x30) && (cmd[3] < 0x3a) )	tmp = (cmd[3]-'0')*10;
				if( (cmd[4]>=0x30) && (cmd[4] < 0x3a) )	tmp += (cmd[4]-'0');

				FPGA_Write(FPGA_XCVR_VOD, tmp & 0x001f);
				usleep(100);
				printf("Set Vod : %d\t0x%04x\n", tmp, FPGA_Read(FPGA_XCVR_VOD));
			}
			else if(!strncmp(cmd, "1post", 5)) {
				unsigned short tmp=0,pol=0, buf=0;

				if ( ((cmd[5]>=0x30) && (cmd[5] < 0x3a)) && ((cmd[6]>=0x30) && (cmd[6] < 0x3a)) ) {
					tmp = (cmd[5]-'0')*10;
					tmp += (cmd[6]-'0');

					buf=FPGA_Read(FPGA_XCVR_1POST_TAP)&0x0040;
					FPGA_Write(FPGA_XCVR_1POST_TAP, buf | (tmp&0x001f));
					usleep(100);
					printf("Set 1post : %d\t0x%04x\n", tmp, FPGA_Read(FPGA_XCVR_1POST_TAP));

				}
				else if( (cmd[5]==0x2b) | (cmd[5]==0x2d) ) {	//0x2b = +,  0x2d = -
					if		(cmd[5]==0x2b)	pol=0;
					else if (cmd[5]==0x2d)	pol=1;

					buf=FPGA_Read(FPGA_XCVR_1POST_TAP)&0x001f;
					FPGA_Write(FPGA_XCVR_1POST_TAP, buf | (pol<<6));
					usleep(100);
					printf("Set 1post polar : %c\t0x%04x\n", pol?'-':'+', FPGA_Read(FPGA_XCVR_1POST_TAP));
				}


			}
			else if(!strncmp(cmd, "2post", 5)) {
				unsigned short tmp=0,pol=0, buf=0;

				if ( ((cmd[5]>=0x30) && (cmd[5] < 0x3a)) && ((cmd[6]>=0x30) && (cmd[6] < 0x3a)) ) {
					tmp = (cmd[5]-'0')*10;
					tmp += (cmd[6]-'0');

					buf=FPGA_Read(FPGA_XCVR_2POST_TAP)&0x0020;
					FPGA_Write(FPGA_XCVR_2POST_TAP, buf | (tmp&0x000f));
					usleep(100);
					printf("Set 2post : %d\t0x%04x\n", tmp, FPGA_Read(FPGA_XCVR_2POST_TAP));

				}
				else if( (cmd[5]==0x2b) | (cmd[5]==0x2d) ) {	//0x2b = +,  0x2d = -
					if		(cmd[5]==0x2b)	pol=0;
					else if (cmd[5]==0x2d)	pol=1;

					buf=FPGA_Read(FPGA_XCVR_2POST_TAP)&0x000f;
					FPGA_Write(FPGA_XCVR_2POST_TAP, buf | (pol<<5));
					usleep(100);
					printf("Set 2post polar : %c\t0x%04x\n", pol?'-':'+', FPGA_Read(FPGA_XCVR_2POST_TAP));
				}
			}

			else if(!strncmp(cmd, "1pre", 4)) {
				unsigned short tmp=0,pol=0, buf=0;

				if ( ((cmd[4]>=0x30) && (cmd[4] < 0x3a)) && ((cmd[5]>=0x30) && (cmd[5] < 0x3a)) ) {
					tmp = (cmd[4]-'0')*10;
					tmp += (cmd[5]-'0');

					buf=FPGA_Read(FPGA_XCVR_1PRE_TAP)&0x0020;
					FPGA_Write(FPGA_XCVR_1PRE_TAP, buf | (tmp&0x001f));
					usleep(100);
					printf("Set 1pre : %d\t0x%04x\n", tmp, FPGA_Read(FPGA_XCVR_1PRE_TAP));

				}
				else if( (cmd[4]==0x2b) | (cmd[4]==0x2d) ) {	//0x2b = +,  0x2d = -
					if		(cmd[4]==0x2b)	pol=0;
					else if (cmd[4]==0x2d)	pol=1;

					buf=FPGA_Read(FPGA_XCVR_1PRE_TAP)&0x001f;
					FPGA_Write(FPGA_XCVR_1PRE_TAP, buf | (pol<<5));
					usleep(100);
					printf("Set 1pre polar : %c\t0x%04x\n", pol?'-':'+', FPGA_Read(FPGA_XCVR_1PRE_TAP));
				}
			}
			else if(!strncmp(cmd, "2pre", 4)) {
				unsigned short tmp=0,pol=0, buf=0;

				if ( ((cmd[4]>=0x30) && (cmd[4] < 0x3a)) && ((cmd[5]>=0x30) && (cmd[5] < 0x3a)) ) {
					tmp = (cmd[4]-'0')*10;
					tmp += (cmd[5]-'0');

					buf=FPGA_Read(FPGA_XCVR_2PRE_TAP)&0x0010;
					FPGA_Write(FPGA_XCVR_2PRE_TAP, buf | (tmp&0x0007));
					usleep(100);
					printf("Set 2pre : %d\t0x%04x\n", tmp, FPGA_Read(FPGA_XCVR_2PRE_TAP));

				}
				else if( (cmd[4]==0x2b) | (cmd[4]==0x2d) ) {	//0x2b = +,  0x2d = -
					if		(cmd[4]==0x2b)	pol=0;
					else if (cmd[4]==0x2d)	pol=1;

					buf=FPGA_Read(FPGA_XCVR_2PRE_TAP)&0x0007;
					FPGA_Write(FPGA_XCVR_2PRE_TAP, buf | (pol<<4));
					usleep(100);
					printf("Set 2pre polar : %c\t0x%04x\n", pol?'-':'+', FPGA_Read(FPGA_XCVR_2PRE_TAP));

				}
			}
			else if(!strncmp(cmd, "pemdef", 6)) {
				FPGA_pre_emphasis_default();
			}
*/
			else if(!strncmp(cmd, "vddon", 5)) {
//				vdd_onoff_control(ENUM_ON);
				i2c_gpio_set(GPIO_EX18, 0x00);
			}
			else if(!strncmp(cmd, "vddoff", 6)) {
//				vdd_onoff_control(ENUM_OFF);
				i2c_gpio_set(GPIO_EX18, 0x01);
			}

			else if(!strncmp(cmd, "mrb", 3)) {
				unsigned short tmp=0;

				if( (cmd[3]>=0x30) && (cmd[3] < 0x3a) )	tmp = (cmd[3]-'0');
				printf("MRB = %d\n", tmp);

				FPGA_Write(FPGA_MEM_RD_BANK, tmp);
			}

			else if(!strncmp(cmd, "gt", 2)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+3);
				sscanf(data,"%d",&in_data[0]);
				printf("gamma=%d\t0x%04x\n", in_data[0], in_data[0]);

				if(in_data[0]<0)	in_data[0]-=1;
				FPGA_Write_signed(FPGA_GRAY_LEVEL_R,in_data[0]);
				FPGA_Write_signed(FPGA_GRAY_LEVEL_G,in_data[0]);
				FPGA_Write_signed(FPGA_GRAY_LEVEL_B,in_data[0]);
			}
			else if(!strncmp(cmd, "gamr", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);
				printf("gamma r=%d\t0x%04x\n", in_data[0], in_data[0]);

				if(in_data[0]<0)	in_data[0]-=1;
				FPGA_Write_signed(FPGA_GRAY_LEVEL_R,in_data[0]);
			}
			else if(!strncmp(cmd, "gamg", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);
				printf("gamma g=%d\t0x%04x\n", in_data[0], in_data[0]);

				if(in_data[0]<0)	in_data[0]-=1;
				FPGA_Write_signed(FPGA_GRAY_LEVEL_G,in_data[0]);
			}
			else if(!strncmp(cmd, "gamb", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);
				printf("gamma b=%d\t0x%04x\n", in_data[0], in_data[0]);

				if(in_data[0]<0)	in_data[0]-=1;
				FPGA_Write_signed(FPGA_GRAY_LEVEL_B,in_data[0]);
			}
			else if(!strncmp(cmd, "aien", 4)) {
				gp.gx_ai_enable=1;

				FPGA_Write(FPGA_MEM_RD_CTRL, 0x0002);			//ai pattern enable
				printf("ai pattern enable\n");
			}

			else if(!strncmp(cmd, "arsize", 6)) {
				printf("pattern head array size=%d\n", sizeof(pattern_head_t));
			}

			else if(!strncmp(cmd, "mvx", 3)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+4);
				sscanf(data,"%d",&in_data[0]);
				printf("mov-x=%d\n", in_data[0]);

				FPGA_Write(FPGA_MOV_POS_X,in_data[0]);
			}
			else if(!strncmp(cmd, "mvy", 3)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+4);
				sscanf(data,"%d",&in_data[0]);
				printf("mov-y=%d\n", in_data[0]);

				FPGA_Write(FPGA_MOV_POS_Y,in_data[0]);
			}

			else if(!strncmp(cmd, "mlist", 5)) {
				model_init();
			}
			else if(!strncmp(cmd, "glist", 5)) {
				group_init();
			}
			else if(!strncmp(cmd, "mset", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);
				printf("model index set=%d\n", in_data[0]);
/*
				model_idx = in_data[0];

				if(model_cnt>0)
				{
//					memset(model_name,0,sizeof(model_name));
//					memcpy(model_name, model_list[model_idx], sizeof(model_name));

					memset(model_name, 0, MAX_MODEL_NAME);
					sprintf(model_name, "%s", model_list[model_idx]);

					reload_model_flag=1;
//					rcb_group_change(group_init());
				}
				else
				{
					reload_model_flag=0;
					memset(old_model_name,0,sizeof(old_model_name));
					printf("Require model list update!!\n");
				}
*/
				printf("model change = %s\n", data);

				set_opmode(MODEL_CHANGE);
				model_update();
				adim_change_flag = 0;



				if(model_select((char*)data, 0)==ACK)
				{
					model_selection_end = ACK;
					set_model_init_status(ACK);
					set_curmodel_idx((char*)data);

					printf("WAITING GROUP...\n");
				}
				else
				{
					model_selection_end = NACK;
					set_model_init_status(NACK);

					memset(old_model_name,0,sizeof(old_model_name));
					model_init();
					rcb_model_change();
				}

				model_variable_reset();

			}
			else if(!strncmp(cmd, "gset", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);
				printf("group index set=%d and start loading\n", in_data[0]);
/*
				group_idx = in_data[0];

				if(group_cnt>0)
				{
					if( (strcmp(old_model_name,model_name)) || (reload_model_flag) )
					{
						model_selection_end = model_select(model_name, 0);
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
//						rcb_model_change();
					}
				}
				else
				{
					group_idx = 0;
					printf("Require group list update!!\n");
//					rcb_write(rcb_fd, RCB_LINE1, TITLE_GROUP_SELECT);
//					rcb_write(rcb_fd, RCB_LINE2, "NO GROUP FILE!");
				}
*/
				printf("group change = %s\n", data);

				set_opmode(GROUP_CHANGE);
				group_list_load();

				if(group_select((char*)data)==ACK)
				{
					group_selection_end = ACK;
					set_curgroup_idx((char*)data);

					set_opmode(READY);
				}
				else group_selection_end = NACK;
			}
			else if(!strncmp(cmd, "onoff", 5)) {
				rcb_onoff_control();
			}


			else if(!strncmp(cmd, "moven", 5)) {
				FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0001);
				printf("reg=0x%02x\n", FPGA_Read(FPGA_MEM_WR_CTRL));
			}
			else if(!strncmp(cmd, "movdis", 6)) {
				FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0001);
				printf("reg=0x%02x\n", FPGA_Read(FPGA_MEM_WR_CTRL));
			}

			else if(!strncmp(cmd, "ii", 2)) {
				set_opmode(MANU_RUN);
				pattern_inc();
			}
			else if(!strncmp(cmd, "dd", 2)) {
				set_opmode(MANU_RUN);
				pattern_dec();
			}

			else if(!strncmp(cmd, "rdshm", 5)) {
#ifdef USE_SHARED_MEM
				read_shmem();
#endif
			}


			//ES628
			else if(!strncmp(cmd, "tcppip", 6))	{
				msg_data_t smsg;
				req_pwrc_ip_addr_set_t	pwrc_data={0,};
				char str1[MAX_IP_ADDR], str2[MAX_IP_ADDR];

				memset(str1, 0, MAX_IP_ADDR);
				memset(str2, 0, MAX_IP_ADDR);

				strcpy(data,cmd+7);
				sscanf(data,"%s %s", &str1, &str2);

				memset(power1_ip_info, 0, MAX_IP_ADDR);
				memset(power2_ip_info, 0, MAX_IP_ADDR);
				sprintf(power1_ip_info, "%s", str1);
				sprintf(power2_ip_info, "%s", str2);

				set_power_ip(power1_ip_info, power2_ip_info);

				//to PWRC
				memset(&smsg, 0, sizeof(msg_data_t));
				memset(&pwrc_data, 0, sizeof(req_pwrc_ip_addr_set_t));
				pwrc_data.hdr.cmd_id = SID_PWR_CTRL_IP_SET_REQ;
				pwrc_data.hdr.data_len = sizeof(req_pwrc_ip_addr_set_t)-sizeof(req_head_t);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &pwrc_data, sizeof(req_pwrc_ip_addr_set_t));
				msq_send_pwrc(&smsg);
			}
			else if(!strncmp(cmd, "tcppset", 7))	{
				msg_data_t smsg;
				req_ip_pwr_onoff_t	stru	={0,};
				int		data1[30]={0,},data2[30]={0,},data3[30]={0,},data4[30]={0,},data5[30]={0,};

				memset(&stru, 0, sizeof(req_ip_pwr_onoff_t));
//				memset(data, 0, sizeof(data));

				strcpy(data,cmd+8);
				sscanf(data,"%d %d %d %d %d", &data1,&data2,&data3,&data4,&data5);

//				printf("%d %d %d %d %d", data1[0],data2[0],data3[0],data4[0],data5[0]);


				stru.onoff=(unsigned short)data1[0];
				stru.pwr_id=(unsigned short)data2[0];
				stru.pwr_ch=(unsigned short)data3[0];
				stru.voltage=(unsigned short)data4[0];
				stru.current=(unsigned short)data5[0];

				//to PWRC
				stru.hdr.cmd_id = SID_PWR_CTRL_ONOFF_REQ;
				stru.hdr.data_len = sizeof(req_ip_pwr_onoff_t)-sizeof(req_head_t);

				memset(&smsg, 0, sizeof(msg_data_t));
				memcpy(&smsg.header, &stru, sizeof(req_ip_pwr_onoff_t));
				msq_send_pwrc(&smsg);

			}
			else if(!strncmp(cmd, "icvs", 4)) {
				memset(data,0,sizeof(data));
				memset(in_data,0,sizeof(in_data));
				strcpy(data,cmd+5);
				sscanf(data,"%d",&in_data[0]);

				ext_cnt_vdd_sel((unsigned short)in_data[0]);	//0=gnd, 1=3.3V, 2=5V

			}

			else if(!strncmp(cmd, "wtchk", 5)) {
				wtime_chk=1;
			}

			else if(!strncmp(cmd, "rdi2crddata", 11)) {
				printf("-fpga reg read i2c rd data-\n");
				for(i=0;i<200;i++)
				{
					printf("0x%02x: 0x%04x = ", i, FPGA_I2C_RD_DATA_Read(i));
					ushort_to_binary(FPGA_I2C_RD_DATA_Read(i));
				}
			}

			else if(!strncmp(cmd, "rdreg", 5)) {
				printf("-fpga reg read-\n");
				for(i=0;i<FPGA_END_OF_REG;i++)
				{
					printf("0x%02x: 0x%04x = ", i, FPGA_Read(i));
					ushort_to_binary(FPGA_Read(i));
				}
			}

			else if(!strncmp(cmd, "pwrseq", 5)) {
				printf("-Power Sequence Check-\n");
				printf("Vdd on seq = %d\n",model_data.on_seq[0]);
				printf("Signal on seq = %d\n",model_data.on_seq[1]);
				printf("VBL on seq = %d\n",model_data.on_seq[2]);
				printf("INV on seq = %d\n\n",model_data.on_seq[3]);
				printf("Vdd off seq = %d\n",model_data.off_seq[0]);
				printf("Signal off seq = %d\n",model_data.off_seq[1]);
				printf("VBL off seq = %d\n",model_data.off_seq[2]);
				printf("INV off seq = %d\n",model_data.off_seq[3]);

				printf("Vdd on delay = %d\n",model_data.on_delay[0]);
				printf("Signal on delay = %d\n",model_data.on_delay[1]);
				printf("VBL on delay = %d\n",model_data.on_delay[2]);
				printf("INV on delay = %d\n\n",model_data.on_delay[3]);
				printf("Vdd off delay = %d\n",model_data.off_delay[0]);
				printf("Signal off delay = %d\n",model_data.off_delay[1]);
				printf("VBL off delay = %d\n",model_data.off_delay[2]);
				printf("INV off delay = %d\n",model_data.off_delay[3]);
			}

			else if(!strncmp(cmd, "pattern", 7) && (strlen(cmd)==9)) {
				printf("-Pattern Spec check-\n");
				printf("Pattern Frequency = %d\n",group_data.pat[*(cmd+8)-'0'].frequency);
				printf("Pattern Name = %s\n",group_data.pat[*(cmd+8)-'0'].name);
				printf("Pattern Time = %d\n",group_data.pat[*(cmd+8)-'0'].time);
				printf("Pattern Type = %d\n\n",group_data.pat[*(cmd+8)-'0'].type);
			}

			else if(!strncmp(cmd, "playerstop", 10)) {
				printf("-Player Stop-\n");
				exec_ops(NVGST_OPS_PAUSE);
			}
			else if(!strncmp(cmd, "timing", 6)) {
				printf("-Tegra time-\n");
				printf("Tegra Htime = %d\n",tegra_time.h_active);
				printf("Tegra Vtime = %d\n",tegra_time.v_active);
				printf("-FPGA time-\n");
				printf("FPGA Htime = %d\n",fpga_time.h_active);
				printf("FPGA Vtime = %d\n",fpga_time.v_active);
			}
			else if(!strncmp(cmd, "killmov", 7)) {
				printf("-Player kill-\n");
				player_stop();
			}
			else if(!strncmp(cmd, "bytemode", 8)) {
				printf("bytemode = %d\n",model_data.mode);
			}
			else if(!strncmp(cmd, "timeout", 7)) {
				printf("VBL timeout = %d\n",model_data.VBL_EN_timeout);
			}
			else if(!strncmp(cmd, "acdetoff", 8)) {
				printf("-ACdet Off-\n");
				ext_cnt_vdd_sel(0);
			}
			else if(!strncmp(cmd, "acdeton", 7)) {
				printf("-ACdet 3.3v On-\n");
				ext_cnt_vdd_sel(1);
			}
			else if(!strncmp(cmd, "pattime", 7)) {
				printf("Pattern time: %d\n",group_data.pat[pattern_index].time);
			}
			else if(!strncmp(cmd, "bgwhite", 7)){
				FPGA_Write(FPGA_PAL0_R, 4095);
				FPGA_Write(FPGA_PAL0_G, 4095);
				FPGA_Write(FPGA_PAL0_B, 4095);
			}
			else if(!strncmp(cmd, "bgblack", 7)){
				FPGA_Write(FPGA_PAL0_R, 0);
				FPGA_Write(FPGA_PAL0_G, 0);
				FPGA_Write(FPGA_PAL0_B, 0);
			}
			else if(!strncmp(cmd, "paton", 7)){
				pat_roll_test = 1;
			}
			else if(!strncmp(cmd, "patoff", 7)){
				pat_roll_test = 0;
			}
			else if(!strncmp(cmd, "vrr", 3)){
				for(i=0;i<100;i++)
				{
					printf("VRR Total: %d\n",FPGA_VRR_DATA_Read_EX(i));
					printf("VRR Frame Number: %d\n",FPGA_VRR_DATA_Read_EX(100+i));
				}
			}
			else if(!strncmp(cmd, "en l", 4)){
				FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0008); // Gray 30 step issue 24.02.21
				printf("BMP Capture Enable Low\n");
			}
			else if(!strncmp(cmd, "en h", 4)){
				FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0008); // Gray 30 step issue 24.02.21
				printf("BMP Capture Enable High\n");
			}
			else if(!strncmp(cmd, "i2c_pwr_seq", strlen("i2c_pwr_seq")))
			{
				req_std_aux_i2c_pwr_seq_test_t pwr_seq_data = {0, };
				std_aux_pwr_seq_cmd_t aux_data = {0, };

				memset(&pwr_seq_data, 0, sizeof(req_std_aux_i2c_pwr_seq_test_t));

				printf("I2C Power Sequence Data Input\n");

				pwr_seq_data.sel = 0x01;

				pwr_seq_data.pwr_seq_timing[0].t1 = 1000;
				pwr_seq_data.pwr_seq_timing[0].t2 = 2000;
				pwr_seq_data.pwr_seq_timing[0].t3 = 4000;
				pwr_seq_data.pwr_seq_timing[0].t3_1 = 8000;
				pwr_seq_data.pwr_seq_timing[0].t3_2 = 10000;
				pwr_seq_data.pwr_seq_timing[0].t3_3 = 12000;
				pwr_seq_data.pwr_seq_timing[0].t3_4 = 14000;
				pwr_seq_data.pwr_seq_timing[0].t4 = 7000;
				pwr_seq_data.pwr_seq_timing[0].t5 = 4000;
				pwr_seq_data.pwr_seq_timing[0].ton = 30000;
				pwr_seq_data.pwr_seq_timing[0].acdet_on = 2000;
				pwr_seq_data.pwr_seq_timing[0].acdet_off = 2000;
				pwr_seq_data.pwr_seq_timing[0].t_i2c_on_1 = 5000;
				pwr_seq_data.pwr_seq_timing[0].t_i2c_off_1 = 6000;
				pwr_seq_data.pwr_seq_timing[0].t_i2c_on_2 = 6000;
				pwr_seq_data.pwr_seq_timing[0].t_i2c_off_2 = 5000;
				pwr_seq_data.pwr_seq_timing[0].repeat = 1;

				for(int i=0; i<4; i++)
				{
					pwr_seq_data.pwr_seq_i2c[i].i2c_clock_sel = 1;
					pwr_seq_data.pwr_seq_i2c[i].reg_addr_size = 1;
					pwr_seq_data.pwr_seq_i2c[i].data_size = 1;
					pwr_seq_data.pwr_seq_i2c[i].dev_addr = 0xaa;
					pwr_seq_data.pwr_seq_i2c[i].byte_num = 1;
					pwr_seq_data.pwr_seq_i2c[i].reg_addr[0] = 0xaa;
					pwr_seq_data.pwr_seq_i2c[i].reg_data[0] = 0xaa;
				}

				pwr_seq_data.onoff = 0x0f;
				pwr_seq_data.scenario = 0x01;
				pwr_seq_data.rpcnt = 0x01;

				memset(&aux_data.reg_addr[0], 0, sizeof(std_aux_pwr_seq_cmd_t));
				aux_data.reg_addr[0] = 0x00;

				printf("pwr_seq_data.sel : %d\n", pwr_seq_data.sel);

				for(int i=0; i<7; i++)
				{
					printf("pwr_seq_data.pwr_seq_timing[%d].t1 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t1);
					printf("pwr_seq_data.pwr_seq_timing[%d].t2 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t2);
					printf("pwr_seq_data.pwr_seq_timing[%d].t3 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t3);
					printf("pwr_seq_data.pwr_seq_timing[%d].t3_1 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t3_1);
					printf("pwr_seq_data.pwr_seq_timing[%d].t3_2 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t3_2);
					printf("pwr_seq_data.pwr_seq_timing[%d].t3_3 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t3_3);
					printf("pwr_seq_data.pwr_seq_timing[%d].t3_4 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t3_4);
					printf("pwr_seq_data.pwr_seq_timing[%d].t4 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t4);
					printf("pwr_seq_data.pwr_seq_timing[%d].t5 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t5);
					printf("pwr_seq_data.pwr_seq_timing[%d].acdet_on : %d\n", i, pwr_seq_data.pwr_seq_timing[i].acdet_on);
					printf("pwr_seq_data.pwr_seq_timing[%d].acdet_off : %d\n", i, pwr_seq_data.pwr_seq_timing[i].acdet_off);
					printf("pwr_seq_data.pwr_seq_timing[%d].t_i2c_on_1 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t_i2c_on_1);
					printf("pwr_seq_data.pwr_seq_timing[%d].t_i2c_off_1 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t_i2c_off_1);
					printf("pwr_seq_data.pwr_seq_timing[%d].t_i2c_on_2 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t_i2c_on_2);
					printf("pwr_seq_data.pwr_seq_timing[%d].t_i2c_off_2 : %d\n", i, pwr_seq_data.pwr_seq_timing[i].t_i2c_off_2);
					printf("pwr_seq_data.pwr_seq_timing[%d].repeat : %d\n", i, pwr_seq_data.pwr_seq_timing[i].repeat);
				}

				for(int i=0; i<4; i++)
				{
					printf("pwr_seq_data.pwr_seq_i2c[%d].i2c_clock_sel : %d\n", i, pwr_seq_data.pwr_seq_i2c[i].i2c_clock_sel);
					printf("pwr_seq_data.pwr_seq_i2c[%d].reg_addr_size : %d\n", i, pwr_seq_data.pwr_seq_i2c[i].reg_addr_size);
					printf("pwr_seq_data.pwr_seq_i2c[%d].data_size : %d\n", i, pwr_seq_data.pwr_seq_i2c[i].data_size);
					printf("pwr_seq_data.pwr_seq_i2c[%d].dev_addr : 0x%04x\n", i, pwr_seq_data.pwr_seq_i2c[i].dev_addr);
					printf("pwr_seq_data.pwr_seq_i2c[%d].byte_num : %d\n", i, pwr_seq_data.pwr_seq_i2c[i].byte_num);

					for(int j=0; j<10; j++)
					{
						printf("pwr_seq_data.pwr_seq_i2c[%d].reg_addr[%d] : 0x%04x\n", i, j, pwr_seq_data.pwr_seq_i2c[i].reg_addr[j]);
						printf("pwr_seq_data.pwr_seq_i2c[%d].reg_data[%d] : 0x%04x\n", i, j, pwr_seq_data.pwr_seq_i2c[i].reg_data[j]);
					}
				}

				printf("pwr_seq_data.onoff : %d\n", pwr_seq_data.onoff);
				printf("pwr_seq_data.scenario : %d\n", pwr_seq_data.scenario);
				printf("pwr_seq_data.rpcnt : %d\n", pwr_seq_data.rpcnt);

				for(int i=0; i<4; i++)
				{
					printf("pwr_seq_data.aux_pwr_seq_cmd.reg_addr[%d] : 0x%04x\n", i, pwr_seq_data.aux_pwr_seq_cmd.reg_addr[i]);
					printf("pwr_seq_data.aux_pwr_seq_cmd.reg_data[%d] : 0x%04x\n", i, pwr_seq_data.aux_pwr_seq_cmd.reg_data[i]);
				}

				for(int i=0; i<4; i++)
				{
					printf("aux_data.reg_addr[%d] : 0x%04x\n", i, aux_data.reg_addr[i]);
					printf("aux_data.reg_data[%d] : 0x%04x\n", i, aux_data.reg_data[i]);
				}

				// reset pwr_seq_aux_addr_data
				memset(&pwr_seq_aux_addr_data.reg_addr[0], 0, sizeof(pwr_seq_aux_addr_data));
				memcpy(&pwr_seq_aux_addr_data.reg_addr[0], &aux_data.reg_addr[0], sizeof(pwr_seq_aux_addr_data.reg_addr));

				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C

				/* 2022.08.25 ksk i2c 2 pwr seq */
				if ((pwr_seq_data.onoff& 0x01)>0)	flag_i2c_1_on_en=1;
				else						flag_i2c_1_on_en=0;

				if ((pwr_seq_data.onoff & 0x02)>0)	flag_i2c_1_off_en=1;
				else						flag_i2c_1_off_en=0;

				if ((pwr_seq_data.onoff & 0x04)>0)	flag_i2c_2_on_en=1;
				else						flag_i2c_2_on_en=0;

				if ((pwr_seq_data.onoff & 0x08)>0)	flag_i2c_2_off_en=1;
				else						flag_i2c_2_off_en=0;

				printf("flag 1 on en: %d\n", 	flag_i2c_1_on_en);
				printf("flag 1 off en: %d\n", 	flag_i2c_1_off_en);
				printf("flag 2 on en: %d\n", 	flag_i2c_2_on_en);
				printf("flag 2 off en: %d\n", 	flag_i2c_2_off_en);

				if(std_power_seq_run==0)
				{
					std_edp_power_seq_i2c_set(&pwr_seq_data);
					std_edp_power_sequence(&pwr_seq_data);

					pwr_seq_aux_state = 0;

					set_std_pseq_power_start_time();
				}
				else printf("Pseq set Req - running already\n");




				printf("I2C Power Sequence On\n");

				FPGA_Write(FPGA_I2C_MASTER_EN, 0x0000);	//FPGA_I2C_SEL = 0 : I2C

				req_std_pseq_onoff_t pwr_seq_onoff_data = {0, };

				memset(&pwr_seq_onoff_data, 0, sizeof(req_std_pseq_onoff_t));
				pwr_seq_onoff_data.onoff = 1;

				if(pwr_seq_onoff_data.onoff==1)
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
				else if(pwr_seq_onoff_data.onoff==2)//pause
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


			else command_system(cmd);
			printf("%s", prompt);
		}
	}

	return 0;
}

void start_shell(void)
{
	task_thread(11, shell_cmd_thread);
}

void set_reboot(int flag)
{
	reboot_flag = flag;
}

int get_reboot(void)
{
	return reboot_flag;
}

void set_opmode(enum_opmode_t mode)
{
	op_mode = mode;
}

enum_opmode_t get_opmode(void)
{
	return op_mode;
}

void set_quhd_enable(int enable)
{
	quhd_enable = enable;
}

int get_quhd_enable(void)
{
	return quhd_enable;
}

void fpga_read_scan(void)
{
	 unsigned short scan = FPGA_Read(FPGA_MEM_WR_CTRL);
	if(old_scan != scan)
	{
//		printf("# FPGA Read Scan : 0x%04x\n",FPGA_Read(FPGA_MEM_WR_CTRL));
		old_scan = scan;
	}
}

uint64_t get_current_time(void)
{
	struct timeval 	cur_time;
	uint64_t 		result;

	gettimeofday(&cur_time, NULL);

	result = (cur_time.tv_sec*1000) + (cur_time.tv_usec/1000);

	return result;
}

void set_pattern_start_time(void)
{
	gettimeofday(&pattern_start_time, NULL);
}

uint64_t get_pattern_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &pattern_start_time, &result_time);

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}

void set_inc_start_time(void)
{
	gettimeofday(&inc_start_time, NULL);
}

uint64_t get_inc_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &inc_start_time, &result_time);

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}


//uint16_t get_pattern_elapse_frame(void) // 23.11.20  ksk pattern frame
//{
//	uint16_t result;
//
//	vde_check();
//	result = vde_cnt;
//
//	return result;
//}

void set_sensing_start_time(void)
{
	gettimeofday(&sensing_start_time, NULL);
}

uint64_t get_sensing_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &sensing_start_time, &result_time);

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}

void set_detect_start_time(void)
{
	gettimeofday(&detect_start_time, NULL);
}

uint64_t get_detect_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &detect_start_time, &result_time);

	result = (result_time.tv_sec*1000) + (result_time.tv_usec/1000);

	return result;
}

void set_aging_start_time(void)
{
	gettimeofday(&aging_start_time, NULL);
}

uint64_t get_aging_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &aging_start_time, &result_time);

	result = result_time.tv_sec;

	return result;
}

void set_schedule_start_time(void)
{
	gettimeofday(&onoff_start_time, NULL);
}

uint64_t get_schedule_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &onoff_start_time, &result_time);

	result = result_time.tv_sec;

	return result;
}

void get_server_ip(char *ip)
{
	FILE	*fp;
	char 	path[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, SERVER_IP_FILE);
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(ip, MAX_IP_ADDR, fp)) {}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", SERVER_IP_FILE);
		sprintf(ip, "%s", DEF_SERVER_IP);
		fp = fopen(path, "w");
		fputs(ip, fp);
		fclose(fp);
		sync();
	}
}

uint8_t get_wifi_select(void)
{
	return wifi_select;
}


void get_qems_info(char *eqpid)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, QEMS_INFO_FILE);

	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(eqpid, MAX_QEMS_EQPID, fp)) {}
		fclose(fp);
		wifi_select=atoi(str);

	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", WIFI_INFO_FILE);
		sprintf(eqpid, "%s", DEF_QEMS_EQPID);
		fp = fopen(path, "w");
		fputs(eqpid, fp);
		fclose(fp);
		sync();
	}
}

void get_qems_master_info(char *eqpid)
{
	FILE	*fp;
	char 	path[MAX_PATH]; //, str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, QEMS_8K_INFO_FILE);

	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(eqpid, MAX_QEMS_EQPID, fp)) {}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", QEMS_8K_INFO_FILE);
		sprintf(eqpid, "%s", DEF_QEMS_EQPID);
		fp = fopen(path, "w");
		fputs(eqpid, fp);
		fclose(fp);
		sync();
	}
}

uint8_t get_wifi_info(char *ssid,char *ip,char *netmask,char *gateway,char *identity,char *password)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, WIFI_INFO_FILE);

	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_PATH, fp)) {}
		wifi_select=atoi(str);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_WIFI_SSID, fp)) {}
		strncpy(ssid,str,strlen(str)-2);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_IP_ADDR, fp)) {}
		strncpy(ip,str,strlen(str)-2);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_IP_ADDR, fp)) {}
		strncpy(netmask,str,strlen(str)-2);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_IP_ADDR, fp)) {}
		strncpy(gateway,str,strlen(str)-2);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_WIFI_IDENTITY, fp)) {}
		strncpy(identity,str,strlen(str)-2);

		memset(str, 0, MAX_PATH);
		if(0==fgets(str, MAX_WIFI_PASSWORD, fp)) {}
		strncpy(password,str,strlen(str)-2);
		fclose(fp);

	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", WIFI_INFO_FILE);
		sprintf(str, "%d\r\n", DEF_WIFI_SEL);
		sprintf(ssid, "%s\r\n", DEF_WIFI_SSID);
		sprintf(ip, "%s\r\n", DEF_WIFI_IP);
		sprintf(netmask, "%s\r\n", DEF_WIFI_NETMASK);
		sprintf(gateway, "%s\r\n", DEF_WIFI_GATEWAY);
		sprintf(identity, "%s\r\n", DEF_WIFI_IDENTITY);
		sprintf(password, "%s\r\n", DEF_WIFI_PASSWORD);
		fp = fopen(path, "w");
		fputs(str, fp);
		fputs(ssid, fp);
		fputs(ip, fp);
		fputs(netmask, fp);
		fputs(gateway, fp);
		fputs(identity, fp);
		fputs(password, fp);
		fclose(fp);
		sync();
		wifi_select=DEF_WIFI_SEL;
	}


	return (uint8_t)wifi_select;
}

void set_group_id(uint8_t id)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, GROUP_ID_FILE);
	fp = fopen(path, "w");
	if(fp!=NULL)
	{
		sprintf(str, "%d", id);
		fputs(str, fp);
		fclose(fp);
		sync();
	}
	else
	{
		fprintf(stderr, "%s file creating failed!\n", GROUP_ID_FILE);
	}
}

uint8_t get_group_id(void)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, GROUP_ID_FILE);
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(str, MAX_PATH, fp)) {}
		fclose(fp);

		return (uint8_t)atoi(str);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", GROUP_ID_FILE);
		sprintf(str, "%d", DEF_GROUP_ID);
		fp = fopen(path, "w");
		fputs(str, fp);
		fclose(fp);
		sync();

		return (uint8_t)DEF_GROUP_ID;
	}
}

void set_pwr_vendor(int vendor)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, PWR_SEL_FILE);
	fp = fopen(path, "w");
	if(fp!=NULL)
	{
		pwr_vendor = vendor;
		sprintf(str, "%d", vendor);
		fputs(str, fp);
		fclose(fp);
		sync();
	}
	else
	{
		fprintf(stderr, "%s file creating failed!\n", PWR_SEL_FILE);
	}
}

int get_pwr_vendor(void)
{
	return pwr_vendor;
}

uint8_t get_pwr_sel(void)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, PWR_SEL_FILE);
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(str, MAX_PATH, fp)) {}
		fclose(fp);

		return (uint8_t)atoi(str);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", PWR_SEL_FILE);
		sprintf(str, "%d", DEF_PWR_SEL);
		fp = fopen(path, "w");
		fputs(str, fp);
		fclose(fp);
		sync();

		return (uint8_t)DEF_PWR_SEL;
	}
}

void delay_us(uint32_t microseconds)
{
	clock_t time_end;
	time_end = clock() + microseconds * CLOCKS_PER_SEC/1000000;
	while(clock()<time_end){}
}

static int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;

	do
	{
		/* Recieve response from the kernel */
		if((readLen = recv(sockFd, bufPtr, 4096 - msgLen, 0)) < 0)
		{
			perror("SOCK READ: ");
			return -1;
		}

		nlHdr = (struct nlmsghdr *)bufPtr;

		/* Check if the header is valid */
		if((NLMSG_OK(nlHdr, (unsigned int)readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
		{
			perror("Error in recieved packet");
			return -1;
		}

		/* Check if the its the last message */
		if(nlHdr->nlmsg_type == NLMSG_DONE)
		{
			break;
		}
		else
		{
			/* Else move the pointer to buffer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}


		/* Check if its a multi part message */
		if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
		{
			/* return if its not */
			break;
		}
	} while((nlHdr->nlmsg_seq != (unsigned int)seqNum) || (nlHdr->nlmsg_pid != (unsigned int)pId));

	return msgLen;
}

/* parse the route info returned */
static void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;

	rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

	/* If the route is not for AF_INET or does not belong to main routing table	then return. */
	if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
		return;

	/* get the rtattr field */
	rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
	rtLen = RTM_PAYLOAD(nlHdr);

	for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen))
	{
		switch(rtAttr->rta_type)
		{
			case RTA_OIF:
			if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
			break;

			case RTA_GATEWAY:
			memcpy(&rtInfo->gateWay, RTA_DATA(rtAttr), sizeof(rtInfo->gateWay));
			break;

			case RTA_PREFSRC:
			memcpy(&rtInfo->srcAddr, RTA_DATA(rtAttr), sizeof(rtInfo->srcAddr));
			break;

			case RTA_DST:
			memcpy(&rtInfo->dstAddr, RTA_DATA(rtAttr), sizeof(rtInfo->dstAddr));
			break;
		}
	}

	return;
}

static int get_gatewayip(unsigned char *gatewayip, socklen_t size)
{
	int found_gatewayip = 0;

	struct nlmsghdr *nlMsg;
	//struct rtmsg *rtMsg;
	struct route_info *rtInfo;
	char msgBuf[4096]; // pretty large buffer

	int sock, len, msgSeq = 0;

	/* Create Socket */
	if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
	{
		perror("Socket Creation: ");
		return(-1);
	}

	/* Initialize the buffer */
	memset(msgBuf, 0, sizeof(msgBuf));

	/* point the header and the msg structure pointers into the buffer */
	nlMsg				 = (struct nlmsghdr *)msgBuf;
	//rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header*/
	nlMsg->nlmsg_len 	= NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type 	= RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags 	= NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq 	= msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid 	= getpid(); // PID of process sending the request.

	/* Send the request */
	if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0)
	{
		fprintf(stderr, "Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0)
	{
		fprintf(stderr, "Read From Socket Failed...\n");
		return -1;
	}

	/* Parse and print the response */
	rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

	for(;NLMSG_OK(nlMsg,(unsigned int)len);nlMsg = NLMSG_NEXT(nlMsg,len))
	{
		memset(rtInfo, 0, sizeof(struct route_info));
		parseRoutes(nlMsg, rtInfo);

		// Check if default gateway
		if (strstr((char *)inet_ntoa(rtInfo->dstAddr), "0.0.0.0"))
		{
			// copy it over
			//inet_ntop(AF_INET, &rtInfo->gateWay, gatewayip, size);
			//printf("gateway:%d.%d.%d.%d\n",rtInfo->gateWay[0],rtInfo->gateWay[1],rtInfo->gateWay[2],rtInfo->gateWay[3]);
			memcpy(gatewayip,&rtInfo->gateWay, 4);
			found_gatewayip = 1;
			break;
		}
	}

	free(rtInfo);
	close(sock);

	return found_gatewayip;
}

int get_ip_set(net_config_t *net)
{
	/*
	int sockfd;
	int req_cnt = 256;
	int cnt;

	struct sockaddr_in *sock;
	struct ifconf ifcnf_s;
	struct ifreq *ifr_s;

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);

	if(sockfd < 0){
		perror("socket()");
		return -1;
	}

	memset((void *)&ifcnf_s, 0x0, sizeof(ifcnf_s));
	ifcnf_s.ifc_len = sizeof(struct ifreq) *req_cnt;
	ifcnf_s.ifc_buf = malloc(ifcnf_s.ifc_len);

	if(ioctl(sockfd, SIOCGIFCONF, (char *)&ifcnf_s) < 0){
		perror("ioctl() - sig error");
		return -1;
	}

	if(ifcnf_s.ifc_len > (signed)(sizeof(struct ifreq)*req_cnt)){
		req_cnt = ifcnf_s.ifc_len;
		ifcnf_s.ifc_buf = realloc(ifcnf_s.ifc_buf, req_cnt);
	}

	ifr_s = ifcnf_s.ifc_req;

	for(cnt = 0; cnt < ifcnf_s.ifc_len; cnt += sizeof(struct ifreq), ifr_s++){
		if(ioctl(sockfd, SIOCGIFFLAGS, ifr_s) < 0){
		    perror("ioctl() - SIOGIFFLAGS");
		    return -1;
		}
		if(ifr_s->ifr_flags & IFF_LOOPBACK)
		    continue;

		///////////////////////////IP///////////////////////////////////
		sock = (struct sockaddr_in*)&ifr_s->ifr_addr;
		memcpy(net->myip, &sock->sin_addr, 4);

		///////////////////////////MAC///////////////////////////////////
		if(ioctl(sockfd, SIOCGIFHWADDR, ifr_s) < 0){
		    perror("ioctl() - SIOGIFHWADDR");
		    return -1;
		}
		memcpy(net->mac, (struct ether_addr *)(ifr_s->ifr_hwaddr.sa_data), 6);

		///////////////////////SUBNETMASK///////////////////////////////////
		if(ioctl(sockfd, SIOCGIFNETMASK, ifr_s) < 0) {
		    perror("ioctl() - SIOCGIFNETMASK");
		    return -1;
		}
		memcpy(net->netmask, &sock->sin_addr, 4);
	}

	get_gatewayip(net->gateway, sizeof(net->gateway));
*/
	int fd;
	//int if_cnt=0;
	struct ifreq ifr;
	struct sockaddr_in *sock;

	memset(&ifr, 0, sizeof(ifr));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct if_nameindex *if_nidxs, *intf;

    if_nidxs = if_nameindex();
    if ( if_nidxs != NULL )
    {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
        {
        	strncpy(ifr.ifr_name , intf->if_name , IFNAMSIZ-1);
            //printf("%s\n", intf->if_name);

//        	if(strncmp(intf->if_name,"enx",3)==0)
        	if(strncmp(intf->if_name,"eth0",4)==0)
        	{
				if (0 == ioctl(fd, SIOCGIFADDR, &ifr))
				{
					sock = (struct sockaddr_in*)&ifr.ifr_addr;
					memcpy(net->myip, &sock->sin_addr, 4);
				}
				if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
				{
					memcpy(net->mac, (struct ether_addr *)(ifr.ifr_hwaddr.sa_data), 6);
				}
				if (0 == ioctl(fd, SIOCGIFNETMASK, &ifr))
				{
					memcpy(net->netmask, &sock->sin_addr, 4);
				}
        	}
        }

        return 1;//test
    }
    return 0;
}

int get_ip_wlan0(net_config_t *net)
{
	int fd;
	//int if_cnt=0;
	struct ifreq ifr;
	struct sockaddr_in *sock;

	memset(&ifr, 0, sizeof(ifr));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct if_nameindex *if_nidxs, *intf;

    if_nidxs = if_nameindex();
    if ( if_nidxs != NULL )
    {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
        {
        	strncpy(ifr.ifr_name , intf->if_name , IFNAMSIZ-1);
            //printf("%s\n", intf->if_name);

        	if(strncmp(intf->if_name,"wlan0",5)==0)
        	{
				if (0 == ioctl(fd, SIOCGIFADDR, &ifr))
				{
					sock = (struct sockaddr_in*)&ifr.ifr_addr;
					memcpy(net->myip, &sock->sin_addr, 4);
				}
				if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
				{
					memcpy(net->mac, (struct ether_addr *)(ifr.ifr_hwaddr.sa_data), 6);
				}
				if (0 == ioctl(fd, SIOCGIFNETMASK, &ifr))
				{
					memcpy(net->netmask, &sock->sin_addr, 4);
				}
        	}
        }
    }
    return 0;
}

//seochihong 20180420
int get_ip_info(net_info_t *net)
{
	int fd;
	int if_cnt=0;
	struct ifreq ifr;
	struct sockaddr_in *sock;

	memset(&ifr, 0, sizeof(ifr));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct if_nameindex *if_nidxs, *intf;

    if_nidxs = if_nameindex();
    if ( if_nidxs != NULL )
    {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++)
        {
        	strncpy(ifr.ifr_name , intf->if_name , IFNAMSIZ-1);
            //printf("%s\n", intf->if_name);

    		memcpy(net[if_cnt].name, intf->if_name, IFNAMSIZ-1);


        	if (0 == ioctl(fd, SIOCGIFADDR, &ifr))
        	{
        		sock = (struct sockaddr_in*)&ifr.ifr_addr;
        		memcpy(net[if_cnt].myip, &sock->sin_addr, 4);
        	}
        	if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
        	{
        		memcpy(net[if_cnt].mac, (struct ether_addr *)(ifr.ifr_hwaddr.sa_data), 6);
        	}
        	if_cnt++;
        }
    }

    return if_cnt;
}

//seochihong 20181205
void get_wifi_level(char *str)
{
	int sockfd;
	struct iw_statistics stats;
	struct iwreq req;
	memset(&stats, 0, sizeof(stats));
	memset(&req, 0, sizeof(req));
	sprintf(req.ifr_name, "wlan0");
	req.u.data.pointer = &stats;
	req.u.data.length = sizeof(stats);


	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Could not create simple datagram socket");
	}

	/* Perform the ioctl */
	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1) {
		perror("Error performing SIOCGIWSTATS");
		sprintf(str,"Signal level:ERR");
		close(sockfd);
		return;
	}

	close(sockfd);

	sprintf(str,"Signal level:%d",stats.qual.level-256);
	return;
}

//seochihong 20181205
int  get_wifi_level_int(void)
{
	int sockfd;
	struct iw_statistics stats;
	struct iwreq req;
	memset(&stats, 0, sizeof(stats));
	memset(&req, 0, sizeof(req));
	sprintf(req.ifr_name, "wlan0");
	req.u.data.pointer = &stats;
	req.u.data.length = sizeof(stats);


	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Could not create simple datagram socket");
	}

	/* Perform the ioctl */
	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1) {
		perror("Error performing SIOCGIWSTATS");
		close(sockfd);
		return (stats.qual.level-256);
	}

	close(sockfd);

	return (stats.qual.level-256);
}

void get_eth_name(char *eth)
{
	FILE 	*fp;
	char	buf[MAX_PATH];
	char	*pos;

//	fp = popen("ifconfig | grep HWaddr", "r");
	fp = popen("ifconfig | grep mtu", "r");//test
	if(NULL != fgets(buf, MAX_PATH, fp))
	{
		//pos = strstr(buf, "HWaddr");
//		pos = strtok(buf, " ");
		pos = strtok(buf, ":");//test		 eth0:
		if(NULL != pos)	memcpy(eth, pos, strlen(pos));
	}
	pclose(fp);
}

int replace(const char* Expression, char* Dest, const char* Find, const char* Replace, unsigned int Start, unsigned int Count)
{

	char* tempStr;
	char* tempStr2;
	size_t tempStrSize;
	size_t ExpLen=strlen(Expression);
	size_t FindLen=strlen(Find);
	size_t ReplaceLen=strlen(Replace);
	unsigned int countFind=0;

	for(unsigned int i=0;i<ExpLen;i++){
		if(strncmp(Expression+i,Find,FindLen)==0) countFind++;
	}

	if(ExpLen==0 || FindLen==0 || Start>ExpLen-1 || countFind==0) {
		strcpy(Dest,Expression);
		return 1;
	}

	if(ReplaceLen>FindLen){
		if(Count==0) tempStrSize=ExpLen+1+((ReplaceLen-FindLen)*countFind);
		else tempStrSize=ExpLen+1+((ReplaceLen-FindLen)*Count);
	}else tempStrSize=ExpLen+1;

	tempStr2=(char *) calloc(tempStrSize,sizeof(char));
	if(tempStr2==NULL) return 0;

	strcpy(Dest,Expression);
	tempStr=Dest+Start;
	for(unsigned int i=0,repCount=0;i<tempStrSize-1;i++){
		if(strncmp(tempStr+i,Find,FindLen)==0){
			strcpy(tempStr2,tempStr+i+FindLen);
			memset(tempStr+i,0,1);
			strcat(tempStr,Replace);
			strcat(tempStr,tempStr2);
			repCount++;
			if(repCount==Count) break;
		}
	}
	free(tempStr2);
    return 1;
}


void re_driver_mux_default(void)
{
	re_driver_eqacu = 0;
	re_driver_eqacl = 0;
	re_driver_eqdc	= 3;

	re_driver_mux_ACnU_set(re_driver_eqacu);
	re_driver_mux_ACnL_set(re_driver_eqacl);
	re_driver_mux_DCn_set(re_driver_eqdc);
}



void re_driver_mux_ACnU_set(unsigned char level)
{
	unsigned char level_a=0, level_b=0;

	if(level>0x03) level =  0x03;
	level_a = level & 0x01;
	level_b = (level & 0x02) >> 1;
	i2c_gpio_set(GPIO_EX25, level_a);
	i2c_gpio_set(GPIO_EX27, level_b);

	switch(level)
	{
		case 0x00	: printf("re-driver ACnU 3.3V\n");		break;
		case 0x01	: printf("re-driver ACnU OPEN\n");		break;
		case 0x02	: printf("re-driver ACnU Pull-down\n");	break;
		case 0x03	: printf("re-driver ACnU GND\n");		break;
		default		: printf("ACnU default\n");				break;
	}
}

void re_driver_mux_ACnL_set(unsigned char level)
{
	unsigned char level_a=0, level_b=0;

	if(level>0x03) level =  0x03;
	level_a = level & 0x01;
	level_b = (level & 0x02) >> 1;
	i2c_gpio_set(GPIO_EX28, level_a);
	i2c_gpio_set(GPIO_EX29, level_b);

	switch(level)
	{
		case 0x00	: printf("re-driver ACnL 3.3V\n");		break;
		case 0x01	: printf("re-driver ACnL OPEN\n");		break;
		case 0x02	: printf("re-driver ACnL Pull-down\n");	break;
		case 0x03	: printf("re-driver ACnL GND\n");		break;
		default		: printf("ACnL default\n");				break;
	}
}

void re_driver_mux_DCn_set(unsigned char level)
{
	unsigned char level_a=0, level_b=0;

	if(level>0x03) level =  0x03;
	level_a = level & 0x01;
	level_b = (level & 0x02) >> 1;
	i2c_gpio_set(GPIO_EX30, level_a);
	i2c_gpio_set(GPIO_EX31, level_b);

	switch(level)
	{
		case 0x00	: printf("re-driver DCn 3.3V\n");		break;
		case 0x01	: printf("re-driver DCn OPEN\n");		break;
		case 0x02	: printf("re-driver DCn Pull-down\n");	break;
		case 0x03	: printf("re-driver DCn GND\n");		break;
		default		: printf("DCn default\n");				break;
	}
}

void get_sub_board_id(void)
{
	uint16_t	reg=0;

	reg = FPGA_Read(FPGA_PGID);

//	gp.sub_board_id = (unsigned char)(0x000f & (reg>>2));							//0b1101:ES620F, 0b0000:ES620E

//	if( (gp.sub_board_id==0x0d) | (gp.sub_board_id==0x0c) )	set_pg_id(ES620F);		//ES620F (EP620T+EP620S | EP620T)
//	else if ( (gp.sub_board_id==0x00) )						set_pg_id(ES620E);		//ES620E (EP620Sx4 or X)
//	else													set_pg_id(ES620E);
}

void set_pg_tx_mode(int mode)
{
	pg_tx_mode = mode;
}

int get_pg_tx_mode(void)
{
	return pg_tx_mode;
}

uint8_t get_reboot_count(void)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, "dev_hub_chk.txt");
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(str, MAX_PATH, fp)) {}
		fclose(fp);

		return (uint8_t)atoi(str);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", "dev_hub_chk.txt");
		sprintf(str, "%d", 0);
		fp = fopen(path, "w");
		fputs(str, fp);
		fclose(fp);
		sync();

		return (uint8_t)0;
	}
}

void set_reboot_count(int count)
{
	FILE	*fp;
	char 	path[MAX_PATH], str[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, "dev_hub_chk.txt");
//	fp = fopen(path, "w");

	if(0==access(path, F_OK))
	{
		sprintf(str, "%d", count);
		fp = fopen(path, "w");
		fputs(str, fp);
		fclose(fp);
		sync();
	}
	else
	{
		fprintf(stderr, "%s file creating failed!\n", "dev_hub_chk.txt");
		sprintf(str, "%d", 0);
		fp = fopen(path, "w");
		fputs(str, fp);
		fclose(fp);
		sync();

	}
}

char* read_storage_volume(void)
{
	char cmd[128];
	static char buffer[128];
	int	ret=0;
	FILE *fp;

	memset(cmd, 0, sizeof(cmd));
	memset(buffer, 0, sizeof(buffer));

//	sprintf(cmd, "df -Ph | grep -vE '^Filesystem|tmpfs|none|/dev/sda1' | awk '{ print $5,$1 }'");
	sprintf(cmd, "df -PhBM | grep -vE '^Filesystem|tmpfs|none|/dev/sda1' | awk '{ print $5$3$2 }'");
	fp = popen(cmd, "r");
	ret = fread(buffer, sizeof(char), sizeof(buffer), fp);
	pclose(fp);

	return buffer;
}

void inspect_hdd_usage(void)
{
	char cmd[128];
	char buffer[128];
	char perc[128];
	int	ret=0;
	uint8_t	use=0;
//	FILE *fp;
	char *ptr;

//	memset(cmd, 0, sizeof(cmd));
	memset(buffer, 0, sizeof(buffer));
	memset(perc, 0, sizeof(perc));

//	sprintf(cmd, "df -Ph | grep -vE '^Filesystem|tmpfs|none|/dev/sda1' | awk '{ print $5,$1 }'");
//	fp = popen(cmd, "r");
//	ret = fread(buffer, sizeof(char), sizeof(buffer), fp);
//	pclose(fp);

//	printf("%s\n", read_storage_volume());

	memcpy(buffer, read_storage_volume(), sizeof(buffer));

//	printf("Storage volume info: %s\n", buffer);

//	if(ret<0)	printf("Failed check hdd usage\n");
//	else
	{
//		printf("%s\n", buffer);

		ptr = strtok(buffer, "%");

//		while(ptr != NULL)
//		{
//			printf("%s\n", ptr);
//			ptr = strtok(NULL, "M");
//		}

		memcpy(perc, ptr, 128);
		use=(uint8_t)atoi(perc);
//		printf("%d\n",use);

		if(use>=STORAGE_LIMIT)
		{
			printf("Storage usage is limit over\n");

			ERRCHECK(system("rm -f /root/hdd/bmp/*/*"));
			ERRCHECK(system("rm -f /root/hdd/png/*/*"));
			ERRCHECK(system("rm -f /root/hdd/jpg/*/*"));
			ERRCHECK(system("rm -f /root/hdd/mov/*"));

			sync();

			printf("Image and video files are deleted\n");
		}
		else	printf("Storage has used %d%%\n", use);

	}
}

void ushort_to_binary(uint16_t hex)
{
	char i,j;
	uint8_t	hexa;

//	hexa = hex;

	for(i=0;i<4;i++)
	{
		hexa = (unsigned char)((hex>>(12-(i*4)))&0x000f);
		switch (hexa)
			{
				case 0:
					printf("0000 "); break;
				case 0x1:
					printf("0001 "); break;
				case 0x2:
					printf("0010 "); break;
				case 0x3:
					printf("0011 "); break;
				case 0x4:
					printf("0100 "); break;
				case 0x5:
					printf("0101 "); break;
				case 0x6:
					printf("0110 "); break;
				case 0x7:
					printf("0111 "); break;
				case 0x8:
					printf("1000 "); break;
				case 0x9:
					printf("1001 "); break;
				case 0xa:
					printf("1010 "); break;
				case 0xb:
					printf("1011 "); break;
				case 0xc:
					printf("1100 "); break;
				case 0xd:
					printf("1101 "); break;
				case 0xe:
					printf("1110 "); break;
				case 0xf:
					printf("1111 "); break;
				default:
					break;
			}
	}
	printf("\n");
}

void get_power_ip(char *ip1, char *ip2)
{
	FILE	*fp;
	char 	path[MAX_PATH];
	char	str1[MAX_PATH], str2[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, POWER_IP_FILE);
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		memset(str1, 0, MAX_PATH);
		if(0==fgets(str1, MAX_IP_ADDR, fp)) {}
		strncpy(ip1, str1, strlen(str1)-2);

		memset(str2, 0, MAX_PATH);
		if(0==fgets(str2, MAX_IP_ADDR, fp)) {}
		strncpy(ip2, str2, strlen(str2)-2);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", POWER_IP_FILE);
		sprintf(str1, "%s\r\n", DEF_SCPI_PWR1_IP);
		sprintf(str2, "%s\r\n", DEF_SCPI_PWR2_IP);
		fp = fopen(path, "w");
		fputs(str1, fp);
		fputs(str2, fp);
		fclose(fp);
		sync();

		strncpy(ip1, str1, strlen(str1)-2);
		strncpy(ip2, str2, strlen(str2)-2);
	}

	printf("get power ip : %s\t%s\r\n", ip1, ip2);
}

void set_power_ip(char *ip1, char *ip2)
{
	FILE	*fp;
	char 	path[MAX_PATH];
	char	str1[MAX_PATH], str2[MAX_PATH];

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, POWER_IP_FILE);
	fp = fopen(path, "w");
	if(fp)
	{
		memset(str1, 0, MAX_PATH);
		sprintf(str1, "%s\r\n", ip1);
		fputs(str1, fp);

		memset(str2, 0, MAX_PATH);
		sprintf(str2, "%s\r\n", ip2);
		fputs(str2, fp);

		fclose(fp);
		sync();
	}
	else
	{
		printf("[%s] %s failed!\n", path, POWER_IP_FILE);
	}

	printf("set power ip : %s\t%s\r\n", ip1, ip2);
}

void set_std_pseq_power_start_time(void)
{
	gettimeofday(&pseq_power_start_time, NULL);
}

uint64_t get_std_pseq_power_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &pseq_power_start_time, &result_time);

	result = (result_time.tv_sec*10000) + (result_time.tv_usec/100);	//100us unit

	return result;
}

void set_std_pseq_power_pause_start_time(void) // for pseq pause 2023.11.13 ksk
{
	gettimeofday(&pseq_power_pause_time, NULL);
}

uint64_t get_std_pseq_power_pause_elapse_time(void) //for pseq pause 2023.11.13 ksk
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &pseq_power_pause_time, &result_time);

	result = (result_time.tv_sec*10000) + (result_time.tv_usec/100);	//100us unit

	return result;
}

void set_std_pseq_signal_start_time(void)
{
	gettimeofday(&pseq_signal_start_time, NULL);
}

uint64_t get_std_pseq_signal_elapse_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &pseq_signal_start_time, &result_time);

	result = (result_time.tv_sec*10000) + (result_time.tv_usec/100);	//100us unit

	return result;
}

void set_w_time(void)
{
	gettimeofday(&w_time, NULL);
}

uint64_t get_w_time(void)
{
	struct timeval 	end_time;
	struct timeval 	result_time;
	uint64_t 		result;

	gettimeofday(&end_time, NULL);
	timersub(&end_time, &w_time, &result_time);

	result = (result_time.tv_sec*1000000) + result_time.tv_usec;

	return result;
}

// ELVDD Alarm
void get_elvdd_alarm_flag_global(void)
{
	FILE	*fp;
	char 	path[MAX_PATH];
	char	flag_temp[2] = {0, };

	memset(flag_temp, 0, 2);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, "elvdd_alarm_flag.txt");
	if(0==access(path, F_OK))
	{
		fp = fopen(path, "r");
		if(0==fgets(flag_temp, 2, fp)) {}
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "%s file not exist. creating file...\n", "elvdd_alarm_flag.txt");
		sprintf(flag_temp, "%s", "1");
		fp = fopen(path, "w");
		fputs(flag_temp, fp);
		fclose(fp);
		sync();
	}

	elvdd_alarm_flag_global = atoi(flag_temp);

	printf("elvdd_alarm_flag_global : %d\n", elvdd_alarm_flag_global);
}

void set_elvdd_alarm_flag_global(int elvdd_alarm_flag_factor)
{
	FILE	*fp;
	char 	path[MAX_PATH];
	char	flag_temp[2] = {0, };

	memset(flag_temp, 0, 2);

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_CONFIG, "elvdd_alarm_flag.txt");
	if(0==access(path, F_OK))
	{
		sprintf(flag_temp, "%d", elvdd_alarm_flag_factor);
		fp = fopen(path, "w");
		fputs(flag_temp, fp);
		fclose(fp);
		sync();
	}
	else
	{
		printf("Setting Error\n");
	}

	elvdd_alarm_flag_global = elvdd_alarm_flag_factor;

	printf("elvdd_alarm_flag_global : %d\n", elvdd_alarm_flag_global);
}

void aux_read_function(int idx_factor)
{
	unsigned short aux_rd_data = 0;
	unsigned short aux_return = 0;

	// SPI Transfer to FPGA
	usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
	FPGA_Write(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));
	usleep(2000);	// This is essential to reconize FPGA_AUX_CTRL_REG
	FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_VALUE_RST));

	// Writing data_size	// 2byte
	FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_DATA_SIZE));

	// Reading AUX Data
	FPGA_Write(FPGA_AUX_BYTE_NUM, (1 << FPGA_LSHIFT_BIT_AUX_BYTE_NUM));
	FPGA_Write(FPGA_AUX_ADDR, pwr_seq_aux_addr_data.reg_addr[idx_factor]);

//	for(int i=0; i<4; i++)
	{
//		FPGA_AND_SET(FPGA_AUX_CTRL_REG, (3 << FPGA_LSHIFT_BIT_AUX_TX_IDX));
//		FPGA_OR_SET(FPGA_AUX_CTRL_REG, (i << FPGA_LSHIFT_BIT_AUX_TX_IDX));

		usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
		FPGA_OR_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_RD_REQ));
		usleep(90000);	// This is essential to reconize FPGA_AUX_CTRL_REG and Timeout
		FPGA_AND_SET(FPGA_AUX_CTRL_REG, (1 << FPGA_LSHIFT_BIT_AUX_RD_REQ));

		aux_return = FPGA_Read(FPGA_AUX_RD_REG);

//		printf("aux_return(%d) : 0x%04x\n", i, aux_return);
		printf("aux_return : 0x%04x\n", aux_return);

		if(aux_return == 0)
		{
			aux_rd_data = FPGA_Read(FPGA_AUX_RD_DATA);
//			printf("aux_rd_data(%d) : 0x%04x\n", i, aux_rd_data);
			printf("aux_rd_data : 0x%04x\n", aux_rd_data);
		}
	}

	pwr_seq_aux_addr_data.reg_data[idx_factor] = aux_rd_data;
}
