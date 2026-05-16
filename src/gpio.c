// ENSIS 
// 2016.09.09 gpio_set

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
//#include "../include/gpio.h"
#include <gpio.h>

#include <sys/fcntl.h>
#include <sys/mman.h>

int gpioExport ( unsigned int gpio )
{
    int fileDescriptor, length;
    char commandBuffer[MAX_GPIO_BUF];

    fileDescriptor = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioExport unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    length = snprintf(commandBuffer, sizeof(commandBuffer), "%d", gpio);
    if (write(fileDescriptor, commandBuffer, length) != length) {
        perror("gpioExport");
        return fileDescriptor ;

    }
    close(fileDescriptor);

    return 1; 
}

int gpioUnexport ( unsigned int gpio )
{
    int fileDescriptor, length;
    char commandBuffer[MAX_GPIO_BUF];

    fileDescriptor = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioUnexport unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    length = snprintf(commandBuffer, sizeof(commandBuffer), "%d", gpio);
    if (write(fileDescriptor, commandBuffer, length) != length) {
        perror("gpioUnexport") ;
        return fileDescriptor ;
    }
    close(fileDescriptor);
    return 1;
}

int gpioSetDirection ( unsigned int gpio, unsigned int out_flag )
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioSetDirection unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (out_flag) {
        if (write(fileDescriptor, "out", 4) != 4) {
            perror("gpioSetDirection") ;
            return fileDescriptor ;
        }
    }
    else {
        if (write(fileDescriptor, "in", 3) != 3) {
            perror("gpioSetDirection") ;
            return fileDescriptor ;
        }
    }
    close(fileDescriptor);
    return 1;
}

int gpioSetValue ( unsigned int gpio, unsigned int value )
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioSetValue unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (value) {
        if (write(fileDescriptor, "1", 2) != 2) {
            perror("gpioSetValue") ;
            return fileDescriptor ;
        }
    }
    else {
        if (write(fileDescriptor, "0", 2) != 2) {
            perror("gpioSetValue") ;
            return fileDescriptor ;
        }
    }
    close(fileDescriptor);
    return 1;
}

int gpioGetValue ( unsigned int gpio, unsigned int *value)
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];
    char ch;

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fileDescriptor = open(commandBuffer, O_RDONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioGetValue unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (read(fileDescriptor, &ch, 1) != 1) {
        perror("gpioGetValue") ;
        return fileDescriptor ;
     }

    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }

    close(fileDescriptor);
    return 1;
}


int gpioSetEdge ( unsigned int gpio, char *edge )
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioSetEdge unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (write(fileDescriptor, edge, strlen(edge) + 1) != ((int)(strlen(edge) + 1))) {
        perror("gpioSetEdge") ;
        return fileDescriptor ;
    }
    close(fileDescriptor);
    return 1;
}

int gpioOpen( unsigned int gpio )
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

    fileDescriptor = open(commandBuffer, O_RDONLY | O_NONBLOCK );
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioOpen unable to open gpio%d",gpio) ;
        perror(errorBuffer);
    }
    return fileDescriptor;
}


int gpioClose ( int fileDescriptor )
{
    return close(fileDescriptor);
}

int gpioActiveLow ( unsigned int gpio, unsigned int value )
{
    int fileDescriptor;
    char commandBuffer[MAX_GPIO_BUF];

    snprintf(commandBuffer, sizeof(commandBuffer), SYSFS_GPIO_DIR "/gpio%d/active_low", gpio);

    fileDescriptor = open(commandBuffer, O_WRONLY);
    if (fileDescriptor < 0) {
        char errorBuffer[128] ;
        snprintf(errorBuffer,sizeof(errorBuffer), "gpioActiveLow unable to open gpio%d",gpio) ;
        perror(errorBuffer);
        return fileDescriptor;
    }

    if (value) {
        if (write(fileDescriptor, "1", 2) != 2) {
            perror("gpioActiveLow") ;
            return fileDescriptor ;
        }
    }
    else {
        if (write(fileDescriptor, "0", 2) != 2) {
            perror("gpioActiveLow") ;
            return fileDescriptor ;
        }
    }
    close(fileDescriptor);
    return 1;
}

void gpio_exports(void)
{
/*
	gpioExport(gpio184);
	gpioSetDirection(gpio184, inputPin);
	gpioExport(gpio38);
	gpioSetDirection(gpio38, inputPin);
	gpioExport(gpio186);
	gpioSetDirection(gpio186, inputPin);
	gpioExport(gpio187);
	gpioSetDirection(gpio187, outputPin);
	gpioExport(gpio219);
	gpioSetDirection(gpio219, outputPin);

	gpioSetValue(gpio219, low);		//SPI Buffer output en
	gpioSetValue(gpio186, low);
	gpioSetValue(gpio187, low);
*/
}

void gpio_unexports(void)
{
/*
	gpioUnexport(gpio38);
	gpioUnexport(gpio184);
	gpioUnexport(gpio186);
	gpioUnexport(gpio187);
	gpioUnexport(gpio219);
*/
}

