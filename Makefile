# Makefile for BreezySTM32F1 examples
#
# Copyright (C) 2016 Simon D. Levy
#
# This file is part of BreezySTM32.
#
# BreezySTM32 is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this code.  If not, see <http://www.gnu.org/licenses/>.

###############################################################################
NAZE32_REV		?= 6

SERIAL_DEVICE	?= /dev/ttyUSB0
SERIAL_BAUD		?= 921600
###############################################################################

ARCH_FLAGS	= -mthumb \
			  -DFIXMATH_NO_CACHE \
			  -std=gnu11

# Change this to wherever you put BreezySTM32
BREEZY_DIR	= $(HERE)/lib/BreezySTM32

F1_DIR		= $(BREEZY_DIR)/f1

include makefile.project

TARGET		?= $(PROJECT_NAME)_naze32_r$(NAZE32_REV)

# Compile-time options
OPTIONS		?=

# Debugger optons, must be empty or GDB
DEBUG		?=

###############################################################################
# Things that need to be maintained as the source changes
#

# Working directories
ROOT		 = ..
HERE         = .
SRC_DIR		 = $(ROOT)
OBJECT_DIR	 = $(HERE)/build
BIN_DIR		 = $(HERE)/build
CMSIS_DIR	 = $(BREEZY_DIR)/lib/CMSIS

ARCH = 1

STDPERIPH_DIR = $(BREEZY_DIR)/lib/STM32F10x_StdPeriph_Driver

DEVICE_INCLUDES = $(CMSIS_DIR)/CM3/CoreSupport \
				  $(CMSIS_DIR)/CM3/DeviceSupport/ST/STM32F10x

ARCH_FLAGS	 += -mcpu=cortex-m3

DEVICE_FLAGS = -DSTM32F10X_MD -DI2CDEV=I2CDEV_2 -DUSART=USART2

SUPPORT		:= $(CMSIS_DIR)/CM3/CoreSupport:$(CMSIS_DIR)/CM3/DeviceSupport/ST/STM32F10x

