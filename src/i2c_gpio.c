/*
 * i2c_gpio.c
 *
 *  Created on: Oct 18, 2017
 *      Author: root
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <i2c_gpio.h>
#include <gpio.h>
#include <i2c.h>

int i2c_gpio_fd0 = 0;
int i2c_gpio_fd1 = 0;
int i2c_gpio_fd2 = 0;

uint32_t value_data=0, value_data_b=0;

void i2c_gpio_init()
{
	i2c_gpio_fd0 = i2cOpen(GPIO_I2C_BUS, GPIO_I2C_ADDRESS0, GPIO_I2C_FREQ);
	i2c_gpio_fd1 = i2cOpen(GPIO_I2C_BUS, GPIO_I2C_ADDRESS1, GPIO_I2C_FREQ);
	i2c_gpio_fd2 = i2cOpen(GPIO_I2C_BUS, GPIO_I2C_ADDRESS2, GPIO_I2C_FREQ);

	i2c_gpio_set_direction(0x000080ff); // IP S/W In-Pin
	//i2c_gpio_set_value(0);
	i2c_gpio_set_direction_b(0x00000000);	//GPIO 0x77

	value_data		= i2c_gpio_get_value();
	value_data_b	= i2c_gpio_get_value_b();

	i2c_gpio_set(GPIO_EX18, 0x01);	// vdd off(default)
	i2c_gpio_set(GPIO_EX20, 0x01);	// DDC_EP_EN1 - HIGH (eeprom dvi selection)

	i2c_gpio_set(GPIO_EX33, 0x00);
	i2c_gpio_set(GPIO_EX32, 0x00);
}

void i2c_gpio_set_direction(uint32_t out_flag)
{
	i2cWriteByte(i2c_gpio_fd0, 0x06, (uint8_t)(out_flag&0x000000ff));
	i2cWriteByte(i2c_gpio_fd0, 0x07, (uint8_t)(((out_flag&0x0000ff00))>>8)&0xff);
	i2cWriteByte(i2c_gpio_fd1, 0x06, (uint8_t)(((out_flag&0x00ff0000))>>16)&0xff);
	i2cWriteByte(i2c_gpio_fd1, 0x07, (uint8_t)(((out_flag&0xff000000)>>24)&0xff));
}

void i2c_gpio_set_direction_b(uint32_t out_flag)
{
	i2cWriteByte(i2c_gpio_fd2, 0x06, (uint8_t)(out_flag&0x000000ff));
	i2cWriteByte(i2c_gpio_fd2, 0x07, (uint8_t)(((out_flag&0x0000ff00)>>8)&0xff));
}

void i2c_gpio_set_value(uint32_t value)
{
	value_data = value;
	i2cWriteByte(i2c_gpio_fd0, 0x02, (uint8_t)(value&0x000000ff));
	i2cWriteByte(i2c_gpio_fd0, 0x03, (uint8_t)(((value&0x0000ff00))>>8)&0xff);
	i2cWriteByte(i2c_gpio_fd1, 0x02, (uint8_t)(((value&0x00ff0000))>>16)&0xff);
	i2cWriteByte(i2c_gpio_fd1, 0x03, (uint8_t)(((value&0xff000000)>>24)&0xff));
}

void i2c_gpio_set(uint8_t gpio, uint8_t level)
{
	if(gpio<GPIO_EX32)
	{
		if(level) 	value_data |=  (0x00000001<<gpio);
		else 		value_data &= ~(0x00000001<<gpio);

		i2cWriteByte(i2c_gpio_fd0, 0x02, (uint8_t)(value_data&0x000000ff));
		i2cWriteByte(i2c_gpio_fd0, 0x03, (uint8_t)(((value_data&0x0000ff00)>>8)&0xff));
		i2cWriteByte(i2c_gpio_fd1, 0x02, (uint8_t)(((value_data&0x00ff0000)>>16)&0xff));
		i2cWriteByte(i2c_gpio_fd1, 0x03, (uint8_t)(((value_data&0xff000000)>>24)&0xff));
	}
	else
	{
		if(level) 	value_data_b |=  (0x00000001<<(gpio-GPIO_EX32));
		else 		value_data_b &= ~(0x00000001<<(gpio-GPIO_EX32));

		i2cWriteByte(i2c_gpio_fd2, 0x02, (uint8_t)(value_data_b&0x000000ff));
		i2cWriteByte(i2c_gpio_fd2, 0x03, (uint8_t)(((value_data_b&0x0000ff00)>>8)&0xff));
	}
}

uint32_t i2c_gpio_get_value()
{
	uint8_t 	read_value=0;
	uint32_t 	read_value_data=0;

	i2cReadByte(i2c_gpio_fd0, 0x00, &read_value);
	read_value_data |= (uint32_t )(read_value&0x000000ff);

	i2cReadByte(i2c_gpio_fd0, 0x01, &read_value);
	read_value_data |= (uint32_t )((read_value<<8)&0x0000ff00);

	i2cReadByte(i2c_gpio_fd1, 0x00, &read_value);
	read_value_data |= (uint32_t )((read_value<<16)&0x00ff0000);

	i2cReadByte(i2c_gpio_fd1, 0x01, &read_value);
	read_value_data |= (uint32_t )((read_value<<24)&0xff000000);

	return read_value_data;
}

uint32_t i2c_gpio_get_value_b(void)
{
	uint8_t 	read_value=0;
	uint32_t 	read_value_data=0;

	i2cReadByte(i2c_gpio_fd2, 0x00, &read_value);
	read_value_data |= (uint32_t )(read_value&0x000000ff);

	i2cReadByte(i2c_gpio_fd2, 0x01, &read_value);
	read_value_data |= (uint32_t )((read_value<<8)&0x0000ff00);

	return read_value_data;
}

void i2c_gpio_close()
{
	if(i2c_gpio_fd0>0)
		i2cClose(&i2c_gpio_fd0);

	if(i2c_gpio_fd0>0)
		i2cClose(&i2c_gpio_fd1);

	if(i2c_gpio_fd2>0)
		i2cClose(&i2c_gpio_fd2);
}

uint8_t i2c_gpio_get_ipsw(void)
{
	uint8_t 	read_value=0;

	i2cReadByte(i2c_gpio_fd0, 0x00, &read_value);

	return read_value;
}

void i2c_ddc_sel(int level)
{

//	if(level)
//	{
//		i2c_gpio_set(GPIO_EX19, (uint8_t)low);	// DDC_EP_EN0 - LOW
//		i2c_gpio_set(GPIO_EX20, (uint8_t)low);	// DDC_EP_EN1 - LOW
//
//		i2c_gpio_set(GPIO_EX20, (uint8_t)high);	// DDC_EP_EN1 - HIGH
//	}
//	else
//	{
//		i2c_gpio_set(GPIO_EX19, (uint8_t)low);	// DDC_EP_EN0 - LOW
//		i2c_gpio_set(GPIO_EX20, (uint8_t)low);	// DDC_EP_EN1 - LOW
//
//		i2c_gpio_set(GPIO_EX19, (uint8_t)high);	// DDC_EP_EN0 - HIGH
//	}

	if( level == EDID_SII9135_I2C_EN)
	{
		i2c_gpio_set(GPIO_EX20, (uint8_t)low);	// DDC_EP_EN1 - LOW
		i2c_gpio_set(GPIO_EX19, (uint8_t)high);	// DDC_EP_EN0 - HIGH
	}
	else if( level == HDMI_I2C_EN)
	{
		i2c_gpio_set(GPIO_EX19, (uint8_t)low);	// DDC_EP_EN0 - LOW
		i2c_gpio_set(GPIO_EX20, (uint8_t)high);	// DDC_EP_EN1 - HIGH
	}
	else {
		i2c_gpio_set(GPIO_EX19, (uint8_t)low);	// DDC_EP_EN0 - LOW
		i2c_gpio_set(GPIO_EX20, (uint8_t)low);	// DDC_EP_EN1 - LOW
	}

	usleep(200000);
}

void i2c_hdmi_hpd_set(int level)
{
	// high : enable
	// low	: disable
	i2c_gpio_set(GPIO_EX24, (uint8_t)level);
}

void i2c_pg_485_set(int level)
{
	// 0: rx(init)
	// 1: tx
	i2c_gpio_set(GPIO_EX25, (uint8_t)level);
}

void i2c_pwr_485_set(int level)
{
	// 0: rx(init)
	// 1: tx
	//printf("485_level(GPIO_EX26) = %s\n", (level==0) ? "low" : "high");
	i2c_gpio_set(GPIO_EX26, (uint8_t)level);
	usleep(1000);
}
