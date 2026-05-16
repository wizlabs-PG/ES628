#include <stdio.h>
#include <stdlib.h>                                                              // malloc srand
#include <string.h>                                                              // abs
#include <stdarg.h>

#include <unistd.h>                                                              // open/close
#include <fcntl.h>                                                               // O_RDWR
#include <sys/ioctl.h>                                                           // ioctl
#include <sys/mman.h>                                                            // mmap PROT_
#include <linux/fb.h>
         
#include <gx.h>
#include <omp.h>
#include <fpga_spi.h>
#include <model_data.h>

frame_buffer_t    gx_fb;
int               gx_error_code;

typedef unsigned short  ush;
typedef unsigned char   uch;


#define ALPHA_COMPOSITE(composite, fg, alpha, bg) {               \
    ush temp = ((ush)(fg)*(ush)(alpha) +                          \
                (ush)(bg)*(ush)(255 - (ush)(alpha)) + (ush)128);  \
    (composite) = (uch)((temp + (temp >> 8)) >> 8);               \
}

static color_t mouse_bmp[POS_H][POS_W] = {
	{{255,255,255,255}, {0,0,0,255}      , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {255,255,255,255}, {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {0,0,0,255}      , {255,255,255,255} , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {0,0,0,255}      , {0,0,0,255}       , {255,255,255,255} , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {0,0,0,255}      , {0,0,0,255}       , {0,0,0,255}       , {255,255,255,255} , {0,0,0,255}      } ,
	{{255,255,255,255}, {0,0,0,255}      , {0,0,0,255}       , {255,255,255,255} , {255,255,255,255}	, {255,255,255,255} } ,
	{{255,255,255,255}, {0,0,0,255}      , {255,255,255,255} , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {255,255,255,255}, {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}      } ,
	{{255,255,255,255}, {0,0,0,255}      , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}       , {0,0,0,255}}
};

static int mouse_p[POS_H][POS_W] = {
	{1, 0, 0, 0, 0, 0},
	{1, 1, 0, 0, 0, 0},
	{1, 1, 1, 0, 0, 0},
	{1, 1, 1, 1, 0, 0},
	{1, 1, 1, 1, 1, 0},
	{1, 1, 1, 1, 1, 1},
	{1, 1, 1, 0, 0, 0},
	{1, 1, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0},
};

/********************************************************************** 32 bpp_10bit*/


int fpga_draw_check(unsigned short flag)
{
	int cnt = 0;

	while((FPGA_Read(FPGA_MEM_WR_CTRL)&flag)!=flag)
	{
//		printf("FPGA_Read(FPGA_MEM_WR_CTRL) = 0x%04X \n", FPGA_Read(FPGA_MEM_WR_CTRL));
		if(++cnt>FPGA_DRAW_CNT)
		{
			printf("fpga write timeout MEM_CTRL(write_end)\n");
			return NACK;
		}
	}

	return ACK;
}

static void draw_box(FPGA_DRAW_DATA *draw_data)
{
	// jschoi
	switch(model_data.disp_mode)
	{
		case FLIP_HV:
			draw_data->x = model_data.h_active - draw_data->x - draw_data->w;
			draw_data->y = model_data.v_active - draw_data->y - draw_data->h;
			break;
		case FLIP_V:
			draw_data->y = model_data.v_active - draw_data->y - draw_data->h;
			break;
		case FLIP_H:
			draw_data->x = model_data.h_active - draw_data->x - draw_data->w;
			break;
		default:
			break;
	}

	//printf("P-X:%d, Y:%d, W:%d, H:%d\n", draw_data->x, draw_data->y, draw_data->w, draw_data->h);

	draw_data->r = LTOB(draw_data->r);
	draw_data->g = LTOB(draw_data->g);
	draw_data->b = LTOB(draw_data->b);
	draw_data->x = LTOB(draw_data->x);
	draw_data->y = LTOB(draw_data->y);
	draw_data->w = LTOB(draw_data->w);
	draw_data->h = LTOB(draw_data->h);

	//printf("A-X:%d, Y:%d\n", draw_data->x, draw_data->y);

	FPGA_SendDraw((void *)draw_data, 16);
}

static int b32_color( color_t color)
{
	return  ((0xff << 24) | ( ((color.red >>4)&0xff) << 16) | ( ((color.green >>4)&0xff) << 8) | ((color.blue >>4)&0xff));
}

static void b32_clear( dc_t *dc, color_t color)
{

#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		FPGA_DRAW_DATA box;
		box.r = dc->brush_color.red;
		box.g = dc->brush_color.green;
		box.b = dc->brush_color.blue;
		box.x = 0x0000;
		box.y = 0x0000;
		box.w = dc->width;
		box.h = dc->height;
		draw_box(&box);
	}
	else
	{
		uint32_t	*ptr;
		int			n_color;
		int			ndx;

		n_color 	= b32_color( color);
		ptr 		= (uint32_t *)dc->mapped;

		#pragma omp parallel for
		for ( ndx = 0; ndx < dc->dots; ndx++)
			*ptr++ 	= n_color;
	}
}

static void b32_set_pixel( dc_t *dc, int coor_x, int coor_y, color_t color)
{

#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		FPGA_DRAW_DATA box;
		box.r = dc->brush_color.red;
		box.g = dc->brush_color.green;
		box.b = dc->brush_color.blue;
		box.x = coor_x;
		box.y = coor_y;
		box.w = 0x0001;
		box.h = 0x0001;
		draw_box(&box);
	}
	else
	{
		uint32_t *ptr;

		// jschoi
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + (dc->width-coor_x-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-coor_x-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
				break;
		}

		*ptr  = b32_color(color);
	}
}

static void b32_get_pixel( dc_t *dc, int coor_x, int coor_y, color_t *color)
{

#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
	}
	else
	{
		uint32_t  *ptr;
		uint32_t   clr_bit;

		// jschoi
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + (dc->width-coor_x-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-coor_x-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
				break;
		}

		clr_bit        = *ptr;

		color->blue    =  clr_bit & 0xff;
		color->green   =  ( clr_bit >> 8 ) & 0xff;
		color->red     =  ( clr_bit >> 16) & 0xff;
	}
}

static int b32_color_bmp( color_tt color)
{
	return  (0xff << 24) |  ( color.red << 16) | ( color.green << 8) | color.blue;
}

static void b32_set_pixel_bmp( dc_t *dc, int coor_x, int coor_y, color_tt color)
{
	uint32_t *ptr;

	if(quhd_copy_mode==1)
	{
		if(is_5k_lvds_quad()) 	ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
		else 						ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
	}
	else
	{
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * ((dc->height-model_data.v_active)+coor_y) + (dc->width-(dc->width-model_data.h_active)-coor_x-1);
				//ptr = (uint32_t *)dc->mapped + dc->width * coor_y + (dc->width-coor_x-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * ((dc->height-model_data.v_active)+coor_y) + coor_x;
				//ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-(dc->width-model_data.h_active)-coor_x-1);
				//ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-coor_x-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
				break;
		}
	}

	*ptr  = b32_color_bmp( color);
}

static void b32_get_pixel_bmp( dc_t *dc, int coor_x, int coor_y, color_tt *color)
{
	uint32_t  *ptr;
	uint32_t   clr_bit;

	if(quhd_copy_mode==1)
	{
		if(is_5k_lvds_quad()) 	ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
		else 					ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
	}
	else
	{
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + (dc->width-coor_x-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + coor_x;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-coor_x-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + coor_x;
				break;
		}
	}

	clr_bit        = *ptr;

	color->blue    =  clr_bit & 0xff;
	color->green   =  ( clr_bit >> 8 ) & 0xff;
	color->red     =  ( clr_bit >> 16) & 0xff;
}

static void b32_hline( dc_t *dc, int x_1st, int x_2nd, int coor_y, color_t color)
{

#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		FPGA_DRAW_DATA box;
		box.r = dc->brush_color.red;
		box.g = dc->brush_color.green;
		box.b = dc->brush_color.blue;
		box.x = x_1st;
		box.y = coor_y;
		box.w = (x_2nd-x_1st)+1;
		box.h = 0x0001;
		draw_box(&box);
	}
	else
	{
		uint32_t 	*ptr;
		int			n_color;
		int			ndx;

		n_color  = b32_color( color);

		// jschoi
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + (dc->width-x_2nd-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * coor_y + x_1st;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + (dc->width-x_2nd-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-coor_y-1) + x_1st;
				break;
		}

		for ( ndx = x_1st; ndx <= x_2nd; ndx++ )
			*ptr++ = n_color;
	}
}

static void b32_vline( dc_t *dc, int coor_x, int y_1st, int y_2nd, color_t color)
{

#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		FPGA_DRAW_DATA box;
		box.r = dc->brush_color.red;
		box.g = dc->brush_color.green;
		box.b = dc->brush_color.blue;
		box.x = coor_x;
		box.y = y_1st;
		box.w = 0x0001;
		box.h = (y_2nd-y_1st)+1;
		draw_box(&box);
	}
	else
	{
		uint32_t *ptr;
		int            n_color;
		int            ndx;

		n_color  = b32_color( color);

		// jschoi
		switch(model_data.disp_mode)
		{
			case FLIP_HV:
				ptr = (uint32_t *)dc->mapped + dc->width * y_1st + (dc->width-coor_x-1);
				break;
			case FLIP_V:
				ptr = (uint32_t *)dc->mapped + dc->width * y_1st + coor_x;
				break;
			case FLIP_H:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-y_2nd-1) + (dc->width-coor_x-1);
				break;
			default:
				ptr = (uint32_t *)dc->mapped + dc->width * (dc->height-y_2nd-1) + coor_x;
				break;
		}

		for ( ndx = y_1st; ndx <= y_2nd; ndx++)
		{
			 *ptr   = n_color;
			  ptr  += dc->width;
		}
	}
}

