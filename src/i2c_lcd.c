/*
 * i2c_lcd.c
 *
 *  Created on: Sep 22, 2017
 *      Author: root
 */


#include <i2c_lcd.h>
#include <i2c.h>

#if !defined(_BV)
#define _BV(bit)	(1<<(bit))
#endif


int i2c_lcd_open(uint8_t lcd_addr)
{
//	int 		mem_fd;
//	void		*map_base;
	char		str_cmd[128];
/*
	mem_fd = open(DEV_MEM, O_RDWR|O_SYNC);
	if(mem_fd == (-1))
	{
		fprintf(stderr, "[I2C LCD] mem open() error!\n");
		return 0;
	}

	map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, I2C_ADDRESS);
	if(map_base == MAP_FAILED)
	{
		fprintf(stderr, "[I2C LCD] map_base error!\n");
		return 0;
	}
	NVIDIA_I2C_DIV = (((10200/LCD_FREQ)<<16)&0xFFFF0000)|0x00000002;
	munmap(map_base, MAP_SIZE);
	close(mem_fd);
*/
	////////////////////////////////////////////////////////////////

	sprintf(str_cmd,"echo %d > /sys/bus/i2c/devices/i2c-%d/bus_clk_rate",LCD_FREQ*1000,2);
	system(str_cmd);
	sync();

	i2c_fd = open(DEV_I2C, O_RDWR);
	if (i2c_fd < 0)
	{
		fprintf(stderr, "[I2C LCD] i2c_fd open() error!\n");
		return 0;
	}

	if (ioctl(i2c_fd, I2C_SLAVE, lcd_addr) < 0)
	{
		fprintf(stderr, "[I2C LCD] i2c_fd ioctl() error!\n");
		return 0;
	}

	return 1;
}

void i2c_lcd_close(void)
{
	if(i2c_fd>=0)
	{
		close(i2c_fd);
	}
}

void write8(uint8_t addr, uint8_t data)
{
	if(0>i2c_smbus_write_i2c_block_data(i2c_fd, addr, 1, &data))
	{
		//printf("LCD write error\n");
	}
}

uint8_t read8(uint8_t addr)
{
	//i2c_smbus_write_byte(i2c_fd, addr);
	return (uint8_t)i2c_smbus_read_byte(i2c_fd);
}

uint8_t read_gpio(void)
{
	return read8(MCP23008_GPIO);
}

void write_gpio(uint8_t gpio)
{
	write8(MCP23008_GPIO, gpio);
}

void digital_write(uint8_t p, uint8_t d)
{
	uint8_t gpio = 0x00;

	if(p>7) return;

	gpio = read_gpio();

	if(d==I2C_LEVEL_HIGH)	gpio |=  (1 << p);
	else					gpio &= ~(1 << p);

	write_gpio(gpio);
}

void pin_mode(uint8_t p, uint8_t d)
{
	uint8_t io_dir = 0x00;

	if(p>7) return;

	io_dir = read8(MCP23008_IODIR);

	if(d==I2C_IO_INPUT)		io_dir |=  (1 << p);
	else					io_dir &= ~(1 << p);

	write8(MCP23008_IODIR, io_dir);
}

void write4bits(uint8_t value)
{
	uint8_t out = 0;
	int		i;

	out = read_gpio();
	for(i=0; i<4; i++)
	{
		out &= ~_BV(_data_pins[i]);
		out |= ((value>>i)&0x1) << _data_pins[i];
	}

	out &= ~_BV(_enable_pin);
	write_gpio(out);

	// pulse enable
	delay_us(1);
	out |= _BV(_enable_pin);
	write_gpio(out);
	delay_us(1);
	out &= ~_BV(_enable_pin);
	write_gpio(out);
	delay_us(100);
}

void send_value(uint8_t value, uint8_t mode)
{
	digital_write(_rs_pin, mode);
	if(_rw_pin!=255) digital_write(_rw_pin, I2C_LEVEL_LOW);

	write4bits(value>>4);
	write4bits(value);
}

void command(uint8_t value)
{
	send_value(value, I2C_LEVEL_LOW);
}

