################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freertos/freertos-kernel/portable/MemMang/heap_4.c 

C_DEPS += \
./freertos/freertos-kernel/portable/MemMang/heap_4.d 

OBJS += \
./freertos/freertos-kernel/portable/MemMang/heap_4.o 


# Each subdirectory must supply rules for building sources it contributes
freertos/freertos-kernel/portable/MemMang/%.o: ../freertos/freertos-kernel/portable/MemMang/%.c freertos/freertos-kernel/portable/MemMang/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXC444VLH -DCPU_MCXC444VLH_cm0plus -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\board" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\source" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\drivers" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\CMSIS" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\CMSIS\m-profile" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\utilities" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\utilities\debug_console\config" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\device" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\device\periph2" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\utilities\debug_console" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\component\serial_manager" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\component\lists" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\utilities\str" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\component\uart" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\freertos\freertos-kernel\include" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\freertos\freertos-kernel\portable\GCC\ARM_CM0" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\freertos\freertos-kernel\template" -I"C:\Users\user\Documents\MCUXpressoIDE_25.6.136\workspace\Final_Project\freertos\freertos-kernel\template\ARM_CM0" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-freertos-2f-freertos-2d-kernel-2f-portable-2f-MemMang

clean-freertos-2f-freertos-2d-kernel-2f-portable-2f-MemMang:
	-$(RM) ./freertos/freertos-kernel/portable/MemMang/heap_4.d ./freertos/freertos-kernel/portable/MemMang/heap_4.o

.PHONY: clean-freertos-2f-freertos-2d-kernel-2f-portable-2f-MemMang

