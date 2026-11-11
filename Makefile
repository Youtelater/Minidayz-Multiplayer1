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
INCLUDES	:= include source /opt/devkitpro/libctru/include $(PORTLIBS)/include

#-----------------------------------------------------------------
# SOURCE & OBJECT MAPPING (Dynamic & Reliable)
#-----------------------------------------------------------------
# Automatically find all source files in the source directory
CPPFILES	:= $(notdir $(wildcard $(SOURCES)/*.cpp))
CFILES		:= $(notdir $(wildcard $(SOURCES)/*.c))
SFILES		:= $(notdir $(wildcard $(SOURCES)/*.s))

# Automatically generate corresponding object files inside the build directory
OFILES		:= $(addprefix $(BUILD)/, $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o))

# Search paths for source files
vpath %.cpp source
vpath %.c source
vpath %.s source

# Include 3ds_rules AFTER configuration and object mapping variables are set
include $(DEVKITARM)/3ds_rules

#-----------------------------------------------------------------
# COMPILATION FLAGS
#-----------------------------------------------------------------
INCLUDE		:= $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
			   -I$(TOPDIR)/$(ROMFS) \
			   -I/opt/devkitpro/libctru/include \
			   -I$(PORTLIBS)/include \
			   -I$(DEVKITARM)/arm-none-eabi/include

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=cp15

# Added -DMSGPACK_NO_BOOST to prevent looking for external boost headers
CFLAGS		:= -g -Wall -O2 -mword-relocations \
			   -ffunction-sections \
			   $(ARCH) $(DEFINES) -DMSGPACK_NO_BOOST \
			   $(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti

ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs -g $(ARCH) -L/opt/devkitpro/libctru/lib -L/opt/devkitpro/portlibs/3ds/lib -Wl,-Map,$(notdir $*.map)

# -lz handles zlib stream decompression
LIBS		:= -lcitro2d -lcitro3d -lctru -lz -lm

# Collect all files inside romfs to track dependencies accurately
# Escape spaces so GNU Make treats each ROMFS filename as one prerequisite.
ROMFS_FILES := $(shell find "$(ROMFS)" -type f -printf '%p\n' 2>/dev/null | sed 's/ /\\ /g')

#-----------------------------------------------------------------
# MAIN RULES
#-----------------------------------------------------------------
.PHONY: all clean stage1-assets

all: stage1-assets $(BUILD) $(TARGET).3dsx

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

# Explicitly depend on generated assets and ROMFS contents so packaging includes them
$(TARGET).3dsx: stage1-assets $(TARGET).elf $(TARGET).smdh $(ROMFS_FILES)
	@echo building 3dsx with romfs ...
	@3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS)

$(TARGET).smdh:
	@smdhtool --create "Mini DAYZ 3DS" "Mini DAYZ Port" "Developer" "" $@ 2>/dev/null || touch $@

$(TARGET).elf: $(OFILES)
	@echo linking ...
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

$(OFILES): | stage1-assets

$(BUILD)/%.o: %.cpp
	@echo cxx $<
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD)/%.o: %.c
	@echo cc $<
	@$(CXX) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s
	@echo as $<
	@$(CC) -c $(ASFLAGS) $< -o $@

#-----------------------------------------------------------------
# STAGE 1 SPRITE CATALOG / FRAME ATLAS GENERATION
#-----------------------------------------------------------------
# The engine loads the generated, frame-indexed .t3x atlases directly from
# romfs/gfx. build_sprite_catalog.py writes those atlases beside the existing
# full-sheet .t3x files and generates the matching C++ lookup header.
# Do not run tools/build_textures.py here: bulk PNG -> T3X conversion is no
# longer part of the normal build.
stage1-assets:
	@mkdir -p $(ROMFS)/gfx
	@python3 tools/stage1/reassemble_snapshot.py
	@python3 tools/stage1/extract_construct.py data/stage1/construct_project_parsed.json --out data/stage1
	@python3 tools/stage1/extract_location_templates.py
	@python3 tools/stage1/audit_event_groups.py data/stage1/construct_project_parsed.json --out data/stage1/event_groups.csv
	@python3 tools/stage1/validate_stage1.py
	@python3 tools/stage1/build_sprite_catalog.py


