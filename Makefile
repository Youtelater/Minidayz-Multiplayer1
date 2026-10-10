.SUFFIXES: .t3s .t3x

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.. export DEVKITARM=<path> அதற்கு பின் . /etc/profile.d/devkit-env.sh")
endif

# Target output name and folders
TARGET		:=	minidayz-3ds
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include
ROMFS		:=	romfs

# Automatically compile all .t3s texture files found in gfx/ into romfs/gfx/
T3SFILES	:=	$(wildcard gfx/*.t3s)
TEXFILES	:=	$(T3SFILES:gfx/%.t3s=$(ROMFS)/gfx/%.t3x)

# Ensure texture compilation runs before building the binary
export RSFS_DEPS := $(TEXFILES)

# Basic toolchain flags
ARCH		:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS		:=	-g -Wall -O2 -ffunction-sections $(ARCH) -D__3DS__
CXXFLAGS	:=	$(CFLAGS) -std=gnu++11 -fno-rtti -fno-exceptions
ASFLAGS		:=	-g $(ARCH)
LDFLAGS		:=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $@).map

LIBS		:=	-lciture2d -lcitro3d -lctru -lm

# Include standard devkitPro build rules
include $(DEVKITARM)/3ds_rules

# Rule to compile texture sheets using tex3ds
$(ROMFS)/gfx/%.t3x: gfx/%.t3s
	@echo "Building texture sheet $<..."
	@mkdir -p $(dir $@)
	@tex3ds -i $< -o $@
