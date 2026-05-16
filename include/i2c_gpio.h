/*
 * i2c_gpio.h
 *
 *  Created on: Oct 18, 2017
 *      Author: root
 */

#ifndef I2C_GPIO_H_
#define I2C_GPIO_H_

#define GPIO_I2C_BUS 			0			//GP1
#define GPIO_I2C_FREQ 			200 		//khz
#define GPIO_I2C_ADDRESS0		0x74		//gpio #0~#15
#define GPIO_I2C_ADDRESS1		0x75		//gpio #16~#31
#define GPIO_I2C_ADDRESS2		0x77		//gpio #32~#47

/* DVI MODE
 * if(single)
 * {
 * 		GPIO_EX10 = high
 * 		GPIO_EX11 = high
 * }
 * else
 * {
 * 		GPIO_EX10 = low
 * 		GPIO_EX10 = high
 * }
*/

//I2C GPIO
typedef enum
{
	//0x74
	GPIO_EX0=0,		// IP S/W 0
	GPIO_EX1,		// IP S/W 1
	GPIO_EX2,		// IP S/W 2
	GPIO_EX3,		// IP S/W 3
	GPIO_EX4,		// IP S/W 4
	GPIO_EX5,		// IP S/W 5
	GPIO_EX6,		// IP S/W 6
	GPIO_EX7,		// IP S/W 7
	GPIO_EX8,		// RXDLSL
	GPIO_EX9,		// RXPIXS
	GPIO_EX10,		// L_MODE0
	GPIO_EX11,		// L_MODE1
	GPIO_EX12,		// L_MODE3
	GPIO_EX13,		// L_MODE2
	GPIO_EX14,		// LVEDGE
	GPIO_EX15,		// SCDT

	//0x75
	GPIO_EX16=16,	// LDVS_Master_gpio
	GPIO_EX17,		// LDVS_Slave_gpio
	GPIO_EX18,		// VDD_ON
	GPIO_EX19,		// DDC_EP_EN0
	GPIO_EX20,		// DDC_EP_EN1
	GPIO_EX21,
	GPIO_EX22,
	GPIO_EX23,
	GPIO_EX24,		// HPD(HDMI)
	GPIO_EX25,		// RS485(FRONT)
	GPIO_EX26,		// RS485(REAR)
	GPIO_EX27,
	GPIO_EX28,
	GPIO_EX29,
	GPIO_EX30,
	GPIO_EX31,

	//0x77
	GPIO_EX32=32,	//Ext i2c vdd sel0
	GPIO_EX33,		//Ext i2c vdd sel1
	GPIO_EX34,
	GPIO_EX35,
	GPIO_EX36,		//TX sub board 1's GPIO0
	GPIO_EX37,		//TX sub board 1's GPIO1
	GPIO_EX38,		//TX sub board 2's GPIO0
	GPIO_EX39,		//TX sub board 2's GPIO1
	GPIO_EX40,		//TX sub board 3's GPIO0
	GPIO_EX41,		//TX sub board 3's GPIO1
	GPIO_EX42,		//TX sub board 4's GPIO0
	GPIO_EX43,		//TX sub board 4's GPIO1
	GPIO_EX44,
	GPIO_EX45,
	GPIO_EX46,
	GPIO_EX47
} I2C_GPIO;

extern void 	i2c_gpio_init();
extern void 	i2c_gpio_set_direction(uint32_t out_flag);
extern void 	i2c_gpio_set_value(uint32_t value);
extern void 	i2c_gpio_set(uint8_t gpio,uint8_t level);
extern uint32_t i2c_gpio_get_value(void);
extern uint8_t 	i2c_gpio_get_ipsw(void);
extern void 	i2c_ddc_sel(int level);
extern void 	i2c_hdmi_hpd_set(int level);
extern void 	i2c_pwr_485_set(int level);
extern void 	i2c_pg_485_set(int level);
extern void 	i2c_gpio_close();

extern void 	i2c_gpio_set_direction_b(uint32_t out_flag);
extern uint32_t i2c_gpio_get_value_b(void);

#endif /* I2C_GPIO_H_ */
