TARGET:=platformer3ds
BUILD:=build
SOURCES:=source
INCLUDES:=include

ARCH:=-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS:=-g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -D__3DS__
CXXFLAGS:=$(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS:=-g $(ARCH)
LDFLAGS=-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS:=-lcitro2d -lcitro3d -lctru -lm
LIBDIRS:=$(CTRULIB)

include $(DEVKITARM)/3ds_rules
