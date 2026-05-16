/*******************************************************************************
������Ʈ : gxLib
��⳻�� : BITMAP ���
�������� : 2008-11-22
�ۼ���   : ��漮(jwjwmx@gmail.com)
���۱�   : �ּ� ������ �������� �ʴ� �� ���� ����
Ȩ������ : http://forum.falinux.com
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gx.h>
#include <gxbmp.h>

#define BI_RGB          0
#define BI_RLE4         1
#define BI_RLE8         2
#define BI_BITFIELD     3

color_tt  gx_colortt( unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
//-------------------------------------------------------------------------------
// ����: Į�� ���� color_t�� ���Ѵ�.
{
   color_tt  color;

   color.red      = red;
   color.green    = green;
   color.blue     = blue;
   color.alpha    = alpha;
   return color;
}

/******************************************************************** 24 pixel */

static void b24_clear( bmp_t *bmp, color_tt color)
//-------------------------------------------------------------------------------
// ����: Bitmap DC�� ������ ������ �������� ä���.
// �μ�: bmp
//       color
{

}

static void b24_get_pixel( bmp_t *bmp, int coor_x, int coor_y, color_tt *color)
//-------------------------------------------------------------------------------
{
   unsigned char *pdata;

   pdata          = bmp->data + ( bmp->height -coor_y -1)*bmp->bytes_per_line +coor_x*3;
   color->blue    = *pdata++;
   color->green   = *pdata++;
   color->red     = *pdata;
   color->alpha   = 255;
}

static void b24_set_pixel( bmp_t *bmp, int coor_x, int coor_y, color_tt color)
//-------------------------------------------------------------------------------
{
   unsigned char *pdata;
   int            offset = ( bmp->height -coor_y -1)*bmp->bytes_per_line +coor_x*3;

   pdata          = bmp->data + offset;
  *pdata++        = color.blue;
  *pdata++        = color.green;
  *pdata          = color.red;
}

static void b24_hline( bmp_t *bmp, int x_1st, int x_2nd, int coor_y, color_tt color)
//-------------------------------------------------------------------------------
// ����: ������ �׸���.
{

}

static void b24_vline( bmp_t *bmp, int coor_x, int y_1st, int y_2nd, color_tt color)
//-------------------------------------------------------------------------------
// ����: �������� �׸���.
{

}

/******************************************************************** 32 pixel */

static void  b32_clear( bmp_t *bmp, color_tt color)
//-------------------------------------------------------------------------------
{

}

static void b32_get_pixel( bmp_t *bmp, int coor_x, int coor_y, color_tt *color)
//-------------------------------------------------------------------------------
{
   unsigned char *pdata;

   pdata          = bmp->data + ( bmp->height -coor_y -1)*bmp->bytes_per_line +coor_x*4;
   color->blue    = *pdata++;
   color->green   = *pdata++;
   color->red     = *pdata++;
   color->alpha   = *pdata;
}

static void b32_set_pixel( bmp_t *bmp, int coor_x, int coor_y, color_tt color)
//-------------------------------------------------------------------------------
{
   unsigned char *pdata;
   int            offset = ( bmp->height -coor_y -1)*bmp->bytes_per_line +coor_x*4;

   pdata          = bmp->data + offset;
  *pdata++        = color.blue;
  *pdata++        = color.green;
  *pdata++        = color.red;
  *pdata          = color.alpha;
}

static void  b32_hline( bmp_t *bmp, int x_1st, int x_2nd, int coor_y, color_tt color)
{
}

static void  b32_vline( bmp_t *bmp, int coor_x, int y_1st, int y_2nd, color_tt color)
{
}

/*******************************************************************************/

static void calculate_boffset(bmp_t *bmp)
//-------------------------------------------------------------------------------
{
   int i;
   unsigned *mask = (unsigned *)(bmp->palette);
   unsigned  temp;

   /* red */
   temp = mask[0];
   for(i = 0; i < 32; i++)
   {
      if(temp & 0x01)
         break;
      temp >>= 1;
   }
   bmp->boffset_red = i;
   for(i = 0; i < 32; i++)
   {
      if(temp & 0x800000UL)
         break;
      temp <<= 1;
   }
   bmp->bsize_red = 32-i;

   /* green */
   temp = mask[1];
   for(i = 0; i < 32; i++)
   {
      if(temp & 0x01)
         break;
      temp >>= 1;
   }
   bmp->boffset_green = i;
   for(i = 0; i < 32; i++)
   {
      if(temp & 0x800000UL)
         break;
      temp <<= 1;
   }
   bmp->bsize_green = 32-i;

   /* blue */
   temp = mask[2];
   for(i = 0; i < 32; i++)
   {
      if(temp & 0x01)
         break;
      temp >>= 1;
   }
   bmp->boffset_blue = i;
   for(i = 0; i < 32; i++)
   {
      if (temp & 0x800000UL)
         break;
      temp <<= 1;
   }
   bmp->bsize_blue = 32-i;
}

