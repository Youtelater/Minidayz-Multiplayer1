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
GFX         := gfx

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
export VPATH          := $(TOPDIR)/$(SOURCES) $(TOPDIR)/$(DATA) $(TOPDIR)/$(GFX)
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

export LIBDIRS        := $(LIBDIRS)
export LIBPATHS       := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export INCLUDE        := $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
                         $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                         -I$(TOPDIR)/$(BUILD)

.PHONY: clean all

define CONVERT_SCRIPT
import os, subprocess
gfx = os.path.abspath("gfx")
build = os.path.abspath("build")
os.makedirs(build, exist_ok=True)
pngs = [f for f in os.listdir(gfx) if f.endswith(".png")] if os.path.exists(gfx) else []
print(f"Converting {len(pngs)} PNG files...")
for f in pngs:
    raw = os.path.splitext(f)[0]
    target = raw.replace("-", "_")
    src = os.path.join(gfx, f)
    out_o = os.path.join(build, f"{target}.o")
    out_h = os.path.join(build, f"{target}.h")
    if os.path.exists(out_o) and os.path.getmtime(out_o) > os.path.getmtime(src):
        continue
    cfg = os.path.join(gfx, f"{raw}.grit")
    cmd = ["grit", src] + (["-ff", cfg] if os.path.exists(cfg) else ["-gt", "-gB16"]) + ["-s", target, "-o", os.path.join(build, target)]
    subprocess.run(cmd, check=True)
    gen_o = os.path.join(build, f"{raw}.o")
    if os.path.exists(gen_o) and gen_o != out_o:
        os.replace(gen_o, out_o)
    gen_h = os.path.join(build, f"{raw}.h")
    if os.path.exists(gen_h) and gen_h != out_h:
        os.replace(gen_h, out_h)
endef
export CONVERT_SCRIPT

all: $(BUILD)
	@python3 -c "$$CONVERT_SCRIPT"
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
