################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
loader.bin \

SECONDARY_SIZE += \
loader.siz \


# All Target
all: loader.elf

# Tool invocations
loader.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C Linker'
	arm-linux-gnueabihf-gcc -mcpu=cortex-a7 -marm -O2  -g -T "/home/jaa/workspace/test/loader/loader.ld" -nostartfiles -nodefaultlibs -nostdlib -Wl,-Map,"loader.map" -v -ffreestanding -o "loader.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

loader.bin: loader.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-linux-gnueabihf-objcopy -O binary "loader.elf"  "loader.bin"
	@echo 'Finished building: $@'
	@echo ' '

loader.siz: loader.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-linux-gnueabihf-size --format=berkeley "loader.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(SECONDARY_FLASH)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS) loader.elf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