static void rle8_decoding( bmp_t *bmp)
//-------------------------------------------------------------------------------
{
   unsigned char *pdata    = bmp->data;
   unsigned char *pend     = pdata + bmp->width*bmp->height;
   unsigned char *penc_data = bmp->encoded_data;
   unsigned char  c_byte;
   int            ndx, jdx;

   while( 1 )
   {
      if(pdata >= pend)
         break;
      c_byte = *penc_data++;
      if( 0 == c_byte) /* escape */
      {
         c_byte = *penc_data++;
         if( 0 == c_byte) /* end of line */
         {
            ndx = pdata - bmp->data;
            ndx %= bmp->width;
            for( ; ndx < bmp->width; ndx++)
               pdata++;
         }
         else if( c_byte == 1) /* end of bitmap */
         {
            return;
         }
         else if( c_byte == 2) /* delta */
         {
            jdx = *penc_data++; /* right */
            ndx = *penc_data++; /* down */
            ndx = jdx +ndx *bmp->width;
            while(0 < ndx--)
               pdata++;
         }
         else /* absolute mode */
         {
            c_byte = *penc_data++;
            while( 0 < c_byte--)
               *pdata++ = *penc_data++;
            /* word boundary */
            while(*penc_data & 0x01)
               penc_data++;
         }
      }
      else
      {
         while( c_byte--)
            *pdata++ = *penc_data;
         penc_data++;
      }
   }
}

static int read_data(FILE *fp, bmp_t *bmp)
//-------------------------------------------------------------------------------
{
   fseek(fp, bmp->data_offset, SEEK_SET);

   if (bmp->compression == BI_RGB || bmp->compression == BI_BITFIELD)
   {
      bmp->data = (unsigned char *)malloc(bmp->bitmap_size);
      if(0==fread(bmp->data, 1, bmp->bitmap_size, fp)){}
   }
   else
   {                     
      bmp->encoded_data = (unsigned char *)malloc(bmp->bitmap_size);
      bmp->data         = (unsigned char *)malloc(bmp->width*bmp->height*bmp->bpp/8);
      memset(bmp->encoded_data, 0, bmp->bitmap_size);
      if(0==fread( bmp->encoded_data, 1, bmp->bitmap_size, fp)){}
      if(bmp->compression == BI_RLE4)
         return GX_FALSE;                                               /* rle4_decoding is not supported ! */
      else
         rle8_decoding( bmp);
   }
   return GX_TRUE;
}

static int read_palette(FILE *fp, bmp_t *bmp)
//-------------------------------------------------------------------------------
{
   int size;

   size = fread( bmp->palette, sizeof( palette_t), bmp->cnt_palette, fp);
   if ( size != bmp->cnt_palette)
      return GX_FALSE;

   return GX_TRUE;
}

static int read_header(FILE *fp, bmp_t *bmp)
//-------------------------------------------------------------------------------
{
   int size;
   int remnant;
   unsigned char ID[2];

   ID[0] = fgetc( fp);                                                           // ID üũ
   ID[1] = fgetc( fp);
   if(ID[0] != 'B' || ID[1] != 'M')
      return GX_FALSE;

   if ( 52 != ( size = fread( &bmp->file_size, 1, 52, fp)))                       // bmp ��� ������ ��� �о� ���δ�.
      return GX_FALSE;

   bmp->cnt_palette  = ( bmp->data_offset-54) >> 2;
   bmp->width        = bmp->img_width;
   bmp->height       = bmp->img_height;

   size     = bmp->img_width * bmp->bpp /8;
   remnant  = size %4;
   if (remnant == 0)
      bmp->bytes_per_line = size;
   else
      bmp->bytes_per_line = size +(4 -remnant);

   if ( 0 == bmp->bitmap_size)                                                    // ���� ������ �ȵ� ���¶�� bitmap_size �� 0 �� ��
      bmp->bitmap_size = bmp->height*bmp->bytes_per_line;

   return GX_TRUE;
}

