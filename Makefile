PDK_DIR ?= /opt/PalmPDK

CC_DEVICE=$(PDK_DIR)/arm-gcc/bin/arm-none-linux-gnueabi-gcc
CC_HOST=gcc

CC_HOST_LIB=-L$(PDK_DIR)/host/lib
CC_HOST_LIBS=-lSDL_host -lpthread -lpdl -lGLESv2 -lSDLmain_host
CC_HOST_LDFLAGS=$(CC_HOST_LIB) $(CC_HOST_LIBS)
CC_HOST_INCLUDE=-I$(PDK_DIR)/include
CC_HOST_FLAGS=-arch i386 $(CC_HOST_INCLUDE)

CC_DEVICE_LIB=-L$(PDK_DIR)/device/lib
CC_DEVICE_LDFLAGS=$(CC_DEVICE_LIB) -lSDL -lpthread -lpdl -lGLESv2
CC_DEVICE_INCLUDE=-I$(PDK_DIR)/include
CC_DEVICE_FLAGS=--sysroot=$(PDK_DIR)/arm-gcc/sysroot -Wl,--allow-shlib-undefined \
    -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp $(CC_DEVICE_INCLUDE)

OUTPUT:=snow

all: host device
.PHONY:all host device makefile clean

host: export TARGET_OUTPUT := $(OUTPUT)-host
host: export TARGET_CC := $(CC_HOST)
host: export TARGET_CFLAGS := $(CFLAGS) $(CC_HOST_FLAGS)
host: export TARGET_LDFLAGS := $(LDFLAGS) $(CC_HOST_LDFLAGS)
host: export TARGET := host
host: makefile
	cd src && $(MAKE)

device: export TARGET_OUTPUT := $(OUTPUT)
device: export TARGET_CC := $(CC_DEVICE)
device: export TARGET_CFLAGS := $(CFLAGS) $(CC_DEVICE_FLAGS)
device: export TARGET_LDFLAGS := $(LDFLAGS) $(CC_DEVICE_LDFLAGS)
device: export TARGET := device
device: makefile
	cd src && $(MAKE)

makefile:
	cd src && ./build-makefile.rb > Makefile

clean:
	$(RM) -r obj/
	$(RM) -r bin/
