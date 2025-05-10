################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/main.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/syscalls.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/sysmem.c \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.c 

OBJS += \
./stm32-ws2812-master/examples/f411/Core/Src/main.o \
./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.o \
./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.o \
./stm32-ws2812-master/examples/f411/Core/Src/syscalls.o \
./stm32-ws2812-master/examples/f411/Core/Src/sysmem.o \
./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.o 

C_DEPS += \
./stm32-ws2812-master/examples/f411/Core/Src/main.d \
./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.d \
./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.d \
./stm32-ws2812-master/examples/f411/Core/Src/syscalls.d \
./stm32-ws2812-master/examples/f411/Core/Src/sysmem.d \
./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
stm32-ws2812-master/examples/f411/Core/Src/main.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/main.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/examples/f411/Core/Src/syscalls.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/syscalls.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/examples/f411/Core/Src/sysmem.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/sysmem.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.c stm32-ws2812-master/examples/f411/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/Apps/STM32IDE/packs/stm32-ws2812-master" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Src

clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Src:
	-$(RM) ./stm32-ws2812-master/examples/f411/Core/Src/main.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/main.d ./stm32-ws2812-master/examples/f411/Core/Src/main.o ./stm32-ws2812-master/examples/f411/Core/Src/main.su ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.d ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.o ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_hal_msp.su ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.d ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.o ./stm32-ws2812-master/examples/f411/Core/Src/stm32f4xx_it.su ./stm32-ws2812-master/examples/f411/Core/Src/syscalls.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/syscalls.d ./stm32-ws2812-master/examples/f411/Core/Src/syscalls.o ./stm32-ws2812-master/examples/f411/Core/Src/syscalls.su ./stm32-ws2812-master/examples/f411/Core/Src/sysmem.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/sysmem.d ./stm32-ws2812-master/examples/f411/Core/Src/sysmem.o ./stm32-ws2812-master/examples/f411/Core/Src/sysmem.su ./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.cyclo ./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.d ./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.o ./stm32-ws2812-master/examples/f411/Core/Src/system_stm32f4xx.su

.PHONY: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Src

