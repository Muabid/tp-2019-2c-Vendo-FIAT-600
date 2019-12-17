################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: SAC-cli

dependents:
	-cd /home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug && $(MAKE) clean all

# Tool invocations
SAC-cli: $(OBJS) $(USER_OBJS) /home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug/libshared.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L/home/utnso/so-commons-library -L"/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug" -o "SAC-cli" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) SAC-cli
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug/libshared.so:

-include ../makefile.targets
