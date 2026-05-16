#ifndef _GX_BMP_H_
#define _GX_BMP_H_

#include <gx.h>



typedef struct palette_t_ palette_t;
struct palette_t_
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char filter;
};

typedef struct bmp_t_ bmp_t;
struct bmp_t_                                                           
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
    void (*clear	  )( bmp_t *bmp, color_tt color);					// ������ ��ü ĥ�ϱ�
	void (*get_pixel )( bmp_t *bmp, int coor_x, int coor_y, color_tt *color);	// Į�� ���� �о� ����
	void (*set_pixel )( bmp_t *bmp, int coor_x, int coor_y, color_tt color 	 );	// �� ���
	void (*get_pixel_bmp )( bmp_t *bmp, int coor_x, int coor_y, color_tt *color);	// Į�� ���� �о� ����
	void (*set_pixel_bmp )( bmp_t *bmp, int coor_x, int coor_y, color_tt color 	 );	// �� ���
    void (*hline	  )( bmp_t *bmp, int x1st  , int x_2nd , int coor_y, color_tt color);	// ���� �߱�
    void (*vline	  )( bmp_t *bmp, int coor_x, int y_1st , int y_2nd , color_tt color);	// ������ �߱�

	int         file_size;                                              // ^  �̹��� ���� ������
	int         reserved;                                               // |  ���� ����
	int         data_offset;                                            // |
	int         header_size;                                            // |
	int         img_width;                                              // |  �̹��� ��
	int         img_height;                                             // |  �̹��� ����
	short       cnt_planes;                                             // |  bmp ��� ���� 52 bytes
	short       bpp;                                                    // |
	int         compression;                                            // |  ���� ����
	int         bitmap_size;                                            // |
	int         hres;                                                   // |
	int         vres;                                                   // |
	int         cnt_colors;                                             // |
	int         important_colors;                                       // v

	int            cnt_palette;                                         // �ȷ�Ʈ ����
	palette_t     *palette;                                             // �ȷ�Ʈ Į�� ����
	unsigned char *data;                                                // �̹��� ������ ������
	unsigned char *encoded_data;
	unsigned       bsize_blue  , bsize_green  , bsize_red;              // R,G,B �� ���� ��Ʈ ũ��
	unsigned       boffset_blue, boffset_green, boffset_red;            // R,G,B �� ���� ���� ���ϱ� ���� ����Ʈ Ƚ��
};

extern dc_t   *gx_bmp_create( int width, int height, int Depth, int palette_size);// Bitmap ��ü ����
extern dc_t   *gx_bmp_open  ( char  *filename);             	        // Bmp ��ü�� ������ �Ŀ�, ������ ����
extern void    gx_bmp_close ( dc_t *dc);                              // ������ Ŭ���� �� bmp ��ü �Ҹ�
extern bmp_t  *gx_given_bmp_mastering( bmp_t * bmp , int width, int height, int depth, int palette_size);

#endif
