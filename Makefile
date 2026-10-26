#-----------------------------------------------------------------
.SUFFIXES:
#-----------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)

#-----------------------------------------------------------------
# TARGET CONFIGURATION
#-----------------------------------------------------------------
TARGET		:= minidayz-3ds
BUILD		:= build
SOURCES		:= source
DATA		:= data
PORTLIBS	:= /opt/devkitpro/portlibs/3ds
ROMFS		:= romfs
INCLUDES	:= include build /opt/devkitpro/libctru/include $(PORTLIBS)/include
GRAPHICS	:= gfx

# Include 3ds_rules *after* configuration variables are set
include $(DEVKITARM)/3ds_rules

#-----------------------------------------------------------------
# FIND SOURCE FILES & OBJECTS
#-----------------------------------------------------------------
CPPFILES	:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
CFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
SFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.s))

OFILES		:= $(patsubst $(SOURCES)/%.cpp,$(BUILD)/%.o,$(CPPFILES)) \
			   $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES)) \
			   $(patsubst $(SOURCES)/%.s,$(BUILD)/%.o,$(SFILES))

INCLUDE		:= $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
			   $(foreach dir,$(SOURCES),-I$(TOPDIR)/$(dir)) \
			   -I$(TOPDIR)/$(ROMFS) \
			   -I/opt/devkitpro/libctru/include \
			   -I$(PORTLIBS)/include \
			   -I$(DEVKITARM)/arm-none-eabi/include

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=cp15

CFLAGS		:= -g -Wall -O2 -mword-relocations \
			   -ffunction-sections \
			   $(ARCH) $(DEFINES) \
			   $(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs -g $(ARCH) -L/opt/devkitpro/libctru/lib -L/opt/devkitpro/portlibs/3ds/lib -Wl,-Map,$(notdir $*.map)

LIBS		:= -lcitro2d -lcitro3d -lctru -lm

#-----------------------------------------------------------------
# MAIN RULES
#-----------------------------------------------------------------
all: graphics $(BUILD) $(TARGET).3dsx

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(ROMFS)/gfx/*.t3x

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	@echo builtin 3dsx ...
	@3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS)

$(TARGET).smdh:
	@if [ -f icon-256.png ]; then \
		magick icon-256.png -resize 48x48 icon_48.png 2>/dev/null || convert icon-256.png -resize 48x48 icon_48.png; \
		smdhtool --create "Mini DAYZ 3DS" "Mini DAYZ Port" "Developer" icon_48.png $@; \
		rm -f icon_48.png; \
	else \
		touch $@; \
	fi

$(TARGET).elf: $(OFILES)
	@echo linking ...
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

$(BUILD)/%.o: $(SOURCES)/%.cpp
	@echo cxx $<
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.c
	@echo cc $<
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.s
	@echo as $<
	@$(CC) -c $(ASFLAGS) $< -o $@

#-----------------------------------------------------------------
# GRAPHICS ASSET PIPELINE
#-----------------------------------------------------------------
graphics:
	@mkdir -p $(ROMFS)/gfx
	@for png in $(GRAPHICS)/*.png; do \
		if [ -f "$$png" ]; then \
			fname=$$(basename "$$png"); \
			fbase=$${fname%.png}; \
			safebase=$$(echo "$$fbase" | tr '-' '_' | tr '.' '_'); \
			echo "Building texture sheet: $$safebase.t3x"; \
			tex3ds -o "$(ROMFS)/gfx/$$safebase.t3x" "$$png" || exit 1; \
		fi; \
	done


