#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <fpga_spi.h>
#include <pattern.h>
#include <gpio.h>
#include <rcb.h>
#include <model_data.h>
#include <math.h>

int spi_fd;
int spi_draw_fd;
unsigned short fpga_write_reg[255];

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static struct spi_ioc_transfer *spi_xfrs = NULL;
static int ntransfers = 0;
static int fpga_speed = 0;
static int fpga_delay = 0;
static int fpga_bits = 0;

int FPGA_Open(unsigned char mode, unsigned char bits, unsigned int speed, unsigned short delay)
{
	int ret = 0;

	memset(&fpga_write_reg, 0, sizeof(fpga_write_reg));

	spi_fd = open(FPGA_SPI_DEV0, O_RDWR);
	if (spi_fd < 0) printf("[ENSIS]can't open device\n");

	/*
	 * spi mode
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't set spi mode\n");

	ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't get spi mode\n");
	/*
	 * bits per word
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't set bits per word\n");

	ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't get bits per word\n");

	/*
	 * max speed hz
	 */
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't set max speed hz\n");

	ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't get max speed hz\n");




	spi_draw_fd = open(FPGA_SPI_DEV1, O_RDWR);
	if (spi_draw_fd < 0) printf("[ENSIS]can't open device\n");

	/*
	 * spi mode
	 */
	ret = ioctl(spi_draw_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't set spi mode\n");

	ret = ioctl(spi_draw_fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't get spi mode\n");
	/*
	 * bits per word
	 */
	ret = ioctl(spi_draw_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't set bits per word\n");

	ret = ioctl(spi_draw_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't get bits per word\n");

	/*
	 * max speed hz
	 */
	ret = ioctl(spi_draw_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't set max speed hz\n");

	ret = ioctl(spi_draw_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't get max speed hz\n");

	fpga_speed	= speed;
	fpga_delay	= delay;
	fpga_bits	= bits;

	//printf("[ENSIS]spi mode: %d\n", mode);
	//printf("[ENSIS]bits per word: %d\n", bits);
	//printf("[ENSIS]max speed: %d Hz (%d KHz)\n", speed, spe
	return ret;
}

unsigned short  read_fpga_c_register(unsigned char reg_addr)
{
	return fpga_write_reg[reg_addr];
}

int FPGA_SpiWrite(char *data)
{
	int ret = write(spi_fd, data, ARRAY_SIZE(data));
	if(ret != ARRAY_SIZE(data)) printf("[ENSIS]spi Write error\n");
	return ret;
}

int FPGA_SpiRead(char *data)
{
	int ret = read(spi_fd, data, ARRAY_SIZE(data));
	if(ret != ARRAY_SIZE(data)) printf("[ENSIS]spi Read error\n");
	return ret;
}

int FpgaWrite(char* out,int outlen)
{
	struct spi_ioc_transfer mesg[1] = { 0 };
	uint8_t num_tr = 0;
	int 	ret;

	if((out != NULL) && (outlen != 0))
	{
		mesg[0].tx_buf = (unsigned long)out;
		mesg[0].rx_buf = (unsigned long)NULL;
		mesg[0].len = outlen;
//		mesg[0].cs_change = 0;
		num_tr++;
	}

	if(num_tr > 0)
	{
		ret= ioctl(spi_fd, SPI_IOC_MESSAGE(num_tr), mesg);
		if(ret == 1)
		{
			return 1;
		}
	}

	return 0;
}

void FPGA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = RegWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
//	printf("## fpga write= Addr: 0x%04x, Data: 0x%02x%02x\n", reg_addr, tx[2], tx[3]);
}

void FPGA_Write_signed(unsigned char reg_addr, signed short data)
{
	char tx[4] = {0,};
	tx[0] = RegWrite;
	tx[1] = reg_addr;
	tx[2] = (signed char)(data >> 8);
	tx[3] = (signed char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
//	printf("fpga signed write= 0x%02x %02x\n", tx[2], tx[3]);
}

void FPGA_OR_SET(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	fpga_write_reg[reg_addr]=fpga_write_reg[reg_addr]|data;
	tx[0] = RegWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(fpga_write_reg[reg_addr] >> 8);
	tx[3] = (unsigned char)(fpga_write_reg[reg_addr] & 0xff);
	FPGA_SpiWrite(tx);
//	printf("FPGA OR SET: 0x%02x%02x\n", tx[2], tx[3]);
}

void FPGA_AND_SET(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	fpga_write_reg[reg_addr]=fpga_write_reg[reg_addr]&(~data);
	tx[0] = RegWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(fpga_write_reg[reg_addr] >> 8);
	tx[3] = (unsigned char)(fpga_write_reg[reg_addr] & 0xff);
	FPGA_SpiWrite(tx);
//	printf("FPGA AND SET: 0x%02x%02x\n", tx[2], tx[3]);
}

void FPGA_ANDOR_SET(unsigned char reg_addr, unsigned short data1, unsigned short data2)
{
	char tx[4] = {0,};
	fpga_write_reg[reg_addr]=fpga_write_reg[reg_addr]&(~data1);
	fpga_write_reg[reg_addr]=fpga_write_reg[reg_addr]|data2;
	tx[0] = RegWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(fpga_write_reg[reg_addr] >> 8);
	tx[3] = (unsigned char)(fpga_write_reg[reg_addr] & 0xff);
	FPGA_SpiWrite(tx);
	printf("FPGA ANDOR SET Addr:0x%04x, Data: 0x%02x%02x\n", reg_addr, tx[2], tx[3]);
}

unsigned short FPGA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = RegRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

//	printf("FPGA Read: 0x%02x%02x\n", tx[0], tx[1]);
	return data;
}


void FPGA_print(void)
{
/*
	printf("FFPGA_RASTER_R\t\t: %04d\n",FPGA_Read(FPGA_RASTER_R));
	printf("FPGA_RASTER_G\t\t: %04d\n",FPGA_Read(FPGA_RASTER_G));
	printf("FPGA_RASTER_B\t\t: %04d\n",FPGA_Read(FPGA_RASTER_B));
	printf("FPGA_MODE\t\t: %04d\n",FPGA_Read(FPGA_MODE));
	printf("FPGA_OUTPUT_SEL\t\t: %04d\n",FPGA_Read(FPGA_OUTPUT_SEL));
	printf("FPGA_DP_RX_CTRL\t\t: %04d\n",FPGA_Read(FPGA_DP_RX_CTRL));
	printf("FPGA_GRAY_LEVEL\t\t: %04d\n",FPGA_Read(FPGA_GRAY_LEVEL));
	printf("FPGA_COLOR_SEL\t\t: %04d\n",FPGA_Read(FPGA_COLOR_SEL));
	printf("FPGA_H_ACTIVE\t\t: %04d\n",FPGA_Read(FPGA_H_ACTIVE));
	printf("FPGA_V_ACTIVE\t\t: %04d\n",FPGA_Read(FPGA_V_ACTIVE));
	printf("FPGA_VERSION\t\t: %04d\n",FPGA_Read(FPGA_VERSION));
	printf("FPGA_INVERSION\t\t: %04d\n",FPGA_Read(FPGA_INVERSION));
	printf("FPGA_H_WIDTH\t\t: %04d\n",FPGA_Read(FPGA_H_WIDTH));
	printf("FPGA_H_BPORCH\t\t: %04d\n",FPGA_Read(FPGA_H_BPORCH));
	printf("FPGA_H_FPORCH\t\t: %04d\n",FPGA_Read(FPGA_H_FPORCH));
	printf("FPGA_V_WIDTH\t\t: %04d\n",FPGA_Read(FPGA_V_WIDTH));



	printf("FPGA_V_BPORCH\t\t: %04d\n",FPGA_Read(FPGA_V_BPORCH));
	printf("FPGA_V_FPORCH\t\t: %04d\n",FPGA_Read(FPGA_V_FPORCH));
	printf("FPGA_QUAD_2DIV\t\t: %04d\n",FPGA_Read(FPGA_QUAD_2DIV));
	printf("FPGA_GRAY_R\t\t: %04d\n",FPGA_Read(FPGA_GRAY_R));
	printf("FPGA_GRAY_G\t\t: %04d\n",FPGA_Read(FPGA_GRAY_G));
	printf("FPGA_GRAY_B\t\t: %04d\n",FPGA_Read(FPGA_GRAY_B));
	printf("FPGA_CTR\t\t: %04d\n",FPGA_Read(FPGA_CTR));
	printf("FPGA_MUXSEL\t\t: %04d\n",FPGA_Read(FPGA_MUXSEL));
	printf("FPGA_DACSEL\t\t: %04d\n",FPGA_Read(FPGA_DACSEL));
	printf("FPGA_DACADDR_WRITE\t\t: %04d\n",FPGA_Read(FPGA_DACADDR_WRITE));
	printf("FPGA_DACDATA_WRITE\t\t: %04d\n",FPGA_Read(FPGA_DACDATA_WRITE));
	printf("FPGA_DELAY_WRITE\t\t: %04d\n",FPGA_Read(FPGA_DELAY_WRITE));
	printf("FPGA_OCP_READ\t\t: %04d\n",FPGA_Read(FPGA_OCP_READ));
	printf("FPGA_ADC_READ\t\t: %04d\n",FPGA_Read(FPGA_ADC_READ));
*/
}

void write_edid(unsigned char *edid)
{
	unsigned char 	i;
	char 			tx[4] = {0,};

	tx[0] = EdidWrite;
	for(i=0; i<128; i++){
		tx[1] = i;
		tx[2] = 0;
		tx[3] = *(edid+i);
//printf("%02X ", tx[3]);
		FPGA_SpiWrite(tx);
	}
//printf("\n");
}

void FPGA_Close(void)
{
	close(spi_fd);
	close(spi_draw_fd);
}

static char send_fpga_data[4096+16];
static int send_fpga_data_cnt=0;

int FPGA_SendDraw(char * data, int len)
{
	memcpy(send_fpga_data+send_fpga_data_cnt, data, len);
	send_fpga_data_cnt += len;

	if(send_fpga_data_cnt >= 4096)
	{
		//printf("send_fpga_data_cnt = %d\n", send_fpga_data_cnt);	// test
		int ret = write(spi_draw_fd, send_fpga_data, send_fpga_data_cnt);
		if(ret<1) printf("[ENSIS]spi Write error\n");
		send_fpga_data_cnt = 0;
	}
	return 0;
}

void FPGA_SendEnd(void)
{
	if(send_fpga_data_cnt > 0)
	{
		//printf("send_fpga_data_cnt = %d\n", send_fpga_data_cnt);	// test
		int ret = write(spi_draw_fd, send_fpga_data, send_fpga_data_cnt);
		send_fpga_data_cnt = 0;
		if(ret<1) printf("[ENSIS]spi Write error\n");
	}
}

char *txbuf;
int packet_transfer(char *data, int len)
{
	
	struct spi_ioc_transfer *xfr;
	txbuf = malloc(len);
	//char *rxbuf = malloc(len);
	/*
	int i=0;
	printf("len=%d\n",len);
	for(i=0;i<len;i++)
	printf("%02x ",data[i]);
	printf("\n");
	*/
	memcpy(txbuf, data, len);
	//memset(rxbuf, 0xff, len);
	
	ntransfers += 1;
	spi_xfrs = realloc(spi_xfrs, sizeof(*spi_xfrs) * ntransfers);
	xfr = &spi_xfrs[ntransfers - 1];

	xfr->tx_buf = (unsigned long)txbuf;
	//xfr->rx_buf = (unsigned long)rxbuf;
	xfr->rx_buf = (unsigned long)NULL;
	xfr->len = len;
	xfr->speed_hz = fpga_speed;
	xfr->delay_usecs = fpga_delay;
	xfr->bits_per_word = fpga_bits;
	xfr->cs_change =  0;
	xfr->pad = 0;
	return ntransfers;
}

int add_transfer(char * data, int len)
{

	memcpy(send_fpga_data+send_fpga_data_cnt,data,len);
	send_fpga_data_cnt+=len;
	if(send_fpga_data_cnt>=4096)
	{
		packet_transfer(send_fpga_data, send_fpga_data_cnt);
		send_fpga_data_cnt=0;
		return 256;
	}
	return 0;
}


void transfer(void)
{
	int ret;
	
	if(ntransfers>0)
	{
		ret = ioctl(spi_draw_fd, SPI_IOC_MESSAGE(ntransfers), spi_xfrs);
		if (ret < 1) printf("can't send spi message %d\n",ret);
		else ntransfers=0;
	}
	if(send_fpga_data_cnt>0)
	{
		packet_transfer(send_fpga_data, send_fpga_data_cnt);
		ret = ioctl(spi_draw_fd, SPI_IOC_MESSAGE(ntransfers), spi_xfrs);
		if (ret < 1) printf("can't send spi message %d\n",ret);
		else ntransfers=0;
		send_fpga_data_cnt=0;
	}
}


int dnFpgaOpen(char *device, uint8_t mode, uint8_t bits, uint32_t speed, uint16_t delay)
{
	int ret = 0;
	int fd = open(device, O_RDWR);
	if (fd < 0)
	{
		printf("[ENSIS]can't open device\n");
		return fd;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't set spi mode\n");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) printf("[ENSIS]can't get spi mode\n");
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't set bits per word\n");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("[ENSIS]can't get bits per word\n");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't set max speed hz\n");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("[ENSIS]can't get max speed hz\n");

	return fd ;
}


int dnFpgaRead(int fd, char* out, int outlen, char *in, int inlen)
{
	struct spi_ioc_transfer mesg[2] = { 0, };
	uint8_t num_tr = 0;
	int ret;

	if((out != NULL) && (outlen != 0))
	{
		mesg[0].tx_buf = (unsigned long)out;
		mesg[0].rx_buf = (unsigned long)NULL;
		mesg[0].len = outlen;
		mesg[0].cs_change = 0;
		num_tr++;
	}

	if((in != NULL) && (inlen != 0))
	{
		mesg[1].tx_buf = (unsigned long)NULL;
		mesg[1].rx_buf = (unsigned long)in;
		mesg[1].len = inlen;
		num_tr++;
	}

	if(num_tr > 0)
	{
		ret= ioctl(fd, SPI_IOC_MESSAGE(num_tr), mesg);
		if(ret == 1)
		{
			return 1;
		}
	}

	return 0;
}

int dnFpgaWrite(int fd, char* out, int outlen)
{
	struct spi_ioc_transfer mesg[1] = { 0 };
	uint8_t num_tr = 0;
	int ret;

	if((out != NULL) && (outlen != 0))
	{
		mesg[0].tx_buf = (unsigned long)out;
		mesg[0].rx_buf = (unsigned long)NULL;
		mesg[0].len = outlen;
//		mesg[0].cs_change = 0;
		num_tr++;
	}

	if(num_tr > 0)
	{
		ret= ioctl(fd, SPI_IOC_MESSAGE(num_tr), mesg);
		if(ret == 1)
		{
			return 1;
		}
	}

	return 0;
}

char *get_rbf_name(int mode, int if_type)
{
	if(mode==-1 && if_type==-1)
	{
			return FPGA_VBY1_DEF_RBF;
	}

#if defined(DVI_TEMP)	// comment
	if(model_data.use_dvi)
	{
		if(if_type == IF_LVDS)
		{
			switch(mode&0xf)
			{
				case MODE_SINGLE: return FPGA_LVDS_SINGLE_DVI_RBF; 	break;
				case MODE_DUAL	: return FPGA_LVDS_DUAL_DVI_RBF; 	break;
				default			: return FPGA_LVDS_DEF_DVI_RBF; 	break;
			}
		}
		else if(if_type == IF_VBY1)
		{
			switch(mode&0xf)
			{
				case MODE_QUAD			: return FPGA_VBY1_4LANE_DVI_RBF;	break;
				case MODE_OCTA			: return FPGA_VBY1_8LANE_DVI_RBF;	break;
				case MODE_HEXA			: return FPGA_VBY1_16LANE_DVI_RBF; 	break;
				case MODE_32LANE_8x4	: return FPGA_VBY1_32LANE_DVI_RBF;	break;
				case MODE_64LANE		: return FPGA_VBY1_64LANE_DVI_RBF; break;
				case MODE_32LANE_16x2	: return FPGA_VBY1_32LANE_DVI_RBF;	break;
				default					: return FPGA_VBY1_DEF_DVI_RBF; 	break;
			}
		}
		else if(if_type == IF_DP)
		{
			switch(mode&0xf)
			{
				case MODE_QUAD	:			return FPGA_eDP_QUAD_DVI_RBF;		break;
				case MODE_OCTA	: 			return FPGA_eDP_OCTA_DVI_RBF; 		break;
				case MODE_HEXA	: 			return FPGA_eDP_HEX_DVI_RBF; 		break;
				case MODE_32LANE_8x4	: 	return FPGA_eDP_32LANE_DVI_RBF;		break;
				case MODE_32LANE_16x2	: 	return FPGA_eDP_32LANE_DVI_RBF;		break;
				default			:			return FPGA_eDP_OCTA_DVI_RBF;		break;
			}
		}
		else // if(if_type == IF_HDMI)
		{
		}
	}
#endif

	if(if_type == IF_LVDS)
	{
		switch(mode&0xf)
		{
			case MODE_SINGLE: return FPGA_LVDS_SINGLE_RBF; 	break;
			case MODE_DUAL	: return FPGA_LVDS_DUAL_RBF; 	break;
			case MODE_QUAD	:
				if(is_5k_lvds_quad())	return FPGA_LVDS_QUAD_5K_RBF;
				else 					return FPGA_LVDS_QUAD_RBF;
				break;
			default			:
				return FPGA_LVDS_DEF_RBF;
				break;
		}
	}
	else if(if_type == IF_VBY1)
	{
		switch(mode&0xf)
		{
			case MODE_QUAD	:		return FPGA_VBY1_4LANE_RBF;		break;
			case MODE_OCTA	:		return FPGA_VBY1_8LANE_RBF;		break;
			case MODE_HEXA	:		return FPGA_VBY1_16LANE_RBF; 	break;
			case MODE_32LANE_8x4 :	return FPGA_VBY1_32LANE_RBF;	break;
			case MODE_64LANE :		return FPGA_VBY1_64LANE_RBF;	break;
			case MODE_4LANEx4 :		return FPGA_VBY1_4LANEx4_RBF;	break;
			case MODE_32LANE_16x2 :	return FPGA_VBY1_32LANE_RBF;	break;
			default			:		return FPGA_VBY1_DEF_RBF;		break;
		}
	}
	else if(if_type == IF_DP)
	{
		switch(mode&0xf)
		{
			case MODE_QUAD	:			return FPGA_DP_4LANE_RBF;		break;
			case MODE_OCTA	: 			return FPGA_DP_8LANE_RBF; 		break;
			case MODE_HEXA	: 			return FPGA_DP_16LANE_RBF; 		break;
			case MODE_32LANE_8x4	: 	return FPGA_DP_32LANE_RBF; 		break;
			case MODE_32LANE_16x2	: 	return FPGA_DP_32LANE_RBF; 		break;
			default			:			return FPGA_DP_DEF_RBF;			break;
		}
	}
//	else if(if_type == IF_eDP) // probably don't need to use 'eDP'... 'DP' role eDP for now KSK 240531
//	{
//		switch(mode&0xf)
//		{
//			case MODE_QUAD	:	return FPGA_DP_4LANE_RBF;		break;
//			case MODE_OCTA	: 	return FPGA_DP_8LANE_RBF; 		break;
//			case MODE_HEXA	: 	return FPGA_DP_16LANE_RBF; 		break;
//			default			:	return FPGA_DP_DEF_RBF;			break;
//		}
//	}

	return FPGA_VBY1_DEF_RBF;
}

/*
	nCONFIG 	- gpio187
	nCE 		- gpio186
	SPI_EN 		- gpio219
	CONF_DONE 	- gpio184
	nSTATUS 	- gpio38
	SPI_LSB_FIRST
*/
int FPGA_update(int mode, int if_type)
{
	FILE 			*fp;
	unsigned int 	i, nstatus, conf_done;
	int 			fd;
	int				ret = ACK;
	const int		LIMIT_CNT = FPGA_UP_LIMIT/BURST_WRITE;
	char 			tx[BURST_WRITE];
	char			path[MAX_PATH];
	char			str[MAX_TEXT_BUF];
	int				progress;

	fd = dnFpgaOpen(FPGA_SPI_DEV0, SPI_LSB_FIRST, 8, FPGA_UP_SPEED, 0);
	if(fd<0)
	{
		fprintf(stderr, "%s device open failed!\n", FPGA_SPI_DEV0);
		return NACK;
	}

//	gpioExport(gpio79);
//	gpioSetDirection(gpio79, inputPin);
//	gpioExport(gpio38);
//	gpioSetDirection(gpio38, inputPin);
//	gpioExport(gpio194);
//	gpioSetDirection(gpio194, outputPin);

//	gpioSetValue(gpio194, low);
	gpio_set_value(gpio194, low);


	while(1){
//		gpioGetValue(gpio38, &nstatus);
		gpio_get_value(gpio38, &nstatus);
		if(!nstatus){
			//printf("nSTATUS Low\n");
			break;
		}
	}
	while(1){
//		gpioGetValue(gpio79, &conf_done);
		gpio_get_value(gpio79, &conf_done);
		if(!conf_done){
			//printf("CONF_DONE Low\n");
			break;
		}
	}
	usleep(1000);
//	gpioSetValue(gpio194, high);
	gpio_set_value(gpio194, high);
	while(1){
//		gpioGetValue(gpio38, &nstatus);
		gpio_get_value(gpio38, &nstatus);
		if(nstatus){
			//printf("nSTATUS High\n");
			break;
		}
	}

	// file loading
	memset(path, 0, MAX_PATH);
	sprintf(path, "%s%s/%s", DIR_ROOT, DIR_FPGA, get_rbf_name(mode, if_type));

	printf("I/F:%d, FPGA:%s\n", if_type, path);

	fp = fopen(path, "r");
	if(fp <= 0) {
		fprintf(stderr, "%s Not found File\n", path);
		return 0;
	}

	printf("FPGA Downloading....\n");
	i = 0;
	progress = 0;
	while(1){
		if(0==fread(&tx[0], BURST_WRITE, 1, fp)){}
		dnFpgaWrite(fd, tx, BURST_WRITE);
//		gpioGetValue(gpio79, &conf_done);
		gpio_get_value(gpio79, &conf_done);
		if(conf_done){
			printf("FPGA Update Success!!\n");
			break;
		}

		if(++i > LIMIT_CNT){
			fprintf(stderr, "FPGA Update Fail!!\n");
			ret = NACK;
			break;
		}

		if((i%100)==0)
		{
			memset(str, 0, sizeof(str));

			progress = (i*39)/(LIMIT_CNT)+10;
//			sprintf(str, "FPGA LOAD....%d%%", (i/100)+10);
//			sprintf(str, "FPGA LOAD....%d%%",  (i*39)/(LIMIT_CNT)+10);	//19-06-19 progress 10% ~ 49%
			sprintf(str, "FPGA LOAD....%d%%",  progress);	//19-06-19 progress 10% ~ 49%

			set_progress_count(progress);

//			printf("FPGA LOAD....%d\n",(i*39)/(LIMIT_CNT)+10);
			rcb_write(rcb_fd, RCB_LINE2, str);
		}
	}
	fclose(fp);
	close(fd);

//	gpioUnexport(gpio79);
//	gpioUnexport(gpio38);
//	gpioUnexport(gpio194);

	return ret;
}

//static char *key_map[] = { KEY_BWD, KEY_FWD, KEY_ONOFF, KEY_AUTOMANU, KEY_GRAY, KEY_POS, KEY_FILE, KEY_UP, KEY_ESC, KEY_MODEL, KEY_FREQ, KEY_FUNC, KEY_OK, KEY_DOWN, NULL };
static char *key_map[] = { "FF", "FF", KEY_ONOFF, KEY_AUTOMANU, KEY_FWD, KEY_BWD, "FF", "FF", "FF", "FF", "FF", "FF", "FF", "FF", NULL };
void FPGA_key_scan(void)
{
	if(spi_fd < 0) return;

	int				i;
	unsigned short 	key = FPGA_Read(FPGA_KEY);

	if( (key!=0xFFFF) && (key&FKEY_MASK) )
	{
		for(i=FKEY_BWD; i<=FKEY_DOWN; i++)
		{
			if( (key >> i) & 0x1 )
			{
//				printf("FKEY : %04X [%s]\n", key, key_map[i]);
				rcb_proc(key_map[i]);
				/*if(i>1)*/ delay_us(300000);	// 300ms
				break;
			}
		}
	}
}

int FPGA_ocp_detect(void)
{
	if(spi_fd < 0) return -1;

	unsigned short 	ocp = FPGA_Read(FPGA_VDD_OCP_DETECT);

	return (ocp & 0x1) ? NACK : ACK;
}

void FPGA_scroll_ctrl(uint16_t dir, uint16_t model_mode, uint16_t pixel)
{
	// direction
	// up	 	: 0x02(b0000 0010)
	// down		: 0x03(b0000 0011)
	// left 	: 0x04(b0000 0100)
	// right 	: 0x05(b0000 0101)

//	printf("fun scroll= dir%x		frame%d		speed%d\n", dir,scroll_frame_num,pixel);

	FPGA_Write(FPGA_SCROLL_CTRL , (dir>0) ? (dir+1) : dir);
//	FPGA_Write(FPGA_SCROLL_FRAME, 0x1);
	FPGA_Write(FPGA_SCROLL_FRAME, scroll_frame_num);
	FPGA_Write(FPGA_SCROLL_PIXEL, pixel);
}

void FPGA_pre_emphasis_default(void)
{
	FPGA_Write(FPGA_XCVR_VOD, 0x001f);
	FPGA_Write(FPGA_XCVR_1POST_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_2POST_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_1PRE_TAP, 0x0000);
	FPGA_Write(FPGA_XCVR_2PRE_TAP, 0x0000);

	printf("Set pre-emphasis default\tvod= 31, 1post= +0, 2post= +0, 1pre= +0, 2pre= +0\n");
}

//es628
void FPGA_VRR_DATA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = VrrDataWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_VRR_DATA_Write_EX(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = VrrDataExWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_VRR_GRAY_R_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = VrrGrayRWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_VRR_GRAY_G_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = VrrGrayGWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_VRR_GRAY_B_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = VrrGrayBWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_1_ON_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c1OnAddrCtrlWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_1_ON_DATA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c1OnDataWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_1_OFF_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c1OffAddrCtrlWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_1_OFF_DATA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c1OffDataWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_2_ON_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c2OnAddrCtrlWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_2_ON_DATA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c2OnDataWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_2_OFF_ADDR_CTRL_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c2OffAddrCtrlWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_I2C_2_OFF_DATA_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = I2c2OffDataWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_CTL_DATA_WRITE(unsigned char reg_addr, unsigned short data) //ksk ctl data write
{
	char tx[4] = {0,};
	tx[0] = CtlWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

unsigned short FPGA_VRR_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = VrrDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_VRR_DATA_Read_EX(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = VrrDataExRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_1_ON_ADDR_CTRL_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c1OnAddrCtrlRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_1_ON_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c1OnDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_1_OFF_ADDR_CTRL_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c1OffAddrCtrlRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_1_OFF_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c1OffDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_2_ON_ADDR_CTRL_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c2OnAddrCtrlRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_2_ON_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c2OnDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_2_OFF_ADDR_CTRL_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c2OffAddrCtrlRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_2_OFF_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2c2OffDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_I2C_RD_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = I2cRdDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}


unsigned short FPGA_CTL_DATA_READ(unsigned char reg_addr) //ksk ctl data read
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = CtlRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

/*============================== OCMD start ==============================*/
void FPGA_OCMD_1_ON_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OnTableDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_ON_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OnTableDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_ON_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OnRegDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_ON_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OnRegDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_OFF_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OffTableDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_OFF_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OffTableDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_OFF_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OffRegDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_1_OFF_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD1OffRegDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_ON_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OnTableDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_ON_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OnTableDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_ON_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OnRegDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_ON_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OnRegDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_OFF_TABLE_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OffTableDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_OFF_TABLE_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OffTableDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_OFF_REG_DATA_H_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OffRegDataHWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

void FPGA_OCMD_2_OFF_REG_DATA_L_Write(unsigned char reg_addr, unsigned short data)
{
	char tx[4] = {0,};
	tx[0] = OCMD2OffRegDataLWrite;
	tx[1] = reg_addr;
	tx[2] = (unsigned char)(data >> 8);
	tx[3] = (unsigned char)(data & 0xff);
	FPGA_SpiWrite(tx);
	fpga_write_reg[reg_addr]=data;
}

unsigned short FPGA_OCMD_1_ON_TABLE_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OnTableDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_ON_TABLE_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OnTableDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_ON_REG_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OnRegDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_ON_REG_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OnRegDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_OFF_TABLE_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OffTableDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_OFF_TABLE_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OffTableDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_OFF_REG_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OffRegDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_1_OFF_REG_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD1OffRegDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_ON_TABLE_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OnTableDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_ON_TABLE_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OnTableDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_ON_REG_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OnRegDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_ON_REG_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OnRegDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_OFF_TABLE_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OffTableDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_OFF_TABLE_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OffTableDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_OFF_REG_DATA_H_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OffRegDataHRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_2_OFF_REG_DATA_L_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMD2OffRegDataLRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_H_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMDHDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

unsigned short FPGA_OCMD_L_DATA_Read(unsigned char reg_addr)
{
	char 			tx[4] = {0,};
	char 			rx[4] = {0,};
	unsigned short 	data;

	tx[0] = OCMDLDataRead;
	tx[1] = reg_addr;
	FPGA_SpiWrite(tx);

	FPGA_SpiRead(rx);
	data = rx[2];
	data <<= 8;
	data |= rx[3];

	return data;
}

/*============================== OCMD end ==============================*/




// DP EDID test

void DP_EDID_change(void)
{
	unsigned short dp_freq;

	dp_freq = (model_data.freq/(model_data.h_total*model_data.v_total));

	FPGA_Write(FPGA_DP_H_TOTAL,		model_data.h_total);
	FPGA_Write(FPGA_DP_H_ACTIVE,	model_data.h_active);
	FPGA_Write(FPGA_DP_H_BPORCH,	model_data.h_bpo);
	FPGA_Write(FPGA_DP_H_WIDTH,		model_data.h_width);
	FPGA_Write(FPGA_DP_V_TOTAL,		model_data.v_total);
	FPGA_Write(FPGA_DP_V_ACTIVE,	model_data.v_active);
	FPGA_Write(FPGA_DP_V_BPORCH,	model_data.v_bpo);
	FPGA_Write(FPGA_DP_V_WIDTH,		model_data.v_width);
	FPGA_Write(FPGA_DP_HZ,			dp_freq);


	printf("DP h_total		: %d \n", FPGA_Read(FPGA_DP_H_TOTAL));
	printf("DP h_active		: %d \n", FPGA_Read(FPGA_DP_H_ACTIVE));
	printf("DP h_bpo		: %d \n", FPGA_Read(FPGA_DP_H_BPORCH));
	printf("DP h_width		: %d \n", FPGA_Read(FPGA_DP_H_WIDTH));
	printf("DP v_total		: %d \n", FPGA_Read(FPGA_DP_V_TOTAL));
	printf("DP v_active		: %d \n", FPGA_Read(FPGA_DP_V_ACTIVE));
	printf("DP v_bpo		: %d \n", FPGA_Read(FPGA_DP_V_BPORCH));
	printf("DP v_width		: %d \n", FPGA_Read(FPGA_DP_V_WIDTH));
	printf("DP hz			: %d \n", FPGA_Read(FPGA_DP_HZ));
}