static void draw_circle( dc_t *dc, int center_x, int center_y, int coor_x, int coor_y, color_t color, void (*fun)( dc_t *c, int, int, int, color_t))
{
	int      y_dot;

	y_dot   = center_y +coor_y;
	fun( dc, center_x -coor_x, center_x +coor_x, y_dot, color);

	y_dot   = center_y -coor_y;
	fun( dc, center_x -coor_x, center_x +coor_x, y_dot, color);

	y_dot   = center_y +coor_x;
	fun( dc, center_x -coor_y, center_x +coor_y, y_dot, color);

	y_dot   = center_y -coor_x;
	fun( dc, center_x -coor_y, center_x +coor_y, y_dot, color);
}

static void circle_dot( dc_t *dc, int x_1st , int x_2nd , int coor_y, color_t color)
{
	gx_set_pixel( dc, x_1st, coor_y, color);
	gx_set_pixel( dc, x_2nd, coor_y, color);
}

static void circle( dc_t *dc, int center_x, int center_y, int radius, color_t color, void (*fun)( dc_t *dc, int, int, int, color_t) )
{
	int      coor_x;
	int      coor_y;
	int      p_value;

	if ( 0 == color.alpha)    return;

	coor_x      = 0;
	coor_y      = radius;
	p_value     = 3 - 2 * radius;
	while   ( coor_x < coor_y)
	{
		draw_circle( dc, center_x, center_y, coor_x, coor_y, color, fun);
		if ( p_value < 0)
		{
			p_value   += 4 * coor_x +6;
		}
		else
		{
			p_value   += 4 * ( coor_x -coor_y) +10;
			coor_y--;
		}
		coor_x++;
	}
	if ( coor_x == coor_y)
		draw_circle( dc, center_x, center_y, coor_x, coor_y, color, fun);
}

static void ellipse( dc_t *dc, int center_x, int center_y, int width, int height, color_t color, void (*fun)( dc_t *dc, int, int, int, color_t) )
{
	int      coor_x, coor_y;
	long     onesqu_x, twosqu_x;
	long     onesqu_y, twosqu_y;
	long     delta, dx, dy;

	if ( 0 == color.alpha)    return;

	coor_x   = 0;
	coor_y   = height;
	onesqu_x = width * width;
	twosqu_x = onesqu_x << 1;
	onesqu_y = height * height;
	twosqu_y = onesqu_y << 1;

	delta    = onesqu_y - onesqu_x *height + (onesqu_x >> 2);
	dx       = 0;
	dy       = twosqu_x * height;

	while( dx < dy )
	{
		fun( dc, center_x-coor_x, center_x +coor_x, center_y +coor_y, color);
		fun( dc, center_x-coor_x, center_x +coor_x, center_y -coor_y, color);

		if( delta > 0 )
		{
			coor_y--;
			dy    -= twosqu_x;
			delta -= dy;
		}
		coor_x++;
		dx       += twosqu_y;
		delta    += onesqu_y + dx;
  	 }

	delta += ( 3*(onesqu_x - onesqu_y)/2 - (dx+dy)/2 );

	while( coor_y >= 0 )
	{
		fun( dc, center_x -coor_x, center_x +coor_x, center_y +coor_y, color);
		fun( dc, center_x -coor_x, center_x +coor_x, center_y -coor_y, color);

		if( delta < 0 )
		{
			coor_x++;
			dx      += twosqu_y;
			delta   += dx;
		}
		coor_y--;
		dy      -= twosqu_x;
		delta   += onesqu_x - dy;
	}
}

static void release_screen_dc( dc_t *dc)
{
	if ( NULL != dc)
		free( dc);
	dc = NULL;
}

static void release_buffer_dc( dc_t *dc)
{
	if ( NULL != dc)
	{
		if ( NULL != dc->mapped)
		{
			free( dc->mapped);
		}
		free( dc);
	}
	dc = NULL;
}

static void set_virtual_func( dc_t *dc, int colors)
{
	dc->clear      		= b32_clear;
	dc->get_pixel  		= b32_get_pixel;
	dc->set_pixel  		= b32_set_pixel;
	dc->get_pixel_bmp  	= b32_get_pixel_bmp;
	dc->set_pixel_bmp  	= b32_set_pixel_bmp;
	dc->hline      		= b32_hline;
	dc->vline      		= b32_vline;
}

static void  byte_bitblt( dc_t *dc_dest, int dest_x, int dest_y, dc_t *dc_sour, int sour_x, int sour_y, int sour_w, int sour_h, int bytes_per_pixel)
{
	int 	i=0;
	char  	*pdest, *psour;
	int   	bytes_per_line  = sour_w *bytes_per_pixel;

	//pdest = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*dest_y) +(dest_x*bytes_per_pixel);
	//psour = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(sour_x*bytes_per_pixel);

	// jschoi
	switch(model_data.disp_mode)
	{
		case FLIP_HV:
			psour = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(bytes_per_pixel*sour_x);
			pdest = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*dest_y) +(bytes_per_pixel*(dc_dest->width-dest_x-sour_w));
			break;
		case FLIP_V:
			psour = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(bytes_per_pixel*sour_x);
			pdest = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*dest_y) +(bytes_per_pixel*dest_x);
			break;
		case FLIP_H:
			psour = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(bytes_per_pixel*sour_x);
			pdest = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*(dc_dest->height-dest_y-sour_h)) +(bytes_per_pixel*(dc_dest->width-dest_x-sour_w));
			break;
		default:
			psour = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(bytes_per_pixel*sour_x);
			pdest = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*(dc_dest->height-dest_y-sour_h)) +(bytes_per_pixel*dest_x);
			break;
	}

	#pragma omp parallel for
	for(i=0;i<sour_h;i++)
	{
		memcpy( pdest, psour, bytes_per_line);
		pdest += dc_dest->bytes_per_line;
		psour += dc_sour->bytes_per_line;
	}

}
/*
static void  byte_bitblt_bmp( dc_t *dc_dest, int dest_x, int dest_y, dc_t *dc_sour, int sour_x, int sour_y, int sour_w, int sour_h, int bytes_per_pixel)
{
	int 	i=0;
	int   	bytes_per_line  = sour_w *bytes_per_pixel;
	char  	*pdest          = ((char *)dc_dest->mapped) +(dc_dest->bytes_per_line*dest_y) +(dest_x*bytes_per_pixel);
	char  	*psour          = ((char *)dc_sour->mapped) +(dc_sour->bytes_per_line*sour_y) +(sour_x*bytes_per_pixel);

	#pragma omp parallel for
	for(i=0; i<sour_h; i++)
	{
		memcpy( pdest, psour, bytes_per_line);
		pdest += dc_dest->bytes_per_line;
		psour += dc_sour->bytes_per_line;
	}
}
*/

color_t  gx_color( int red, int green, int blue, int alpha)
{
   color_t  color;

   color.red      = red;
   color.green    = green;
   color.blue     = blue;
   color.alpha    = alpha;

   return color;
}

void gx_set_alpha( color_t *color, int alpha)
{
   if       (   0 > alpha)       color->alpha   = 0;
   else if  ( 255 < alpha)       color->alpha   = 255;
   else                          color->alpha   = alpha;
}

void  gx_clear( dc_t *dc, color_t color)
{
   dc->clear( dc, color);
}

void  gx_clear_area( dc_t *dc, int x1, int y1, int x2, int y2, color_t color)
{
    int      tmp;
    
    if ( x2 < x1)
    {
        tmp   = x2;
        x2    = x1;
        x1    = tmp;
    }   
        
    if ( y2 < y1)
    {   
        tmp   = y2;
        y2    = y1;
        y1    = tmp;
    }
    
    tmp   = y1;
    for ( ; y1 <= y2; y1++)
        gx_hline( dc, x1, x2, y1, color);
    y1    = tmp;
    
    gx_hline( dc, x1, x2, y1, color);
    gx_hline( dc, x1, x2, y2, color);
    gx_vline( dc, x1, y1, y2, color);
    gx_vline( dc, x2, y1, y2, color);
}

void  gx_pen_color( dc_t *dc, color_t color)
{
	dc->pen_color = color;
}

void  gx_brush_color( dc_t *dc, color_t color)
{
	dc->brush_color = color;
}

void  gx_get_pixel( dc_t  *dc, int coor_x, int coor_y, color_t *color)
{
	if ( 0 > coor_x || dc->width  <= coor_x)     return;
	if ( 0 > coor_y || dc->height <= coor_y)     return;

	dc->get_pixel( dc, coor_x, coor_y, color);
}

void  gx_set_pixel( dc_t *dc, int coor_x, int coor_y, color_t color)
{
	if ( 0 > coor_x || dc->width  <= coor_x)     return;
	if ( 0 > coor_y || dc->height <= coor_y)     return;

	dc->set_pixel( dc, coor_x, coor_y, color);
}

void  gx_get_pixel_bmp( dc_t  *dc, int coor_x, int coor_y, color_tt *color)
{
	if ( 0 > coor_x || dc->width  <= coor_x)     return;
	if ( 0 > coor_y || dc->height <= coor_y)     return;

	dc->get_pixel_bmp( dc, coor_x, coor_y, color);
}

void  gx_set_pixel_bmp( dc_t *dc, int coor_x, int coor_y, color_tt color)
{
	if ( 0 > coor_x || dc->width  <= coor_x)     return;
	if ( 0 > coor_y || dc->height <= coor_y)     return;

	dc->set_pixel_bmp( dc, coor_x, coor_y, color);
}
void  gx_move_to( dc_t *dc, int coor_x, int coor_y)
{
	dc->coor_x  = coor_x;
	dc->coor_y  = coor_y;
}

