#---------------------------------------------------------------------------------
# TARGET SETTINGS
#---------------------------------------------------------------------------------
.SUFFIXES:

# Set TOPDIR to current directory if not already set by parent make process
TOPDIR      ?= $(CURDIR)
TARGET      := $(notdir $(TOPDIR))
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        := data
GFX         := gfx

export TOPDIR
export TARGET
export GFXDIR   := $(TOPDIR)/$(GFX)

#---------------------------------------------------------------------------------
# COMPILER AND TOOL SELECTION
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to devkitARM>")
endif

include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# BUILD FLAGS
#---------------------------------------------------------------------------------
ARCH      := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS    := -g -Wall -O2 -mword-relocations \
             -fomit-frame-pointer \
             $(ARCH)

CFLAGS    += $(INCLUDE) -D__3DS__

CXXFLAGS  := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS   := -g $(ARCH)

LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# LIBRARIES TO LINK
#---------------------------------------------------------------------------------
LIBDIRS   := $(PORTLIBS) $(CTRULIB)
LIBS      := -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# BUILD PROCESS RULES
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT         := $(TOPDIR)/$(TARGET)
export VPATH          := $(TOPDIR)/$(SOURCES) \
                         $(TOPDIR)/$(DATA) \
                         $(TOPDIR)/$(GFX)

export DEPSDIR        := $(TOPDIR)/$(BUILD)

CFILES                := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.c))
CPPFILES              := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.cpp))
SFILES                := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.s))
PICAFILES             := $(notdir $(wildcard $(TOPDIR)/$(SOURCES)/*.v.pica))
BINFILES              := $(notdir $(wildcard $(TOPDIR)/$(DATA)/*.*))
PNGFILES              := $(notdir $(wildcard $(TOPDIR)/$(GFX)/*.png))

export OFILES_BIN     := $(addsuffix .o, $(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(PICAFILES:.v.pica=.o)
export OFILES_GFX     := $(subst -,_,$(PNGFILES:.png=.o))
export OFILES         := $(OFILES_BIN) $(OFILES_SOURCES) $(OFILES_GFX)

export HFILES         := $(addsuffix .h, $(subst .,_,$(BINFILES))) $(subst -,_,$(PNGFILES:.png=.h))

export LIBDIRS        := $(LIBDIRS)
export LIBPATHS       := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export INCLUDE        := $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
                         $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                         -I$(TOPDIR)/$(BUILD)

.PHONY: clean all

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(TOPDIR)/Makefile

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf

else

#---------------------------------------------------------------------------------
# COMPILATION TARGETS
#---------------------------------------------------------------------------------
DEPENDS   := $(OFILES:.o=.d)

all : $(OUTPUT).3dsx

$(OUTPUT).3dsx : $(OUTPUT).elf

$(OUTPUT).elf : $(OFILES)
	@echo linking $(notdir $@)
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

# Direct matching using standard shell expansion in TOPDIR/gfx
%.o %.h :
	@TARGET_BASE="$*"; \
	MATCH=$$(for f in "$$GFXDIR"/*.png; do \
		fname=$$(basename "$$f" .png); \
		uname=$$(echo "$$fname" | tr '-' '_'); \
		if [ "$$uname" = "$$TARGET_BASE" ]; then echo "$$f"; break; fi; \
	done); \
	if [ -n "$$MATCH" ]; then \
		echo converting $$(basename "$$MATCH"); \
		grit "$$MATCH" -ff "$${MATCH%.png}.grit" -o$*; \
	else \
		echo "Error: Could not find PNG matching object $* in $$GFXDIR"; exit 1; \
	fi

-include $(DEPENDS)

endif
