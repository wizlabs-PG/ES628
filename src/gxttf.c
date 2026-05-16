

#include "gxttf.h"

unsigned char * init_utf(unsigned char *q, int i)
{
	memset(q, 0x00, i*2);

	*q = 0xEF;
	*(q + 1) = 0xBB;
	*(q + 2) = 0xBF;
	q += 3;
	return q;
}


int confirm_unicode(unsigned char *p)
{
	if( (*p == 0xFF) && (*(p+1) == 0xFE) )
		return LITTLE_ENDIAN;
	else 
		return BIG_ENDIAN;
}


int make_utf(unsigned char *q, unsigned short k, int flag)
{
	if( (flag == UTF_7) && (k == 0x0000) ) {
		*q = *q | 0xC0;
		*q = *q | (unsigned char)((k & 0x07C0) >> 6);
		
		*(q+1) = *(q+1) | 0x80;
		*(q+1) = *(q+1) | (unsigned char)(k & 0x003F);
		
		return CONVERT_TWO;
	}
	
	else if( (flag == UTF_8) && (k == 0x0000) ) {
		*q = (unsigned char)k;
		return CONVERT_ONE;
	}
	
	else if( (k > 0x0000) && (k <= 0x007F) ) {
		*q = (unsigned char)k;
		return CONVERT_ONE;
	}
	
	else if( (k >= 0x0080) && (k <= 0x07FF) ) {
		*q = *q | 0xC0;
		*q = *q | (unsigned char)((k & 0x07C0) >> 6);
		
		*(q+1) = *(q+1) | 0x80;
		*(q+1) = *(q+1) | (unsigned char)(k & 0x003F);
		
		return CONVERT_TWO;
	}
	else if( (k >= 0x0800) && (k < 0xFFFF) ) {
		*q = *q | 0xE0;
		*q = *q | (unsigned char)((k & 0xF000) >> 12);
				
		*(q+1) = *(q+1) | 0x80;
		*(q+1) = *(q+1) | (unsigned char)((k & 0x0FC0) >> 6);
				
		*(q+2) = *(q+2) | 0x80;
		*(q+2) = *(q+2) | (unsigned char)(k & 0x003F);
		
		return CONVERT_THREE;
	}
	
	else {
	}

	return CONVERT_FAIL;
}


int uni2utf(unsigned char *unicode, unsigned char *utf, int i, int flag)
{
	int t, j;

	unsigned char *p, *q;
	unsigned short k;
	
	p = unicode;
	q = utf;

	q = init_utf(q, i);
	
	for(t = 2; t < i; t += 2) {
		k = make_syllable((p+t), LITTLE_ENDIAN);
		j = make_utf(q, k, flag);
		q += j;
	}
	/*	
	// a -> 61 00
	if( confirm_unicode(p) == LITTLE_ENDIAN) {
		for(t = 2; t < i; t += 2) {
			k = make_syllable((p+t), LITTLE_ENDIAN);
			j = make_utf(q, k, flag);
			q += j;
		}
	}
	// a -> 00 61
	else {
		for(t = 2; t < i; t += 2) {
			k = make_syllable((p+t), BIG_ENDIAN);
			j = make_utf(q, k, flag);
			q += j;
		}
	}
	*/
	return q-utf;
}

/****************** utf를 unicode로 변환시키는 부분 시작 *****************/

// unicode로 변환할 배열의 앞 2바이트에 0xFF, 0xFE를 넣기
// 0xFE, 0xFF를 넣을 수도 있지만, 이렇게 넣으면 big-endian 방식임
unsigned char *init_uni(unsigned char *q, int i)
{
	memset(q, 0x00, i*2);

	*q = 0xFF;
	*(q + 1) = 0xFE;

	q += 2;
	
	return q;
}

// utf-8의 맨 앞 3바이트에 0xEF, 0xBB, 0xBF 가 있는 것 같음
// 있으면 3바이트를 제외
unsigned char * confirm_utf(unsigned char *p, int *i)
{
	if( (*p == 0xEF) && (*(p + 1) == 0xBB) && (*(p + 2) == 0xBF) ) {
		p += 3;
		*i -= 3;
	}

	return p;
}

/*
 * int utf2uni(unsigned *, unsigned *, int)
 *
 * 첫번째 인수는 UTF-8이 들어 있는 unsigned char 형 배열의 주소
 * 두번째 인수는 변환된 UNICODE가 들어 갈 unsigned char 형 배열의 주소
 * 세번째 인수는 첫번째 인수의 크기(UNICODE가 들어있는 크기)
 *
 * 첫번째 바이트를 확인후에 1바이트나 2 ~ 3 바이트씩 처리
 *
 * 리턴값 : 변환된 UNICODE의 길이
 *
 */
