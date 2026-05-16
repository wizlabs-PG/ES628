/*
 * gpio.h
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <sys/types.h>

 /****************************************************************
 * Constants
 ****************************************************************/
 
#define SYSFS_GPIO_DIR 	"/sys/class/gpio"
#define POLL_TIMEOUT 	(3 * 1000) /* 3 seconds */
#define MAX_GPIO_BUF 	64
#define EDID_SII9135_I2C_EN 0
#define HDMI_I2C_EN 1

#define GPIO_ADDR_216		0x6000d60C	// Jetson Nano  7[AUDIO_MCLK]
#define GPIO_ADDR_50		0x6000d108	// Jetson Nano 11[UART2_RTS]
#define GPIO_ADDR_79		0x6000d204	// Jetson Nano 12
#define GPIO_ADDR_14		0x6000d004	// Jetson Nano 13[SPI2_SCK]
#define GPIO_ADDR_194		0x6000d600	// Jetson Nano 15[LCD_TE]
#define GPIO_ADDR_16		0x6000d008	// Jetson Nano 19[SPI1_MOSI]
#define GPIO_ADDR_38		0x6000d100	// Jetson Nano 33[GPIO_PE6]
#define GPIO_ADDR_76		0x6000d204	// Jetson Nano 35
#define GPIO_ADDR_51		0x6000d108	// Jetson Nano 36
#define GPIO_ADDR_77		0x6000d204	// Jetson Nano 38[I2S4_SDIN]
#define GPIO_ADDR_78		0x6000d204	// Jetson Nano 40

#define GPIO_MASK_216		0x01
#define GPIO_MASK_50		0x04
#define GPIO_MASK_79		0x80
#define GPIO_MASK_14		0x40
#define GPIO_MASK_194		0x04
#define GPIO_MASK_16		0x01
#define GPIO_MASK_38		0x40
#define GPIO_MASK_76		0x10
#define GPIO_MASK_51		0x08
#define GPIO_MASK_77		0x20
#define GPIO_MASK_78		0x40

#define GPIO_INT_LVL_MASK			0x010101
#define GPIO_INT_LVL_EDGE_RISING	0x000101
#define GPIO_INT_LVL_EDGE_FALLING	0x000100
#define GPIO_INT_LVL_EDGE_BOTH		0x010100
#define GPIO_INT_LVL_LEVEL_HIGH		0x000001
#define GPIO_INT_LVL_LEVEL_LOW		0x000000

enum pinDirections {
	inputPin  = 0,
	outputPin = 1
} ;

enum pinValues {
    low = 0,
    high = 1,
    high_high = 2,
    low_low = 3,
    off = 0,  // synonym for things like lights
    on = 1
}  ;

enum jetsonGPIONumber {
    gpio57  =  57,    // J3A1 - Pin 50
	gpio160 = 160,	  // J3A2 - Pin 40	
	gpio161 = 161,    // J3A2 - Pin 43
	gpio162 = 162,    // J3A2 - Pin 46
	gpio163 = 163,    // J3A2 - Pin 49
	gpio164 = 164,    // J3A2 - Pin 52
	gpio165 = 165,    // J3A2 - Pin 55
	gpio166 = 166     // J3A2 - Pin 58
}  ;

enum jetsonnanogpioNumber {
	gpio216	= 216,		// pin 07	- [AUDIO_MCLK]	-> USER_GPIO_00		- FPGA_TEGRA_IO4
	gpio50	= 50,		// pin 11	- [UART1_RTS]	-> USER_GPIO_01		- FPGA_TEGRA_IO3	- DNI
	gpio79	= 79,		// pin 12	- [I2S0_SCLK]	-> USER_GPIO_06		- F_CONF_DONE
	gpio14	= 14,		// pin 13	- [SPI2_SCK]
	gpio194	= 194,		// pin 15	- [LCD_TE]		-> USER_GPIO_07		- nCONFIG
	gpio16	= 16,		// pin 19	- [SPI1_MOSI]
	gpio38	= 38,		// pin 33	- [GPIO_PE6]	-> USER_GPIO_08		- FPGA_nSTATUS
	gpio76	= 76,		// pin 35	- [I2S0_FS]		-> USER_GPIO_03		- FPGA_TEGRA_IO1	- DNI
	gpio51	= 51,		// pin 36	- [UART1_CTS]	-> USER_GPIO_02		- SDCARD_VDD_EN
	gpio77	= 77,		// pin 38	- [I2S0_SDIN]	-> USER_GPIO_04		- SDCARD_CD_L
	gpio78	= 78		// pin 40	- [I2S0_DOUT]	-> USER_GPIO_05		- FPGA_TEGRA_IO2	- DNI
};

enum INOUT {
    INPUT, OUTPUT=0xFF
};

typedef struct {
    u_int CNF;
    u_int _padding1[3];
    u_int OE;
    u_int _padding2[3];
    u_int OUT;
    u_int _padding3[3];
    u_int IN;
    u_int _padding4[3];
    u_int INT_STA;
    u_int _padding5[3];
    u_int INT_ENB;
    u_int _padding6[3];
    u_int INT_LVL;
    u_int _padding7[3];
    u_int INT_CLR;
    u_int _padding8[3];
} gpio_t;

int gpioExport ( unsigned int gpio ) ;
int gpioUnexport ( unsigned int gpio ) ;
int gpioSetDirection ( unsigned int, unsigned int out_flag ) ;
int gpioSetValue ( unsigned int gpio, unsigned int value ) ;
int gpioGetValue ( unsigned int gpio, unsigned int *value ) ;
int gpioSetEdge ( unsigned int gpio, char *edge ) ;
int gpioOpen ( unsigned int gpio ) ;
int gpioClose ( int fileDescriptor ) ;
int gpioActiveLow ( unsigned int gpio, unsigned int value ) ;

extern void gpio_exports(void);
extern void gpio_unexports(void);

extern void	gpio_set_value(unsigned int, unsigned int);
extern void	gpio_get_value(unsigned int, unsigned int *);

#endif
