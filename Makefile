# KURUI (TRIMUI Model S Custom Launcher) Makefile
# Supports native PC compilation and ARMv5TE cross-compilation

TARGET := kurui

CC ?= gcc
STRIP ?= $(CROSS_COMPILE)strip
SDL_CONFIG ?= $(CROSS_COMPILE)sdl-config

SRC_DIR := src
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Version determination (Defaults to v0.0.1 if unset)
VERSION_TAG ?= $(shell if [ -f VERSION ]; then echo "v$$(cat VERSION).1"; else echo "v0.0.1"; fi)

# Detection of cross-compilation environment
ifneq ($(filter arm%,$(CC)),)
    # Cross-compiling for ARM (TRIMUI Model S / Allwinner F1C100s)
    CFLAGS ?= -O2 -Wall -Wextra -march=armv5te -mtune=arm926ej-s -fomit-frame-pointer
    SDL_CFLAGS ?= $(shell $(SDL_CONFIG) --cflags 2>/dev/null || echo -I/usr/arm-linux-gnueabi/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT)
    SDL_LIBS ?= $(shell $(SDL_CONFIG) --libs 2>/dev/null || echo -lSDL -lpthread -lm)
else
    # Host PC compilation (Linux / macOS / WSL / MinGW)
    CFLAGS ?= -O2 -Wall -Wextra
    SDL_CFLAGS ?= $(shell $(SDL_CONFIG) --cflags 2>/dev/null || echo -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT)
    SDL_LIBS ?= $(shell $(SDL_CONFIG) --libs 2>/dev/null || echo -lSDL -lpthread -lm)
endif

CFLAGS += $(SDL_CFLAGS) -DKURUI_VERSION=\"$(VERSION_TAG)\"
LDFLAGS += $(SDL_LIBS)

.PHONY: all clean strip

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "[LD] $@"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

strip: $(TARGET)
	@echo "[STRIP] $(TARGET)"
	$(STRIP) $(TARGET)

clean:
	@echo "[CLEAN]"
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe
