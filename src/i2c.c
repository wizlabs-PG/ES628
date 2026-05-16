
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <sys/ioctl.h>
#include <i2c.h>
#include <sys/mman.h>


int i2cOpen(int bus, int address, int freq)
{
	char fileNameBuffer[32];
//	void *map_base;
//	char str_cmd[128];

/*
	int mem = open("/dev/mem", O_RDWR|O_SYNC);
	if(mem==-1)
	{
		printf("mem error\n");
		return 0;
	}

	if(bus==0)
	{
		map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem, I2C_ADDRESS);
		if(map_base==MAP_FAILED)
		{
			printf("map_base error\n");
			return 0;
		}
		NVIDIA_I2C_DIV = (((10200/freq)<<16)&0xFFFF0000)|0x00000002;
		munmap(map_base, MAP_SIZE);
	}
	else if(bus==1)
	{
		map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem, I2C_ADDRESS);
		if(map_base==MAP_FAILED)
		{
			printf("map_base error\n");
			return 0;
		}
		NVIDIA_I2C_DIV = (((10200/freq)<<16)&0xFFFF0000)|0x00000002;
		munmap(map_base, MAP_SIZE);
	}
	else if(bus==2)
	{
		map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem, I2C2_ADDRESS);
		if(map_base==MAP_FAILED)
		{
			printf("map_base error\n");
			return 0;
		}
		NVIDIA_I2C_DIV = (((10200/freq)<<16)&0xFFFF0000)|0x00000002;
		munmap(map_base, MAP_SIZE);
	}
*/
//	sprintf(str_cmd,"echo %d > /sys/bus/i2c/devices/i2c-%d/bus_clk_rate",freq*1000,bus);
//	system(str_cmd);


	sprintf(fileNameBuffer, "/dev/i2c-%d", bus);
	int fd = open(fileNameBuffer, O_RDWR);
	if (fd < 0) {
		printf("i2c fd open error\n");
		return -1;
	}

	if (ioctl(fd, I2C_SLAVE, address) < 0) {
		printf("i2c slave addr error\n");
		return -1;
	}

//	close(mem);

	return fd ;
}

void i2cClose(int *fd)
{
   if (*fd > 0) {
       close(*fd);
   }
}
/*
int i2cWrite(int *fd, unsigned char writeValue)
{
	return i2c_smbus_write_byte(*fd, writeValue);	
}

int i2cRead(int *fd)
{
	return i2c_smbus_read_byte(*fd);	
}
*/
int i2cReadByte(int fd, unsigned char command, unsigned char *readValue)
{
	return i2c_smbus_read_i2c_block_data(fd, command, 1, readValue);
}

int i2cWriteByte(int fd, unsigned char command, unsigned char writeValue)
{
	return i2c_smbus_write_i2c_block_data(fd, command, 1, &writeValue);
}
/*
int i2cReadData(int fd, unsigned char command, unsigned char len, unsigned char *readValue)
{
	return i2c_smbus_read_i2c_block_data(fd, command, len, readValue);
}

int i2cWriteData(int fd, unsigned char command, unsigned char len, unsigned char *writeValue)
{
	return i2c_smbus_write_i2c_block_data(fd, command, len, writeValue);
}
*/


