################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.s 

OBJS += \
./stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.o 

S_DEPS += \
./stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.d 


# Each subdirectory must supply rules for building sources it contributes
stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.o: D:/Apps/STM32IDE/packs/stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.s stm32-ws2812-master/examples/f411/Core/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Startup

clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Startup:
	-$(RM) ./stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.d ./stm32-ws2812-master/examples/f411/Core/Startup/startup_stm32f411ceux.o

.PHONY: clean-stm32-2d-ws2812-2d-master-2f-examples-2f-f411-2f-Core-2f-Startup

