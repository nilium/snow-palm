TARGET=unknown
OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	TARGET=osx
endif

APP_OUT=bin/snow

CFLAGS+= -Isrc -g -Wall
LDFLAGS+= -g

.PHONY: all clean Makefile.sources

all: Makefile.sources $(APP_OUT)

Makefile.sources:
	./build-sources.rb --target $(TARGET) > Makefile.sources

include Makefile.$(TARGET)
include Makefile.sources

clean:
	$(RM) $(APP_OUT) $(OBJECTS)

$(APP_OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
