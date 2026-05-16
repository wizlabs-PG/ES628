/*
 * i2c_sensing.h
 *
 *  Created on: Feb 12, 2016
 *      Author: root
 */

#ifndef I2C_SENSING_H_
#define I2C_SENSING_H_

#include <global.h>
#include <linux/i2c-dev.h>

// I2C Settings
#if !defined(DEV_MEM)
#define DEV_MEM					"/dev/mem"
#endif
#if !defined(DEV_I2C)
#define DEV_I2C					"/dev/i2c-2"
#endif
#if !defined(I2C_ADDRESS)
#define I2C_ADDRESS				0x7000C000
#endif
#if !defined(MAP_SIZE)
#define MAP_SIZE 				0xFF
#endif
#if !defined(NVIDIA_I2C_DIV)
#define NVIDIA_I2C_DIV 			(*((volatile unsigned int*)(map_base+0x6C)))
#endif

#define I2C_LIMIT_SET_ADDR		0x28
#define I2C_READ_IDD_ADDR		0x48
#define	I2C_IDD_FREQ			25		// KHz
#define I2C_IDD_SENSING_TIME	1000	// ms

#define IDD_SET_CMD				0xaa
#define IDD_READ_CMD			0x00
#define IDD_SET_OFFSET			100
//#define IDD_READ_OFFSET			0x120
#define IDD_READ_OFFSET			0xe0

#define MODEL_TURN				0
#define TIME_TURN				1
#define VDD_TURN				2
#define IDD_TURN				3
#define VBL_TURN				4
#define IBL_TURN				5
#define DIM_TURN				6
#define VSYNC_TURN				7

#define	MAX_ROLL				7

// Variables
int				i2c_idd_fd;
unsigned int	idd_value, vdd_value, ibl_value, vbl_value;
int				rcb_display_roll;
uint64_t		aging_time;	// sec

// Functions
extern int		i2c_idd_limit_set(uint32_t mili_amp);
extern uint32_t	i2c_read_idd(void);
extern void 	i2c_idd_sensing(void);

#endif /* I2C_SENSING_H_ */
