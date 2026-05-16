

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <i2c_gpio.h>
#include <gpio.h>
#include <i2c_sii9135.h>
#include <i2c.h>


int i2c_sii9135_open(uint8_t chip_addr)
{
//	int 		mem_fd;
//	void		*map_base;
	char		str_cmd[128];
/*
	mem_fd = open(DEV_MEM, O_RDWR|O_SYNC);
	if(mem_fd == (-1))
	{
		fprintf(stderr, "[I2C SII9135] mem open() error!\n");
		return 0;
	}

	map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, I2C_ADDRESS);
	if(map_base == MAP_FAILED)
	{
		fprintf(stderr, "[I2C SII9135] map_base error!\n");
		return 0;
	}
	NVIDIA_I2C_DIV = (((10200/I2C_SII9135_FREQ)<<16)&0xFFFF0000)|0x00000002;
	munmap(map_base, MAP_SIZE);
	close(mem_fd);
*/
	////////////////////////////////////////////////////////////////

//	sprintf(str_cmd,"echo %d > /sys/bus/i2c/devices/i2c-%d/bus_clk_rate",I2C_SII9135_FREQ*1000,1);
//	system(str_cmd);

	sii9135_fd = open(DEV_I2C, O_RDWR);
	if (sii9135_fd < 0)
	{
		fprintf(stderr, "[I2C SII9135] sii9135_fd open() error!\n");
		return 0;
	}

	if (ioctl(sii9135_fd, I2C_SLAVE, chip_addr) < 0)
	{
		fprintf(stderr, "[I2C SII9135] sii9135_fd ioctl() error!\n");
		return 0;
	}
	printf("i2c sii9135 open\n");
	return 1;
}

void i2c_sii9135_close(void)
{
	if(sii9135_fd>=0)
	{
		close(sii9135_fd);
		printf("i2c sii9135 close\n");
	}
}

int sii9135_init(void)
{
	i2c_ddc_sel(EDID_SII9135_I2C_EN);

	i2c_gpio_set(GPIO_EX8, 0x00);
	usleep(200000);
	i2c_gpio_set(GPIO_EX8, 0x01);

	usleep(100000);
	if(0 == i2c_sii9135_open(SII9135_ADDR))
	{
		return 0;
	}

//	i2cWriteByte(i2c_sil9135, 0x08, 0x07);	//24bit, Invert clock, Normal operation
//	i2cWriteByte(i2c_sil9135, 0x09, 0x91);	//Enable DDC, Enable Port0, Disable Port1
//	i2cWriteByte(i2c_sil9135, 0x4a, 0x00);	//
//	i2cWriteByte(i2c_sil9135, 0x48, 0x01);
//	i2cWriteByte(i2c_sil9135, 0x5f, 0x00);	//Digital RGB output
//	i2cWriteByte(i2c_sil9135, 0x81, 0x20);
//	i2cWriteByte(i2c_sil9135, 0xd6, 0x10);	//Receiver is in HDMI mode



	sii9135_write8(0x08, 0x07);	//24bit, normal clock, Normal operation
	sii9135_write8(0x09, 0x91);	//Enable DDC, Enable Port0, Disable Port1
	sii9135_write8(0x4a, 0x00);		// video mode #1
	sii9135_write8(0x48, 0x01);		// video control
	sii9135_write8(0x5f, 0x00);		// output rgb
	sii9135_write8(0x81, 0x20);		// TMDS analog core 8bit 1 clock
	sii9135_write8(0xd6, 0x10);		// Downstream repeater


//	sii9135_write8(0x5f, 0x00);		// output rgb
//	sii9135_write8(0x4a, 0x80);		// video mode #1
////	sii9135_write8(0x48, 0x01);		// video control
////	sii9135_write8(0x81, 0x20);		// TMDS analog core 8bit 1 clock
//	sii9135_write8(0xd6, 0x10);		// Downstream repeater
//	sii9135_write8(0x08, 0x07);	//24bit, normal clock, Normal operation
//	sii9135_write8(0x09, 0x91);	//Enable DDC, Enable Port0, Disable Port1

	i2c_sii9135_close();
	i2c_ddc_sel(HDMI_I2C_EN);

	return 1;
}

void sii9135_write8(uint8_t addr, uint8_t data)
{
	if(0>i2c_smbus_write_i2c_block_data(sii9135_fd, addr, 1, &data))
	{
//		printf("LCD write error\n");
	}
}


uint32_t sii9135_read(uint8_t cmd)
{
	uint8_t 	read_val[5];
	uint16_t	tmp=0;
//	uint32_t	ret_val = 0;

	memset(read_val, 0, sizeof(read_val));
	if(0 > i2c_smbus_read_i2c_block_data(sii9135_fd, cmd, 1, read_val))
	{
		fprintf(stderr, "[I2C IDD] i2c_read_idd() error!\n");
//		close(sii9135_fd);
		return 0;
	}

	tmp = ((read_val[0]&0x0f)<<12) + (read_val[1]<<4) + ((read_val[2]&0xf0)>>4);

	return read_val[0];
}


