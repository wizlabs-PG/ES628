#include <pattern.h>
#include <nvgstplayer.h>
#include <model_data.h>
#include <group_data.h>
#include <fpga_spi.h>
#include <gxttf.h>
#include <gxfile.h>

pattern_head_t pattern_head;
unsigned short indirect_enable=0;
int even_odd=0;
int mov_run=0;
TOKEN_CONFIG token[STR_SIZE];


static void draw_tool(dc_t *dc, tool_t *t, float px1, float py1, float px2, float py2, float px3, float py3, float step_x, float step_y, float step_r, float step_g, float step_b)
{
	int 		x1, y1, x2, y2, x3, y3;
	int 		half_w, half_h;
	char		text[TEXT_TEMP_SIZE];
	color_t 	fg_c, bg_c, bg1_c;
	color_t		start_c, end_c;

	int			ai_fcnt=1;

	x1 			= (int)(px1+step_x+.5);
	y1 			= (int)(py1+step_y+.5);
	x2 			= (int)(px2+step_x+.5);
	y2 			= (int)(py2+step_y+.5);
	x3 			= (int)(px3+step_x+.5);
	y3 			= (int)(py3+step_y+.5);

	half_w		= (x2-x1)>>1;
	half_h		= (y2-y1)>>1;


#ifdef USE_8K_LOGIC_PATTERN
	if(DRAWCHECK(dc->output_display))
#else
	if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
	{
		if(pattern_head.colormode>0) //indirect
		{
			fg_c.red 		= t->sb;
			fg_c.green 		= t->sb;
			fg_c.blue 		= t->sb;
			//printf("indirect write color fg_c.red=%d fg_c.green=%d fg_c.blue=%d\n",fg_c.red,fg_c.green,fg_c.blue);

			bg_c.red 		= (unsigned short)((unsigned short)t->param[2]);
			bg_c.green	 	= (unsigned short)((unsigned short)t->param[2]);
			bg_c.blue 		= (unsigned short)((unsigned short)t->param[2]);
			bg1_c.red 		= (unsigned short)((unsigned short)t->param[5]);
			bg1_c.green 	= (unsigned short)((unsigned short)t->param[5]);
			bg1_c.blue 		= (unsigned short)((unsigned short)t->param[5]);
			}
		else //direct
		{
			switch(model_data.mode & 0xf)
			{
				case MODE_HEXA: //for 11bit
					if(model_data.if_type == IF_VBY1)
					{
						fg_c.red		= (unsigned short)((unsigned short)(t->sr+step_r));
						fg_c.green 		= (unsigned short)((unsigned short)(t->sg+step_g));
						fg_c.blue 		= (unsigned short)((unsigned short)(t->sb+step_b));


						bg_c.red 		= (unsigned short)((unsigned short)t->param[0]);
						bg_c.green 		= (unsigned short)((unsigned short)t->param[1]);
						bg_c.blue 		= (unsigned short)((unsigned short)t->param[2]);
						bg1_c.red 		= (unsigned short)((unsigned short)t->param[3]);
						bg1_c.green 	= (unsigned short)((unsigned short)t->param[4]);
						bg1_c.blue 		= (unsigned short)((unsigned short)t->param[5]);
//						printf("11bit direct Vx1 hexa\n");
					}
					else
					{
						fg_c.red		= (unsigned short)((unsigned short)(t->sr+step_r)>>2);
						fg_c.green 		= (unsigned short)((unsigned short)(t->sg+step_g)>>2);
						fg_c.blue 		= (unsigned short)((unsigned short)(t->sb+step_b)>>2);


						bg_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
						bg_c.green 		= (unsigned short)((unsigned short)t->param[1]>>2);
						bg_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
						bg1_c.red 		= (unsigned short)((unsigned short)t->param[3]>>2);
						bg1_c.green 	= (unsigned short)((unsigned short)t->param[4]>>2);
						bg1_c.blue 		= (unsigned short)((unsigned short)t->param[5]>>2);
					}
					break;
				default:
					fg_c.red		= (unsigned short)((unsigned short)(t->sr+step_r)>>2);
					fg_c.green 		= (unsigned short)((unsigned short)(t->sg+step_g)>>2);
					fg_c.blue 		= (unsigned short)((unsigned short)(t->sb+step_b)>>2);


					bg_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
					bg_c.green 		= (unsigned short)((unsigned short)t->param[1]>>2);
					bg_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
					bg1_c.red 		= (unsigned short)((unsigned short)t->param[3]>>2);
					bg1_c.green 	= (unsigned short)((unsigned short)t->param[4]>>2);
					bg1_c.blue 		= (unsigned short)((unsigned short)t->param[5]>>2);
//					printf("11bit direct else\n");
			}
			/*
			fg_c.red		= (unsigned short)((unsigned short)(t->sr+step_r)>>2);
			fg_c.green 		= (unsigned short)((unsigned short)(t->sg+step_g)>>2);
			fg_c.blue 		= (unsigned short)((unsigned short)(t->sb+step_b)>>2);


			bg_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
			bg_c.green 		= (unsigned short)((unsigned short)t->param[1]>>2);
			bg_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
			bg1_c.red 		= (unsigned short)((unsigned short)t->param[3]>>2);
			bg1_c.green 	= (unsigned short)((unsigned short)t->param[4]>>2);
			bg1_c.blue 		= (unsigned short)((unsigned short)t->param[5]>>2);
			*/
		}
	}
	else
	{
		if(pattern_head.colormode>0) //indirect
		{
			fg_c.red	= (unsigned short)(pattern_head.color[t->sb][0]+step_r);
			fg_c.green 	= (unsigned short)(pattern_head.color[t->sb][1]+step_g);
			fg_c.blue 	= (unsigned short)(pattern_head.color[t->sb][2]+step_b);
			//printf("(%d,%d,%d,%d)indirect color (%d, %d, %d) t->sb=%d\n", x1,y1,x2,y2 , fg_c.red, fg_c.green, fg_c.blue, t->sb);

			bg_c.red 	= (t->param[2]);
			bg_c.green 	= (t->param[2]);
			bg_c.blue 	= (t->param[2]);
			bg1_c.red 	= (t->param[5]);
			bg1_c.green = (t->param[5]);
			bg1_c.blue 	= (t->param[5]);
		}
		else //direct
		{
			fg_c.red	= (unsigned short)(t->sr+step_r);
			fg_c.green 	= (unsigned short)(t->sg+step_g);
			fg_c.blue 	= (unsigned short)(t->sb+step_b);
			bg_c.red 	= (t->param[0]);
			bg_c.green 	= (t->param[1]);
			bg_c.blue 	= (t->param[2]);
			bg1_c.red	= (t->param[3]);
			bg1_c.green	= (t->param[4]);
			bg1_c.blue	= (t->param[5]);
		}

	}

	pattern_head.ai = 0;			//test
	pattern_head.ai_frame_cnt=1;

	switch(t->id){
	case T_LINE:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_line(dc, x1, y1, x2, y2);
		break;

	case T_BOX:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_rectangle(dc, x1, y1, x2, y2);
		break;

	case T_FBOX:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_rectangle(dc, x1, y1, x2, y2);
		break;

	case T_TRI:	
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_triangle(dc, x1, y1, x2, y2, x3, y3);
		break;

	case T_FTRI:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_triangle(dc, x1, y1, x2, y2, x3, y3);
		break;

	case T_CIRCLE:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_ellipse(dc, x1+half_w, y1+half_h, half_w, half_h);
		break;

	case T_FCIRCLE:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_ellipse(dc, x1+half_w, y1+half_h, half_w, half_h);
		break;

	case T_HOLE:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_hole(dc, x1+half_w, y1+half_h, half_w, half_h);
		break;

	case T_ARC:
		break;

	case T_FARC:
		break;

	case T_HGRAY:
	case T_VGRAY:
	case T_CGRAY:

#ifdef USE_8K_LOGIC_PATTERN
		if(DRAWCHECK(dc->output_display))
#else
		if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
		{
			switch(model_data.mode & 0xf)
			{
				case MODE_HEXA: //for 11bit
					if(model_data.if_type == IF_VBY1)
					{
						start_c.red 	= (unsigned short)((unsigned short)t->sr);
						start_c.green 	= (unsigned short)((unsigned short)t->sg);
						start_c.blue 	= (unsigned short)((unsigned short)t->sb);
						end_c.red 		= (unsigned short)((unsigned short)t->param[0]);
						end_c.green 	= (unsigned short)((unsigned short)t->param[1]);
						end_c.blue 		= (unsigned short)((unsigned short)t->param[2]);
//					printf("11bit direct hexa\n");
					}
					else
					{
						start_c.red 	= (unsigned short)((unsigned short)t->sr>>2);
						start_c.green 	= (unsigned short)((unsigned short)t->sg>>2);
						start_c.blue 	= (unsigned short)((unsigned short)t->sb>>2);
						end_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
						end_c.green 	= (unsigned short)((unsigned short)t->param[1]>>2);
						end_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
					}
					break;
				default:
					start_c.red 	= (unsigned short)((unsigned short)t->sr>>2);
					start_c.green 	= (unsigned short)((unsigned short)t->sg>>2);
					start_c.blue 	= (unsigned short)((unsigned short)t->sb>>2);
					end_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
					end_c.green 	= (unsigned short)((unsigned short)t->param[1]>>2);
					end_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
//					printf("11bit direct else\n");
			}
			/*
			start_c.red 	= (unsigned short)((unsigned short)t->sr>>2);
			start_c.green 	= (unsigned short)((unsigned short)t->sg>>2);
			start_c.blue 	= (unsigned short)((unsigned short)t->sb>>2);
			end_c.red 		= (unsigned short)((unsigned short)t->param[0]>>2);
			end_c.green 	= (unsigned short)((unsigned short)t->param[1]>>2);
			end_c.blue 		= (unsigned short)((unsigned short)t->param[2]>>2);
			*/
		}
		else
		{
			start_c.red 	= t->sr;
			start_c.green 	= t->sg;
			start_c.blue 	= t->sb;
			end_c.red 		= t->param[0];
			end_c.green 	= t->param[1];
			end_c.blue 	= t->param[2];

		}
		gx_gradation(dc, (int)t->id, x1, y1, x2, y2, (int)t->param[3], start_c, end_c);
		break;

	case T_AI:
#ifdef USE_8K_LOGIC_PATTERN
		if(DRAWCHECK(dc->output_display))
#else
		if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
		{
			switch(model_data.mode & 0xf)
			{
				case MODE_HEXA: //for 11bit
					if(model_data.if_type == IF_VBY1)
					{
						FPGA_Write(FPGA_PAL0_R, (unsigned short)(t->sr));
						FPGA_Write(FPGA_PAL0_G, (unsigned short)(t->sg));
						FPGA_Write(FPGA_PAL0_B, (unsigned short)(t->sb));
						FPGA_Write(FPGA_PAL1_R, (unsigned short)(t->param[0]));
						FPGA_Write(FPGA_PAL1_G, (unsigned short)(t->param[1]));
						FPGA_Write(FPGA_PAL1_B, (unsigned short)(t->param[2]));
						FPGA_Write(FPGA_PAL2_R, (unsigned short)(t->param[3]));
						FPGA_Write(FPGA_PAL2_G, (unsigned short)(t->param[4]));
						FPGA_Write(FPGA_PAL2_B, (unsigned short)(t->param[5]));
//						printf("11bit direct hexa\n");
					}
					else
					{
						FPGA_Write(FPGA_PAL0_R, (unsigned short)(t->sr>>2));
						FPGA_Write(FPGA_PAL0_G, (unsigned short)(t->sg>>2));
						FPGA_Write(FPGA_PAL0_B, (unsigned short)(t->sb>>2));
						FPGA_Write(FPGA_PAL1_R, (unsigned short)(t->param[0]>>2));
						FPGA_Write(FPGA_PAL1_G, (unsigned short)(t->param[1]>>2));
						FPGA_Write(FPGA_PAL1_B, (unsigned short)(t->param[2]>>2));
						FPGA_Write(FPGA_PAL2_R, (unsigned short)(t->param[3]>>2));
						FPGA_Write(FPGA_PAL2_G, (unsigned short)(t->param[4]>>2));
						FPGA_Write(FPGA_PAL2_B, (unsigned short)(t->param[5]>>2));
					}
					break;
				default:
					FPGA_Write(FPGA_PAL0_R, (unsigned short)(t->sr>>2));
					FPGA_Write(FPGA_PAL0_G, (unsigned short)(t->sg>>2));
					FPGA_Write(FPGA_PAL0_B, (unsigned short)(t->sb>>2));
					FPGA_Write(FPGA_PAL1_R, (unsigned short)(t->param[0]>>2));
					FPGA_Write(FPGA_PAL1_G, (unsigned short)(t->param[1]>>2));
					FPGA_Write(FPGA_PAL1_B, (unsigned short)(t->param[2]>>2));
					FPGA_Write(FPGA_PAL2_R, (unsigned short)(t->param[3]>>2));
					FPGA_Write(FPGA_PAL2_G, (unsigned short)(t->param[4]>>2));
					FPGA_Write(FPGA_PAL2_B, (unsigned short)(t->param[5]>>2));
//					printf("11bit direct else\n");
			}
			/*
			FPGA_Write(FPGA_PAL0_R, (unsigned short)(t->sr>>2));
			FPGA_Write(FPGA_PAL0_G, (unsigned short)(t->sg>>2));
			FPGA_Write(FPGA_PAL0_B, (unsigned short)(t->sb>>2));
			FPGA_Write(FPGA_PAL1_R, (unsigned short)(t->param[0]>>2));
			FPGA_Write(FPGA_PAL1_G, (unsigned short)(t->param[1]>>2));
			FPGA_Write(FPGA_PAL1_B, (unsigned short)(t->param[2]>>2));
			FPGA_Write(FPGA_PAL2_R, (unsigned short)(t->param[3]>>2));
			FPGA_Write(FPGA_PAL2_G, (unsigned short)(t->param[4]>>2));
			FPGA_Write(FPGA_PAL2_B, (unsigned short)(t->param[5]>>2));
			*/
			gx_ai_fpga(dc, t->param[6],t->param[7]);

		}
		else
		{
			gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
			gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
			gx_rectangle(dc, x1, y1, x2, y2);
			gx_ai(dc,dc_ai1_buffer,dc_ai2_buffer,&fg_c, &bg_c, &bg1_c, t->param[6],t->param[7]);
		}

		ai_fcnt = ((int)t->param[7])/(1000/gp.module_hz);
		if(ai_fcnt<1) ai_fcnt=1;

		pattern_head.ai = 1;
		pattern_head.ai_frame_cnt = (unsigned short)ai_fcnt;

		printf("ai frame count= %d\n",pattern_head.ai_frame_cnt);


		break;

	case T_CHESS:
		fg_c.alpha	= 255;
		bg_c.alpha	= 255;

#ifdef USE_8K_LOGIC_PATTERN
		if(DRAWCHECK(dc->output_display))
#else
		if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
		{
//			bg_c.red 	= (unsigned short)((unsigned short)t->param[0]>>2);
//			bg_c.green 	= (unsigned short)((unsigned short)t->param[1]>>2);
//			bg_c.blue 	= (unsigned short)((unsigned short)t->param[2]>>2);
			gx_chess(dc, x1, y1, x2, y2, (int)t->param[3], (int)t->param[4], fg_c, bg_c);
		}
		else
		{
			gx_chess(dc, x1, y1, x2, y2, (int)t->param[3], (int)t->param[4], fg_c, bg_c);
		}

		break;

	case T_HATCH:
		gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
		gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
		gx_hatch(dc, x1, y1, x2, y2, (int)t->param[0], (int)t->param[3], (int)t->param[4]);
		break;

	case T_FLICKER:
		{
			
			int 			 i, size;
			color_us 		*color_p;
			unsigned short 	*line_p;

			size = sizeof(color_us);
			color_p = (color_us*)malloc(size*8);
			for(i=0; i<4; i++) memcpy(color_p+i, t->px3+(i*size), size);
			for(i=4; i<8; i++) memcpy(color_p+i, t->py3+(i*size), size);

			line_p = (unsigned short*)malloc(sizeof(unsigned short)*4);
			for(i=0; i<4; i++) line_p[i] = t->param[i];

			gx_flicker(dc, x1, y1, x2, y2, color_p, line_p);

			free(color_p);
			free(line_p);

			color_p = NULL;
			line_p = NULL;
			
		}
		break;

	case T_TEXT:
			memset(text, 0, TEXT_TEMP_SIZE);
			sprintf(text, "%s", (char*)t->px2);
			gx_pen_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 0));
			gx_brush_color(dc, gx_color(fg_c.red, fg_c.green, fg_c.blue, 255));
			dc->font_color=gx_color(fg_c.red, fg_c.green, fg_c.blue, 255);
			string_out(dc, x1, y1, text,t->param[1]);
		break;

	case T_BMP:
		{
			/*
			bmp_t	*bmp;
			char	filename[TEXT_TEMP_SIZE];

			memset(filename, 0, TEXT_TEMP_SIZE);
			sprintf(filename, "%s", (char*)t->px2);
			bmp = gx_bmp_open(filename);
			if( NULL == bmp ){
				fprintf(stderr, "%s not found! \n", filename);
			}
			else{
				gx_bitblt(dc, x1, y1, (dc_t*)bmp, 0, 0, gx_fb.width-x1, gx_fb.height-y1);
				gx_bmp_close(bmp);
			}
			*/
		}
		break;		
	}
}

