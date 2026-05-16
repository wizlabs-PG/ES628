/*******************************************************************************
������Ʈ : gxLib
��⳻�� : gxLib���� JPEG �̹��� ���
�������� : 2008-11-23
�ۼ���   : Ǫ������(ssseo88@chol.com)
������   : ��漮( jwjw, jwjwmx@gmail.com)
���۱�   : �ּ� ������ �������� �ʴ� �� ���� ����
�������� :
            - 2009-10-25
                JPG����� 128 �ȷ�Ʈ ��뿡�� ����Į��� ����ϵ��� ����       
Ȩ������ : http://forum.falinux.com
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "gx.h"
#include "gxbmp.h"
#include "gxjpg.h"
#else
#include <gx.h>
#include <gxbmp.h>
#include <gxjpg.h>
#endif

#include <jpeglib.h>

#include <setjmp.h>

//#define BI_RGB          0
//#define BI_RLE4         1
//#define BI_RLE8         2
//#define BI_BITFIELD     3
//
//typedef unsigned char   uch;
//typedef unsigned short  ush;
//typedef unsigned int    uln;
//typedef unsigned long   ulg;

//typedef struct
//{
//  unsigned char red;
//  unsigned char green;
//   unsigned char blue;
//} rgb_color_struct;

struct ima_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct ima_error_mgr *ima_error_ptr;


// struct Iterator__                                                            // jwjw : �ʿ� ��� �ּ� ó��
// {                                                                            // jwjw : �ʿ� ��� �ּ� ó��
//   int               Itx, Ity;                       // Counters              // jwjw : �ʿ� ��� �ּ� ó��
//   int               Stepx, Stepy;                                            // jwjw : �ʿ� ��� �ּ� ó��
//   unsigned char    *IterImage;                      //  Image pointer        // jwjw : �ʿ� ��� �ּ� ó��
//   jpg_t            *ima;                                                     // jwjw : �ʿ� ��� �ּ� ó��
// };                                                                           // jwjw : �ʿ� ��� �ּ� ó��
// typedef struct Iterator__ Iterator;                                          // jwjw : �ʿ� ��� �ּ� ó��

//static int IterItOK ( Iterator* iter )
//{
//  if ( iter->Itx <= iter->ima->width && iter->Ity <= iter->ima->height )
//   return TRUE;
//  else
//   return FALSE;
//}

//static void IterReset( Iterator* iter )
//{
//  iter->IterImage = iter->ima->data;
//  iter->Itx = iter->Ity = 0;
//}

//static int IterNextRow(Iterator* iter )
//{
//  if (++(iter->Ity) >= iter->ima->height) return 0;
//  iter->IterImage += iter->ima->bytes_per_line;
//  return 1;
//}

//static int IterPrevRow(Iterator* iter )                                       // jwjw : �ʿ� ��� �ּ� ó��
//{                                                                             // jwjw : �ʿ� ��� �ּ� ó��
//  if (--(iter->Ity) < 0) return 0;                                            // jwjw : �ʿ� ��� �ּ� ó��
//  iter->IterImage -= iter->ima->bytes_per_line;                               // jwjw : �ʿ� ��� �ּ� ó��
//  return 1;                                                                   // jwjw : �ʿ� ��� �ּ� ó��
//}                                                                             // jwjw : �ʿ� ��� �ּ� ó��
                                                                              
//static void IterUpset(Iterator* iter )                                        // jwjw : �ʿ� ��� �ּ� ó��
//{                                                                             // jwjw : �ʿ� ��� �ּ� ó��
//  iter->Itx         = 0;                                                      // jwjw : �ʿ� ��� �ּ� ó��
//  iter->Ity         = iter->ima->height-1;                                    // jwjw : �ʿ� ��� �ּ� ó��
//  iter->IterImage   = iter->ima->data + iter->ima->bytes_per_line*(iter->ima->height-1);  // jwjw : �ʿ� ��� �ּ� ó��
//}

////////////////////////// AD - for interlace ///////////////////////////////
//static void IterSetY(Iterator* iter ,int y)
//{
//  if ((y < 0) || (y > iter->ima->height)) return;
//  iter->Ity = y;
//  iter->IterImage = iter->ima->data + iter->ima->bytes_per_line*y;
//}

/////////////////////////////////////////////////////////////////////////////

//static void IterSetRow( Iterator* iter ,unsigned char *buf, int n )           // jwjw : �ʿ� ��� �ּ� ó��
//{                                                                             // jwjw : �ʿ� ��� �ּ� ó��
//// Here should be bcopy or memcpy                                             // jwjw : �ʿ� ��� �ּ� ó��
//  //_fmemcpy(IterImage, (void far *)buf, n);                                  // jwjw : �ʿ� ��� �ּ� ó��
//	int i;                                                                      // jwjw : �ʿ� ��� �ּ� ó��
//  if (n<0 || n > iter->ima->width )                                           // jwjw : �ʿ� ��� �ּ� ó��
//	 n = iter->ima->width;                                                      // jwjw : �ʿ� ��� �ּ� ó��
//                                                                              // jwjw : �ʿ� ��� �ּ� ó��
//  for (i=0; i<n; i++)                                                         // jwjw : �ʿ� ��� �ּ� ó��
//	  iter->IterImage[i] = buf[i];                                              // jwjw : �ʿ� ��� �ּ� ó��
//}                                                                             // jwjw : �ʿ� ��� �ּ� ó��

//static void IterGetRow(Iterator* iter ,unsigned char *buf, int n)
//{
//  int i;
//  for (i=0; i<n; i++) buf[i] = iter->IterImage[i];
//}

//static unsigned char* IterGet(Iterator* iter )
//{
//  return iter->IterImage;
//}

//static int IterNextByte(Iterator* iter )
//{
//  if (++(iter->Itx )< iter->ima->width)
//   return 1;
//  else
//   if (++(iter->Ity) < iter->ima->height)
//   {
//      iter->IterImage += iter->ima->bytes_per_line;
//      iter->Itx = 0;
//      return 1;
//   } else
//      return 0;
//}
//
//static int IterPrevByte(Iterator* iter )
//{
//  if (--(iter->Itx) >= 0)
//   return 1;
//  else
//   if (--(iter->Ity) >= 0)
//   {
//      iter->IterImage -= iter->ima->bytes_per_line;
//      iter->Itx = 0;
//      return 1;
//   } else
//      return 0;
//}
//
//static int IterNextStep(Iterator* iter )
//{
//  iter->Itx += iter->Stepx;
//  if (iter->Itx < iter->ima->bytes_per_line)
//   return 1;
//  else {
//   iter->Ity += iter->Stepy;
//   if (iter->Ity < iter->ima->height)
//   {
//      iter->IterImage += iter->ima->bytes_per_line;
//      iter->Itx = 0;
//      return 1;
//   } else
//      return 0;
//  }
//}
//
//static int IterPrevStep(Iterator* iter )
//{
//  iter->Itx -= iter->Stepx;
//  if (iter->Itx >= 0)
//   return 1;
//  else {
//   iter->Ity -= iter->Stepy;
//   if (iter->Ity >= 0 && iter->Ity < iter->ima->height)
//   {
//      iter->IterImage -= iter->ima->bytes_per_line;
//      iter->Itx = 0;
//      return 1;
//   } else
//      return 0;
//  }
//}


/*
 * Here's the routine that will replace the standard error_exit method:
 */

