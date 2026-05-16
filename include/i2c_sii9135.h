
#include <global.h>
#include <linux/i2c-dev.h>

#define SII9135_ADDR 		(0x60 >> 1)
#define SII9135_AUDIO_ADDR	(0x68 >> 1)

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


#define	I2C_SII9135_FREQ			25		// KHz
int			sii9135_fd;


int  sii9135_init(void);
void sii9135_id_read(void);
void sii9135_reg_read(void);
void sii9135_reg_write(void);
void sii9135_write8(uint8_t addr, uint8_t data);
int sii9135_video_in_reg_read(void);
