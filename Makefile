CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
TARGET = kurui
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

# Version determination
VERSION_TAG ?= $(shell if [ -f VERSION ]; then echo "v$$(cat VERSION).1"; else echo "v0.0.1"; fi)

# Allow override for cross-compilation
ifeq ($(CC),gcc)
    SDL_CFLAGS ?= $(shell sdl-config --cflags 2>/dev/null || echo -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT)
    SDL_LIBS ?= $(shell sdl-config --libs 2>/dev/null || echo -lSDL -lpthread)
else
    # Default paths for ARM (TRIMUI Model S / Allwinner F1C100s ARM926EJ-S)
    CFLAGS += -mcpu=arm926ej-s -mtune=arm926ej-s
    SDL_CFLAGS ?= -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
    SDL_LIBS ?= -L/usr/lib/arm-linux-gnueabi -lSDL -lpthread
endif

CFLAGS += $(SDL_CFLAGS) $(EXTRA_CFLAGS) -DKURUI_VERSION=\"$(VERSION_TAG)\"
LIBS += $(SDL_LIBS) $(EXTRA_LIBS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all clean