static void parsing(int start, int end)
{
	int i,j,k;

	k = end;

	for(i=start; i<k; i++) {
		while((token[i].type == PAR_MULT) || (token[i].type == PAR_DIV)) {
			if(token[i].type == PAR_MULT) token[i-1].val = token[i-1].val * token[i+1].val;
			else token[i-1].val = token[i-1].val / token[i+1].val;
			for(j=i; j<k-1; j++) {
				token[j].type = token[j+2].type;
				token[j].val = token[j+2].val;
			}
			token[k-1].type = PAR_NULL;
			token[k].type = PAR_NULL;
			k -= 2;
		}
	}

	for(i=start; i<k; i++) {
		while((token[i].type == PAR_PLUS) || (token[i].type == PAR_MINUS)) {
			if(token[i].type == PAR_PLUS) token[i-1].val = token[i-1].val + token[i+1].val;
			else token[i-1].val = token[i-1].val - token[i+1].val;
			for(j=i; j<k-1; j++) {
				token[j].type = token[j+2].type;
				token[j].val = token[j+2].val;
			}
			token[k-1].type = PAR_NULL;
			token[k].type = PAR_NULL;
			k -= 2;
		}
	}
}

static float get_point(char *string)
{
	int i,j,k,jari,sl,tokcnt=0,val;
	char paren,str[STR_SIZE],numcnt=0;
	float result;	// jschoi

	memset(str,0,STR_SIZE);
	strcpy(str,string);
	for(i=0; i<STR_SIZE; i++) {
		token[i].type = 0;
		token[i].val = 0;
	}
	sl = strlen(str);
	if(sl == 0) return(0);

	for(i=0; i<sl; i++) {
		if((str[i] >= '0') && (str[i] <= '9')) {
			numcnt++;
			if(i == sl-1) {
				jari = 1;
				val = 0;
				for(j=0; j<numcnt; j++) {
					val += (str[i-j] - 0x30) * jari;
					jari *= 10;
				}
				token[tokcnt].type = PAR_VALUE;
				token[tokcnt++].val = val;
				numcnt = 0;
			}
		}
		else {
			if(numcnt) {
				jari = 1;
				val = 0;
				for(j=1; j<numcnt+1; j++) {
					val += (str[i-j] - 0x30) * jari;
					jari *= 10;
				}
				token[tokcnt].type = PAR_VALUE;
				token[tokcnt++].val = val;
				numcnt = 0;
			}

			if     (str[i] == 'H') { token[tokcnt].type = PAR_VALUE; token[tokcnt++].val = model_data.h_active; }
			else if(str[i] == 'V') { token[tokcnt].type = PAR_VALUE; token[tokcnt++].val = model_data.v_active; }
			else if(str[i] == '(') token[tokcnt++].type = PAR_START;
			else if(str[i] == ')') token[tokcnt++].type = PAR_END;
			else if(str[i] == '*') token[tokcnt++].type = PAR_MULT;
			else if(str[i] == '/') token[tokcnt++].type = PAR_DIV;
			else if(str[i] == '+') token[tokcnt++].type = PAR_PLUS;
			else if(str[i] == '-') token[tokcnt++].type = PAR_MINUS;
			else if(str[i] == ' ') {}
			else return(0);
		}
	}

	while(1) {
		paren = 0;
		for(i=0; i<tokcnt; i++) {
			if(token[i].type == PAR_START) {
				paren = 1;
				for(j=i+1; j<tokcnt; j++) {
					if(token[j].type == PAR_END) {
						parsing(i+1,j-1);
						token[i].type = token[i+1].type;
						token[i].val = token[i+1].val;
						tokcnt -= (j-i);
						for(k=i+1; k<tokcnt; k++) {
							token[k].type = token[k+(j-i)].type;
							token[k].val = token[k+(j-i)].val;
						}
						break;
					}
					else if(token[j].type == PAR_START) break;
				}
			}
		}
		if(!paren) break;
	}
	parsing(0,tokcnt-1);
	result = token[0].val;

	return(result);
}

