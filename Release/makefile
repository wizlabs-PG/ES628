################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := ES628
BUILD_ARTIFACT_EXTENSION :=
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: main-build

# Main-build Target
main-build: ES628

# Tool invocations
ES628: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/wizlabs/eclipse-workspace/ES628/lib" -L/usr/lib/aarch64-linux-gnu/gstreamer-1.0 -L/usr/lib/aarch64-linux-gnu/tegra-egl -L/usr/lib/aarch64-linux-gnu/tegra -o "ES628" $(OBJS) $(USER_OBJS) $(LIBS) -lpthread -lg -lm -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lX11 -lXext -lasound -lGL -lpng16 -ljpeg -lgomp -lfreetype
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) ES628
	-@echo ' '

.PHONY: all clean dependents main-build

-include ../makefile.targets
