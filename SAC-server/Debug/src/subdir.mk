################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/SAC-server.c \
../src/aux.c \
../src/nodes.c \
../src/operations.c \
../src/sac_config.c 

OBJS += \
./src/SAC-server.o \
./src/aux.o \
./src/nodes.o \
./src/operations.o \
./src/sac_config.o 

C_DEPS += \
./src/SAC-server.d \
./src/aux.d \
./src/nodes.d \
./src/operations.d \
./src/sac_config.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