void en_draw_pattern(dc_t *dc, tool_t *t)
{
	float 	step_x, step_y;			// X,Y loop step
	float 	step_r, step_g, step_b;	// color step
	float	width, height;
	float 	cnt_x, cnt_y;
	float 	size_x, size_y;
	float 	px1, py1, px2, py2, px3, py3;
	float 	min_x, min_y, max_x, max_y;
	int		loop_dir, i, j;

	if( NULL == t ) return;
	if( 0 == tool_cnt ) return;

	cnt_x 	= get_point((char*)t->loop_xcnt);
	cnt_y 	= get_point((char*)t->loop_ycnt);
	size_x 	= get_point((char*)t->loop_xsize);
	size_y 	= get_point((char*)t->loop_ysize);


	px1 	= get_point((char*)t->px1);
	py1 	= get_point((char*)t->py1);
	px2 	= get_point((char*)t->px2);
	py2 	= get_point((char*)t->py2);
	px3 	= get_point((char*)t->px3);
	py3 	= get_point((char*)t->py3);

	if( t->id==T_TRI || t->id==T_FTRI ){
		min_x = min(min(px1, px2), px3);
		min_y = min(min(py1, py2), py3);
		max_x = max(max(px1, px2), px3);
		max_y = max(max(py1, py2), py3);
	}
	else{
		min_x = min(px1, px2);
		min_y = min(py1, py2);
		max_x = max(px1, px2);
		max_y = max(py1, py2);
	}

	width 	= max_x - min_x;
	height 	= max_y - min_y;

	loop_dir = NO_LOOP;
	if( t->loop_opt == SINGLE_LOOP ){
		if( size_x > 0.0 ){
			if( t->loop_range == AUTO_RANGE ){
				if( size_x-px1 > width ) 	loop_dir = X_LOOP;
			}
			else							loop_dir = X_LOOP;			
		}
		if( size_y > 0.0 ){
			if( t->loop_range == AUTO_RANGE ){
				if( size_y-py1 > height ) 	loop_dir = Y_LOOP;
				else						loop_dir = XY_LOOP;
			}
			else{
				if( loop_dir == NO_LOOP )	loop_dir = Y_LOOP;
				else						loop_dir = XY_LOOP;
			}
		}
		if( loop_dir==NO_LOOP || cnt_x<=1.0 ){
			draw_tool(dc, t, px1, py1, px2, py2, px3, py3, 0.0, 0.0, 0.0, 0.0, 0.0);
			return;
		}
	}
	else if( t->loop_opt == DUAL_LOOP ){
		float xcnt = 1.0;
		float ycnt = 1.0;

		if( size_x > 0.0 ){
			if( t->loop_range == AUTO_RANGE ){
				if( size_x-px1 > width )	xcnt = cnt_x;
			}
			else							xcnt = cnt_x;
		}

		if( size_y > 0.0 ){
			if( t->loop_range == AUTO_RANGE ){
				if( size_y-py1 > height )	ycnt = cnt_y;
			}
			else							ycnt = cnt_y;
		}

		if( xcnt<=1 && ycnt<=1 ){
			draw_tool(dc, t, px1, py1, px2, py2, px3, py3, 0.0, 0.0, 0.0, 0.0, 0.0);
			return;
		}

		cnt_x = xcnt;
		cnt_y = ycnt;
	}

	if( t->loop_opt == SINGLE_LOOP ){
		if( t->loop_range == AUTO_RANGE ){
			if( loop_dir == X_LOOP ){
				step_x = (size_x-min_x-width) / (cnt_x-1);
				step_y = 0.0;
			}
			else if( loop_dir == Y_LOOP ){
				step_x = 0.0;
				step_y = (size_y-min_y-height) / (cnt_x-1);
			}
			else{
				step_x = (size_x-min_x-width) / (cnt_x-1);
				step_y = (size_y-min_y-height) / (cnt_x-1);
			}

			if( t->loop_color == CHANGE_COLOR ){
				for(i=0; i<(int)cnt_x; i++){
					step_r = (float)(t->er-t->sr) / cnt_x * i;
					step_g = (float)(t->eg-t->sg) / cnt_x * i;
					step_b = (float)(t->eb-t->sb) / cnt_x * i;

					draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x*i, step_y*i, step_r, step_g, step_b);
				}
			}
			else{	// NOCHANGE_COLOR
				for(i=0; i<(int)cnt_x; i++){
					draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x*i, step_y*i, 0.0, 0.0, 0.0);
				}
			}
		}
		else{	// MANU_RANGE
			if( t->loop_color == CHANGE_COLOR ){
				for(i=0; i<(int)cnt_x; i++){
					step_x = size_x * i;
					step_y = size_y * i;
					step_r = (float)(t->er-t->sr) / cnt_x * i;
					step_g = (float)(t->eg-t->sg) / cnt_x * i;
					step_b = (float)(t->eb-t->sb) / cnt_x * i;

					draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x, step_y, step_r, step_g, step_b);
				}
			}
			else{	// NOCHANGE_COLOR
				for(i=0; i<(int)cnt_x; i++){
					step_x = size_x * i;
					step_y = size_y * i;

					draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x, step_y, 0.0, 0.0, 0.0);
				}
			}
		}
	}
	else if( t->loop_opt == DUAL_LOOP ){
		if( t->loop_range == AUTO_RANGE ){
			if( loop_dir == X_LOOP ){
				step_x = (size_x-min_x-width) / (cnt_x-1);
				step_y = 0.0;
			}
			else if( loop_dir == Y_LOOP ){
				step_x = 0.0;
				step_y = (size_y-min_y-height) / (cnt_y-1);
			}
			else{
				step_x = (size_x-min_x-width) / (cnt_x-1);
				step_y = (size_y-min_y-height) / (cnt_y-1);
			}

			if( t->loop_color == CHANGE_COLOR ){
				for(i=0; i<(int)cnt_x; i++){
					for(j=0; j<(int)cnt_y; j++){
						step_r = (float)(t->er-t->sr) / (cnt_x+cnt_y) * (i+j);
						step_g = (float)(t->eg-t->sg) / (cnt_x+cnt_y) * (i+j);
						step_b = (float)(t->eb-t->sb) / (cnt_x+cnt_y) * (i+j);

						draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x*i, step_y*j, step_r, step_g, step_b);
					}
				}
			}
			else{	// NOCHANGE_COLOR
				for(i=0; i<(int)cnt_x; i++){
					for(j=0; j<(int)cnt_y; j++){
						draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x*i, step_y*j, 0.0, 0.0, 0.0);
					}
				}
			}
		}
		else{	// MANU_RANGE
			if( t->loop_color == CHANGE_COLOR ){
				for(i=0; i<(int)cnt_x; i++){
					for(j=0; j<(int)cnt_y; j++){
						step_x = size_x * i;
						step_y = size_y * j;
						step_r = (float)(t->er-t->sr) / (cnt_x+cnt_y) * (i+j);
						step_g = (float)(t->eg-t->sg) / (cnt_x+cnt_y) * (i+j);
						step_b = (float)(t->eb-t->sb) / (cnt_x+cnt_y) * (i+j);

						draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x, step_y, step_r, step_g, step_b);
					}
				}
			}
			else{	// NOCHANGE_COLOR
				for(i=0; i<(int)cnt_x; i++){
					for(j=0; j<(int)cnt_y; j++){
						step_x = size_x * i;
						step_y = size_y * j;

						draw_tool(dc, t, px1, py1, px2, py2, px3, py3, step_x, step_y, 0.0, 0.0, 0.0);
					}
				}
			}
		}
	}
	else{	// NO_LOOP
		draw_tool(dc, t, px1, py1, px2, py2, px3, py3, 0.0, 0.0, 0.0, 0.0, 0.0);
	}
}

