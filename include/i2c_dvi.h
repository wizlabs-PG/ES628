/*
 * i2c_dvi.h
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#ifndef I2C_DVI_H_
#define I2C_DVI_H_

#include <global.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>

// I2C Settings
#if !defined(DEV_MEM)
#define DEV_MEM					"/dev/mem"
#endif
#if !defined(DEV_I2C)
#define DEV_I2C					"/dev/i2c-1"
#endif
#if !defined(I2C_ADDRESS)
#define I2C_ADDRESS				0x7000C000
#endif
#if !defined(MAP_SIZE)
#define MAP_SIZE 				0xFF
#endif
#if !defined(VIDIA_I2C_DIV)
#define NVIDIA_I2C_DIV 			(*((volatile unsigned int*)(map_base+0x6C)))
#endif

#define	I2C_EEPROM_FREQ			100		// KHz
#define EDID_E2P_ADDR			(0xa0 >> 1)
#define MAX_EDID_CNT			128

typedef struct
{
	char	*dev;	// device file i.e /dev/i2c-N
	int		addr;	// i2c address
	int		fd;		// file descriptor
	int		type;	// eeprom type
} eeprom_t;

typedef enum
{
	EEPROM_TYPE_UNKNOWN = 0,
	EEPROM_TYPE_8BIT_ADDR,
	EEPROM_TYPE_16BIT_ADDR
} enum_eeprom_type_t;


extern int 		i2c_eeprom_open(uint8_t addr, eeprom_t *e);
extern int 		i2c_eeprom_read_byte(eeprom_t *e, uint16_t mem_addr);
extern int 		i2c_eeprom_write_byte(eeprom_t *e, uint16_t mem_addr, uint8_t data);
extern void 	i2c_eeprom_close(eeprom_t *e);

extern void 	write_edid_to_eeprom(uint8_t *edid);
extern void 	read_edid_to_eeprom(void);

#endif /* I2C_DVI_H_ */
