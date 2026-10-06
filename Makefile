#---------------------------------------------------------------------------------
# CLEAR VARIABLES
#---------------------------------------------------------------------------------
.SUFFIXES:

#---------------------------------------------------------------------------------
# TARGET SETTINGS
#---------------------------------------------------------------------------------
TOPDIR      := $(CURDIR)
TARGET      := $(notdir$(CURDIR))
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        := data
GFX         := gfx

#---------------------------------------------------------------------------------
# COMPILER AND TOOL SELECTION
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to devkitARM>")
endif

include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# BUILD FLAGS
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-fomit-frame-pointer \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)

LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir$*.map)

#---------------------------------------------------------------------------------
# LIBRARIES TO LINK
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS)$(CTRULIB)
LIBS	:= -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# BUILD PROCESS RULES
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir$(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GFX),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard$(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard$(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard$(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard$(dir)/*.v.pica)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard$(dir)/*.*)))
PNGFILES	:=	$(foreach dir,$(GFX),$(notdir $(wildcard$(dir)/*.png)))

export OFILES_BIN	  := $(addsuffix .o, $(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o)$(CFILES:.c=.o) $(SFILES:.s=.o)$(
