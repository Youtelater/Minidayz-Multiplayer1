#-----------------------------------------------------------------
.SUFFIXES:
#-----------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#-----------------------------------------------------------------
# TARGET CONFIGURATION
#-----------------------------------------------------------------
TARGET		:= minidayz-3ds
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= include
GRAPHICS	:= gfx
ROMFS		:= romfs

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=cp15

CFLAGS		:= -g -Wall -O2 -mword-relocations \
			   -ffunction-sections \
			   $(ARCH) $(DEFINES)

CFLAGS		+= $(INCLUDE) -I$(BUILD)

CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS		:= -lcitro2d -lcitro3d -lctru -lm

#-----------------------------------------------------------------
# FIND SOURCE FILES
#-----------------------------------------------------------------
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

#-----------------------------------------------------------------
# OBJECT FILES & BUILDS
#-----------------------------------------------------------------
OFILES		:= $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

INCLUDE		:= $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
			   $(foreach dir,$(SOURCES),-I$(TOPDIR)/$(dir)) \
			   -I$(TOPDIR)/$(ROMFS)

export VPATH	:= $(foreach dir,$(SOURCES),$(TOPDIR)/$(dir)) \
				   $(foreach dir,$(DATA),$(TOPDIR)/$(dir))

CFILES		:= $(CFILES)
CPPFILES	:= $(CPPFILES)

#-----------------------------------------------------------------
# MAIN RULES
#-----------------------------------------------------------------
ifeq ($(BUILD),$(notdir $(BUILD)))
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

all: graphics
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(ROMFS)/gfx

else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).3dsx:	$(OUTPUT).elf

$(OUTPUT).elf:	$(OFILES)

# Include dependencies
-include $(DEPENDS)

endif

#-----------------------------------------------------------------
# GRAPHICS ASSET PIPELINE (With hyphen-to-underscore sanitization)
#-----------------------------------------------------------------
graphics:
	@mkdir -p $(ROMFS)/gfx
	@for png in $(GRAPHICS)/*.png; do \
		if [ -f "$$png" ]; then \
			fname=$$(basename "$$png"); \
			fbase=$${fname%.png}; \
			safebase=$$(echo "$$fbase" | tr '-' '_'); \
			echo "Auto-padding $$fbase to power-of-2 (512x512)..."; \
			magick "$$png" -background transparent -gravity center -extent 512x512 "$$png" 2>/dev/null || \
			convert "$$png" -background transparent -gravity center -extent 512x512 "$$png" || true; \
			echo "Building standalone texture sheet: $$safebase.t3x"; \
			tex3ds --format=rgba5551 "$$png" -o "$(ROMFS)/gfx/$$safebase.t3x" || exit 1; \
		fi; \
	done
