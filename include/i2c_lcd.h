/*
 * i2c_lcd.h
 *
 *  Created on: Sep 22, 2017
 *      Author: root
 */

#ifndef I2C_LCD_H_
#define I2C_LCD_H_

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
#if !defined(MAX_LINE_TEXT)
#define MAX_LINE_TEXT			16
#endif

#define I2C_LCD_ADDR			0x20
#define	LCD_FREQ				100		// KHz

#ifndef NVIDIA_I2C_DIV
#define NVIDIA_I2C_DIV 			(*((volatile unsigned int*)(map_base+0x6C)))
#endif

// MCP23X08
#define MCP23008_IODIR			0x00
#define MCP23008_IPOL			0x01
#define MCP23008_GPINTEN		0x02
#define MCP23008_DEFVAL			0x03
#define MCP23008_INTCON			0x04
#define MCP23008_IOCON			0x05
#define MCP23008_GPPU  			0x06
#define MCP23008_INTF			0x07
#define MCP23008_INTCAP			0x08
#define MCP23008_GPIO			0x09
#define MCP23008_OLAT			0x0A

// commands
#define LCD_CLEARDISPLAY 		0x01
#define LCD_RETURNHOME 			0x02
#define LCD_ENTRYMODESET 		0x04
#define LCD_DISPLAYCONTROL 		0x08
#define LCD_CURSORSHIFT 		0x10
#define LCD_FUNCTIONSET 		0x20
#define LCD_SETCGRAMADDR 		0x40
#define LCD_SETDDRAMADDR 		0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 			0x00
#define LCD_ENTRYLEFT 			0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 			0x04
#define LCD_DISPLAYOFF 			0x00
#define LCD_CURSORON 			0x02
#define LCD_CURSOROFF 			0x00
#define LCD_BLINKON 			0x01
#define LCD_BLINKOFF 			0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 		0x08
#define LCD_CURSORMOVE 			0x00
#define LCD_MOVERIGHT 			0x04
#define LCD_MOVELEFT 			0x00

// flags for function set
#define LCD_8BITMODE 			0x10
#define LCD_4BITMODE 			0x00
#define LCD_2LINE 				0x08
#define LCD_1LINE 				0x00
#define LCD_5x10DOTS 			0x04
#define LCD_5x8DOTS 			0x00

// flags for backlight control
#define LCD_BACKLIGHT 			0x08
#define LCD_NOBACKLIGHT 		0x00

// Enumerations
typedef enum
{
	I2C_LEVEL_LOW				= 0x00,
	I2C_LEVEL_HIGH
} enum_i2c_level_t;

typedef enum
{
	I2C_IO_INPUT				= 0x00,
	I2C_IO_OUTPUT
} enum_i2c_io_t;

// Variables
int			i2c_fd;
uint8_t 	_addr;

uint8_t 	_rs_pin;		// Low:command, High:character
uint8_t 	_rw_pin;		// Low:write to LCD, High:read to LCD
uint8_t 	_enable_pin;	// activated by a High pulse
uint8_t 	_data_pins[8];

uint8_t 	_displayfunction;
uint8_t 	_displaycontrol;
uint8_t 	_displaymode;
uint8_t 	_numlines, _currline;
uint8_t 	_cols;
uint8_t 	_rows;
uint8_t 	_backlightval;


// Functions
int 		i2c_lcd_init(void);
void 		i2c_lcd_close(void);

void 		i2c_lcd_clear(void);
void 		i2c_lcd_home(void);
void 		i2c_lcd_no_display(void);
void 		i2c_lcd_display(void);
void 		i2c_lcd_no_cursor(void);
void 		i2c_lcd_cursor(void);
void 		i2c_lcd_no_blink(void);
void 		i2c_lcd_blink(void);
void 		i2c_lcd_scoll_left(void);
void 		i2c_lcd_scoll_right(void);
void 		i2c_lcd_left_to_right(void);
void 		i2c_lcd_right_to_left(void);
void 		i2c_lcd_auto_scroll(void);
void 		i2c_lcd_no_auto_scroll(void);

void 		i2c_lcd_set_cursor(uint8_t col, uint8_t row);
void 		i2c_lcd_set_backlight(uint8_t status);
void 		i2c_lcd_set_data(char line, char *buf);

void 		i2c_lcd_change_cursor(uint8_t col, uint8_t row);

void 		write8(uint8_t addr, uint8_t data);
uint8_t 	read8(uint8_t addr);
uint8_t 	read_gpio(void);
void 		write_gpio(uint8_t gpio);
void 		digital_write(uint8_t p, uint8_t d);
void 		pin_mode(uint8_t p, uint8_t d);
void 		write4bits(uint8_t value);
void 		write4bits2(uint8_t value, uint8_t out);
void 		send_value(uint8_t value, uint8_t mode);
void 		command(uint8_t value);
size_t 		write_data(uint8_t value);


#endif /* I2C_LCD_H_ */