static int read_tool_cnt(FILE *fp)
{
	unsigned short 	tc = 0;
	int				size = 0;
	int		 ret		= 0;

	if(NULL == fp) return 0;

	size = sizeof(unsigned short);

//	fseek(fp, size*10, SEEK_SET);	// �� ���� ������ġ�� ã�´�

//	ret=fread(&pattern_head,sizeof(pattern_head),1, fp);
	ret=fread(&pattern_head,sizeof(pattern_head)-4,1, fp);

	if(pattern_head.colormode>0) //indirect
	{
		switch(model_data.mode & 0xf)
		{
			case MODE_HEXA: //for 11bit
				if(model_data.if_type == IF_VBY1)
				{
					FPGA_Write(FPGA_PAL0_R, pattern_head.color[0][0]);
					FPGA_Write(FPGA_PAL0_G, pattern_head.color[0][1]);
					FPGA_Write(FPGA_PAL0_B, pattern_head.color[0][2]);
					FPGA_Write(FPGA_PAL1_R, pattern_head.color[1][0]);
					FPGA_Write(FPGA_PAL1_G, pattern_head.color[1][1]);
					FPGA_Write(FPGA_PAL1_B, pattern_head.color[1][2]);
					FPGA_Write(FPGA_PAL2_R, pattern_head.color[2][0]);
					FPGA_Write(FPGA_PAL2_G, pattern_head.color[2][1]);
					FPGA_Write(FPGA_PAL2_B, pattern_head.color[2][2]);
//						printf("11bit direct hexa\n");
				}
				else
				{
					FPGA_Write(FPGA_PAL0_R, pattern_head.color[0][0]>>2);
					FPGA_Write(FPGA_PAL0_G, pattern_head.color[0][1]>>2);
					FPGA_Write(FPGA_PAL0_B, pattern_head.color[0][2]>>2);
					FPGA_Write(FPGA_PAL1_R, pattern_head.color[1][0]>>2);
					FPGA_Write(FPGA_PAL1_G, pattern_head.color[1][1]>>2);
					FPGA_Write(FPGA_PAL1_B, pattern_head.color[1][2]>>2);
					FPGA_Write(FPGA_PAL2_R, pattern_head.color[2][0]>>2);
					FPGA_Write(FPGA_PAL2_G, pattern_head.color[2][1]>>2);
					FPGA_Write(FPGA_PAL2_B, pattern_head.color[2][2]>>2);
				}
				break;
			default:
				FPGA_Write(FPGA_PAL0_R, pattern_head.color[0][0]>>2);
				FPGA_Write(FPGA_PAL0_G, pattern_head.color[0][1]>>2);
				FPGA_Write(FPGA_PAL0_B, pattern_head.color[0][2]>>2);
				FPGA_Write(FPGA_PAL1_R, pattern_head.color[1][0]>>2);
				FPGA_Write(FPGA_PAL1_G, pattern_head.color[1][1]>>2);
				FPGA_Write(FPGA_PAL1_B, pattern_head.color[1][2]>>2);
				FPGA_Write(FPGA_PAL2_R, pattern_head.color[2][0]>>2);
				FPGA_Write(FPGA_PAL2_G, pattern_head.color[2][1]>>2);
				FPGA_Write(FPGA_PAL2_B, pattern_head.color[2][2]>>2);
//				printf("11bit direct else\n");
		}
		/*
		FPGA_Write(FPGA_PAL0_R, pattern_head.color[0][0]>>2);
		FPGA_Write(FPGA_PAL0_G, pattern_head.color[0][1]>>2);
		FPGA_Write(FPGA_PAL0_B, pattern_head.color[0][2]>>2);
		FPGA_Write(FPGA_PAL1_R, pattern_head.color[1][0]>>2);
		FPGA_Write(FPGA_PAL1_G, pattern_head.color[1][1]>>2);
		FPGA_Write(FPGA_PAL1_B, pattern_head.color[1][2]>>2);
		FPGA_Write(FPGA_PAL2_R, pattern_head.color[2][0]>>2);
		FPGA_Write(FPGA_PAL2_G, pattern_head.color[2][1]>>2);
		FPGA_Write(FPGA_PAL2_B, pattern_head.color[2][2]>>2);
		*/
		//printf("INDIRECT R0:%d G0:%d B0:%d\n",pattern_head.color[0][0],pattern_head.color[0][1],pattern_head.color[0][2]);
		//printf("INDIRECT R1:%d G1:%d B1:%d\n",pattern_head.color[1][0],pattern_head.color[1][1],pattern_head.color[1][2]);
		//printf("INDIRECT R2:%d G2:%d B2:%d\n",pattern_head.color[2][0],pattern_head.color[2][1],pattern_head.color[2][2]);
	}

	ret=fread(&tc, size, 1, fp);
	if(ret<0) printf("pat open fail\n");

	return (int)tc;
}
/*
static void set_color_mode(char *path)
{
	FILE 			*fp 	= NULL;

	fp = fopen(path, "rb");
	if( NULL == fp ){
		fprintf(stderr, "set_color_mode() : cannot open %s ! \n", path);
		return;
	}

	if(0==fread(&pattern_head,sizeof(pattern_head),1, fp)){}

	if(pattern_head.colormode>0) //indirect
	{
		FPGA_Write(FPGA_PAL0_R, pattern_head.color[0][0]>>2);
		FPGA_Write(FPGA_PAL0_G, pattern_head.color[0][1]>>2);
		FPGA_Write(FPGA_PAL0_B, pattern_head.color[0][2]>>2);
		FPGA_Write(FPGA_PAL1_R, pattern_head.color[1][0]>>2);
		FPGA_Write(FPGA_PAL1_G, pattern_head.color[1][1]>>2);
		FPGA_Write(FPGA_PAL1_B, pattern_head.color[1][2]>>2);
		FPGA_Write(FPGA_PAL2_R, pattern_head.color[2][0]>>2);
		FPGA_Write(FPGA_PAL2_G, pattern_head.color[2][1]>>2);
		FPGA_Write(FPGA_PAL2_B, pattern_head.color[2][2]>>2);
		//printf("INDIRECT R0:%d G0:%d B0:%d\n",pattern_head.color[0][0],pattern_head.color[0][1],pattern_head.color[0][2]);
		//printf("INDIRECT R1:%d G1:%d B1:%d\n",pattern_head.color[1][0],pattern_head.color[1][1],pattern_head.color[1][2]);
		//printf("INDIRECT R2:%d G2:%d B2:%d\n",pattern_head.color[2][0],pattern_head.color[2][1],pattern_head.color[2][2]);
		indirect_enable=1;
	}
	else
	{
		indirect_enable=0;
	}

	fclose(fp);
}
*/
uint32_t get_gray_scale_value(int idx)
{
	uint32_t	ret = 0;
	int			i;
	tool_t		*tools;
	char 		path[MAX_PATH];

	printf("get_gray_scale_value()\n");

	if(group_data.pat[idx].type != TYPE_USER)
	{
		fprintf(stderr, "Invalid pattern type! \n");
		return 0;
	}

	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s%s", DIR_ROOT, DIR_PATTERN, group_data.pat[idx].name, EXT_PATTERN);

	tools = en_pat_open(path);
	if( NULL == tools )
	{
		return 0;
	}

	for(i=0; i<tool_cnt; i++)
	{
		if( (tools[i].id==T_HGRAY) || (tools[i].id==T_VGRAY) || (tools[i].id==T_CGRAY) )
		{
			ret = tools[i].param[3];
			break;
		}
	}

	en_pat_close(tools);

	return ret;
}

