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

.PHONY: all clean graphics

all: graphics $(OUTPUT).3dsx

graphics:
	@mkdir -p $(ROMFS)/gfx
	@for t3s in $(GRAPHICS)/*.t3s; do \
		if [ -f "$$t3s" ]; then \
			filename=$$(basename $$t3s .t3s); \
			echo "Building texture sheet: $$filename.t3x"; \
			tex3ds -i "$$t3s" -o "$(ROMFS)/gfx/$$filename.t3x"; \
		fi; \
	done

$(BUILD):
	@mkdir -p $@

$(OUTPUT).3dsx: $(BUILD)
	@$(MAKE) DEVKITARM="$(DEVKITARM)" LIBPATHS="$(LIBPATHS)" --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo cleaning build artifacts...
	@rm -fr $(BUILD) $(ROMFS)/gfx $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).map

else

dependency := $(OFILES:.o=.d)

-include $(dependency)

$(OUTPUT).3dsx : $(OUTPUT).elf

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
