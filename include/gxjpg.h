#ifndef _GX_JPG_H_
#define _GX_JPG_H_

#include <gx.h>
#include <gxjpg.h>

typedef struct jpg_t_ jpg_t;
struct jpg_t_
{             
    char        dc_type;                                                // DC�� ���·� Screen, Bitmap�� �����Ѵ�.
    int         width;                                                  // ��Ʈ ������ ��
    int         height;                                                 // ��Ʈ ������ ����
    int         dots;                                                   // ��ü ��Ʈ ���� width * height
    int         bytes;                                                  // �޸��� ��ü Byte ũ��
    int         colors;                                                 // Į�� ����
    int         bytes_per_line;                                         // ���δ� ����Ʈ ����
    int         bits_per_pixel;                                         // ��Ʈ�� �ȼ� ����
    int         coor_x;                                                 // ������ �׸��� �ߴ� ������ ��ǥ
    int         coor_y;                                                 // ������ �׸��� �ߴ� ������ ��ǥ
    int         output_display;
    color_t     pen_color;                                              // ������ �� Į��
    color_t     brush_color;                                            // ������ �귯�� Į��
    color_t     font_color;                                             // �۾� ����          
    font_t     *font;                                                   // ���ڿ� ����� ���� �۲�
    void       *mapped;                                                 // �޸� ���ε� ������

    void (*release_dc)( dc_t *dc);                                      // Device Context �Ҹ� �� ���� �޸𸮸� ����
    void (*clear	  )( bmp_t *bmp, color_t color);					// ������ ��ü ĥ�ϱ�
	void (*get_pixel )( bmp_t *bmp, int coor_x, int coor_y, color_t *color);	   // Į�� ���� �о� ����
	void (*set_pixel )( bmp_t *bmp, int coor_x, int coor_y, color_t color 	 );	// �� ���
    void (*hline	  )( bmp_t *bmp, int x1st  , int x_2nd , int coor_y, color_t color);	// ���� �߱�
    void (*vline	  )( bmp_t *bmp, int coor_x, int y_1st , int y_2nd , color_t color);	// ������ �߱�

	int   file_size;                                                    // ^  �̹��� ���� ������
	int   reserved;                                                     // |  ���� ����
	int   data_offset;                                                  // |
	int   header_size;                                                  // |
	int   img_width;                                                    // |  �̹��� ��
	int   img_height;                                                   // |  �̹��� ����
	short cnt_planes;                                                   // |  bmp ��� ���� 52 bytes
	short bpp;                                                          // |
	int   compression;                                                  // |  ���� ����
	int   bitmap_size;                                                  // |
	int   hres;                                                         // |
	int   vres;                                                         // |
	int   cnt_colors;                                                   // |
	int   important_colors;                                             // v

	int            cnt_palette;                                         // �ȷ�Ʈ ����
	palette_t     *palette;                                             // �ȷ�Ʈ Į�� ����
	unsigned char *data;                                                // �̹��� ������ ������
	unsigned char *encoded_data;
	unsigned       bsize_blue  , bsize_green  , bsize_red;              // R,G,B �� ���� ��Ʈ ũ��
	unsigned       boffset_blue, boffset_green, boffset_red;            // R,G,B �� ���� ���� ���ϱ� ���� ����Ʈ Ƚ��
};

extern dc_t   *gx_jpg_open  ( char  *filename);             	        // png ��ü�� ������ �Ŀ�, ������ ����
extern void    gx_jpg_close ( dc_t  *dc);                             // ������ Ŭ���� �� png ��ü �Ҹ�

#endif