static int set_header( bmp_t *bmp, int width, int height, int depth, int palette_size)
//-------------------------------------------------------------------------------
{
   int size;
   int remnant;

   bmp->width        = width;
   bmp->height       = height;
   bmp->img_width    = width;
   bmp->img_height   = height;
   bmp->dots         = width * height;
   bmp->data         = NULL;
   bmp->palette      = NULL;
   bmp->compression  = BI_RGB;                                                   // BITMAP ���� ����� RGB��

   if ( 0 < palette_size)                                                         // �ȷ�Ʈ ����� �����Ǿ� �ִٸ�
   {
      bmp->cnt_palette  = palette_size;
      bmp->palette      = ( palette_t *)malloc( sizeof( palette_t) * bmp->cnt_palette);
      memset( bmp->palette, 0, sizeof( palette_t) * bmp->cnt_palette);
   }

   bmp->bpp = depth;
   size     = bmp->img_width * bmp->bpp /8;
   remnant  = size %4;

   if ( 0 == remnant)
      bmp->bytes_per_line = size;
   else
      bmp->bytes_per_line = size + (4-remnant);

   bmp->bitmap_size  = bmp->height * bmp->bytes_per_line;
   bmp->bytes        = bmp->bitmap_size;
   
   return GX_TRUE;
}

static void  release_dc( dc_t *dc)
//-------------------------------------------------------------------------------
{
   bmp_t  *bmp  = (bmp_t *)dc;

   if ( NULL == bmp)             return;
   if ( NULL != bmp->palette)    free( bmp->palette);
   if ( NULL != bmp->data   )    free( bmp->data   );
   free( bmp);
}

static void free_bmp_resource( bmp_t *bmp, FILE *fp)
//-------------------------------------------------------------------------------
{
   release_dc( ( dc_t *)bmp);
   if ( fp)    fclose(fp);
}

static bmp_t *setup_bitmap( bmp_t *bmp)
//-------------------------------------------------------------------------------
// ����: bmp ������ �ϼ�
{                           
    bmp->dc_type            = DCTYPE_BMP;                               // DC ���¸� BITMAP��
    bmp->coor_x             = 0;                                        // LineTo�� ���� ��ǥ�� 0���� �ʱ�ȭ
    bmp->coor_y             = 0;
    bmp->pen_color          = gx_color( 255, 255, 255, 255);            // �⺻ �� ������ ���
    bmp->brush_color        = gx_color(   0,   0,   0, 255);            // �⺻ �귯�� ������ ����
    bmp->font_color         = gx_color( 255, 255, 255, 255);            // �⺻ �۾� ������ ���
    bmp->release_dc         = release_dc;                               // �Ҹ��� ���� �Լ�
    bmp->mapped             = bmp->data;                                // �̹��� �κ��� �޸� ������
    bmp->colors             = bmp->bpp;                                 // Į�� ����
    bmp->font               = NULL; 
    bmp->bits_per_pixel		= gx_fb.bits_per_pixel;						// jschoi
    
    switch(bmp->bpp)
    {
    case  24:               
            bmp->clear      = b24_clear;
            bmp->get_pixel  = b24_get_pixel;
            bmp->set_pixel  = b24_set_pixel;
            bmp->get_pixel_bmp  = b24_get_pixel;
            bmp->set_pixel_bmp  = b24_set_pixel;
            bmp->vline      = b24_vline;
            bmp->hline      = b24_hline;
            break;          
    case  32:               
            bmp->clear      = b32_clear;
            bmp->get_pixel  = b32_get_pixel;
            bmp->set_pixel  = b32_set_pixel;
            bmp->get_pixel_bmp  = b32_get_pixel;
            bmp->set_pixel_bmp  = b32_set_pixel;
            bmp->vline      = b32_vline;
            bmp->hline      = b32_hline;
            if  (bmp->compression == BI_RGB)
            {
                unsigned *mask;
                
                if  ( bmp->palette != NULL) /* something wrong */
                {
                    printf( "setup_bitmap() : palette info error.\n");
                    free_bmp_resource( bmp, NULL);
                    return NULL;
                }
                
                mask                 = (unsigned *)malloc(sizeof(unsigned)*3);
                mask[2]              = 0x000000FF; /* blue mask */
                mask[1]              = 0x0000FF00; /* green mask */
                mask[0]              = 0x00FF0000; /* red mask */
                bmp->palette         = ( palette_t *)mask;
                bmp->boffset_blue    = 0;
                bmp->boffset_green   = 8;
                bmp->boffset_red     = 16;
                bmp->bsize_blue      = 8;
                bmp->bsize_green     = 8;
                bmp->bsize_red       = 8;
            }
            else /* BI_BITFILED */
            {
                if ( bmp->palette == NULL) /* something wrong */
                {
                    printf( "setup_bitmap() : palette info error.\n");
                    free_bmp_resource( bmp, NULL);
                    return NULL;
                }
                calculate_boffset( bmp);
            }
            break;
    default:
            printf( "setup_bitmap() : color depth error.\n");
            free_bmp_resource( bmp, NULL);
            return NULL;
    }
    return bmp;
}

