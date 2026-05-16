/*
 * pattern_control.h
 *
 *  Created on: Nov 7, 2017
 *      Author: root
 */

#ifndef PATTERN_CONTROL_H_
#define PATTERN_CONTROL_H_

#include <global.h>
#include <rcb.h>

#define	MAX_PATTERN_TIME_OFFSET	5000
//#define MAX_SCHEDULE_INDEX		10
#define MAX_SCHEDULE_INDEX		256
#define MAX_SCHEDULE_CNT		256
#define MAX_SCHEDULE_NAME		128

typedef struct schedule_info_t_ schedule_info_t;
struct schedule_info_t_
{
	uint8_t			mode;
	uint8_t			hour;	// 0~99
	uint8_t			min;	// 0~59
	uint8_t			sec;	// 0~59
	uint32_t		reserved;
};

typedef struct schedule_info_t_new_ schedule_info_t_new;
struct schedule_info_t_new_
{
	uint16_t			index;
	uint16_t			start_pattern_index;
	uint16_t			end_pattern_index;
	uint16_t			on_hour;			// 0~65535
	uint16_t			on_min;				// 0~59
	uint16_t			on_sec;				// 0~59
	uint16_t			off_hour;			// 0~65535
	uint16_t			off_min;			// 0~59
	uint16_t			off_sec;			// 0~59
	uint16_t			cycle_count;
	uint16_t			vdd_mode;			// 0:NONE, 1:10%low, 2:10%high
	uint16_t			vbl_mode;			// 0:NONE, 1:10%low, 2:10%high
	uint16_t			clock_mode;			// 0~65535 (vsync, Hz)
	uint16_t			adim_mode;			// 0:NONE, 1:10%low, 2:10%high
	uint16_t			pdim_freq_mode;		// 0:NONE, 1:10%low, 2:10%high
	uint16_t			pdim_duty_mode;		// 0:NONE, 1:10%low, 2:10%high
	uint32_t			reserved;

};

typedef struct schedule_data_t_ schedule_data_t;
struct schedule_data_t_
{
	uint16_t		count;
	schedule_info_t	schedule_info[MAX_SCHEDULE_INDEX];
};

typedef struct schedule_data_t_new_ schedule_data_t_new;
struct schedule_data_t_new_
{
	uint16_t		total_schedule_count;				//1~255
	uint16_t		step_count;			//1~65535
	uint16_t		total_time_hour;	//total time = step count * end time
	uint16_t		total_time_min;
	uint16_t		total_time_sec;
	uint16_t		reserved;
	schedule_info_t_new	schedule_info[MAX_SCHEDULE_INDEX];
};


typedef enum
{
	SCHED_MODE_NONE = 0,
	SCHED_MODE_ON,
	SCHED_MODE_OFF,
	SCHED_MODE_VDD_UP,
	SCHED_MODE_VDD_DOWN
} enum_schedule_mode_t;

int 				pattern_change_error;
int 				pattern_index;
uint64_t			pattern_time_offset;	// ms
schedule_data_t		schedule_data;
schedule_data_t_new		schedule_data_new;
int					schedule_flag, schedule_index, schedule_count, schedule_first;
int					schedule_list_count, schedule_list_index;
char				schedule_list[MAX_SCHEDULE_CNT][MAX_SCHEDULE_NAME];
int					pre_sche_index, pre_sche_step, schedule_move_menu, schedule_move_check,schedule_on_off_t, schedule_start;
int					schedule_step_count, schedule_cycle_count;
uint64_t			schedule_offset_time, one_step_time;
char				video_start;
char 				under_video_path[512];


uint16_t			old_index, new_index;//test


extern void 		onoff_by_power_seq(enum_onoff_t onoff);
extern int 			pattern_change(int index);
extern int	 		pattern_inc(void);
extern int  		pattern_dec(void);
extern char 		*get_pattern_name(void);
extern int 			get_pattern_index(void);
extern void 		set_pattern_index(int index);
extern void 		pattern_roll(void);
extern uint64_t 	get_pattern_time_offset(void);
extern void 		set_pattern_time_offset(uint64_t offset);
extern void 		pattern_time_offset_inc(void);
extern void 		pattern_time_offset_dec(void);
extern void			vdd_onoff_control(enum_onoff_t onoff);

extern int 			schedule_list_load(void);
extern int 			get_schedule_info(char *name);
extern void 		reset_schedule_func(void);
extern int 			get_schedule_flag(void);
extern void 		set_schedule_flag(int flag);
extern void 		pwr_schedule_test(void);

//new schedule	2019-08==============================
extern int 			get_schedule_index(void);
extern void 		set_schedule_index(int index);
extern void 		set_schedule_step(int step);
extern uint64_t		get_schedule_offset_time(int step, int index);
extern void 		schedule_menu_step_dec(void);
extern void			schedule_menu_step_inc(void);
extern void 		schedule_menu_ok(void);
extern void			schedule_menu_index_inc(void);
extern void			schedule_menu_index_dec(void);
extern void			schedule_rcb_init(void);
extern void 		schedule_menu_reset(void);
extern void 		set_schedule_vsync(uint16_t vsync);
//====================================================

extern int 			gray_change_func(uint16_t r, uint16_t g, uint16_t b);
extern int 			gray_level_change_func(void);
extern int 			cursor_func(pos_data_t *pt);
extern int 			vdd_set_func(uint16_t vdd);
extern int 			freq_set_func(uint64_t freq);
extern int 			bporch_set_func(int32_t flag, var_t *pvar);
extern void 		set_pattern_voltage(uint16_t vdd);
extern void 		set_pattern_vsync(uint16_t vsync);
extern void 		set_pattern_scroll(uint16_t scroll_direction, uint16_t model_mode, uint16_t scroll_speed);

extern void 		gray_change_init(void);		//19-07-05
extern void 		gray_color_set(uint8_t color_mode);		//19-07-05

extern uint32_t		cdce913_d_freq_set(unsigned int f);

extern int			cursor_func_for_donga(pos_data_t *pt);	//20-02-05
extern int			vrr_set_for_donga(var_t *pvar);

extern int			vde_check(void);
extern void			gamma_change(signed short, signed short, signed short);

extern int 			indirect_gray_change_func(uint16_t pal, uint16_t r, uint16_t g, uint16_t b);

//request by sdc giheung
extern int			fporch_set_func(int32_t flag, var_t *pvar);
extern int			width_set_func(int32_t flag, var_t *pvar);
extern int			active_set_func(int32_t flag, var_t *pvar);

//20-09-09	request by sdc giheung
extern void			inst_ind_pat_draw(req_inst_ind_pat_onoff_t *istnp);
extern void 		inst_ind_pat_color(req_inst_ind_pat_color_t *istnc);
extern void			inst_black_pat_draw(void);

extern void			STD_Rolling_func(req_std_rolling_set_set_t *);
extern void			std_i2c_test_fpga_set(req_std_i2c_test_wr_t data, uint16_t flag);
extern void 		std_ocmd_test_fpga_set(req_std_ocmd_test_wr_t data, uint16_t flag);;
extern void 		std_ctl_fpga_set(req_std_ctl_wr_t data, uint16_t flag);

#endif /* PATTERN_CONTROL_H_ */
