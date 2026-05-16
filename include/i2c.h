
#ifndef _I2C_H
#define _I2C_H

#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#if !defined(MAP_SIZE)
#define MAP_SIZE 		0x00ff
#endif

#if !defined(I2C_ADDRESS)
#define I2C_ADDRESS 	0x7000C000
#endif

#define NVIDIA_I2C_DIV 	(*((volatile unsigned int*)(map_base+0x6C)))

inline __s32 i2c_smbus_access(int file, char read_write, __u8 command, int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

inline __s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data))
			return -1;
		else
			return 0x0FF & data.byte;
}

inline __s32 i2c_smbus_write_byte(int file, __u8 value)
{
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, 0, I2C_SMBUS_BYTE, NULL);
}

inline __s32 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data))
			return -1;
		else
			return 0x0FF & data.byte;
}

inline __s32 i2c_smbus_write_byte_data(int file, __u8 command, __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
}

inline __s32 i2c_smbus_read_word_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data))
			return -1;
		else
			return 0x0FFFF & data.word;
}

inline __s32 i2c_smbus_write_word_data(int file, __u8 command, __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file, I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
}



inline __s32 i2c_smbus_write_i2c_block_data(int file, __u8 command, __u8 length, const __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command, I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

inline __s32 i2c_smbus_read_i2c_block_data(int file, __u8 command, __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;

	if (length > 32)
		length = 32;
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_READ,command, length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN : I2C_SMBUS_I2C_BLOCK_DATA,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

/* Returns the number of read bytes */
inline __s32 i2c_smbus_block_process_call(int file, __u8 command, __u8 length, __u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	for (i = 1; i <= length; i++)
		data.block[i] = values[i-1];
	data.block[0] = length;
	if (i2c_smbus_access(file,I2C_SMBUS_WRITE,command, I2C_SMBUS_BLOCK_PROC_CALL,&data))
		return -1;
	else {
		for (i = 1; i <= data.block[0]; i++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

int 	i2cOpen(int bus, int address, int freq);
void 	i2cClose(int *fd);
int 	i2cWrite(int *fd,unsigned char writeValue);
int 	i2cRead(int *fd);
int 	i2cReadData(int fd, unsigned char command,unsigned char len,unsigned char *readValue);
int 	i2cWriteData(int fd, unsigned char command,unsigned char len,unsigned char *writeValue);
int 	i2cReadByte(int fd, unsigned char command,unsigned char *readValue);
int 	i2cWriteByte(int fd, unsigned char command,unsigned char writeValue);
#endif