dc_t *gx_bmp_create( int width, int height, int depth, int palette_size)
//-------------------------------------------------------------------------------
{
    bmp_t    *bmp;
    
    if  ( NULL == ( bmp = malloc( sizeof( bmp_t))) )
    {
        printf( "gx_bmp_create() : out of memory.\n");
        free_bmp_resource( bmp, NULL);
        return NULL;
    }
    memset( bmp, 0, sizeof( bmp_t));
    
    set_header( bmp, width, height, depth, palette_size);
           
    if  ( NULL == ( bmp->data = malloc( bmp->bitmap_size)) )
    {
        printf( "gx_bmp_create() : out of memory.\n");
        free_bmp_resource( bmp, NULL);
        return NULL;
    }
    
    return ( dc_t *)setup_bitmap( bmp);
}

bmp_t *gx_given_bmp_mastering( bmp_t *bmp , int width, int height, int depth, int palette_size)
//-------------------------------------------------------------------------------
{                                   
    set_header( bmp, width, height, depth, palette_size);
    
    if ( NULL == ( bmp->data = malloc( bmp->bitmap_size)) )
    {
        printf( "gx_given_bmp_mastering() : out of memory.\n");
        free_bmp_resource( bmp, NULL);
        return NULL;
    }
    
    return setup_bitmap( bmp);
}

void  gx_bmp_close( dc_t *dc)
//-------------------------------------------------------------------------------
// ����: bmp ����� ����
{
    release_dc( dc);
}

dc_t *gx_bmp_open( char *filename)
//-------------------------------------------------------------------------------
// ����: bmp ������ �о� ����
// ����: bmp�� Į�� ���̿� ���߾� ���� �Լ� ����
// �μ�: fil
{            
    FILE    *fp    = NULL;
    bmp_t   *bmp   = NULL;

    if  ( NULL == ( bmp = malloc( sizeof( bmp_t))) )
    {                
        printf( "gx_bmp_open(): out of memory.\n");
        free_bmp_resource( bmp, NULL);
        return NULL;
    }
    
    memset( bmp, 0, sizeof( bmp_t));

    fp = fopen( filename, "r+b");	// .bmp
    if  ( NULL == ( fp) )
    {
    	int file_len = strlen(filename);
    	if(file_len>4)
    	{
    		memcpy(&filename[file_len-4], EXT_BIG_BMP, 4);
    		fp = fopen( filename, "r+b");	// .BMP
    		if  ( NULL == ( fp) )
    		{
    			printf( "gx_bmp_open(): no file.\n");
				free_bmp_resource( bmp, fp);
				return NULL;
    		}
    	}
    }
    
    if ( !read_header( fp, bmp))
    {
        printf( "gx_bmp_open(): header info error.\n");
        free_bmp_resource( bmp, fp);
        return NULL;	// jschoi
        //return ( dc_t *)bmp;
    }

    if  ( 0 != bmp->cnt_palette)
    {
        bmp->palette = ( palette_t *)malloc( sizeof( palette_t) * bmp->cnt_palette);
        memset( bmp->palette, 0, sizeof( palette_t) * bmp->cnt_palette);
        if ( !read_palette(fp, bmp))
        {
            printf( "gx_bmp_open(): palette info error.\n");
            free_bmp_resource( bmp, fp);
            return NULL;
        }
    }
    
    if  ( !read_data(fp, bmp))
    {
        printf( "gx_bmp_open(): read error.\n");
        free_bmp_resource( bmp, fp);
        return NULL;
    }
    
    fclose(fp);
    return ( dc_t *)setup_bitmap( bmp);
}


