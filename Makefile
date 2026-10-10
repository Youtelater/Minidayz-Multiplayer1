#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to devkitARM>")
endif

export TOPDIR ?= $(CURDIR)

# Include devkitARM 3DS rules
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

# Explicitly define libctru and portlibs include directories
LIBDIRS		:= $(CTRULIB) $(PORTLIBS)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# Automatically find and compile all texture sheets in gfx/
T3SFILES    := $(wildcard $(GRAPHICS)/*.t3s)
TEXFILES    := $(T3SFILES:$(GRAPHICS)/%.t3s=$(ROMFS)/gfx/%.t3x)

.PHONY: all clean graphics

all: graphics $(TARGET).3dsx

graphics:
	@mkdir -p $(ROMFS)/gfx
	@for t3s in $(GRAPHICS)/*.t3s; do \
		if [ -f "$$t3s" ]; then \
			filename=$$(basename $$t3s .t3s); \
			echo "Building texture sheet: $$filename.t3x"; \
			tex3ds -i "$$t3s" -o "$(ROMFS)/gfx/$$filename.t3x"; \
		fi; \
	done

$(TARGET).3dsx: $(TARGET).elf

$(TARGET).elf: $(OFILES)

clean:
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(ROMFS)/gfx $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map

# Include auto-generated dependency files
-include $(OFILES:.o=.d)