int sii9135_video_in_reg_read(void)
{
	i2c_ddc_sel(EDID_SII9135_I2C_EN);
	if(0 == i2c_sii9135_open(SII9135_ADDR))
	{
		return 0;
	}
	usleep(200);
	printf("H RES L		= %02x\n", sii9135_read(0x3a));
	printf("H RES H		= %02x\n", sii9135_read(0x3B));
	printf("V RES L		= %02x\n", sii9135_read(0x3c));
	printf("V RES H		= %02x\n", sii9135_read(0x3d));
	printf("DE PIX L	= %02x\n", sii9135_read(0x4e));
	printf("DE PIX H	= %02x\n", sii9135_read(0x4f));
	printf("DE LINE L	= %02x\n", sii9135_read(0x50));
	printf("DE LINE H	= %02x\n", sii9135_read(0x51));
	printf("VID VTAVL	= %02x\n", sii9135_read(0x52));
	printf("VID VFP		= %02x\n", sii9135_read(0x53));
	printf("VID STAT	= %02x\n", sii9135_read(0x55));
	printf("VID HFP1	= %02x\n", sii9135_read(0x59));
	printf("VID HFP2	= %02x\n", sii9135_read(0x5a));
	printf("VID HSWIDL	= %02x\n", sii9135_read(0x5b));
	printf("VID HSWIDH	= %02x\n", sii9135_read(0x5c));
	printf("VID XPCNT1	= %02x\n", sii9135_read(0x6e));
	printf("VID XPCNT2	= %02x\n", sii9135_read(0x6f));

	printf("0x08		= %02x\n", sii9135_read(0x08));
	printf("0x09		= %02x\n", sii9135_read(0x09));
	printf("0x4a		= %02x\n", sii9135_read(0x4a));
	printf("0x48		= %02x\n", sii9135_read(0x48));
	printf("0x5f		= %02x\n", sii9135_read(0x5f));
	printf("0x81		= %02x\n", sii9135_read(0x81));
	printf("0xd6		= %02x\n", sii9135_read(0xd6));

	usleep(200);
	i2c_sii9135_close();
	i2c_ddc_sel(HDMI_I2C_EN);
	return 1;
}
/*
void sil9135_init(void)
{
	*R_P1oLOW = (1<<0);		//DVI_RESET_C
	delayms(100);
	*R_P1oHIGH = (1<<0);

	sil9135_reg_write();
}

void sil9135_id_read(void)
{
	gp.i2c_sel = I2C_HDMI;
	printf("ID = SIL%x%x\r\n", i2c_read_byte(SIL9135_ADDR, 0x03), i2c_read_byte(SIL9135_ADDR, 0x02));
}

void sil9135_reg_write(void)
{
	gp.i2c_sel = I2C_HDMI;

	i2c_write_byte(SIL9135_ADDR, 0x5f, 0xc0);
	i2c_write_byte(SIL9135_ADDR, 0x4a, 0x9a);
	i2c_write_byte(SIL9135_ADDR, 0x48, 0x01);
	i2c_write_byte(SIL9135_ADDR, 0x81, 0x20);
	i2c_write_byte(SIL9135_ADDR, 0xd6, 0x10);
	i2c_write_byte(SIL9135_ADDR, 0x08, 0x07);	//24bit, Invert clock, Normal operation
	i2c_write_byte(SIL9135_ADDR, 0x09, 0x91);	//Enable DDC, Enable Port0, Disable Port1

	//audio register
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x00, 0x00);
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x02, 0x52);
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x26, 0x40);
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x27, 0xfd);
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x29, 0x1c);
	i2c_write_byte(SIL9135_AUDIO_ADDR, 0x32, 0x00);
}


void sil9135_reg_read(void)
{
	gp.i2c_sel = I2C_HDMI;

	printf("System Status Register BIT0:SYNC DETECT, BIT1:CLOCK DETECT\r\n");
	printf("-SII9135--------------------------------------------------\r\n");
	printf("REG[06] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x06));
	printf("----------------------------------------------------------\r\n");
	printf("REG[08] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x08));
	printf("REG[09] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x09));
	printf("REG[2E] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x2E));
	printf("REG[2F] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x2F));
	printf("REG[30] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x30));
	printf("REG[31] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x31));
	printf("REG[48] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x48));
	printf("REG[49] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x49));
	printf("REG[4A] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x4A));
	printf("REG[5F] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x5F));
	printf("REG[81] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0x81));
	printf("REG[D6] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0xD6));
	printf("REG[B5] = %02x\r\n", i2c_read_byte(SIL9135_ADDR, 0xB5));
	printf("----------------------------------------------------------\r\n");
	printf("Audio Registers\r\n");
	printf("----------------------------------------------------------\r\n");
	printf("REG[00] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x00));
	printf("REG[02] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x02));
	printf("REG[26] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x26));
	printf("REG[27] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x27));
	printf("REG[28] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x28));
	printf("REG[29] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x29));
	printf("\r\n\r\n");
}

void sil9135_reg_read_all(void)
{
	int i;

	gp.i2c_sel = I2C_HDMI;

	printf("System Status Register BIT0:SYNC DETECT, BIT1:CLOCK DETECT\r\n");
	printf("-SII9135--------------------------------------------------\r\n");
	for(i=0; i<0xff; i++){
		printf("REG[%02x] = %02x\r\n", i, i2c_read_byte(SIL9135_AUDIO_ADDR, i));
	}
	printf("----------------------------------------------------------\r\n");

	printf("----------------------------------------------------------\r\n");
	printf("Audio Registers\r\n");
	printf("----------------------------------------------------------\r\n");
	printf("REG[00] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x00));
	printf("REG[02] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x02));
	printf("REG[26] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x26));
	printf("REG[27] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x27));
	printf("REG[28] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x28));
	printf("REG[29] = %02x\r\n", i2c_read_byte(SIL9135_AUDIO_ADDR, 0x29));
	printf("\r\n\r\n");
}
*/
