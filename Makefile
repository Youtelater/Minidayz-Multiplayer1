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
T3SFILES              := $(notdir $(wildcard $(TOPDIR)/$(GRAPHICS)/*.t3s))

export OFILES_BIN     := $(addsuffix .o, $(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(PICAFILES:.v.pica=.o)
export OFILES_T3X     := $(T3SFILES:.t3s=.o)
export OFILES         := $(OFILES_BIN) $(OFILES_SOURCES) $(OFILES_T3X)

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
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf

else

DEPENDS   := $(OFILES:.o=.d)

all : $(OUTPUT).3dsx

$(OUTPUT).3dsx : $(OUTPUT).elf

$(OUTPUT).elf : $(OFILES)
	@echo Linking $(notdir $@)...
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

-include $(DEPENDS)

endif
