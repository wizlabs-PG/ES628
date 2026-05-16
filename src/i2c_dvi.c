/*
 * i2c_dvi.c
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#include <linux/i2c.h>
#include <i2c_dvi.h>
#include <gpio.h>
#include <i2c_gpio.h>
#include <i2c.h>

#define CHECK_I2C_FUNC(var, label) 		\
	do { if(0==(var&label)){			\
			fprintf(stderr, "\nerror: " \
					#label " function is required. program halted.\n\n"); \
			exit(1); } \
	} while(0);

int i2c_eeprom_open(uint8_t addr, eeprom_t *e)
{
//	int 			mem_fd;
	int				eeprom_fd;
//	void			*map_base;
	unsigned long	funcs;
//	char			str_cmd[128];

/*
	mem_fd = open(DEV_MEM, O_RDWR|O_SYNC);
	if(mem_fd == (-1))
	{
		fprintf(stderr, "[I2C EEPROM] mem open() error!\n");
		return 0;
	}

	map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, I2C_ADDRESS);
	if(map_base == MAP_FAILED)
	{
		fprintf(stderr, "[I2C EEPROM] map_base error!\n");
		return 0;
	}
	NVIDIA_I2C_DIV = (((10200/I2C_EEPROM_FREQ)<<16)&0xFFFF0000)|0x00000002;
	munmap(map_base, MAP_SIZE);
	close(mem_fd);
	////////////////////////////////////////////////////////////////
*/
//	sprintf(str_cmd,"echo %d > /sys/bus/i2c/devices/i2c-%d/bus_clk_rate",I2C_EEPROM_FREQ*1000,1);
//	system(str_cmd);

	eeprom_fd = open(DEV_I2C, O_RDWR);
	if (eeprom_fd < 0)
	{
		fprintf(stderr, "[I2C EEPROM] i2c_idd_fd open() error!\n");
		return 0;
	}

	if (ioctl(eeprom_fd, I2C_FUNCS, &funcs) < 0)
	{
		fprintf(stderr, "[I2C EEPROM] i2c_idd_fd ioctl(I2C_FUNCS) error!\n");
		return 0;
	}

	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_WORD_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_WORD_DATA);

	if (ioctl(eeprom_fd, I2C_SLAVE, addr) < 0)
	{
		fprintf(stderr, "[I2C EEPROM] i2c_idd_fd ioctl(I2C_SLAVE) error!\n");
		return 0;
	}

	e->fd	= eeprom_fd;
	e->addr	= addr;
	e->dev	= DEV_I2C;
	e->type	= EEPROM_TYPE_8BIT_ADDR;

	return 1;
}

static int i2c_write_1b(eeprom_t *e, uint8_t buf)
{
	int ret_val = i2c_smbus_write_byte(e->fd, buf);
	if(ret_val<0)
	{
		fprintf(stderr, "[I2C EEPROM] i2c_write_1b() %s\n", strerror(errno));
	}
	usleep(10);

	return ret_val;
}

static int i2c_write_2b(eeprom_t *e, uint8_t *buf)
{
	int ret_val;
	int time_out = 0;

	while( (ret_val = i2c_smbus_write_byte_data(e->fd, buf[0], buf[1]))<0 )
	{
		time_out++;
		if(time_out>500)
		{
			fprintf(stderr, "[I2C EEPROM] i2c_write_2b() %s\n", strerror(errno));
		}
		else usleep(10);
	}

	return ret_val;
}

static int i2c_write_3b(eeprom_t *e, uint8_t *buf)
{
	int ret_val = i2c_smbus_write_word_data(e->fd, buf[0], (buf[2]<<8) | buf[1]);
	if(ret_val<0)
	{
		fprintf(stderr, "[I2C EEPROM] i2c_write_3b() %s\n", strerror(errno));
	}
	usleep(10);

	return ret_val;
}

int i2c_eeprom_read_byte(eeprom_t *e, uint16_t mem_addr)
{
	int ret_val;

	ioctl(e->fd, BLKFLSBUF);

	if(e->type == EEPROM_TYPE_8BIT_ADDR)
	{
		uint8_t buf = mem_addr & 0x0ff;
		ret_val = i2c_write_1b(e, buf);
		if(ret_val<0) return ret_val;
	}
	else if(e->type == EEPROM_TYPE_16BIT_ADDR)
	{
		uint8_t buf[2] = { (mem_addr>>8)&0x0ff, mem_addr&0x0ff };
		ret_val = i2c_write_2b(e, buf);
		if(ret_val<0) return ret_val;
	}
	else
	{
		fprintf(stderr, "[I2C EEPROM] unknown eeprom type\n");
		return -1;
	}

	ret_val = i2c_smbus_read_byte(e->fd);

	return ret_val;
}

int i2c_eeprom_write_byte(eeprom_t *e, uint16_t mem_addr, uint8_t data)
{
	if(e->type == EEPROM_TYPE_8BIT_ADDR)
	{
		uint8_t buf[2] = { mem_addr & 0x00ff, data };
		return i2c_write_2b(e, buf);
	}
	else if(e->type == EEPROM_TYPE_16BIT_ADDR)
	{
		uint8_t buf[3] = { (mem_addr>>8) & 0x00ff, mem_addr&0x00ff, data };
		return i2c_write_3b(e, buf);
	}
	else
	{
		fprintf(stderr, "[I2C EEPROM] unknown eeprom type\n");
		return -1;
	}
}

void i2c_eeprom_close(eeprom_t *e)
{
	close(e->fd);

	e->fd	= -1;
	e->dev	= 0;
	e->type	= EEPROM_TYPE_UNKNOWN;
}

void write_edid_to_eeprom(uint8_t *edid)
{
	int 		i;
	eeprom_t 	epr;

	i2c_ddc_sel(EDID_SII9135_I2C_EN);

	if( 0 == i2c_eeprom_open(EDID_E2P_ADDR, &epr) )
		return;

	usleep(10000);

	printf("= write edid to eeprom ================\n");
	for(i=0; i<MAX_EDID_CNT; i++)
	{
		i2c_eeprom_write_byte(&epr, i, edid[i]);
		printf("%02X ", edid[i]);
		if( ((i+1)%8)==0 ) printf("\n");
	}
	printf("=======================================\n");

	i2c_eeprom_close(&epr);
	i2c_ddc_sel(HDMI_I2C_EN);
}

void read_edid_to_eeprom(void)
{
	int 		i;
	eeprom_t 	epr;

	i2c_ddc_sel(EDID_SII9135_I2C_EN);

	if( 0 == i2c_eeprom_open(EDID_E2P_ADDR, &epr) )
		return;

	printf("= read edid from eeprom ===============\n");
	for(i=0; i<MAX_EDID_CNT; i++)
	{
		printf("%02X ", i2c_eeprom_read_byte(&epr, i));
		if( ((i+1)%8)==0 ) printf("\n");
	}
	printf("=======================================\n");

	i2c_eeprom_close(&epr);
	i2c_ddc_sel(HDMI_I2C_EN);
}











