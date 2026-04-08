################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/str/fsl_str.c 

C_DEPS += \
./utilities/str/fsl_str.d 

OBJS += \
./utilities/str/fsl_str.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/str/%.o: ../utilities/str/%.c utilities/str/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXC444VLH -DCPU_MCXC444VLH_cm0plus -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_DEBUGCONSOLE_UART -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\board" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\source" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\drivers" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\CMSIS" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\CMSIS\m-profile" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\utilities" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\utilities\debug_console\config" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\device" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\device\periph2" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\utilities\debug_console" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\component\serial_manager" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\component\lists" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\utilities\str" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\component\uart" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\freertos\freertos-kernel\include" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\freertos\freertos-kernel\portable\GCC\ARM_CM0" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\freertos\freertos-kernel\template" -I"C:\Users\Yue Yang\Documents\MCUXpressoIDE_25.6.136\workspace\2271Projecy\freertos\freertos-kernel\template\ARM_CM0" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities-2f-str

clean-utilities-2f-str:
	-$(RM) ./utilities/str/fsl_str.d ./utilities/str/fsl_str.o

.PHONY: clean-utilities-2f-str

