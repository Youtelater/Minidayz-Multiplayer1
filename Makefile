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

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS		:= -g -Wall -Wextra -O3 -mword-relocations \
			   -ffunction-sections \
			   $(ARCH)

CXXFLAGS	:= $(CFLAGS) -std=gnu++11

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs -g $(ARCH) -L/opt/devkitpro/libctru/lib -Wl,-Map,$(notdir $*.map)

LIBS		:= -lcitro2d -lcitro3d -lctru -lm
LIBDIRS		:= $(CTRULIB) $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES	:= $(OFILES_SOURCES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export ROMFS_DIR:=	$(CURDIR)/$(ROMFS)

.PHONY: all clean graphics

all: graphics $(OUTPUT).3dsx

graphics:
	@mkdir -p $(ROMFS)/gfx
	@rm -f $(GRAPHICS)/*.t3s
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
	
$(BUILD):
	@mkdir -p $@

$(OUTPUT).3dsx: $(BUILD)
	@export DEVKITPRO="$(DEVKITPRO)"; \
	 export DEVKITARM="$(DEVKITARM)"; \
	 $(MAKE) DEVKITARM="$(DEVKITARM)" DEVKITPRO="$(DEVKITPRO)" ARCH="$(ARCH)" LIBPATHS="$(LIBPATHS)" ROMFS_DIR="$(ROMFS_DIR)" --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(ROMFS)/gfx $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map

else

dependency := $(OFILES:.o=.d)

-include $(dependency)

$(OUTPUT).3dsx : $(OUTPUT).elf
	@echo 3dsxtool $< $@ --romfs=$(ROMFS_DIR)
	@3dsxtool $< $@ --romfs=$(ROMFS_DIR)

$(OUTPUT).elf : $(OFILES)
	@echo LD $(notdir $@)
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

%.o: %.cpp
	@echo g++ $(notdir $<)
	@$(CXX) -c $(CXXFLAGS) $(INCLUDE) $< -o $@

%.o: %.c
	@echo gcc $(notdir $<)
	@$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

%.o: %.s
	@echo cc -x assembler-with-cpp $(notdir $<)
	@$(CC) -c -x assembler-with-cpp $(ASFLAGS) $(INCLUDE) $< -o $@

endif
