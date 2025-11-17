################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UsrSrc/command_handler.c \
../UsrSrc/commands_tracking.c \
../UsrSrc/packet_builder.c \
../UsrSrc/uart_handler.c 

C_DEPS += \
./UsrSrc/command_handler.d \
./UsrSrc/commands_tracking.d \
./UsrSrc/packet_builder.d \
./UsrSrc/uart_handler.d 

OBJS += \
./UsrSrc/command_handler.o \
./UsrSrc/commands_tracking.o \
./UsrSrc/packet_builder.o \
./UsrSrc/uart_handler.o 


# Each subdirectory must supply rules for building sources it contributes
UsrSrc/%.o UsrSrc/%.su UsrSrc/%.cyclo: ../UsrSrc/%.c UsrSrc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L432xx -c -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/home/oguz/gitlocalRepos/work_2/sta_new_cam_v1/UsrInc" -I"/home/oguz/gitlocalRepos/work_2/sta_new_cam_v1/UsrSrc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-UsrSrc

clean-UsrSrc:
	-$(RM) ./UsrSrc/command_handler.cyclo ./UsrSrc/command_handler.d ./UsrSrc/command_handler.o ./UsrSrc/command_handler.su ./UsrSrc/commands_tracking.cyclo ./UsrSrc/commands_tracking.d ./UsrSrc/commands_tracking.o ./UsrSrc/commands_tracking.su ./UsrSrc/packet_builder.cyclo ./UsrSrc/packet_builder.d ./UsrSrc/packet_builder.o ./UsrSrc/packet_builder.su ./UsrSrc/uart_handler.cyclo ./UsrSrc/uart_handler.d ./UsrSrc/uart_handler.o ./UsrSrc/uart_handler.su

.PHONY: clean-UsrSrc

