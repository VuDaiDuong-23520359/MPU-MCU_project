################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/color_values.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/ws2812.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/ws2812_demos.c 

OBJS += \
./stm32-ws2812-master/src/color_values.o \
./stm32-ws2812-master/src/ws2812.o \
./stm32-ws2812-master/src/ws2812_demos.o 

C_DEPS += \
./stm32-ws2812-master/src/color_values.d \
./stm32-ws2812-master/src/ws2812.d \
./stm32-ws2812-master/src/ws2812_demos.d 


# Each subdirectory must supply rules for building sources it contributes
stm32-ws2812-master/src/color_values.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/color_values.c stm32-ws2812-master/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/src/ws2812.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/ws2812.c stm32-ws2812-master/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/src/ws2812_demos.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/src/ws2812_demos.c stm32-ws2812-master/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-stm32-2d-ws2812-2d-master-2f-src

clean-stm32-2d-ws2812-2d-master-2f-src:
	-$(RM) ./stm32-ws2812-master/src/color_values.cyclo ./stm32-ws2812-master/src/color_values.d ./stm32-ws2812-master/src/color_values.o ./stm32-ws2812-master/src/color_values.su ./stm32-ws2812-master/src/ws2812.cyclo ./stm32-ws2812-master/src/ws2812.d ./stm32-ws2812-master/src/ws2812.o ./stm32-ws2812-master/src/ws2812.su ./stm32-ws2812-master/src/ws2812_demos.cyclo ./stm32-ws2812-master/src/ws2812_demos.d ./stm32-ws2812-master/src/ws2812_demos.o ./stm32-ws2812-master/src/ws2812_demos.su

.PHONY: clean-stm32-2d-ws2812-2d-master-2f-src