tool_t *en_pat_open(char *path)
{
	FILE 	*fp 		= NULL;
	tool_t	*tools		= NULL;
	int		 tool_size	= 0;

	fp = fopen(path, "rb");	// .pat
	if( NULL == fp ){
		int file_len = strlen(path);
		if(file_len>4){
			memcpy(&path[file_len-4], EXT_BIG_PATTERN, 4);
			fp = fopen( path, "rb");	// .PAT
			if( NULL == fp ){
				fprintf(stderr, "en_pat_open() : cannot open %s ! \n", path);
				return NULL;
			}
		}
	}

	tool_cnt = read_tool_cnt(fp);
	if( 0 == tool_cnt ){ 
		fprintf(stderr, "en_pat_open() : tool_cnt => 0 \n");
		return NULL;
	}

	tool_size = sizeof(tool_t);
	tools = (tool_t*)malloc(tool_size*tool_cnt);
	if( NULL == tools ){
		fprintf(stderr, "en_pat_open() : Out of memory! \n");
		return NULL;
	}

	// read data
	if(0==fread(tools, tool_size, tool_cnt, fp)){}

	fclose(fp);
	return tools;
}

void en_pat_close(tool_t *t)
{
	if( NULL == t ) return;

	free(t);
	t = NULL;
}