void  gx_line_to( dc_t *dc, int coor_x, int coor_y)
{
	int      rx, ry;
	int      dx, dy;
	int      inc_x;
	int      inc_y;
	int      offset;
	color_t  color;

	rx = dc->coor_x;
	ry = dc->coor_y;

	dc->coor_x  = coor_x;
	dc->coor_y  = coor_y;

	dx = coor_x -rx;
	if ( 0 == dx)
	{
		gx_vline( dc, rx, ry, coor_y, dc->pen_color);
		return;
	}
	else if ( 0 < dx)    inc_x = 1;
	else
	{
		dx    = -dx;
    	inc_x = -1;
	}

	dy = coor_y -ry;
	if ( 0 == dy)
	{
		gx_hline( dc, rx, coor_x, ry, dc->pen_color);
		return;
	}
	else if ( 0 < dy)    inc_y = 1;
	else
	{
		dy    = -dy;
		inc_y = -1;
	}

	color = dc->pen_color;
	gx_set_pixel( dc, rx, ry, color);
	if ( dy <= dx)
	{
		offset   = dx / 2;

		for (; rx != coor_x; rx += inc_x)
		{
			offset   += dy;
			if ( dx <= offset)
			{
				offset   -= dx;
				ry       += inc_y;
			}
			gx_set_pixel( dc, rx, ry, color);
		}
	}
	else
	{
		offset   = dy /2;

		for (; ry != coor_y; ry += inc_y)
		{
			offset   += dx;
			if ( dy <= offset)
			{
				offset   -= dy;
				rx       += inc_x;
			}
			gx_set_pixel( dc, rx, ry, color);
		}
	}
}

void gx_hline( dc_t *dc, int x_1st  , int x_2nd , int coor_y, color_t color)
{
	int ndx;

	if ( 0 > coor_y || dc->height <= coor_y)  return;
	if ( x_2nd < x_1st)
	{
		ndx      = x_1st;
		x_1st    = x_2nd;
		x_2nd    = ndx;
	}

	if       ( 0         >  x_1st)  x_1st   = 0;
	else if  ( dc->width <= x_1st)  x_1st   = dc->width-1;

	if       ( 0         >  x_2nd)  x_2nd   = 0;
	else if  ( dc->width <= x_2nd)  x_2nd   = dc->width-1;

	dc->hline( dc, x_1st, x_2nd, coor_y, color);
}

void gx_vline( dc_t *dc, int coor_x, int y_1st , int y_2nd , color_t color)
{
   int             ndx;

   if ( 0 > coor_x || dc->width <= coor_x)   return;

   if ( y_2nd < y_1st)
   {
      ndx     = y_1st;
      y_1st    = y_2nd;
      y_2nd    = ndx;
   }

   if       ( 0           >  y_1st)  y_1st   = 0;
   else if  ( dc->height  <= y_1st)  y_1st   = dc->height-1;

   if       ( 0           >  y_2nd)  y_2nd   = 0;
   else if  ( dc->height  <= y_2nd)  y_2nd   = dc->height-1;

   dc->vline( dc, coor_x, y_1st, y_2nd, color);
}

void  gx_line( dc_t *dc, int x1, int y1, int x2, int y2)
{
	gx_move_to( dc, x1, y1);
	gx_line_to( dc, x2, y2);
}

void  gx_rectangle( dc_t *dc, int x1, int y1, int x2, int y2)
{
	int				tmp;
	unsigned short	hsize;
	unsigned short	vsize;
	FPGA_DRAW_DATA 	box;
	
	if ( x2 < x1)
	{
		tmp   = x2;
		x2    = x1;
		x1    = tmp;
	}

	if ( y2 < y1)
	{
		tmp   = y2;
		y2    = y1;
		y1    = tmp;
	}
   
	if ( 0 != dc->brush_color.alpha)
	{

#ifdef USE_8K_LOGIC_PATTERN
		if(DRAWCHECK(dc->output_display))
#else
		if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
		{
			hsize = (x2-x1)+1;
			vsize = (y2-y1)+1;

			box.r = dc->brush_color.red;
			box.g = dc->brush_color.green;
			box.b = dc->brush_color.blue;
			box.x = x1;
			box.y = y1;
			box.w = hsize;
			box.h = vsize;
			draw_box(&box);
		}
		else
		{
			tmp = y1;
			for (; y1 <= y2; y1++)
				gx_hline( dc, x1, x2, y1, dc->brush_color);
			y1 = tmp;
		}
	}

	if ( 0 != dc->pen_color.alpha)
	{
		gx_hline( dc, x1, x2, y1, dc->pen_color);
		gx_hline( dc, x1, x2, y2, dc->pen_color);
		gx_vline( dc, x1, y1, y2, dc->pen_color);
		gx_vline( dc, x2, y1, y2, dc->pen_color);
	}
}

void gx_circle( dc_t *dc, int center_x, int center_y, int radius)
{
	if ( 0 == radius) return;

	circle( dc, center_x, center_y, radius, dc->brush_color, gx_hline);
	circle( dc, center_x, center_y, radius, dc->pen_color  , circle_dot);
}

void  gx_ellipse(  dc_t *dc, int center_x, int center_y, int width, int height)
{
	if ( ( 0 == width) || ( 0 == height) ) return;

	ellipse( dc, center_x, center_y, width, height, dc->brush_color, gx_hline);
	ellipse( dc, center_x, center_y, width, height, dc->pen_color  , circle_dot);
}

static void triangle( dc_t *dc, int x1, int y1, int x2, int y2, int x3, int y3, color_t color, void (*fun)( dc_t *dc, int, int, int, color_t) )
{
	float d21, d31, d32;
	float sx, ex, x, y;

	if(y2 < y1) { int t=y1; y1=y2; y2=t; t=x1; x1=x2; x2=t;}
	if(y3 < y1) { int t=y1; y1=y3; y3=t; t=x1; x1=x3; x3=t;}
	if(y3 < y2) { int t=y2; y2=y3; y3=t; t=x2; x2=x3; x3=t;}

	if(y2-y1 > 0)	d21 = (float)(x2-x1)/(float)(y2-y1);
	else			d21 = 0;
	if(y3-y1 > 0)	d31 = (float)(x3-x1)/(float)(y3-y1);
	else			d31 = 0;
	if(y3-y2> 0)	d32 = (float)(x3-x2)/(float)(y3-y2);
	else			d32 = 0;

	sx = ex = x1;
	y = y1;
	if(d21 > d31){
		for(; y<y2; y++){
			x = sx;
			fun( dc, en_round(x), en_round(ex), en_round(y), color);
			sx += d31;
			ex += d21;
		}
		
		ex = x2; y = y2;
		for(; y<y3; y++){
			x = sx;
			fun( dc, en_round(x), en_round(ex), en_round(y), color);
			sx += d31;
			ex += d32;
		}
	}
	else{
		for(; y<y2; y++){
			x = sx;
			fun( dc, en_round(x), en_round(ex), en_round(y), color);
			sx += d21;
			ex += d31;
		}
		
		sx = x2; y = y2;
		for(; y<y3; y++){
			x = sx;
			fun( dc, en_round(x), en_round(ex), en_round(y), color);
			sx += d32;
			ex += d31;
		}
	}
}

static void hole( dc_t *dc, int center_x, int center_y, int width, int height, color_t color, void (*fun)( dc_t *dc, int, int, int, color_t) )
{
   int	    start_x, end_x;
   int      coor_x, coor_y;
   long     onesqu_x, twosqu_x;
   long     onesqu_y, twosqu_y;
   long     delta, dx, dy;

   if ( 0 == color.alpha)    return;

   start_x  = center_x - width;
   end_x    = center_x + width;

   coor_x   = 0;
   coor_y   = height;
   onesqu_x = width * width;
   twosqu_x = onesqu_x << 1;
   onesqu_y = height * height;
   twosqu_y = onesqu_y << 1;

   delta    = onesqu_y - onesqu_x *height + (onesqu_x >> 2);
   dx       = 0;
   dy       = twosqu_x * height;

   while( dx < dy )
   {
      fun( dc, start_x        , center_x-coor_x, center_y+coor_y, color);
      fun( dc, center_x+coor_x, end_x          , center_y+coor_y, color);
      fun( dc, start_x        , center_x-coor_x, center_y-coor_y, color);
      fun( dc, center_x+coor_x, end_x          , center_y-coor_y, color);

      if( delta > 0 )
      {
         coor_y--;
         dy    -= twosqu_x;
         delta -= dy;
      }
      coor_x++;
      dx       += twosqu_y;
      delta    += onesqu_y + dx;
   }

   delta += ( 3*(onesqu_x - onesqu_y)/2 - (dx+dy)/2 );

   while( coor_y >= 0 )
   {
      fun( dc, start_x        , center_x-coor_x, center_y+coor_y, color);
      fun( dc, center_x+coor_x, end_x          , center_y+coor_y, color);
      fun( dc, start_x        , center_x-coor_x, center_y-coor_y, color);
      fun( dc, center_x+coor_x, end_x          , center_y-coor_y, color);

      if( delta < 0 )
      {
         coor_x++;
         dx      += twosqu_y;
         delta   += dx;
      }
      coor_y--;
      dy      -= twosqu_x;
      delta   += onesqu_x - dy;
   }
}

void  gx_hole(  dc_t *dc, int center_x, int center_y, int width, int height)
{
   if ( ( 0 == width) || ( 0 == height) ) return;

   hole   ( dc, center_x, center_y, width, height, dc->brush_color, gx_hline);
   ellipse( dc, center_x, center_y, width, height, dc->pen_color  , circle_dot);
}

