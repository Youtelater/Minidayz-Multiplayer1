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
GRAPHICS	:= assets/gfx

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
all: graphics $(BUILD) $(TARGET).3dsx

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh

# Explicitly depend on graphics and ROMFS contents to force rebuilding .3dsx when romfs changes
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh graphics $(ROMFS_FILES)
	@echo building 3dsx with romfs ...
	@3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh --romfs=$(ROMFS)

$(TARGET).smdh:
	@smdhtool --create "Mini DAYZ 3DS" "Mini DAYZ Port" "Developer" "" $@ 2>/dev/null || touch $@

$(TARGET).elf: $(OFILES)
	@echo linking ...
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

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
# GRAPHICS ASSET PIPELINE
#-----------------------------------------------------------------
# Python handles arbitrary original filenames safely and converts only changed PNGs.
graphics:
	@mkdir -p $(ROMFS)/gfx
	@python3 tools/build_textures.py "$(GRAPHICS)" "$(ROMFS)/gfx"


