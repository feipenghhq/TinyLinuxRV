
LOG_LEVEL ?= LOG_LEVEL_INFO

CC       := gcc
CPPFLAGS := -Iinclude
CPPFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
CFLAGS 	 := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -O2
DEPFLAGS := -MMD -MP
LDFLAGS  :=
LDLIBS   :=

EMULATOR := rvemu

BUILD_DIR := build

SOURCES := src/cpu/cpu.c
SOURCES += src/memory/memory.c
SOURCES += src/main.c

OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS	:= $(OBJECTS:.o=.d)

.PHONY: build clean sanity-test

build: $(EMULATOR)

#-------------------------------------------------
# Run sanity test
#-------------------------------------------------

SANITY_DIR := tests/sanity/build

sanity-test: $(EMULATOR)
	$(MAKE) -C tests/sanity all
	./$(EMULATOR) $(SANITY_DIR)/rv64i.bin

#-------------------------------------------------
# Build emulator
#-------------------------------------------------

$(EMULATOR): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(EMULATOR) $(BUILD_DIR)