void  gx_invrectangle( dc_t *dc, int x1, int y1, int x2, int y2)
{
    color_t color;
    int     tmp;

    if ( x2 < x1)
    {
       tmp   = x2;
       x2    = x1;
       x1    = tmp;
    }

    if ( y2 < y1)
    {
       tmp   = y2;
       y2    = y1;
       y1    = tmp;
    }

    tmp = x1;
    for ( ; y1 <= y2; y1++)
    {
        x1  = tmp;
        for ( ; x1 <= x2; x1++)
        {
            gx_get_pixel( dc, x1, y1, &color);
            color.red   ^= 0xff;
            color.green ^= 0xff;
            color.blue  ^= 0xff;
            gx_set_pixel( dc, x1, y1, color);
        }
    }
}

void gx_triangle( dc_t *dc, int x1, int y1, int x2, int y2, int x3, int y3)
{
    if(0 != dc->brush_color.alpha){
		triangle( dc, x1, y1, x2, y2, x3, y3, dc->brush_color, gx_hline);
    }

    if(0 != dc->pen_color.alpha){
		gx_line( dc, x1, y1, x2, y2);
		gx_line( dc, x1, y1, x3, y3);
		gx_line( dc, x2, y2, x3, y3);
    }
}

void gx_gradation( dc_t *dc, int type, int x1, int y1, int x2, int y2, int level, color_t start_c, color_t end_c)
{
	float  step_r, step_g, step_b;
	float  cur_r, cur_g, cur_b;
	float  start_x, end_x, step_x;
	float  start_y, end_y, step_y; 
	int	   r=0, g=0, b=0;
	int	   level_p,i;

	level_p = min(level, 1024);
	/*
	if(change_data.gray_scale==0) 	level_p = min(level, 1024);
	else 							level_p = min(change_data.gray_scale, 1024);
	change_data.gray_scale = level_p;
	*/

	// start color
	cur_r = start_c.red;
	cur_g = start_c.green;
	cur_b = start_c.blue;

	// color step
	step_r = (float)(end_c.red-start_c.red    ) / (float)level_p;
	step_g = (float)(end_c.green-start_c.green) / (float)level_p;
	step_b = (float)(end_c.blue-start_c.blue  ) / (float)level_p;

	gx_pen_color  ( dc,	gx_color(r, g, b, 0));
	
	switch(type){
	case T_HGRAY:
		start_x = (float)x1;
		step_x = (float)(x2-x1) / (float)level_p;
		for(i=0; i<level_p; i++){
			end_x = start_x + step_x;

			r = en_round(cur_r);
			g = en_round(cur_g);
			b = en_round(cur_b);
			gx_brush_color( dc,	gx_color(r, g, b, 255));
			gx_rectangle( dc, en_round(start_x), y1, en_round(end_x), y2);

			start_x += step_x;
			cur_r += step_r;
			cur_g += step_g;
			cur_b += step_b;
		}
		break;

	case T_VGRAY:
		start_y = (float)y1;
		step_y = (float)(y2-y1) / (float)level_p;
		for(i=0; i<level_p; i++){
			end_y = start_y + step_y;

			r = en_round(cur_r);
			g = en_round(cur_g);
			b = en_round(cur_b);
			gx_brush_color( dc,	gx_color(r, g, b, 255));
			gx_rectangle( dc, x1, en_round(start_y), x2, en_round(end_y));

			start_y += step_y;
			cur_r += step_r;
			cur_g += step_g;
			cur_b += step_b;
		}
		break;

	case T_CGRAY:
		step_x = ((float)(x2-x1)/2.) / (float)level_p;
		step_y = ((float)(y2-y1)/2.) / (float)level_p;
		for(i=0; i<level_p; i++){
			start_x = step_x * i;
			end_x   = step_x * (i+1);
			start_y = step_y * i;
			end_y   = step_y * (i+1);

			r = en_round(cur_r);
			g = en_round(cur_g);
			b = en_round(cur_b);
			gx_brush_color( dc,	gx_color(r, g, b, 255));
			gx_rectangle( dc, x1+en_round(start_x), y1+en_round(start_y), x1+en_round(end_x), y2-en_round(start_y));
			gx_rectangle( dc, x2-en_round(end_x), y1+en_round(start_y), x2-en_round(start_x), y2-en_round(start_y));
			gx_rectangle( dc, x1+en_round(start_x), y1+en_round(start_y), x2-en_round(start_x), y1+en_round(end_y));
			gx_rectangle( dc, x1+en_round(start_x), y2-en_round(end_y), x2-en_round(start_x), y2-en_round(start_y));
			cur_r += step_r;
			cur_g += step_g;
			cur_b += step_b;
		}
		break;
	}
}

void gx_chess( dc_t *dc, int x1, int y1, int x2, int y2, int cnt_x, int cnt_y, color_t bg_c, color_t fg_c)
{
	float 	step_x, step_y;
	int		x, y;

	// background
	//gx_pen_color  ( dc, bg_c);
	//gx_brush_color( dc, bg_c);

	gx_pen_color(dc, gx_color(bg_c.red, bg_c.green, bg_c.blue, 0));
	gx_brush_color(dc, gx_color(bg_c.red, bg_c.green, bg_c.blue, 255));
	gx_rectangle( dc, x1, y1, x2, y2);

	// step
//	step_x = (float)(x2-x1)/(float)cnt_x;
//	step_y = (float)(y2-y1)/(float)cnt_y;
	step_x = (float)(x2-x1+1)/(float)cnt_x;		//19-09-20	jschoi		input value(H-1) is 1 less than actual size(H)
	step_y = (float)(y2-y1+1)/(float)cnt_y;		//19-09-20	jschoi		input value(H-1) is 1 less than actual size(H)

	//gx_pen_color  ( dc, fg_c);
	//gx_brush_color( dc, fg_c);

	gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));

	for( y=0; y<cnt_y; y++){
		for( x=0; x<cnt_x; x++){
			if(y%2){	// OddY => EvenX
				if(x%2==0){
					gx_rectangle( dc, en_round(x1+(step_x*x)), en_round(y1+(step_y*y)), en_round(x1+(step_x*(x+1)))-1, en_round(y1+(step_y*(y+1)))-1);
				}
			}
			else{		// EvenY => OddX
				if(x%2){
					gx_rectangle( dc, en_round(x1+(step_x*x)), en_round(y1+(step_y*y)), en_round(x1+(step_x*(x+1)))-1, en_round(y1+(step_y*(y+1)))-1);
				}
			}
		}
	}
}

void gx_hatch( dc_t *dc, int x1, int y1, int x2, int y2, int line, int cnt_x, int cnt_y)
{
	int		i, j;
	int		x, y;
	int 	div_r, div_e;
	float 	step_x, step_y;

    if(0 == dc->pen_color.alpha) return;

	step_x = (float)(x2-x1) / (float)cnt_x;
	step_y = (float)(y2-y1) / (float)cnt_y;

	if(line<=1){
		for(i=0; i<=cnt_x; i++){
			x = en_round(x1+(step_x*i));
			gx_vline( dc, x, y1, y2, dc->pen_color); 
		}
		for(i=0; i<=cnt_y; i++){
			y = en_round(y1+(step_y*i));
			gx_hline( dc, x1, x2, y, dc->pen_color);
		}		
	}
	else{
		div_r = line >> 1;	// ��
		div_e = line % 2;	// ������
		if(div_e){	// Ȧ�� ����
			for(i=0; i<=cnt_x; i++){
				x = en_round(x1+(step_x*i));
				// �߽ɼ��� �������� ������ �յ��ϰ� �׸���.
				for(j=(x-div_r); j<=(x+div_r); j++){
					if(j<0) 				gx_vline( dc, j+line, y1, y2, dc->pen_color);	// 0���� �������� ����� ���δ�
					else if(j>dc->width-1) 	gx_vline( dc, j-line, y1, y2, dc->pen_color);	// �ִ밪���� ū���� ���������� ���δ�
					else					gx_vline( dc, j, y1, y2, dc->pen_color);
				}
			}
			for(i=0; i<=cnt_y; i++){
				y = en_round(y1+(step_y*i));
				for(j=(y-div_r); j<=(y+div_r); j++){
					if(j<0)					gx_hline( dc, x1, x2, j+line, dc->pen_color);
					else if(j>dc->height-1)	gx_hline( dc, x1, x2, j, dc->pen_color);
					else 					gx_hline( dc, x1, x2, j, dc->pen_color);
				}
			}
		}
		else{		// ¦�� ����
			for(i=0; i<=cnt_x; i++){
				x = en_round(x1+(step_x*i));
				// �߽ɼ��� �������� ������ �ϳ� ���� �׸���
				for(j=(x-div_r+1); j<=(x+div_r); j++){
					if(j<0) 				gx_vline( dc, j+line, y1, y2, dc->pen_color);	// 0���� �������� ����� ���δ�
					else if(j>dc->width-1) 	gx_vline( dc, j-line, y1, y2, dc->pen_color);	// �ִ밪���� ū���� ���������� ���δ�
					else					gx_vline( dc, j, y1, y2, dc->pen_color);
				}
			}
			for(i=0; i<=cnt_y; i++){
				y = en_round(y1+(step_y*i));
				for(j=(y-div_r+1); j<=(y+div_r); j++){
					if(j<0)					gx_hline( dc, x1, x2, j+line, dc->pen_color);
					else if(j>dc->height-1)	gx_hline( dc, x1, x2, j, dc->pen_color);
					else 					gx_hline( dc, x1, x2, j, dc->pen_color);
				}
			}
		}
	}
}

