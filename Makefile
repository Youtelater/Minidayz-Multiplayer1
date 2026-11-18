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
	@python3 tools/stage1/extract_construct.py data.deobfuscated.js --out data/stage1
	@python3 tools/stage1/extract_npc_spawn_calls.py
	@python3 tools/stage1/extract_location_templates.py
	@python3 tools/stage1/audit_event_groups.py data/stage1/construct_project_parsed.json --out data/stage1/event_groups.csv
	@python3 tools/stage1/validate_stage1.py
	@python3 tools/stage1/build_sprite_catalog.py
	@python3 tools/stage1/build_ground_floor.py
	@test -s $(ROMFS)/gfx/original_map_ground_layout.bin || { echo 'ERROR: original Map ground layout missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_ground_p000.t3x || { echo 'ERROR: original Map floor atlas missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground.t3x || { echo 'ERROR: original source grass background missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl2.t3x || { echo 'ERROR: original level 2 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl3.t3x || { echo 'ERROR: original level 3 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl4.t3x || { echo 'ERROR: original level 4 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl5.t3x || { echo 'ERROR: original level 5 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl6.t3x || { echo 'ERROR: original level 6 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl7.t3x || { echo 'ERROR: original level 7 terrain missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl2.t3x || { echo 'ERROR: original level 2 ground texture missing'; exit 1; }
	@test -s $(ROMFS)/gfx/original_map_base_ground_lvl3.t3x || { echo 'ERROR: original level 3 ground texture missing'; exit 1; }
	@test -s $(ROMFS)/gfx/main_menu_logo_sheet0.t3x || { echo 'ERROR: original menu logo missing'; exit 1; }
	@test -s $(ROMFS)/gfx/menu_btn_wood_sheet0.t3x || { echo 'ERROR: original menu buttons missing'; exit 1; }
	@test -s $(ROMFS)/gfx/tiledbgmenu_city_up.t3x || { echo 'ERROR: original menu background missing'; exit 1; }
	@python3 tools/stage2/build_item_sprite_catalog.py
	@python3 tools/stage2/build_inventory_ui_art.py
	@python3 tools/stage2/build_item_source_catalog.py
	@python3 tools/stage2/build_item_description_catalog.py
	@python3 tools/stage2/build_backpack_capacity_catalog.py
	@python3 tools/stage2/build_weather_cold_catalog.py
	@python3 tools/stage2/build_item_secondary_crafting_catalog.py
	@python3 tools/stage2/build_item_crafting_catalog.py
	@python3 tools/stage2/build_source_craft_events.py
	@python3 tools/stage2/build_source_loot_catalog.py
	@python3 tools/stage2/build_original_timed_loot.py
	@python3 tools/stage2/extract_item_behavior_trace.py
	@python3 tools/stage2/validate_item_condition_trace.py
	@python3 tools/stage2/audit_source_wear.py
	@python3 tools/stage2/build_source_wear_catalog.py
	@python3 tools/stage2/build_item_effect_catalog.py
	@python3 tools/stage2/build_item_medical_catalog.py
	@python3 tools/stage2/build_item_equipment_catalog.py