void ima_jpeg_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  ima_error_ptr myerr = (ima_error_ptr) cinfo->err;

  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  myerr->pub.format_message (cinfo, buffer);

  /* Send it to stderr, adding a newline */
//        AfxMessageBox(buffer);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}



static void CreateGrayColourMap( palette_t* palette , int n )
{
  int i;
  unsigned char g;

  for (i=0; i<n; i++)
  {
    g = (unsigned char)(256*i/n);
    palette[i].blue = palette[i].green = palette[i].red = g;
    palette[i].filter = 0;
  }
}

static void  release_dc( dc_t *dc)
//-------------------------------------------------------------------------------
{
   jpg_t  *jpg  = (jpg_t *)dc;

   if ( NULL == jpg)             return;
   if ( NULL != jpg->palette)    free( jpg->palette);
   if ( NULL != jpg->data   )    free( jpg->data   );
   free( jpg );
}

static void free_jpg_resource( jpg_t *jpg, FILE *fp)
//-------------------------------------------------------------------------------
{
   release_dc( ( dc_t *)jpg );
   if ( fp)    fclose(fp);
}

static void copy_1byte( unsigned char *pdata, unsigned char *buff, int width)
//-------------------------------------------------------------------------------
// ����: ��Ʈ �� Į�� ����Ʈ�� �� �� ����Ʈ�� ��� �Ѱ� ���ξ� ����
{
    int     ndx;

    for ( ndx = 0; ndx < width; ndx++)
    {
        *pdata++    = *buff++;
    }
}

