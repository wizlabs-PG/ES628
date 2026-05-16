################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/file_manager.c \
../src/fontinfo.c \
../src/fpga_spi.c \
../src/global.c \
../src/gpio.c \
../src/group_data.c \
../src/gx.c \
../src/gxbdf.c \
../src/gxbmp.c \
../src/gxfile.c \
../src/gximagelist.c \
../src/gxjpg.c \
../src/gxlayer.c \
../src/gxmosaic.c \
../src/gxpanel.c \
../src/gxpng.c \
../src/gxttf.c \
../src/i2c.c \
../src/i2c_dvi.c \
../src/i2c_gpio.c \
../src/i2c_lcd.c \
../src/i2c_sensing.c \
../src/i2c_sii9135.c \
../src/ksc5601.c \
../src/main.c \
../src/model_data.c \
../src/msg_comu.c \
../src/nvgst_asound_common.c \
../src/nvgst_x11_common.c \
../src/nvgstplayer.c \
../src/pattern.c \
../src/pattern_control.c \
../src/pollmanager.c \
../src/pwr_control.c \
../src/pwr_tcpclient.c \
../src/rcb.c \
../src/rcb_485.c \
../src/usb_storage.c \
../src/videooverlay.c 

C_DEPS += \
./src/file_manager.d \
./src/fontinfo.d \
./src/fpga_spi.d \
./src/global.d \
./src/gpio.d \
./src/group_data.d \
./src/gx.d \
./src/gxbdf.d \
./src/gxbmp.d \
./src/gxfile.d \
./src/gximagelist.d \
./src/gxjpg.d \
./src/gxlayer.d \
./src/gxmosaic.d \
./src/gxpanel.d \
./src/gxpng.d \
./src/gxttf.d \
./src/i2c.d \
./src/i2c_dvi.d \
./src/i2c_gpio.d \
./src/i2c_lcd.d \
./src/i2c_sensing.d \
./src/i2c_sii9135.d \
./src/ksc5601.d \
./src/main.d \
./src/model_data.d \
./src/msg_comu.d \
./src/nvgst_asound_common.d \
./src/nvgst_x11_common.d \
./src/nvgstplayer.d \
./src/pattern.d \
./src/pattern_control.d \
./src/pollmanager.d \
./src/pwr_control.d \
./src/pwr_tcpclient.d \
./src/rcb.d \
./src/rcb_485.d \
./src/usb_storage.d \
./src/videooverlay.d 

OBJS += \
./src/file_manager.o \
./src/fontinfo.o \
./src/fpga_spi.o \
./src/global.o \
./src/gpio.o \
./src/group_data.o \
./src/gx.o \
./src/gxbdf.o \
./src/gxbmp.o \
./src/gxfile.o \
./src/gximagelist.o \
./src/gxjpg.o \
./src/gxlayer.o \
./src/gxmosaic.o \
./src/gxpanel.o \
./src/gxpng.o \
./src/gxttf.o \
./src/i2c.o \
./src/i2c_dvi.o \
./src/i2c_gpio.o \
./src/i2c_lcd.o \
./src/i2c_sensing.o \
./src/i2c_sii9135.o \
./src/ksc5601.o \
./src/main.o \
./src/model_data.o \
./src/msg_comu.o \
./src/nvgst_asound_common.o \
./src/nvgst_x11_common.o \
./src/nvgstplayer.o \
./src/pattern.o \
./src/pattern_control.o \
./src/pollmanager.o \
./src/pwr_control.o \
./src/pwr_tcpclient.o \
./src/rcb.o \
./src/rcb_485.o \
./src/usb_storage.o \
./src/videooverlay.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DDEBUG=DEBUG -I"/home/wizlabs/eclipse-workspace/ES628/include" -I/usr/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include -I/usr/include/ -I/usr/lib/aarch64-linux-gnu/gstreamer-1.0/include -I/usr/include/freetype2/ -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/file_manager.d ./src/file_manager.o ./src/fontinfo.d ./src/fontinfo.o ./src/fpga_spi.d ./src/fpga_spi.o ./src/global.d ./src/global.o ./src/gpio.d ./src/gpio.o ./src/group_data.d ./src/group_data.o ./src/gx.d ./src/gx.o ./src/gxbdf.d ./src/gxbdf.o ./src/gxbmp.d ./src/gxbmp.o ./src/gxfile.d ./src/gxfile.o ./src/gximagelist.d ./src/gximagelist.o ./src/gxjpg.d ./src/gxjpg.o ./src/gxlayer.d ./src/gxlayer.o ./src/gxmosaic.d ./src/gxmosaic.o ./src/gxpanel.d ./src/gxpanel.o ./src/gxpng.d ./src/gxpng.o ./src/gxttf.d ./src/gxttf.o ./src/i2c.d ./src/i2c.o ./src/i2c_dvi.d ./src/i2c_dvi.o ./src/i2c_gpio.d ./src/i2c_gpio.o ./src/i2c_lcd.d ./src/i2c_lcd.o ./src/i2c_sensing.d ./src/i2c_sensing.o ./src/i2c_sii9135.d ./src/i2c_sii9135.o ./src/ksc5601.d ./src/ksc5601.o ./src/main.d ./src/main.o ./src/model_data.d ./src/model_data.o ./src/msg_comu.d ./src/msg_comu.o ./src/nvgst_asound_common.d ./src/nvgst_asound_common.o ./src/nvgst_x11_common.d ./src/nvgst_x11_common.o ./src/nvgstplayer.d ./src/nvgstplayer.o ./src/pattern.d ./src/pattern.o ./src/pattern_control.d ./src/pattern_control.o ./src/pollmanager.d ./src/pollmanager.o ./src/pwr_control.d ./src/pwr_control.o ./src/pwr_tcpclient.d ./src/pwr_tcpclient.o ./src/rcb.d ./src/rcb.o ./src/rcb_485.d ./src/rcb_485.o ./src/usb_storage.d ./src/usb_storage.o ./src/videooverlay.d ./src/videooverlay.o

.PHONY: clean-src