int ai_sts=0;
ai_pat_t ai_data;
struct timeval AiStartTime;
struct timeval AiEndTime;
void timer_handler()
{
	if(gp.gx_ai_enable)
	{
		if(model_data.if_type!=IF_HDMI)
		{
			struct timeval tDiffTime;
			gettimeofday(&AiEndTime, 0);
			timersub(&AiEndTime, &AiStartTime, &tDiffTime);

			if((tDiffTime.tv_usec/1000)>=(ai_data.time))
			{
				gettimeofday(&AiStartTime, 0);
				glFinish();
				glXSwapBuffers(xdisp,window);
				if(ai_sts)
				{
					glDrawPixels(model_data.h_active,model_data.v_active,GL_BGRA,GL_UNSIGNED_BYTE,dc_ai1_buffer->mapped);
					ai_sts=0;
				}
				else
				{
					glDrawPixels(model_data.h_active,model_data.v_active,GL_BGRA,GL_UNSIGNED_BYTE,dc_ai2_buffer->mapped);
					ai_sts=1;
				}
			}
		}
	}
}

void gx_ai(dc_t *dc, dc_t *dc1, dc_t *dc2, color_t *bg, color_t *fg1, color_t *fg2, int size, int time)
{
	gx_pen_color(dc, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_brush_color(dc, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_rectangle(dc, 0, 0,dc->width,dc->height);

	gx_pen_color(dc1, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_brush_color(dc1, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_rectangle(dc1, 0, 0,dc1->width,dc1->height);

	gx_pen_color(dc2, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_brush_color(dc2, gx_color(bg->red, bg->green, bg->blue, 255));
	gx_rectangle(dc2, 0, 0,dc2->width,dc2->height);
	if(size) {

		int sx = (dc->width / 2) - (dc->height  * size / 20);
		int sy = (dc->width  / 2) - (dc->height * size / 20);
		int ex = (dc->width  / 2) + (dc->height * size / 20);
		int ey = (dc->width  / 2) + (dc->height * size / 20);

		gx_pen_color(dc_buffer, gx_color(fg1->red, fg1->green, fg1->blue, 255));
		gx_brush_color(dc_buffer, gx_color(fg1->red, fg1->green, fg1->blue, 255));
		gx_rectangle(dc_buffer, sx, sy, ex, ey);

		gx_pen_color(dc_ai1_buffer, gx_color(fg1->red, fg1->green, fg1->blue, 255));
		gx_brush_color(dc_ai1_buffer, gx_color(fg1->red, fg1->green, fg1->blue, 255));
		gx_rectangle(dc_ai1_buffer, sx, sy, ex, ey);

		gx_pen_color(dc_ai2_buffer, gx_color(fg2->red, fg2->green, fg2->blue, 255));
		gx_brush_color(dc_ai2_buffer, gx_color(fg2->red, fg2->green, fg2->blue, 255));
		gx_rectangle(dc_ai2_buffer, sx, sy, ex, ey);
	}
	ai_data.time=time;
	ai_data.size=size;
	gettimeofday(&AiStartTime, 0);
	ai_sts=0;
	gp.gx_ai_enable=1;
}

void gx_ai_fpga( dc_t *dc, int size, int time)
{
	int ai_frame_cnt=1;

	gx_pen_color(dc, gx_color(0, 0, 0, 0));
	gx_brush_color(dc, gx_color(0, 0, 0, 255));
	gx_rectangle(dc, 0, 0, dc->width, dc->height);
	if(size) {

//		int sx = (dc->width  / 2) - ((float)dc->width * (float)((float)size  / 20));
//		int sy = (dc->height / 2) - ((float)dc->height * (float)((float)size / 20));
//		int ex = (dc->width  / 2) + ((float)dc->width * (float)((float)size  / 20));
//		int ey = (dc->height / 2) + ((float)dc->height *(float)((float)size / 20));

		int sx = (model_data.h_active  / 2) - ((float)model_data.h_active * (float)((float)size  / 20));
		int sy = (model_data.v_active / 2) - ((float)model_data.v_active * (float)((float)size / 20));
		int ex = (model_data.h_active  / 2) + ((float)model_data.h_active * (float)((float)size  / 20));
		int ey = (model_data.v_active / 2) + ((float)model_data.v_active *(float)((float)size / 20));

		gx_pen_color(dc, gx_color(1, 1, 1, 0));
		gx_brush_color(dc, gx_color(1, 1, 1, 255));
		gx_rectangle(dc, sx, sy, ex, ey);
		//printf("ai sx=%d sy=%d ex=%d ey=%d\n",sx, sy, ex, ey);
	}
	ai_frame_cnt=time/(1000/gp.module_hz);
	if(ai_frame_cnt<1) ai_frame_cnt=1;

	FPGA_Write(AI_FRAME_COUNT,ai_frame_cnt);
	gp.gx_ai_enable=1;
}

#define FLIC_UNIT	4
void gx_flicker( dc_t *dc, int x1, int y1, int x2, int y2, color_us *pcolor, unsigned short *pline)
{
	int 	x,y;
	int		width, height;
   	FPGA_DRAW_DATA box;
	color_t c;
	dc_t 	*dc_4by4;


#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		width 	= (x2 - x1)+1;
		height 	= (y2 - y1)+1;

		memset(&box,0,sizeof(FPGA_DRAW_DATA));

		//fillbox end
		{
			FPGA_SendEnd();
			transfer();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);//fill box write flag
			fpga_draw_check(DRAW_PATTERN_CHECK);
		}

		//copy&paste start
		FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0004); //paste write flag

		FPGA_OR_SET(FPGA_SRAM_WR_CTRL, 0x0010);//wen
		for(y=0; y<FLIC_UNIT; y++){
			FPGA_ANDOR_SET(FPGA_SRAM_WR_CTRL, 0x0003, y&0x0003);

			FPGA_Write(FPGA_PAL0_R, pcolor[(pline[y]>>(12-FLIC_UNIT*0))&0xF].red>>2);
			FPGA_Write(FPGA_PAL0_G, pcolor[(pline[y]>>(12-FLIC_UNIT*0))&0xF].green>>2);
			FPGA_Write(FPGA_PAL0_B, pcolor[(pline[y]>>(12-FLIC_UNIT*0))&0xF].blue>>2);
			FPGA_Write(FPGA_PAL1_R, pcolor[(pline[y]>>(12-FLIC_UNIT*1))&0xF].red>>2);
			FPGA_Write(FPGA_PAL1_G, pcolor[(pline[y]>>(12-FLIC_UNIT*1))&0xF].green>>2);
			FPGA_Write(FPGA_PAL1_B, pcolor[(pline[y]>>(12-FLIC_UNIT*1))&0xF].blue>>2);
			FPGA_Write(FPGA_PAL2_R, pcolor[(pline[y]>>(12-FLIC_UNIT*2))&0xF].red>>2);
			FPGA_Write(FPGA_PAL2_G, pcolor[(pline[y]>>(12-FLIC_UNIT*2))&0xF].green>>2);
			FPGA_Write(FPGA_PAL2_B, pcolor[(pline[y]>>(12-FLIC_UNIT*2))&0xF].blue>>2);
			FPGA_Write(FPGA_PAL3_R, pcolor[(pline[y]>>(12-FLIC_UNIT*3))&0xF].red>>2);
			FPGA_Write(FPGA_PAL3_G, pcolor[(pline[y]>>(12-FLIC_UNIT*3))&0xF].green>>2);
			FPGA_Write(FPGA_PAL3_B, pcolor[(pline[y]>>(12-FLIC_UNIT*3))&0xF].blue>>2);
			FPGA_OR_SET(FPGA_SRAM_WR_CTRL, 0x0020);
			FPGA_AND_SET(FPGA_SRAM_WR_CTRL, 0x0020);

			usleep(1000);	// important! some flicker pattern has not be displayed
			// read register
			/*{
				unsigned short r0, r1, r2, r3;
				unsigned short g0, g1, g2, g3;
				unsigned short b0, b1, b2, b3;

				r0 = FPGA_Read(FPGA_PAL0_R); g0 = FPGA_Read(FPGA_PAL0_G); b0 = FPGA_Read(FPGA_PAL0_B);
				r1 = FPGA_Read(FPGA_PAL1_R); g1 = FPGA_Read(FPGA_PAL1_G); b1 = FPGA_Read(FPGA_PAL1_B);
				r2 = FPGA_Read(FPGA_PAL2_R); g2 = FPGA_Read(FPGA_PAL2_G); b2 = FPGA_Read(FPGA_PAL2_B);
				r3 = FPGA_Read(FPGA_PAL3_R); g3 = FPGA_Read(FPGA_PAL3_G); b3 = FPGA_Read(FPGA_PAL3_B);

				printf("FPGA_SRAM_WR_CTRL:0x%x, LINE:%d, RGB: \t|(%d, %d, %d)\t|(%d, %d, %d)\t|(%d, %d, %d)\t|(%d, %d, %d)\t|\n", FPGA_Read(FPGA_SRAM_WR_CTRL), y, r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3);
			}*/
		}
		FPGA_AND_SET(FPGA_SRAM_WR_CTRL, 0x0010); //wen

		box.x = x1;
		box.y = y1;
		box.w = width;
		box.h = height;
		draw_box(&box);

		//copy&paste end
		{
			FPGA_SendEnd();
			transfer();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0004); //paste write flag
			fpga_draw_check(DRAW_PATTERN_CHECK);
		}
		//fillbox end
		FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002); //fill box write flag
	}
	else
	{
		width 	= x2 - x1;
		height 	= y2 - y1;

		dc_4by4 = gx_get_buffer_dc(FLIC_UNIT, FLIC_UNIT);
		dc_4by4->output_display = dc->output_display;
		for(y=0; y<FLIC_UNIT; y++){
			for(x=0; x<FLIC_UNIT; x++){
				c = gx_color( pcolor[(pline[y]>>(12-FLIC_UNIT*x))&0xF].red,
							  pcolor[(pline[y]>>(12-FLIC_UNIT*x))&0xF].green,
							  pcolor[(pline[y]>>(12-FLIC_UNIT*x))&0xF].blue,
							  255);
				gx_set_pixel( dc_4by4, x, y, c);
			}
		}

		for (x=0; x<width; x+=FLIC_UNIT) {
			for (y=0; y<height; y+=FLIC_UNIT) {
				gx_bitblt( dc, x1+x, y1+y, dc_4by4, 0, 0, FLIC_UNIT, FLIC_UNIT);
			}
		}

		gx_release_dc(dc_4by4);
	}
}