void indirect_change(dc_t *dc)
{
	int i=0,j=0;
	color_t c;
	color_t c_dset;
	c_dset.alpha=255;
	for(i=0;i<dc->width;i++)
	{
		for(j=0;j<dc->height;j++)
		{
			dc->get_pixel(dc,i,j,&c);
			c_dset.red	= pattern_head.color[c.red][0];
			c_dset.green= pattern_head.color[c.green][1];
			c_dset.blue	= pattern_head.color[c.blue][2];
			dc->set_pixel(dc,i,j,c_dset);
		}
	}
}

int pattern_load(dc_t *dc, char *path, pattern_head_t *indirect)
{
	tool_t		*tools;
	int		 	i, ret = ACK;
	char     	file_ext[4];

	memcpy( file_ext, path+strlen(path)-3, 3);
	file_ext[3] = '\0';
	gp.gx_ai_enable = 0;

	if ( (0==strcasecmp(file_ext, "pat")) || (0==strcasecmp(file_ext, "Pat")) ||
		(0==strcasecmp(file_ext, "pAt")) || (0==strcasecmp(file_ext, "paT")) ||
		(0==strcasecmp(file_ext, "PAt")) || (0==strcasecmp(file_ext, "pAT")) ||
		(0==strcasecmp(file_ext, "PaT")) || (0==strcasecmp(file_ext, "PAT")) )
	{
#ifdef USE_8K_LOGIC_PATTERN
		FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag
#else
		if((get_quhd_enable()==0))
		{
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag
		}
		else if( is_5k_lvds_quad() )
		{
			FPGA_OR_SET(FPGA_MEM_WR_CTRL, 0x0002);		// fill box write flag
		}
		else
		{

		}
#endif

		tools = en_pat_open(path);
		if( NULL == tools ){
			gx_clear(dc, gx_color(0, 0, 0, 255));	// clear dc
			dc->font_color = gx_color(1023, 1023, 1023, 255);
			string_out(dc, (dc->width/5)*2, (dc->height/3), path, 24);
			string_out(dc, (dc->width/5)*2, (dc->height/3)*2, "FILE NOT FOUND!", 24);
			return 2;
		}
		gx_clear(dc, gx_color(0,0,0,255));	// clear dc
		#pragma omp parallel for ordered
		for(i=0; i<tool_cnt; i++){
			#pragma omp ordered
			en_draw_pattern(dc, &tools[i]);
		}
		en_pat_close(tools);

#ifdef USE_8K_LOGIC_PATTERN
		if(DRAWCHECK(dc->output_display))
#else
		if(DRAWCHECK(dc->output_display) && (get_quhd_enable()==0))
#endif
		{
			FPGA_SendEnd();
			transfer();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);	// fill box write flag
			ret = fpga_draw_check(DRAW_PATTERN_CHECK);
			FPGA_Write(FPGA_MEM_RD_CTRL, pattern_head.colormode);	// indirect_enable
		}
		//=[

#ifndef USE_8K_LOGIC_PATTERN
		else if(DRAWCHECK(dc->output_display) && is_5k_lvds_quad() )
		{
			FPGA_SendEnd();
			transfer();
			FPGA_AND_SET(FPGA_MEM_WR_CTRL, 0x0002);	// fill box write flag
			ret = fpga_draw_check(DRAW_PATTERN_CHECK);
			FPGA_Write(FPGA_MEM_RD_CTRL, pattern_head.colormode);	// indirect_enable
		}
#endif
		//=]
		else
		{
			/*
			if(pattern_head.colormode>0) // indirect
				indirect_change(dc);
			*/
		}
		memcpy(indirect, &pattern_head, sizeof(pattern_head));
	}
	else
	{
		if(GXERR_NONE != gx_open_file(dc, path))
		{
			return 3;
		}
	}

	return ret;
}

int en_round(float x)
{
	return ((x>0) ? (int)floor(x+.5) : (int)ceil(x-.5));
}

