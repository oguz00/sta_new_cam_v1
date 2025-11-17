################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../User_Src/command_tracking.cpp 

C_SRCS += \
../User_Src/command_handler.c \
../User_Src/packet_builder.c 

C_DEPS += \
./User_Src/command_handler.d \
./User_Src/packet_builder.d 

OBJS += \
./User_Src/command_handler.o \
./User_Src/command_tracking.o \
./User_Src/packet_builder.o 

CPP_DEPS += \
./User_Src/command_tracking.d 


# Each subdirectory must supply rules for building sources it contributes
User_Src/%.o User_Src/%.su User_Src/%.cyclo: ../User_Src/%.c User_Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L432xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/home/oguz/gitlocalRepos/work_2/sta_new_cam_v1/User_Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
User_Src/%.o User_Src/%.su User_Src/%.cyclo: ../User_Src/%.cpp User_Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L432xx -c -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I"/home/oguz/gitlocalRepos/work_2/sta_new_cam_v1/User_Inc" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-User_Src

clean-User_Src:
	-$(RM) ./User_Src/command_handler.cyclo ./User_Src/command_handler.d ./User_Src/command_handler.o ./User_Src/command_handler.su ./User_Src/command_tracking.cyclo ./User_Src/command_tracking.d ./User_Src/command_tracking.o ./User_Src/command_tracking.su ./User_Src/packet_builder.cyclo ./User_Src/packet_builder.d ./User_Src/packet_builder.o ./User_Src/packet_builder.su

.PHONY: clean-User_Src