void gx_ellipse_gradation( dc_t *dc, int x1, int y1, int x2, int y2, int level/*, color_t start_c, color_t end_c*/)
{
	float 	color;
	int	 	step_r, step_g, step_b;
	int		cur_r, cur_g, cur_b;
	float  	w, h, half_w, half_h;
	int	   	level_p, x, y;

	w 		= (float)(x2 - x1);
	h 		= (float)(y2 - y1);
	half_w 	= w * .5;
	half_h 	= h * .5;

	level_p = min(level, 255);
	
	// start color
//	cur_r = start_c.red;
//	cur_g = start_c.green;
//	cur_b = start_c.blue;

	cur_r = cur_g = cur_b = 0;

	// color step
//	step_r = (float)(end_c.red-start_c.red    ) / (float)level_p;
//	step_g = (float)(end_c.green-start_c.green) / (float)level_p;
//	step_b = (float)(end_c.blue-start_c.blue  ) / (float)level_p;


	for(y=0; y<h; y++){
		for(x=0; x<w; x++){
			color = ((x-half_w)*(x-half_w) + (y-half_h)*(y-half_h)) / level_p;

			step_r = en_round(color/8) % 256;
			step_g = en_round(color/4) % 256;
			step_b = en_round(color/2) % 256;

			cur_r |= step_r;
			cur_g |= step_g;
			cur_b |= step_b;

			gx_set_pixel(dc, x, y, gx_color(cur_r, cur_g, cur_b, 255));
		}
	}
}

void gx_mouse( dc_t *dc_screen, int x, int y, dc_t *dc_mem, int oldx, int oldy, int enable)
{
	int w, h;
	int copy_w, copy_h;

	if(oldx<gx_fb.width-POS_W)  	copy_w = POS_W;
	else							copy_w = (gx_fb.width-oldx) - 1;
	if(oldy<gx_fb.height-POS_H) 	copy_h = POS_H;
	else							copy_h = (gx_fb.height-oldy) - 1;

	gx_bitblt( dc_screen, oldx+1, oldy+1, dc_mem, oldx+1, oldy+1, copy_w, copy_h);

	if(0==enable) return;

	if(x<gx_fb.width-POS_W)  	copy_w = POS_W;
	else						copy_w = (gx_fb.width-x) - 1;
	if(y<gx_fb.height-POS_H) 	copy_h = POS_H;
	else						copy_h = (gx_fb.height-y) - 1;

	for(h=0; h<copy_h; h++){
		for(w=0; w<copy_w; w++){
			if(1==mouse_p[h][w])
				gx_set_pixel( dc_screen, x+w+1, y+h+1, mouse_bmp[h][w]);
		}
	}
}

void gx_to_screen_dc( dc_t *dc_screen, dc_t *dc_buffer)
{
   memcpy( dc_screen->mapped, dc_buffer->mapped, dc_screen->bytes);
}

void  gx_bitblt_bmp( dc_t *dc_dest, int dest_x, int dest_y, dc_t *dc_sour, int sour_x, int sour_y, int sour_w, int sour_h)
{
    int         coor_x;
    int         coor_y;
    color_tt     color_sour;
    color_tt     color_dest;
    int         ncheck;

    if ( 0 > dest_x)
    {
       sour_x -= dest_x;
       sour_w += dest_x;
       dest_x = 0;
       if ( 0 >= sour_w)     return;
    }
    if ( dc_dest->width <= dest_x)      return;

    if ( 0 > dest_y)
    {
        sour_y -= dest_y;
        sour_h += dest_y;
        if ( 0 >= sour_h)     return;

        dest_y = 0;
    }
    if ( dc_dest->height <= dest_y)     return;

    if ( 0 > sour_x)   sour_x = 0;
    else if ( dc_sour->width <= sour_x)   return;

    if ( 0 > sour_y)   sour_y = 0;
    else if ( dc_sour->height <= sour_y)  return;

    ncheck   = dest_x + sour_w;
    if ( dc_dest->width < ncheck)   sour_w = dc_dest->width -dest_x;

    ncheck   = sour_x + sour_w;
    if ( dc_sour->width < ncheck)   sour_w = dc_sour->width -sour_x;

    ncheck   = dest_y + sour_h;
    if ( dc_dest->height < ncheck)  sour_h = dc_dest->height -dest_y;

    ncheck   = sour_y + sour_h;
    if ( dc_sour->height < ncheck)  sour_h = dc_sour->height -sour_y;

    /*
    if ( (dc_sour->dc_type == dc_dest->dc_type) &&  (dc_sour->bits_per_pixel == dc_dest->bits_per_pixel) )
    {
       switch( dc_dest->colors)
       {
       case  8  :
       case  15 :
       case  16 :
       case  24 :
       case  32 :
			byte_bitblt_bmp( dc_dest, dest_x, dest_y, dc_sour, sour_x, sour_y, sour_w, sour_h, dc_sour->bits_per_pixel / 8);
			break;
       default  :   printf( "8 bit ÀÌÇÏÀÇ byte_bitblt()žŠ ±žÇöÇØŸß ÇÕŽÏŽÙ. \n"); break;
       }
    }*/
    if ( DCTYPE_PNG == dc_sour->dc_type)
    {
		for ( coor_y = 0; coor_y < sour_h; coor_y++)
		{
			for ( coor_x = 0; coor_x < sour_w; coor_x++)
			{
				dc_sour->get_pixel_bmp( dc_sour, coor_x+sour_x, coor_y+sour_y, &color_sour);
				if ( 0 < color_sour.alpha)
				{
					if ( 255 > color_sour.alpha)
					{
						dc_dest->get_pixel_bmp( dc_dest, coor_x+dest_x, coor_y+dest_y, &color_dest);
						ALPHA_COMPOSITE( color_sour.red  , color_sour.red  , color_sour.alpha, color_dest.red  );
						ALPHA_COMPOSITE( color_sour.green, color_sour.green, color_sour.alpha, color_dest.green);
						ALPHA_COMPOSITE( color_sour.blue , color_sour.blue , color_sour.alpha, color_dest.blue );
					} // if
					dc_dest->set_pixel_bmp( dc_dest, coor_x+dest_x, coor_y+dest_y, color_sour);
				} // if
			} // for
		} // for
    }
    else
    {
		for ( coor_y = 0; coor_y < sour_h; coor_y++)
    	{
    		for ( coor_x = 0; coor_x < sour_w; coor_x++)
			{
    			dc_sour->get_pixel_bmp( dc_sour, coor_x+sour_x, coor_y+sour_y, &color_sour);
    			dc_dest->set_pixel_bmp( dc_dest, coor_x+dest_x, coor_y+dest_y,  color_sour);
			}
		}
    }
}

void  gx_bitblt( dc_t *dc_dest, int dest_x, int dest_y, dc_t *dc_sour, int sour_x, int sour_y, int sour_w, int sour_h)
{               
    int         coor_x;
    int         coor_y;
    color_t     color_sour;
    color_t     color_dest;
    int         ncheck;

    if ( 0 > dest_x)
    {
       sour_x -= dest_x;
       sour_w += dest_x;
       dest_x = 0;

       if ( 0 >= sour_w)     return;
    }
    if ( dc_dest->width <= dest_x)      return;
 
    if ( 0 > dest_y)
    {   
        sour_y -= dest_y;
        sour_h += dest_y;
        if ( 0 >= sour_h)     return;

        dest_y = 0;
    }
    if ( dc_dest->height <= dest_y)     return;
 
    if ( 0 > sour_x)   sour_x = 0;
    else if ( dc_sour->width <= sour_x)   return;
 
    if ( 0 > sour_y)   sour_y = 0;
    else if ( dc_sour->height <= sour_y)  return;
 
    ncheck   = dest_x + sour_w;
    if ( dc_dest->width < ncheck)   sour_w = dc_dest->width -dest_x;
 
    ncheck   = sour_x + sour_w;
    if ( dc_sour->width < ncheck)   sour_w = dc_sour->width -sour_x;
 
    ncheck   = dest_y + sour_h;
    if ( dc_dest->height < ncheck)  sour_h = dc_dest->height -dest_y;
 
    ncheck   = sour_y + sour_h;
    if ( dc_sour->height < ncheck)  sour_h = dc_sour->height -sour_y;
 
    if (    ( dc_sour->dc_type        == dc_dest->dc_type        )
        &&  ( dc_sour->bits_per_pixel == dc_dest->bits_per_pixel ) )
    {
    	byte_bitblt( dc_dest, dest_x, dest_y, dc_sour, sour_x, sour_y, sour_w, sour_h, dc_sour->bits_per_pixel / 8);
    }
    else if ( DCTYPE_PNG == dc_sour->dc_type)
    {
		for ( coor_y = 0; coor_y < sour_h; coor_y++)
		{
          for ( coor_x = 0; coor_x < sour_w; coor_x++)
          {
             dc_sour->get_pixel( dc_sour, coor_x+sour_x, coor_y+sour_y, &color_sour);
             if ( 0 < color_sour.alpha)
             {
                if ( 255 > color_sour.alpha)
                {
                   dc_dest->get_pixel( dc_dest, coor_x+dest_x, coor_y+dest_y, &color_dest);
                   ALPHA_COMPOSITE( color_sour.red  , color_sour.red  , color_sour.alpha, color_dest.red  );
                   ALPHA_COMPOSITE( color_sour.green, color_sour.green, color_sour.alpha, color_dest.green);
                   ALPHA_COMPOSITE( color_sour.blue , color_sour.blue , color_sour.alpha, color_dest.blue );
                } // if
                dc_dest->set_pixel( dc_dest, coor_x+dest_x, coor_y+dest_y, color_sour);
             } // if
          } // for
       } // for
    }
    else
    {
    	for ( coor_y = 0; coor_y < sour_h; coor_y++)
		{
			for ( coor_x = 0; coor_x < sour_w; coor_x++)
			{
				dc_sour->get_pixel( dc_sour, coor_x+sour_x, coor_y+sour_y, &color_sour);
				dc_dest->set_pixel( dc_dest, coor_x+dest_x, coor_y+dest_y,  color_sour);
			}
		}
    }
}