int utf2uni(unsigned char *utf, unsigned char *uni, int i)
{
	int t;
	
	unsigned char *p, *q;
	unsigned char k;
	
	p = utf;
	q = uni;

	//q = init_uni(q, i);// big-endian 방식임

	p = confirm_utf(p, &i);
	
	for( t = 0; t < i; ) {
		// 첫 바이트를 읽어서 0x80, 0xC0, 0xE0으로 각각 & 연산해서 1바이트를 읽을 지 2~3바이트를 읽을 지 결정.
		// 단, 2 바이트를 읽어야 하는 경우 0xC0만으로 & 연산을 할 경우 3 바이트를 읽어야 하는 부분도 처리가 되므로
		// 0xE0가 아닌 경우를 포함

		// 1 바이트를 읽어야 하는 부분
		if( (*p & 0x80) == 0x00 ) {
			// 0xFE, 0xFF 방식이면 다음 두개의 문을 변경
			*q = *p;
			*(q + 1) = 0x00;
			p++;
			t += 1;
		}
		// 2 바이트를 읽어야 하는 부분
		else if( ((*p & 0xC0) == 0xC0) && ((*p & 0xE0) != 0xE0)) {
			*q = (*p & 0x1C) >> 2;
			*(q + 1) = (*p & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 1) & 0x3F);

			// 0xFE, 0xFF 방식이면 필요 없음
			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;
			//

			p += 2;
			t += 2;
		}
		// 3 바이트를 읽어야 하는 부분
		else if( (*p & 0xE0) == 0xE0 ) {
			*q = *p << 4;
			*q = *q | ((*(p + 1) & 0x3C) >> 2);

			*(q + 1) = (*(p + 1) & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 2) & 0x3F);

			// 0xFE, 0xFF 방식이면 필요 없음
			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;
			//

			p += 3;
			t += 3;
		}
		// 여기서 4 바이트 이상은 제외
		else {
		}
		q += 2;
	}

	return q-uni;
}

unsigned short make_syllable(unsigned char *p, int flag)
{
	int k;
	if( flag == LITTLE_ENDIAN ) {
		k = ((unsigned short)*(p+1) << 8);
		k = k | (unsigned short)*p;
	}
	else {
		k = ((unsigned short)*p << 8);
		k = k | (unsigned short)*(p+1);
	}

	return k;
}

void draw_bitmap( dc_t *dc,FT_Bitmap*  bitmap,FT_Int x,FT_Int y)
{
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;
	FT_Int  pixel=0;

	gx_pen_color(dc, gx_color(0, 0, 0, 0));
	gx_brush_color(dc,dc->font_color);
	for ( i = x, p = 0; i < x_max; i++, p++ )
	{
		for ( j = y, q = 0; j < y_max; j++, q++ )
		{
			if ( i < 0|| j < 0||i >= dc->width|| j >= dc->height ) continue;
			if(bitmap->buffer[q * bitmap->width + p]) ++pixel;
			else if(pixel>0)
			{
				gx_rectangle(dc, i,j-pixel,i,j-1);
				pixel=0;
			}
		}
		if(pixel>0)
		{
			gx_rectangle(dc,i,j-pixel,i,j-1);
			pixel=0;
		}
	}
}


void font_init(void)
{
	char		path[MAX_PATH];
	FT_Error	error;
	error = FT_Init_FreeType( &library );              /* initialize library */
  
 	if ( error ) printf("FT_Init_FreeType Error\n");
	
 	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_FONT, FONT_FILE_NAME);
 	error = FT_New_Face( library, path, 0, &face );
	
 	if ( error ){
	  printf("FONT LOADING ERROR\n");
	  return;
	}
}

void font_dispos(void)
{
	FT_Done_Face    ( face );
	FT_Done_FreeType( library );
}


void string_out(dc_t *dc, int x, int y, char *text, int size)
{
	FT_GlyphSlot  slot;
	FT_Error      error;
	FT_UInt  glyph_index;
	unsigned short	uniChar;
	unsigned char	uChar[256];
	unsigned int    n, num_chars;
	unsigned int 	pen_x,pen_y;
	
	memset(uChar, 0, sizeof(uChar));
	
	num_chars     = strlen( text );
	
// 	error = FT_Init_FreeType( &library );              /* initialize library */
//   
//  	if ( error ) printf("FT_Init_FreeType Error\n");
// 	
//  	error = FT_New_Face( library,"/bin/NanumBarunGothic.ttf",0,&face );
// 	
//  	if ( error ){
// 	  printf("FONT LOADING Error\n");
// 	  return;
// 	}
	
	
	//error = FT_Select_Charmap( face, FT_ENCODING_UNICODE );

	/* use 50pt at 100dpi */
	error = FT_Set_Char_Size( face, size * 64, 0,100, 0 );                /* set character size */
 	if ( error ){
	  printf("FONT Error\n");
	  return;
	}
	
	num_chars=utf2uni((unsigned char*)text,(unsigned char*)uChar, num_chars);
	slot = face->glyph;
	
	pen_x = x;
	pen_y = y+size;
	//pen.x = 300 * 64;
	//pen.y = ( target_height -200 ) * 64;
  
	for ( n = 0; n < num_chars; n+=2 )
	{
		uniChar=make_syllable((unsigned char*)uChar+n,LITTLE_ENDIAN);
		
		glyph_index = FT_Get_Char_Index( face, uniChar );
		
		error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if ( error )
		  continue;  /* ignore errors */
		  
		error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
		if ( error )
		  continue;

		/* now, draw to our target surface (convert position) */
		draw_bitmap(dc, &slot->bitmap,pen_x +slot->bitmap_left,pen_y -slot->bitmap_top);

		/* increment pen position */
		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6; /* not useful for now */
	}
// 	FT_Done_Face    ( face );
// 	FT_Done_FreeType( library );
}

