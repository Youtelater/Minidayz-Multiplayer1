#---------------------------------------------------------------------------------
# TARGET SETTINGS
#---------------------------------------------------------------------------------
.SUFFIXES:

TOPDIR      ?= $(CURDIR)
TARGET      := $(notdir $(TOPDIR))
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        := data
GRAPHICS    := gfx
ROMFS       := romfs

# Metadata settings
APP_TITLE       := MiniDAYZ 3DS
APP_DESCRIPTION := MiniDAYZ Port
APP_AUTHOR      := Homebrew

export TOPDIR
export TARGET

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment.")
endif

include $(DEVKITARM)/3ds_rules

ARCH      := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS    := -g -Wall -O2 -mword-relocations -fomit-frame-pointer $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS  := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS   := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBDIRS   := $(PORTLIBS) $(CTRULIB)
LIBS      := -lcitro2d -lcitro3d -lctru -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT         := $(TOPDIR)/$(TARGET)
export VPATH          := $(TOPDIR)/$(SOURCES) $(TOPDIR)/$(DATA) $(TOPDIR)/$(GRAPHICS)
export DEPSDIR        := $(TOPDIR)/$(BUILD)

CFILES                := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.c))
CPPFILES              := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.cpp))
SFILES                := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.s))
PICAFILES             := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.v.pica))
BINFILES              := $(notdir $(wildcard $(TOPDIR)/$(DATA)/*.*))

export OFILES_BIN     := $(addsuffix .o, $(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(PICAFILES:.v.pica=.o)
export OFILES         := $(OFILES_BIN) $(OFILES_SOURCES)

export LIBDIRS        := $(LIBDIRS)
export LIBPATHS       := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export INCLUDE        := $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
                         $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                         -I$(TOPDIR)/$(BUILD)

.PHONY: clean all

all: $(BUILD)

$(BUILD):
	@mkdir -p $(TOPDIR)/$(ROMFS)/gfx
	@if [ -f $(TOPDIR)/$(GRAPHICS)/sprites.t3s ]; then \
		echo "Building texture sheet with tex3ds..."; \
		tex3ds -i $(TOPDIR)/$(GRAPHICS)/sprites.t3s -o $(TOPDIR)/$(ROMFS)/gfx/sprites.t3x; \
	fi
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

clean:
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(ROMFS)/gfx/*.t3x

else

DEPENDS   := $(OFILES:.o=.d)

all : $(OUTPUT).3dsx

$(OUTPUT).smdh :
	@echo "Creating SMDH metadata..."
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(DEVKITPRO)/libctru/default_icon.png $@

$(OUTPUT).3dsx : $(OUTPUT).elf $(OUTPUT).smdh
	@echo "Packaging 3DSX with RomFS..."
	@_3dsx_cmd="$$(which 3dsxtool 2>/dev/null || which elf23dsx 2>/dev/null || echo $(DEVKITARM)/bin/elf23dsx)"; \
	$$_3dsx_cmd $< $@ --smdh=$(OUTPUT).smdh --romfs=$(TOPDIR)/$(ROMFS)

$(OUTPUT).elf : $(OFILES)
	@echo Linking $(notdir $@)...
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

-include $(DEPENDS)

endif