static void copy_3byte( unsigned char *pdata, unsigned char *buff, int width)
//-------------------------------------------------------------------------------
// ����: ��Ʈ �� Į�� ����Ʈ�� �� �� ����Ʈ�� ��� �Ѱ� ���ξ� ����
{
    char    r_color;
    char    g_color;
    char    b_color;
    int     ndx;

    for ( ndx = 0; ndx < width; ndx++)
    {
        b_color     = *buff++;
        g_color     = *buff++;
        r_color     = *buff++;
        *pdata++    = r_color;
        *pdata++    = g_color;
        *pdata++    = b_color;
    }
}

void  gx_jpg_close( dc_t *dc)
//-------------------------------------------------------------------------------
// ����: PNG ���� ��� ����
{
   release_dc( dc);
}

dc_t *gx_jpg_open( char  *filename )
{
    static jpg_t   *jpg;                                                // static : might be clobbered by 'longjmp' or 'vfork' warinig �޽����� ���ֱ� ����
    static FILE    *fp      = NULL;                                     // static : might be clobbered by 'longjmp' or 'vfork' warinig �޽����� ���ֱ� ����
//    Iterator        iter;                                             // jwjw : �ʿ� ��� �ּ� ó��
    struct          jpeg_decompress_struct cinfo;
    struct          ima_error_mgr jerr;
    JSAMPARRAY      buffer;
    int             row_stride;
    unsigned char  *pbmp_buffer;
    void          (*copy_bitmap)( unsigned char *pdata, unsigned char *buff, int width);

    if ( NULL == ( jpg = malloc( sizeof( jpg_t))) )
    {                    
        printf( "gx_jpg_open() : out of memory.\n");        
        free_jpg_resource( jpg, fp);
        return NULL;
    }
    memset( jpg, 0, sizeof( jpg_t));

//    memset( &iter, 0x00, sizeof(Iterator));                           // jwjw : �ʿ� ��� �ּ� ó��
//    iter.ima = jpg;                                                   // jwjw : �ʿ� ��� �ּ� ó��

//original
/*
    fp = fopen((const char *)filename, "rb");
    if ( NULL == ( fp) )                                                // ������ ���ų� ���⿡ �����ߴٸ�
    {
        printf( "gx_jpg_open() : no file.\n");        
        free_jpg_resource( jpg, fp);
        return NULL;
    }
*/
    fp = fopen( filename, "rb");
	if  (NULL == (fp))
	{
		int file_len = strlen(filename);
		if(file_len>4)
		{
			memcpy(&filename[file_len-4], EXT_BIG_JPG, 4);
			fp = fopen( filename, "r+b");	// .JPG
			if  ( NULL == (fp) )
			{
				printf( "gx_jpg_open(): no file.\n");
				free_jpg_resource( jpg, fp);
				return NULL;
			}
		}
	}

    cinfo.err           = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = ima_jpeg_error_exit;

    if ( setjmp(jerr.setjmp_buffer))
    {
        printf( "gx_jpg_open() : processing error.\n");        
        free_jpg_resource( jpg, fp);
        jpeg_destroy_decompress(&cinfo);
        
        return NULL;
    }
    jpeg_create_decompress( &cinfo);
    jpeg_stdio_src( &cinfo, fp);                                        
    jpeg_read_header( &cinfo, TRUE);

    if ( JCS_GRAYSCALE == cinfo.jpeg_color_space)                       // jwjw: Gray Scale �̹����� ��� �޸� ũ�⸦ �ٿ� �����ϱ� ���� �ȷ�Ʈ�� �����Ѵ�
    {
        cinfo.quantize_colors   = TRUE;                                 // jwjw: TRUE - �ȷ�Ʈ�� ����
        cinfo.out_color_space   = JCS_GRAYSCALE;                        // jwjw: ��Ʈ�� Į���� �� ���� ����Ʈ�� �����ϵ���. cinfo.output_components �� 1 �̵�
        jpeg_calc_output_dimensions( &cinfo);                           // jwjw:
        cinfo.desired_number_of_colors = 256;                           // jwjw: 256 - TRUE Color �Ǵ� Direct Color �� ���Ѵ�

        jpeg_start_decompress( &cinfo);                                         
        gx_given_bmp_mastering((bmp_t*)jpg, cinfo.image_width, cinfo.image_height, 8*cinfo.output_components, cinfo.actual_number_of_colors);
        CreateGrayColourMap( jpg->palette ,256);                                
    }
    else                                                                // Į�� �̹����� ��� �ȷ�Ʈ�� �������� �ʰ� True Color�� ���
    {
        cinfo.quantize_colors   = FALSE;                                // jwjw: FALSE - �ȷ�Ʈ�� ���� ���� �ʰ�
        cinfo.out_color_space   = JCS_RGB;                              // jwjw:
        jpeg_calc_output_dimensions( &cinfo);                           // jwjw:
        cinfo.desired_number_of_colors = 256;                           // jwjw: 256 - TRUE Color �Ǵ� Direct Color �� ���Ѵ�

        jpeg_start_decompress( &cinfo);                                         
        gx_given_bmp_mastering( (bmp_t*)jpg, cinfo.image_width, cinfo.image_height, 8*cinfo.output_components, cinfo.actual_number_of_colors);
//        SetPalette( jpg->palette, cinfo.actual_number_of_colors, cinfo.colormap[0], cinfo.colormap[1], cinfo.colormap[2]);    // jwjw: Į�󿡼� �ȷ�Ʈ�� ������� �ʰ� �ϱ� ���� �ּ� ó��
    }
    jpg->dc_type        = DCTYPE_JPG;                                   // DC ���¸� JPEG����
    jpg->coor_x         = 0;                                            // LineTo�� ���� ��ǥ�� 0���� �ʱ�ȭ
    jpg->coor_y         = 0;                                            
    jpg->pen_color      = gx_color( 255, 255, 255, 255);                // �⺻ �� ������ ���
    jpg->brush_color    = gx_color(   0,   0,   0, 255);                // �⺻ �귯�� ������ ����
    jpg->font_color     = gx_color( 255, 255, 255, 255);                // �⺻ �۾� ������ ���
    jpg->font           = NULL;
    jpg->release_dc     = release_dc;                                   // �Ҹ��� ���� �Լ�
    row_stride          = cinfo.output_width * cinfo.output_components;
    buffer              = ( *cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

//    IterUpset(&iter);                                                 // jwjw : �ʿ� ��� �ּ� ó��

    switch( cinfo.output_components)
    {                
    case 1 :    copy_bitmap = copy_1byte;   break;                      // jwjw : ��Ʈ �� Į�� ���� �� ���� ����Ʈ�� ���� ����
    case 3 :    copy_bitmap = copy_3byte;   break;                      // jwjw : ��Ʈ �� Į�� ���� �� ���� ����Ʈ�� ���� ����
    default:    copy_bitmap = copy_1byte;                               // jwjw : �ܴ̿� ������ �߻����� �ʵ��� �Լ� �����ʹ� ����
                printf( "JPEG ERROR : no assigned copy bitmap function.\n");    // jwjw : �Լ� �����Ͱ� ����� �������� �ʾ����� ���� ���
                break;
    }
    
    pbmp_buffer = jpg->data + jpg->bytes_per_line *jpg->height;         // jwjw : gx_given_bmp_mastering()���� ������ bmp_t �޸𸮿� JPEG �̹����� ���� �غ�
    while ( cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines( &cinfo, buffer, 1);
        pbmp_buffer -= row_stride;                                      // jwjw : bitmap�� �̹��� ���� ������  
        copy_bitmap( pbmp_buffer, buffer[0], jpg->width);

//        IterSetRow( &iter, buffer[0], row_stride);                    // jwjw : �ʿ� ��� �ּ� ó��
//        IterPrevRow( &iter );                                         // jwjw : �ʿ� ��� �ּ� ó��
    }

    jpeg_finish_decompress( &cinfo);
    jpeg_destroy_decompress( &cinfo);

    fclose( fp);
    
    return ( dc_t *)jpg;
}