void gpio_set_value(unsigned int gpio, unsigned int output)
{
//	u_int8_t	cnt;
	u_int32_t	gpio_base_addr;
	u_int32_t	gpio_mask=0;

	//  read physical memory (needs root)
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
	fprintf(stderr, "usage : $ sudo (with root privilege)\n");
	exit(1);
	}

	//  map a particular physical address into our address space
	int pagesize = getpagesize();
	int pagemask = pagesize-1;

	switch(gpio)
	{
		case gpio216	:
			gpio_base_addr = GPIO_ADDR_216;
			gpio_mask = GPIO_MASK_216;
			break;
		case gpio50	:
			gpio_base_addr = GPIO_ADDR_50;
			gpio_mask = GPIO_MASK_50;
			break;
		case gpio79	:
			gpio_base_addr = GPIO_ADDR_79;
			gpio_mask = GPIO_MASK_79;
			break;
		case gpio14	:
			gpio_base_addr = GPIO_ADDR_14;
			gpio_mask = GPIO_MASK_14;
			break;
		case gpio194	:
			gpio_base_addr = GPIO_ADDR_194;
			gpio_mask = GPIO_MASK_194;
			break;
		case gpio16	:
			gpio_base_addr = GPIO_ADDR_16;
			gpio_mask = GPIO_MASK_16;
			break;
		case gpio38	:
			gpio_base_addr = GPIO_ADDR_38;
			gpio_mask = GPIO_MASK_38;
			break;
		case gpio76	:
			gpio_base_addr = GPIO_ADDR_76;
			gpio_mask = GPIO_MASK_76;
			break;
		case gpio51	:
			gpio_base_addr = GPIO_ADDR_51;
			gpio_mask = GPIO_MASK_51;
			break;
		case gpio77	:
			gpio_base_addr = GPIO_ADDR_77;
			gpio_mask = GPIO_MASK_77;
			break;
		case gpio78	:
			gpio_base_addr = GPIO_ADDR_78;
			gpio_mask = GPIO_MASK_78;
			break;
		default		: break;
	}




	//  This page will actually contain all the GPIO controllers, because they are co-located
	void *base = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (gpio_base_addr & ~pagemask));
	if (base == NULL) {
	perror("mmap()");
	exit(1);
	}

	//  set up a pointer for convenient access -- this pointer is to the selected GPIO controller
	gpio_t volatile *pinLed = (gpio_t volatile *)((char *)base + (gpio_base_addr & pagemask));




	// for LED : GPIO OUT
	if(gpio_mask>0)
	{
//		pinLed->CNF = 0x000f;
		pinLed->CNF |= gpio_mask;
		pinLed->OE = OUTPUT;

		if(output==high)		pinLed->OUT |= gpio_mask;
		else if(output==low)	pinLed->OUT &= (!gpio_mask);
		else					pinLed->OUT &= (!gpio_mask);

		//  disable interrupts
		pinLed->INT_ENB = 0x00;
	}

	/* unmap */
	munmap(base, pagesize);

	/* close the /dev/mem */
	close(fd);

}

void gpio_get_value(unsigned int gpio, unsigned int *input)
{
//	u_int8_t cnt;
	u_int32_t gpio_base_addr;
	u_int32_t gpio_mask=0;

	//  read physical memory (needs root)
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
	fprintf(stderr, "usage : $ sudo (with root privilege)\n");
	exit(1);
	}

	//  map a particular physical address into our address space
	int pagesize = getpagesize();
	int pagemask = pagesize-1;

	switch(gpio)
	{
		case gpio216	:
			gpio_base_addr = GPIO_ADDR_216;
			gpio_mask = GPIO_MASK_216;
			break;
		case gpio50	:
			gpio_base_addr = GPIO_ADDR_50;
			gpio_mask = GPIO_MASK_50;
			break;
		case gpio79	:
			gpio_base_addr = GPIO_ADDR_79;
			gpio_mask = GPIO_MASK_79;
			break;
		case gpio14	:
			gpio_base_addr = GPIO_ADDR_14;
			gpio_mask = GPIO_MASK_14;
			break;
		case gpio194	:
			gpio_base_addr = GPIO_ADDR_194;
			gpio_mask = GPIO_MASK_194;
			break;
		case gpio16	:
			gpio_base_addr = GPIO_ADDR_16;
			gpio_mask = GPIO_MASK_16;
			break;
		case gpio38	:
			gpio_base_addr = GPIO_ADDR_38;
			gpio_mask = GPIO_MASK_38;
			break;
		case gpio76	:
			gpio_base_addr = GPIO_ADDR_76;
			gpio_mask = GPIO_MASK_76;
			break;
		case gpio51	:
			gpio_base_addr = GPIO_ADDR_51;
			gpio_mask = GPIO_MASK_51;
			break;
		case gpio77	:
			gpio_base_addr = GPIO_ADDR_77;
			gpio_mask = GPIO_MASK_77;
			break;
		case gpio78	:
			gpio_base_addr = GPIO_ADDR_78;
			gpio_mask = GPIO_MASK_78;
			break;
		default		: break;
	}




	//  This page will actually contain all the GPIO controllers, because they are co-located
	void *base = mmap(0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (gpio_base_addr & ~pagemask));
	if (base == NULL) {
	perror("mmap()");
	exit(1);
	}

	//  set up a pointer for convenient access -- this pointer is to the selected GPIO controller
	gpio_t volatile *pinLed = (gpio_t volatile *)((char *)base + (gpio_base_addr & pagemask));




	// for LED : GPIO OUT
	if(gpio_mask>0)
	{
//		pinLed->CNF = 0x000f;
		pinLed->CNF |= gpio_mask;
		pinLed->OE = INPUT;


		if((pinLed->IN & gpio_mask)>0)	*input=high;
		else							*input=low;

		//  disable interrupts
		pinLed->INT_ENB = 0x00;
	}

	/* unmap */
	munmap(base, pagesize);

	/* close the /dev/mem */
	close(fd);
}
