/*
 * group_data.h
 *
 *  Created on: Sep 21, 2017
 *      Author: root
 */

#ifndef GROUP_DATA_H_
#define GROUP_DATA_H_

#include <global.h>
#include <fpga_spi.h>
#include <pwr_control.h>
#include <i2c_gpio.h>
#include <rcb.h>

// Definitions
#define MAX_GROUP_CNT			256
//#define MAX_GROUP_RESERVED		8			// 19-08-02 10->8  add scroll info by sdc request
#define MAX_GROUP_RESERVED		4				// 21-04-01 8->5  add unsigned int frequency, unsigned short vbporch
												// 23-12-04 5->4  add roll_frame
#define GROUP_SELECT

#ifndef MAX_PAT_NAME
#define MAX_PAT_NAME			128
#endif
#ifndef MAX_BMP_PATH
#define MAX_BMP_PATH			260
#endif
#ifndef MAX_PAT_CNT
#define MAX_PAT_CNT				256
#endif


// Enumerations
typedef enum
{
	TYPE_USER,
	TYPE_BMP,
	TYPE_IMAGE,
	TYPE_MOV
} enum_pat_type_t;


// Structures
typedef struct
{
	uint16_t		count;
	int8_t			bmp_path[MAX_BMP_PATH];
} group_head_t;

//typedef struct
//{
//	int8_t			name[MAX_PAT_NAME];
//	uint16_t		roll_frame;
//	uint16_t		type;
//	uint16_t		time;
//	uint16_t		vdd;
//	uint16_t		vsync;
//	uint16_t		scroll_direction;			//0:stop, 1:top, 2:bottom, 3:left, 4:right
//	uint16_t		scroll_speed;				//0~99
//	uint32_t		frequency;
//	uint16_t		vbporch;
//	uint16_t		reserved[MAX_GROUP_RESERVED];
//} pattern_data_t;

typedef struct
{
	int8_t			name[64];
	uint16_t		onoff_cycle;
	uint16_t		on_time;
	uint16_t		off_time;
	uint16_t		autogray_total_min;
	uint16_t		autogray_total_max;
	uint16_t		variable_time;
	uint16_t		sbd_onoff;
	uint16_t		sbd_bit;
	uint16_t		sbd_red;
	uint16_t		sbd_green;
	uint16_t		sbd_blue;
	uint16_t		exco_cmd_index;
	uint16_t		exco_dim_value;
	uint16_t		autogray_R_min;
	uint16_t		autogray_R_max;
	uint16_t		autogray_G_min;
	uint16_t		autogray_G_max;
	uint16_t		autogray_B_min;
	uint16_t		autogray_B_max;
	uint16_t		time_upper;
	uint16_t		linear_flag;
	uint16_t		msbd_flag;
	uint16_t		dqhd_dummy;	// [0] : None, [1] : W24G, [2] : W63G, [3] : W127G, [4] : W255G, [5] : R, [6] : G, [7] : B
	uint16_t		bar_scroll_flag;
	uint16_t		bar_scroll_speed;
	uint16_t		bar_scroll_vsize;
	uint16_t		bar_scroll_cnt;
	uint16_t		dummy[4];
	uint16_t		roll_frame;
	uint16_t		type;
	uint16_t		time;
	uint16_t		vdd;
	uint16_t		vsync;
	uint16_t		scroll_direction;			//0:stop, 1:top, 2:bottom, 3:left, 4:right
	uint16_t		scroll_speed;				//0~99
	uint32_t		frequency;						//1188.12MHz -> 118812
	uint16_t		vbporch;
	uint16_t		vwidth;
	uint16_t		vfporch;
	uint16_t		hbporch;
	uint16_t		hwidth;
	uint16_t		hfporch;
} pattern_data_t;

typedef struct
{
	group_head_t	hdr;
	pattern_data_t	pat[MAX_PAT_CNT];
} group_data_t;


// Variables
group_data_t		group_data;
char				group_name[MAX_GROUP_NAME];	// current group name
char				group_list[MAX_GROUP_CNT][MAX_GROUP_NAME];
char				pattern_list[MAX_PAT_CNT][MAX_PAT_NAME], old_pattern_list[MAX_PAT_CNT][MAX_PAT_NAME];
uint				group_cnt, group_idx, group_init_status, group_selection_end;

int					old_pattern_cnt;			// old pattern count
int					pattern_cnt;				// current pattern count

// Functions
#ifdef GROUP_SELECT
extern int 			group_init(void);
extern int 			group_select(char* name);
extern void			group_update(void);
extern void			set_group_init_status(int status);
extern int 			get_group_init_status(void);
extern void 		set_curgroup_idx(char *name);
extern int			group_list_load(void);
#endif

#endif /* GROUP_DATA_H_ */
