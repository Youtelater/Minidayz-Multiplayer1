#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to devkitARM>")
endif

export TOPDIR ?= $(CURDIR)

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
LIBDIRS		:= $(CTRULIB) $(PORTLIBS)

INCLUDE		:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

OFILES		:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
OFILES		:=	$(addprefix $(BUILD)/,$(OFILES))

dependency	:=	$(OFILES:.o=.d)

.PHONY: all clean graphics

all: graphics $(TARGET).3dsx

graphics:
	@mkdir -p $(ROMFS)/gfx
	@# Purge legacy wrapper scripts entirely
	@rm -f $(GRAPHICS)/*.t3s
	@# Automatically resize/pad PNGs to a valid power-of-2 (512x512) and compile directly
	@for png in $(GRAPHICS)/*.png; do \
		if [ -f "$$png" ]; then \
			fname=$$(basename "$$png"); \
			fbase=$${fname%.png}; \
			echo "Auto-padding $$fbase to power-of-2 (512x512)..."; \
			magick "$$png" -background transparent -gravity center -extent 512x512 "$$png" 2>/dev/null || \
			convert "$$png" -background transparent -gravity center -extent 512x512 "$$png" || true; \
			echo "Building standalone texture sheet: $$fbase.t3x"; \
			tex3ds --format=rgba5551 "$$png" -o "$(ROMFS)/gfx/$$fbase.t3x" || exit 1; \
		fi; \
	done

$(BUILD):
	@mkdir -p $@

$(TARGET).3dsx: $(TARGET).elf
	@echo 3dsxtool $(notdir $<) $(notdir $@)
	@3dsxtool $< $@

$(TARGET).elf: $(OFILES)
	@echo LD $(notdir $@)
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

-include $(dependency)

$(BUILD)/%.o: $(SOURCES)/%.cpp
	@mkdir -p $(dir $@)
	@echo g++ $(notdir $<)
	@$(CXX) -c $(CXXFLAGS) $(INCLUDE) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(dir $@)
	@echo gcc $(notdir $<)
	@$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.s
	@mkdir -p $(dir $@)
	@echo cc -x assembler-with-cpp $(notdir $<)
	@$(CC) -c -x assembler-with-cpp $(ASFLAGS) $(INCLUDE) $< -o $@

clean:
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(ROMFS)/gfx $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map
