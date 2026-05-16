#ifndef _PATTERN_H_
#define _PATTERN_H_

#include <global.h>
#include <gx.h>

#define min(x,y)	(x>y) ? y : x
#define max(x,y)	(x>y) ? x : y

#define STR_SIZE		128

#define PAR_NULL		0
#define PAR_START		1
#define PAR_END 		2
#define PAR_MULT		3
#define PAR_DIV			4
#define PAR_PLUS		5
#define PAR_MINUS		6
#define PAR_VALUE		7

//GX
#define T_LINE			1
#define T_BOX			2
#define T_FBOX			3
#define T_TRI			4
#define T_FTRI			5
#define T_CIRCLE		6
#define T_FCIRCLE		7
#define T_HOLE			8
#define T_ARC			9
#define T_FARC			10
#define T_HGRAY			11
#define T_VGRAY			12
#define T_CGRAY			13
#define T_AI			14
#define T_CHESS			15
#define T_HATCH			16
#define T_FLICKER		17
#define T_TEXT			18
#define T_BMP			19

#define DECR			-1
#define ZERO			0		
#define INCR			1

#define MULTI			0
#define PLUS			1
#define BLANK			2

#define MAX_FLIC_COLOR	8
#define MAX_FLIC_LINE	4

#define NO_LOOP			0x00
#define SINGLE_LOOP		0x01
#define DUAL_LOOP		0x02

#define AUTO_RANGE		0x00
#define MANU_RANGE		0x01

#define NOCHANGE_COLOR	0x00
#define CHANGE_COLOR	0x01

#define POS_S			0
#define POS_E			1

#define POS_X			0
#define POS_Y			1

#define X_LOOP			1
#define Y_LOOP			2
#define XY_LOOP			3

#define MAX_HEADER		4
#define MAX_DATA		4096
#define MAX_BUF			256
#define MAX_NAME		32
#define CORD_SIZE		24
#define TEXT_TEMP_SIZE	128

#define DEF_HOR			640
#define DEF_VER			480

#define POS_W   		6
#define POS_H   		9

typedef struct {
	char type;
	float val;
}__PACKED__ TOKEN_CONFIG;


typedef struct ai_pat_t_
{
	float bg[3];
	float fg1[3];
	float fg2[3];
	float sx,sy,ex,ey;
	int time;
	int size;
}ai_pat_t;


#pragma pack(1)

typedef struct {
   long	data_type;
   int	data_num;
   char	data[1024];
} t_data;
	
typedef struct 
{
	unsigned short res;
	unsigned short h;
	unsigned short w;
	unsigned short y;
	unsigned short x;
	unsigned short b;
	unsigned short g;
	unsigned short r;
} FPGA_DRAW_DATA;


typedef struct {
	unsigned short	mode;			// mode[2:0]:0-single,1-dual,2-quad,3-octa
									// Bit[5:4]:0-6bit,1-8bit,2-10bit
									// Bit[9:8]:0-jeida,1-vesa
	unsigned int	freq;			// dot clock
	unsigned int	h_total;		// horizontal total
	unsigned int	h_active;		// horizontal active
	unsigned int	h_bpo;			// horizontal back porch
	unsigned int	h_width;		// horizontal pulse width
	unsigned int	v_total;		// vertical total
	unsigned int	v_active;		// vertical active
	unsigned int	v_bpo;			// vertical back porch
	unsigned int	v_width;		// vertical pulse width
	unsigned short	hvsync;			// hsync[0](+:1,-:0),vsync[1](+:1,-:0)
} MODEL_INFO;


// structure for model(xxx.cfg)
typedef struct model_t_ model_t;
struct model_t_ {
	unsigned char	mode;			// 0x00:single, 0x01:dual
	unsigned char	type;			// 0x00:6b-A, 0x01:6b-B, 0x02:8b-A, 0x03:8b-B, 0x04:10b-A, 0x05:10b-B, 0x06:12b-A, 0x07:12b-B
	unsigned char	sync;			// clock delay(0x00~0x07)
	unsigned char	did;			// 0x00:general, 0x01:did
	unsigned char 	pgn[MAX_NAME];
	unsigned int	freq;			// dot clock
	unsigned int	h_total;		// horizontal total
	unsigned int	h_active;		// horizontal active
	unsigned int	h_bpo;			// horizontal back porch
	unsigned int	h_width;		// horizontal pulse width
	unsigned int	v_total;		// vertical total
	unsigned int	v_active;		// vertical active
	unsigned int	v_bpo;			// vertical back porch
	unsigned int	v_width;		// vertical pulse width
};


typedef struct pattern_head_t_ pattern_head_t;
struct pattern_head_t_ {
	unsigned short 	colormode;
	unsigned short  color[3][3];
	unsigned short	ai;
	unsigned short	ai_frame_cnt;
};

// structure for pattern(xxx.pat)
typedef struct tool_t_ tool_t;
struct tool_t_ {
	unsigned short 	id;
	unsigned char	px1[CORD_SIZE], py1[CORD_SIZE];
	unsigned char	px2[CORD_SIZE], py2[CORD_SIZE];
	unsigned char	px3[CORD_SIZE], py3[CORD_SIZE];
	unsigned short	sr, sg, sb;
	unsigned short 	param[8];
	unsigned char	loop_opt;
	unsigned char	loop_range;
	unsigned char	loop_xsize[CORD_SIZE], loop_ysize[CORD_SIZE];
	unsigned char	loop_xcnt[CORD_SIZE], loop_ycnt[CORD_SIZE];
	unsigned short	loop_color;
	unsigned short	er, eg, eb;
	unsigned short	reserved[3];
};

// structure for mouse position
typedef struct coor_t_ coor_t;
struct coor_t_ {
	int				x;
	int				y;
};

#pragma pack()

int				 		tool_cnt;
unsigned short			enable_mouse;
coor_t				 	old_pos, cur_pos;

extern tool_t 			*en_pat_open 		(char *path);
extern void		 		en_pat_close		(tool_t *t);
extern int				en_round			(float x);
extern int 				pattern_load(dc_t *dc, char *patname, pattern_head_t *indirect);

extern uint32_t 		get_gray_scale_value(int idx);

#endif // _PATTERN_H_
