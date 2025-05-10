################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
ELF_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.elf 

C_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.c 

O_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.o 

OBJS += \
./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.o 

C_DEPS += \
./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.d 


# Each subdirectory must supply rules for building sources it contributes
stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.c stm32-ws2812-master/examples/libopencm3/f4/f411/demo/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-libopencm3-2f-f4-2f-f411-2f-demo

clean-stm32-2d-ws2812-2d-master-2f-examples-2f-libopencm3-2f-f4-2f-f411-2f-demo:
	-$(RM) ./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.cyclo ./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.d ./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.o ./stm32-ws2812-master/examples/libopencm3/f4/f411/demo/demo.su

.PHONY: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-libopencm3-2f-f4-2f-f411-2f-demo

