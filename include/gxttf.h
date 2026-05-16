#ifndef font_HEADER_H
#define font_HEADER_H


#include <stdio.h>
#include <stdlib.h>                   // malloc srand
#include <string.h>                   // abs
#include <unistd.h>                   // open/close
#include <fcntl.h>                    // O_RDWR

#include <gx.h>
#include <math.h>
//sudo apt-get install python-dev libfreetype6-dev
//#include <ft2build.h>
#include <freetype2/ft2build.h>
#include FT_GLYPH_H

#define CONVERT_FAIL		0
#define CONVERT_ONE		1
#define CONVERT_TWO		2
#define CONVERT_THREE		3
#define UTF_7			0
#define UTF_8			1

FT_Library  library;   /* handle to library     */
FT_Face     face;      /* handle to face object */



int utf2uni(unsigned char *utf, unsigned char *uni, int i);
unsigned short make_syllable(unsigned char *p, int flag);
unsigned char * init_utf(unsigned char *q, int i);
int confirm_unicode(unsigned char *p);
int make_utf(unsigned char *q, unsigned short k, int flag);
int uni2utf(unsigned char *unicode, unsigned char *utf, int i, int flag);
void font_init(void);
void font_dispos(void);
void string_out(dc_t *dc,int x,int y,char *text,int size);
void font_init(void);
void font_dispos(void);
unsigned char *init_uni(unsigned char *q, int i);
unsigned char * confirm_utf(unsigned char *p, int *i);
void draw_bitmap( dc_t *dc, FT_Bitmap *bitmap, FT_Int x, FT_Int y);

#endif
