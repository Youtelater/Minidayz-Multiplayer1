#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to devkitARM>")
endif

include $(DEVKITARM)/3ds_rules

TARGET		:=	minidayz-3ds
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include
ROMFS		:=	romfs
GRAPHICS	:=	gfx

CFLAGS		:= -g -Wall -Wextra -O3 -mword-relocations \
			   -ffunction-sections \
			   $(ARCH)

CXXFLAGS	:= $(CFLAGS) -std=gnu++11

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS		:= -lcitro2d -lcitro3d -lctru -lm

# Automatically compile any .t3s file found in gfx/ into romfs/gfx/
T3SFILES    := $(wildcard $(GRAPHICS)/*.t3s)
TEXFILES    := $(T3SFILES:$(GRAPHICS)/%.t3s=$(ROMFS)/gfx/%.t3x)

# Ensure graphics build rules are evaluated
all: $(TEXFILES)

$(ROMFS)/gfx/%.t3x: $(GRAPHICS)/%.t3s
	@mkdir -p $(dir $@)
	tex3ds -i $< -o $@

# Standard devkitPro build targets follow...