CMSIS_SRC	 = $(notdir $(wildcard $(CMSIS_DIR)/CM3/CoreSupport/*.c \
			               $(CMSIS_DIR)/CM3/DeviceSupport/ST/STM32F10x/*.c))

DEVICE_SRC = $(F1_DIR)/startup_stm32f10x_md_gcc.S \
			 $(F1_DIR)/system_stm32f10x.c \
			 $(F1_DIR)/uart_stm32f10x.c \
			 $(F1_DIR)/timer.c \
			 $(F1_DIR)/gpio.c

TARGET_IMG = $(TARGET_HEX)

DO_FLASH  = stm32flash -w $(TARGET_HEX) -v -g 0x0 -b $(SERIAL_BAUD) $(SERIAL_DEVICE)

# Source files common to all targets
$(TARGET)_SRC = $(PROJECT_SRC_FILES) \
			    $(PROJECT_SRC_PARAM_GEN) \
			    $(PROJECT_SRC_LIBFIXMATH) \
			    $(PROJECT_SRC_LIBFIXMATRIX) \
			    $(PROJECT_BREEZY_FILES) \
			    $(F1_DIR)/system.c \
			    $(F1_DIR)/serial.c \
			    $(DEVICE_SRC) \
			    $(CMSIS_SRC) \
			    $(STDPERIPH_SRC)

# In some cases, %.s regarded as intermediate file, which is actually not.
# This will prevent accidental deletion of startup code.
.PRECIOUS: %.s

VPATH		:= $(SRC_DIR)

# Search path and source files for the CMSIS sources
VPATH		:= $(VPATH):$(SUPPORT)

# Search path and source files for the ST stdperiph library
VPATH		:= $(VPATH):$(STDPERIPH_DIR):$(STDPERIPH_DIR)/src
STDPERIPH_SRC	 = $(notdir $(wildcard $(STDPERIPH_DIR)/src/*.c))

###############################################################################
# Things that might need changing to use different tools
#

# Tool names
CC		 = arm-none-eabi-gcc
OBJCOPY	 = arm-none-eabi-objcopy

#
# Tool options.
#
INCLUDE_DIRS = ./include \
			   ./lib \
			   $(PROJECT_DIR_LIBFIXMATH) \
			   $(PROJECT_DIR_LIBFIXMATRIX) \
			   $(BREEZY_DIR) \
			   $(F1_DIR) \
			   $(STDPERIPH_DIR)/inc \
			   $(DEVICE_INCLUDES)


ifeq ($(DEBUG),GDB)
OPTIMIZE	 = -Og
LTO_FLAGS	 = $(OPTIMIZE)
else
OPTIMIZE	 = -Os
LTO_FLAGS	 = -flto -fuse-linker-plugin $(OPTIMIZE)
endif

DEBUG_FLAGS	 = -ggdb3

#XXX: (-Wno-char-subscripts -Wno-sign-compare) used to disable warnings
#	  in the libfixmath and libfixmatrix compilations
CFLAGS	 = $(ARCH_FLAGS) \
		   $(LTO_FLAGS) \
		   $(addprefix -D,$(OPTIONS)) \
		   $(addprefix -I,$(INCLUDE_DIRS)) \
		   $(DEBUG_FLAGS) \
		   -Wall -pedantic -Wextra -Wshadow -Wunsafe-loop-optimizations \
		   -Wno-char-subscripts -Wno-sign-compare \
		   -ffunction-sections \
		   -fdata-sections \
		   $(DEVICE_FLAGS) \
		   -DUSE_STDPERIPH_DRIVER \
		   -D$(TARGET) \
		   -Wno-unused-parameter \
		   -DNAZE32_REV=$(NAZE32_REV) \
		   -DGIT_VERSION_FLIGHT_STR=\"$(GIT_VERSION_FLIGHT)\" \
		   -DGIT_VERSION_OS_STR=\"$(GIT_VERSION_OS)\" \
		   -DGIT_VERSION_MAVLINK_STR=\"$(GIT_VERSION_MAVLINK)\" \
		   -DEEPROM_CONF_VERSION_STR=\"$(EEPROM_CONF_VERSION)\"

ASFLAGS		 = $(ARCH_FLAGS) \
		   -x assembler-with-cpp \
		   $(addprefix -I,$(INCLUDE_DIRS))

# XXX Map/crossref output?
LD_SCRIPT	 = $(F1_DIR)/stm32_flash.ld
LDFLAGS	 = -lm \
		   -nostartfiles \
		   -lc \
  		   --specs=rdimon.specs \
		   $(ARCH_FLAGS) \
		   $(LTO_FLAGS) \
		   $(DEBUG_FLAGS) \
		   -static \
		   -Wl,-gc-sections,-Map,$(TARGET_MAP) \
		   -T$(LD_SCRIPT)

###############################################################################
# No user-serviceable parts below
###############################################################################

#
# Things we will build
#

TARGET_BIN	 = $(BIN_DIR)/$(TARGET).bin
TARGET_HEX	 = $(BIN_DIR)/$(TARGET).hex
TARGET_ELF	 = $(BIN_DIR)/$(TARGET).elf
TARGET_OBJS	 = $(addsuffix .o,$(addprefix $(OBJECT_DIR)/$(TARGET)/,$(basename $($(TARGET)_SRC))))
TARGET_MAP   = $(OBJECT_DIR)/$(TARGET).map

# List of buildable ELF files and their object dependencies.
# It would be nice to compute these lists, but that seems to be just beyond make.

$(TARGET_BIN): $(TARGET_ELF)
		$(V0) $(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) -O ihex --set-start 0x8000000 $< $@

$(TARGET_ELF):  $(TARGET_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

MKDIR_OBJDIR = @mkdir -p $(dir $@)

# Compile C
$(OBJECT_DIR)/$(TARGET)/%.o: %.c
	$(MKDIR_OBJDIR)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(CFLAGS) $<

# Compile C++
$(OBJECT_DIR)/$(TARGET)/%.o: %.cpp
	$(MKDIR_OBJDIR)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(CFLAGS) $<

# Assemble
$(OBJECT_DIR)/$(TARGET)/%.o: %.S
	$(MKDIR_OBJDIR)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASFLAGS) $<

clean:
#	rm -rf *.o obj $(TARGET_HEX) $(TARGET_ELF) $(TARGET_OBJS) $(TARGET_MAP)
	rm -rf ./build
	rm ./lib/param_generator/param_gen.h
	rm ./lib/param_generator/param_gen.c
	rm ./lib/param_generator/PARAMS.md

flash: flash_$(TARGET)

flash_$(TARGET): $(TARGET_IMG)
#	stty -F $(SERIAL_DEVICE) raw speed $(SERIAL_BAUD) -crtscts cs8 -parenb -cstopb -ixon
#	echo -n 'R' >$(SERIAL_DEVICE)
#	sleep 1
	$(DO_FLASH)

mavlink_bootloader:
	./lib/scripts/reboot_bootloader --device $(SERIAL_DEVICE) --baudrate $(SERIAL_BAUD)
	sleep 1

reflash: reflash_$(TARGET)

reflash_$(TARGET): $(TARGET_IMG) mavlink_bootloader flash_$(TARGET)

unbrick: unbrick_$(TARGET)

unbrick_$(TARGET): $(TARGET_IMG)
	$(DO_FLASH)

listen:
	#picocom $(SERIAL_DEVICE) -b $(SERIAL_BAUD)
	stty -F $(SERIAL_DEVICE) raw speed $(SERIAL_BAUD) -crtscts cs8 -parenb -cstopb -ixon
	od -x < $(SERIAL_DEVICE)
