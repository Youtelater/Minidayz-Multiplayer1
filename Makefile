#---------------------------------------------------------------------------------
# TARGET & DIRECTORY SETTINGS
#---------------------------------------------------------------------------------
.SUFFIXES:

export TOPDIR   := $(CURDIR)
export TARGET   := $(notdir $(CURDIR))
export BUILD    := build
export SOURCES  := source
export INCLUDES := include
export DATA     := data
export GFX      := gfx
export GFXDIR   := $(CURDIR)/$(GFX)

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

export OUTPUT         := $(CURDIR)/$(TARGET)
export VPATH          := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                         $(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
                         $(foreach dir,$(GFX),$(CURDIR)/$(dir))

export DEPSDIR        := $(CURDIR)/$(BUILD)

CFILES                := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES              := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES                := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES             := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
BINFILES              := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
PNGFILES              := $(foreach dir,$(GFX),$(notdir $(wildcard $(dir)/*.png)))

export OFILES_BIN     := $(addsuffix .o, $(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(PICAFILES:.v.pica=.o)
export OFILES_GFX     := $(subst -,_,$(PNGFILES:.png=.o))
export OFILES         := $(OFILES_BIN) $(OFILES_SOURCES) $(OFILES_GFX)

export HFILES         := $(addsuffix .h, $(subst .,_,$(BINFILES))) $(subst -,_,$(PNGFILES:.png=.h))

export LIBDIRS        := $(LIBDIRS)
export LIBPATHS       := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export INCLUDE        := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                         $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                         -I$(CURDIR)/$(BUILD)

.PHONY: clean all

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

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

# Direct matching using standard shell expansion instead of nested find loops
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