void  gx_bitblt90(  dc_t *dc_dest, dc_t *dc_sour)
{           
    int     sour_x, sour_y;
    int     dest_x, dest_y;
    int     width , height;
    color_tt color_sour;
    color_tt color_dest;
              
    width   = dc_sour->width;
    height  = dc_sour->height;

    if ( dc_dest->height < width )  width  = dc_dest->height;
    if ( dc_dest->width  < height)  height = dc_dest->width;
              
    if ( DCTYPE_PNG == dc_sour->dc_type)                                           // ������ PNG �����̸� ���� ������ ó���Ѵ�.
    {   
        dest_x = 0;              
        for ( sour_y = 0; sour_y < height; sour_y++)
        {   
            dest_y = dc_dest->height-1;
            for ( sour_x = 0; sour_x < width; sour_x++)
            {               
                gx_get_pixel_bmp( dc_sour, sour_x, sour_y, &color_sour);
                if ( 0 < color_sour.alpha)
                {
                   if ( 255 > color_sour.alpha)
                   {
                      dc_dest->get_pixel_bmp( dc_dest, dest_x, dest_y, &color_dest);
                      ALPHA_COMPOSITE( color_sour.red  , color_sour.red  , color_sour.alpha, color_dest.red  );
                      ALPHA_COMPOSITE( color_sour.green, color_sour.green, color_sour.alpha, color_dest.green);
                      ALPHA_COMPOSITE( color_sour.blue , color_sour.blue , color_sour.alpha, color_dest.blue );
                   } // if
                    gx_set_pixel_bmp( dc_dest, dest_x, dest_y,  color_sour);
                } // if
                dest_y--;            
            }
            dest_x++;
        }
    }
    else
    {
        dest_x = 0;              
        for ( sour_y = 0; sour_y < height; sour_y++)
        {
            dest_y = dc_dest->height-1;
            for ( sour_x = 0; sour_x < width; sour_x++)
            {            
                gx_get_pixel_bmp( dc_sour, sour_x, sour_y, &color_sour);
                gx_set_pixel_bmp( dc_dest, dest_x, dest_y,  color_sour);
                dest_y--;            
            }
            dest_x++;
        }
    }        
}

void  gx_bitblt270(  dc_t *dc_dest, dc_t *dc_sour)
{
    int     sour_x, sour_y;
    int     dest_x, dest_y;
    int     width , height;
    color_tt color_sour;
    color_tt color_dest;
              
    width   = dc_sour->width;
    height  = dc_sour->height;
    
    if ( dc_dest->height < width )  width  = dc_dest->height;
    if ( dc_dest->width  < height)  height = dc_dest->width;

    if ( DCTYPE_PNG == dc_sour->dc_type)                                           // ������ PNG �����̸� ���� ������ ó���Ѵ�.
    {
        dest_x = dc_dest->width-1;              
        for ( sour_y = 0; sour_y < height; sour_y++)
        {
            dest_y = 0;
            for ( sour_x = 0; sour_x < width; sour_x++)
            {            
                gx_get_pixel_bmp( dc_sour, sour_x, sour_y, &color_sour);
                if ( 0 < color_sour.alpha)
                {
                   if ( 255 > color_sour.alpha)
                   {
                      dc_dest->get_pixel_bmp( dc_dest, dest_x, dest_y, &color_dest);
                      ALPHA_COMPOSITE( color_sour.red  , color_sour.red  , color_sour.alpha, color_dest.red  );
                      ALPHA_COMPOSITE( color_sour.green, color_sour.green, color_sour.alpha, color_dest.green);
                      ALPHA_COMPOSITE( color_sour.blue , color_sour.blue , color_sour.alpha, color_dest.blue );
                   } // if
                    gx_set_pixel_bmp( dc_dest, dest_x, dest_y,  color_sour);
                } // if
                dest_y++;            
            }
            dest_x--;
        }
    }
    else
    {
        dest_x = dc_dest->width-1;              
        for ( sour_y = 0; sour_y < height; sour_y++)
        {
            dest_y = 0;
            for ( sour_x = 0; sour_x < width; sour_x++)
            {            
                gx_get_pixel_bmp( dc_sour, sour_x, sour_y, &color_sour);
                gx_set_pixel_bmp( dc_dest, dest_x, dest_y,  color_sour);
                dest_y++;            
            }
            dest_x--;
        }
    }
}

/*
void  gx_bitblt_mask  ( dc_t *dc_dest, dc_t *dc_sour, dc_t *dc_mask, color_t color, int left, int top, int width, int height)
{
    int     offset_dest;
    int     origin_dest;
    int     ndx_x, ndx_y;
    int     src_width;
    int     b_color;

    src_width   = dc_dest->width;
    origin_dest = left + src_width *top;

    b_color = b16_color( color);

    for ( ndx_y = 0; height > ndx_y; ndx_y++)
    {
        offset_dest = origin_dest;
        for ( ndx_x = 0; width > ndx_x; ndx_x++)
        {
            if ( b_color == *( (unsigned short *)dc_mask->mapped +offset_dest))
            {
                *( (unsigned short *)dc_dest->mapped +offset_dest) = *( (unsigned short *)dc_sour->mapped +offset_dest);
            }
            offset_dest++;
        }
        origin_dest += src_width;
    }

}*/

void  gx_release_dc( dc_t *dc)
{
   dc->release_dc( dc);
}                     

dc_t    *gx_get_compatible_dc( dc_t *dc_sour)
{
   dc_t  *dc;

   dc = malloc( sizeof( dc_t));
   if ( NULL != dc)
   {
      dc->dc_type           = dc_sour->dc_type;
      dc->width             = dc_sour->width;
      dc->height            = dc_sour->height;
      dc->colors            = dc_sour->colors;
      dc->dots              = dc_sour->dots;
      dc->coor_x            = 0;
      dc->coor_y            = 0;
      dc->pen_color         = gx_color( 255, 255, 255, 255);
      dc->brush_color       = gx_color(   0,   0,   0, 255);
      dc->font_color        = gx_color( 255, 255, 255, 255);
      dc->font              = NULL;
      dc->release_dc        = release_buffer_dc;

      dc->bits_per_pixel    = dc_sour->bits_per_pixel;
      dc->bytes_per_line    = dc_sour->bytes_per_line;

      dc->bytes             = dc_sour->bytes;
      dc->mapped            = malloc( dc->bytes);
      set_virtual_func( dc, dc_sour->colors);
   }

   return dc;
}

dc_t *gx_get_buffer_dc( int width, int height)
{
   int   sz_struct;
   dc_t  *dc;        
   sz_struct = sizeof( dc_t);
   dc = malloc( sz_struct);
   if ( NULL != dc)
   {                                                                    
      dc->dc_type        	= DCTYPE_SCREEN;
                                                                        
      if ( 0 > width ) width  = gx_fb.width;
      if ( 0 > height) height = gx_fb.height;                           
                                                                        
      dc->width             = width;
      dc->height            = height;
      dc->colors            = gx_fb.colors;

      dc->dots              = width*height;
      dc->coor_x            = 0;
      dc->coor_y            = 0;                                        
      dc->pen_color         = gx_color( 4095, 4095, 4095, 4095);
      dc->brush_color       = gx_color(   0,   0,   0, 255);
      dc->font_color        = gx_color( 4095, 4095, 4095, 4095);
      dc->font              = NULL;
      dc->release_dc        = release_buffer_dc;

      dc->bits_per_pixel    = gx_fb.bits_per_pixel;
      dc->bytes_per_line  	= width * 4;
      dc->bytes   = dc->bytes_per_line *height;
      dc->mapped  = malloc( dc->bytes);
      set_virtual_func( dc, gx_fb.colors);
   }

   return dc;
}

dc_t *gx_get_frame_dc( frame_buffer_t *ap_fb)
{
    int   sz_struct;
    dc_t  *dc;
                                                                        
    sz_struct = sizeof( dc_t);
    dc = malloc( sz_struct);
    if ( NULL != dc)
    {                         
        dc->dc_type           = DCTYPE_SCREEN;
        dc->width             = ap_fb->width;
        dc->height            = ap_fb->height;
        dc->dots              = ap_fb->dots;
        dc->bytes             = ap_fb->bytes;
        dc->colors            = ap_fb->colors;
        dc->bytes_per_line    = ap_fb->bytes_per_line;
        dc->bits_per_pixel    = ap_fb->bits_per_pixel;
        
        dc->mapped            = ap_fb->mapped;
        dc->coor_x            = 0;
        dc->coor_y            = 0;
        dc->pen_color         = gx_color( 255, 255, 255, 255);
        dc->brush_color       = gx_color(   0,   0,   0, 255);
        dc->font_color        = gx_color( 255, 255, 255, 255);
        dc->font              = NULL;
        dc->release_dc        = release_screen_dc;
        
        set_virtual_func( dc, ap_fb->colors);
    }
    else
    {   
        printf( "gx_get_screen_dc() : out of memory.\n");
    }

    return dc;
}

dc_t *gx_get_screen_dc( void)
{
	return gx_get_frame_dc( &gx_fb);
}

