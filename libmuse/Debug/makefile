################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include libmuse/subdir.mk
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
all: liblibmuse.so

dependents:
	-cd /home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug && $(MAKE) clean all

# Tool invocations
liblibmuse.so: $(OBJS) $(USER_OBJS) /home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug/libshared.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug" -shared -o "liblibmuse.so" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(LIBRARIES)$(OBJS)$(C_DEPS) liblibmuse.so
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/tp-2019-2c-Vendo-FIAT-600/shared/Debug/libshared.so:

-include ../makefile.targets