size_t write_data(uint8_t value)
{
	send_value(value, I2C_LEVEL_HIGH);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////

void i2c_lcd_clear(void)
{
	command(LCD_CLEARDISPLAY);	// clear display, set cursor position to zero
	delay_us(2000);				// this command takes a long time!
}

void i2c_lcd_home(void)
{
	command(LCD_RETURNHOME);	// set cursor position to zero
	delay_us(2000);				// this command takes a long time!
}

// Turn the display on/off (quickly)
void i2c_lcd_no_display(void)
{
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void i2c_lcd_display(void)
{
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void i2c_lcd_no_cursor(void)
{
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void i2c_lcd_cursor(void)
{
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void i2c_lcd_no_blink(void)
{
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void i2c_lcd_blink(void)
{
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void i2c_lcd_scoll_left(void)
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void i2c_lcd_scoll_right(void)
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void i2c_lcd_left_to_right(void)
{
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void i2c_lcd_right_to_left(void)
{
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void i2c_lcd_auto_scroll(void)
{
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void i2c_lcd_no_auto_scroll(void)
{
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void i2c_lcd_set_cursor(uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if(row>_numlines)
	{
		row = _numlines - 1;
	}

	command(LCD_SETDDRAMADDR | (col+row_offsets[row]));
}

void i2c_lcd_set_backlight(uint8_t status)
{
	digital_write(7, status);
}

void write4bits2(uint8_t value, uint8_t out)
{
	int	i;

	for(i=0; i<4; i++)
	{
		out &= ~_BV(_data_pins[i]);
		out |= ((value>>i)&0x1) << _data_pins[i];
	}

	out &= ~_BV(_enable_pin);
	write_gpio(out);

	// pulse enable
	delay_us(1);
	out |= _BV(_enable_pin);
	write_gpio(out);
	delay_us(1);
	out &= ~_BV(_enable_pin);
	write_gpio(out);
	delay_us(100);
}

int i2c_lcd_init(void)
{
	uint8_t		i;

	if(0 == i2c_lcd_open(I2C_LCD_ADDR))
	{
		return 0;
	}

	_displayfunction 	= LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;

	// the I/O expander pinout
	_rs_pin 			= 1;
	_rw_pin 			= 255;
	_enable_pin 		= 2;
	_data_pins[0] 		= 3;	// really d4
	_data_pins[1] 		= 4;	// really d5
	_data_pins[2] 		= 5;	// really d6
	_data_pins[3] 		= 6;	// really d7

	i2c_smbus_write_byte(i2c_fd, MCP23008_IODIR);
	i2c_smbus_write_byte(i2c_fd, 0xFF);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);

	// backlight
	pin_mode(7, I2C_IO_OUTPUT);
	digital_write(7, I2C_LEVEL_HIGH);

	for(i=0; i<4; i++)
		pin_mode(_data_pins[i], I2C_IO_OUTPUT);

	pin_mode(_rs_pin, I2C_IO_OUTPUT);
	pin_mode(_enable_pin, I2C_IO_OUTPUT);

	_numlines = 2;
	_currline = 0;

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay_us(50000);

	// Now we pull both RS and R/W low to begin commands
	digital_write(_rs_pin, I2C_LEVEL_LOW);
	digital_write(_enable_pin, I2C_LEVEL_LOW);
	if(_rw_pin!=255) digital_write(_rw_pin, I2C_LEVEL_LOW);

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03);
	delay_us(4500);	// wait min 4.1ms

	// second try
	write4bits(0x03);
	delay_us(4500);	// wait min 4.1ms

	// third go!
	write4bits(0x03);
	delay_us(150);

	// finally, set to 8bit interface
	write4bits(0x02);

	// finally, set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON;
	i2c_lcd_display();

	// clear it off
	i2c_lcd_clear();

	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	i2c_lcd_set_backlight(I2C_LEVEL_HIGH);
	i2c_lcd_no_blink();
	close(i2c_fd);

	return 1;
}

void i2c_lcd_set_data(char line, char *buf)
{
	if(0 == i2c_lcd_open(I2C_LCD_ADDR))
	{
		return;
	}

	uint8_t	line_buf[MAX_LINE_TEXT];
	int32_t	len, i;
	uint8_t out;

	i2c_lcd_set_cursor(0, line);

	len = strlen(buf);
	if(len>MAX_LINE_TEXT) len = MAX_LINE_TEXT;

	digital_write(_rs_pin, I2C_LEVEL_HIGH);
	if(_rw_pin!=255) digital_write(_rw_pin, I2C_LEVEL_LOW);

	memset(line_buf, ' ', MAX_LINE_TEXT);
	memcpy(line_buf, buf, len);

	out = read_gpio();

	for(i=0; i<MAX_LINE_TEXT; i++)
	{
		write4bits2(line_buf[i]>>4, out);
		write4bits2(line_buf[i], out);
	}

	close(i2c_fd);
}


void i2c_lcd_change_cursor(uint8_t col, uint8_t row)
{
	if(0 == i2c_lcd_open(I2C_LCD_ADDR))
	{
		return;
	}

	i2c_lcd_set_cursor(col, row);

	close(i2c_fd);
}



/*
int i2c_lcd_init(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows)
{
	int 		mem_fd;
	void		*map_base;
	uint8_t		i;

	mem_fd = open(DEV_MEM, O_RDWR|O_SYNC);
	if(mem_fd == (-1))
	{
		fprintf(stderr, "mem open() error!\n");
		return 0;
	}

	map_base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, I2C_ADDRESS);
	if(map_base == MAP_FAILED)
	{
		fprintf(stderr, "map_base error!\n");
		return 0;
	}
	NVIDIA_I2C_DIV = (((10200/LCD_FREQ)<<16)&0xFFFF0000)|0x00000002;
	munmap(map_base, MAP_SIZE);
	close(mem_fd);
	////////////////////////////////////////////////////////////////

	i2c_fd = open(DEV_I2C, O_RDWR);
	if (i2c_fd < 0)
	{
		fprintf(stderr, "i2c_fd open() error!\n");
		return 0;
	}

	if (ioctl(i2c_fd, I2C_SLAVE, lcd_addr) < 0)
	{
		fprintf(stderr, "i2c_fd ioctl() error!\n");
		return 0;
	}

	_displayfunction 	= LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;

	// the I/O expander pinout
	_rs_pin 			= 1;
	_rw_pin 			= 255;
	_enable_pin 		= 2;
	_data_pins[0] 		= 3;	// really d4
	_data_pins[1] 		= 4;	// really d5
	_data_pins[2] 		= 5;	// really d6
	_data_pins[3] 		= 6;	// really d7

	i2c_smbus_write_byte(i2c_fd, MCP23008_IODIR);
	i2c_smbus_write_byte(i2c_fd, 0xFF);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);
	i2c_smbus_write_byte(i2c_fd, 0x00);

	// backlight
	pin_mode(7, I2C_IO_OUTPUT);
	digital_write(7, I2C_LEVEL_HIGH);

	for(i=0; i<4; i++)
		pin_mode(_data_pins[i], I2C_IO_OUTPUT);

	pin_mode(_rs_pin, I2C_IO_OUTPUT);
	pin_mode(_enable_pin, I2C_IO_OUTPUT);

	_numlines = 2;
	_currline = 0;

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay_us(50000);

	// Now we pull both RS and R/W low to begin commands
	digital_write(_rs_pin, I2C_LEVEL_LOW);
	digital_write(_enable_pin, I2C_LEVEL_LOW);
	if(_rw_pin!=255) digital_write(_rw_pin, I2C_LEVEL_LOW);

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03);
	delay_us(4500);	// wait min 4.1ms

	// second try
	write4bits(0x03);
	delay_us(4500);	// wait min 4.1ms

	// third go!
	write4bits(0x03);
	delay_us(150);

	// finally, set to 8bit interface
	write4bits(0x02);

	// finally, set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	i2c_lcd_display();

	// clear it off
	i2c_lcd_clear();

	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	i2c_lcd_set_backlight(I2C_LEVEL_HIGH);

	return 1;
}
*/
