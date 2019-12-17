################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libmuse/libmuse.c 

OBJS += \
./libmuse/libmuse.o 

C_DEPS += \
./libmuse/libmuse.d 


# Each subdirectory must supply rules for building sources it contributes
libmuse/%.o: ../libmuse/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