char  *gx_error_string( int error_code)
{                                                                               
   char *error_string[] ={ "no error",                                  // GXERR_NONE
                           "no device",                                 // GXERR_NO_DEVICE            
                           "no privilege to access device",             // GXERR_ACCESS_DEVICE        
                           "no FBIOGET_VSCREENINFO",                    // GXERR_VSCREEN_INFO         
                           "no FBIOGET_FSCREENINFO",                    // GXERR_FSCREEN_INFO         
                           "memory mapping failure",                    // GXERR_MEMORY_MAPPING       
                           "no file",                                   // GXERR_NOFILE               
                           "header info error",                         // GXERR_HEADER_INFO          
                           "read error",                                // GXERR_READ_FILE            
                           "palette info error",                        // GXERR_PALETTE_INFO         
                           "color depth error",                         // GXERR_COLOR_DEPTH          
                           "no font",                                   // GXERR_NO_ASSIGNED_FONT     
                           "signature error",                           // GXERR_SIGNATURE            
                           "out of memory",                             // GXERR_OUT_OF_MEMORY        
                           "processing error",                          // GXERR_PROCESSING           
                           "no canvas handle",                          // GXERR_NO_CANVAS            
                           "filename error"                             // GXERR_FILE_NAME            
                        };
   return( error_string[error_code]);
}

void gx_print_error( int error_code, char *remark)
{
   printf( "[gx error:%d]%s (%s)\n", error_code, gx_error_string( error_code), remark);
}

void gx_printf_error( int error_code, const char *fmt, ... )
{
    va_list ap;

    printf( "[gx error:%d]%s - ", error_code, gx_error_string( error_code));

	va_start(ap, fmt);
	vprintf( fmt, ap);
	va_end(ap);   
	printf( "\n");
}

dc_t *gx_to_fast_dc( dc_t *dc)
{
    dc_t   *dc_fast;
    
    if ( NULL == dc)
    {
        printf( "gx_to_fast_dc() : NULL cannot convert to fast dc.\n");
        return NULL;
    }
    
    dc_fast = gx_get_buffer_dc( dc->width, dc->height);                 // dc ũ�⿡ �ش��ϴ� ���� DC�� ����
    if ( NULL == dc_fast)
    {
        printf( "gx_to_fast_dc() : out of memory.\n");
    }
    else
    {
        gx_bitblt( dc_fast, 0, 0, dc, 0, 0, dc->width, dc->height);         // ���� DC�� ���� ���� ����
        gx_release_dc( dc);                                                 // ���� dc�� �ڿ� ��ȯ
    }    
    return dc_fast;
}

void gx_close( void)
{
   if ( 0 <= gx_fb.mapped)
   {
      munmap( gx_fb.mapped, gx_fb.bytes);
      gx_fb.mapped = MAP_FAILED;
   }
   if ( 0 <= gx_fb.fd)
   {
      close( gx_fb.fd);
      gx_fb.fd = -1;
   }
}

int gx_init(int width, int height, int bit)
{
	gx_fb.width				= width;
	gx_fb.height			= height;
	gx_fb.dots           	= gx_fb.width * gx_fb.height;
	gx_fb.bytes_per_line 	= width*4;
	gx_fb.bytes          	= gx_fb.bytes_per_line *gx_fb.height;
	gx_fb.colors			= bit;
	gx_fb.bits_per_pixel 	= 32;
	return 1;
}

int gx_open( char *dev_name)
{
    struct   fb_var_screeninfo  fbvar;
    struct   fb_fix_screeninfo  fbfix;

    gx_fb.fd          = -1;
    gx_fb.mapped      = MAP_FAILED;

    if ( access( dev_name, F_OK))
    {
        printf( "gx_open() : no device.\n");
        return GXERR_NO_DEVICE;
    }
    if ( 0 >  ( gx_fb.fd = open( dev_name, O_RDWR))   )
    {
        printf( "gx_open() : access device.\n");
        return GXERR_ACCESS_DEVICE;
    }
    if ( ioctl( gx_fb.fd, FBIOGET_VSCREENINFO, &fbvar))
    {
        gx_close();
        printf( "gx_open() : vscreen info.\n");
        return GXERR_VSCREEN_INFO;
    }
    if ( ioctl( gx_fb.fd, FBIOGET_FSCREENINFO, &fbfix))
    {
        gx_close();
        printf( "gx_open() : fscreen info.\n");
        return GXERR_FSCREEN_INFO;
    }
    gx_fb.width          = fbvar.xres;
    gx_fb.height         = fbvar.yres;
    gx_fb.dots           = gx_fb.width * gx_fb.height;
    gx_fb.bits_per_pixel = fbvar.bits_per_pixel;
    gx_fb.bytes_per_line = fbfix.line_length;

    gx_fb.bytes          = gx_fb.bytes_per_line *gx_fb.height;
    gx_fb.mapped         = ( void *)mmap( 0,
                                       gx_fb.bytes,
                                       PROT_READ|PROT_WRITE,
                                       MAP_SHARED,
                                       gx_fb.fd,
                                       0);
    switch( gx_fb.bits_per_pixel)
    {
        case 16  :  gx_fb.colors   =   fbvar.red.length
                                      +fbvar.green.length
                                      +fbvar.blue.length;
        	break;
        default  :  gx_fb.colors   =  gx_fb.bits_per_pixel;
    }
    if ( 0 > gx_fb.mapped)
    {
        gx_close();
        printf( "gx_open() : memory mapping.\n");
        return GXERR_MEMORY_MAPPING;
    }

    switch( gx_fb.colors)
    {
        case 24  :  gx_fb.bits_per_pixel = 32;
        	break;
    }
    return GXERR_NONE;
}

void gx_close_frame( frame_buffer_t *ap_fb)
{
	if ( MAP_FAILED != ap_fb->mapped)
	{
		if ( 0 <= ap_fb->mapped)
		{
			munmap( ap_fb->mapped, ap_fb->bytes);
		}
   }
   if ( 0 <= ap_fb->fd)
   {
      close( ap_fb->fd);
   }
   free( ap_fb);
}

frame_buffer_t *gx_open_frame( char *dev_name)
{
    struct   fb_var_screeninfo  fbvar;
    struct   fb_fix_screeninfo  fbfix;
    
    frame_buffer_t *p_fb;
    
    if ( access( dev_name, F_OK))
    {
        printf( "gx_open_frame( %s ) : no device.\n", dev_name);
        gx_error_code	= GXERR_NO_DEVICE;
        return NULL;
    }

    p_fb	= malloc( sizeof( frame_buffer_t));
    if ( NULL == p_fb)
    {
        gx_error_code	= GXERR_OUT_OF_MEMORY;
        return NULL;
    }
    p_fb->fd          = -1;
    p_fb->mapped      = MAP_FAILED;

    if ( 0 >  ( p_fb->fd = open( dev_name, O_RDWR))   )
    {
        gx_close_frame( p_fb);
        printf( "gx_open_frame( %s ) : access device.\n", dev_name);
        gx_error_code	= GXERR_ACCESS_DEVICE;
        return NULL;
    }
    if ( ioctl( p_fb->fd, FBIOGET_VSCREENINFO, &fbvar))
    {
        gx_close_frame( p_fb);
        printf( "gx_open_frame( %s ) : vscreen info.\n", dev_name);
        gx_error_code	= GXERR_VSCREEN_INFO;
        return NULL;
    }
    if ( ioctl( p_fb->fd, FBIOGET_FSCREENINFO, &fbfix))
    {
        gx_close_frame( p_fb);
        printf( "gx_open_frame( %s ) : fscreen info.\n", dev_name);
        gx_error_code	= GXERR_FSCREEN_INFO;
        return NULL;
    }

    p_fb->width          = fbvar.xres;
    p_fb->height         = fbvar.yres;
    p_fb->dots           = p_fb->width * p_fb->height;
    p_fb->bits_per_pixel = fbvar.bits_per_pixel;
    p_fb->bytes_per_line = fbfix.line_length;
    
    p_fb->bytes          = p_fb->bytes_per_line *p_fb->height;
    p_fb->mapped         = ( void *)mmap( 0,
                                       p_fb->bytes,
                                       PROT_READ|PROT_WRITE,
                                       MAP_SHARED,
                                       p_fb->fd,
                                       0);
    switch( p_fb->bits_per_pixel)
    {
        case 16  :  p_fb->colors   =   fbvar.red.length
                                      +fbvar.green.length
                                      +fbvar.blue.length;
                    break;
        default  :  p_fb->colors   =  p_fb->bits_per_pixel;
    }
    if ( 0 > p_fb->mapped)
    {
        gx_close_frame( p_fb);
        printf( "gx_open_frame() : memory mapping.\n");
        gx_error_code	= GXERR_MEMORY_MAPPING;
        return NULL;
    }
    
    switch( p_fb->colors)
    {
        case 24  :  p_fb->bits_per_pixel = 32;
                    break;
    }
    return p_fb;
}

void gx_pan_display(int n)
{
	struct fb_var_screeninfo fbinfo, fbv;

	memset(&fbv, 0, sizeof(fbv));

	fbv.xoffset = 0;
	fbv.vmode |= FB_VMODE_YWRAP; //FB_VMODE_NONINTERLACED;

	if(n) 	fbv.yoffset = gx_fb.height;
	else	fbv.yoffset = 0;

	fprintf(stderr, "yoffset : %d\n", fbv.yoffset);

	if( ioctl(gx_fb.fd, FBIOPAN_DISPLAY, &fbv) ){
		fprintf(stderr, "FBIOPAN_DISPLAY Error !\n");
	}

	if( ioctl(gx_fb.fd, FBIOGET_VSCREENINFO, &fbinfo) ){
		fprintf(stderr, "FBIOGET_VSCREENINFO Error !\n");
	}
}
