
LOG_LEVEL ?= LOG_LEVEL_DEBUG

CC       := gcc
CPPFLAGS := -Iinclude
CPPFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
CFLAGS 	 := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -MMD -MP
LDFLAGS  :=
LDLIBS   :=

TOP := rvemu
OUTPUT := build

SOURCES := src/cpu/cpu.c
SOURCES += src/memory/memory.c
SOURCES += src/main.c

OBJECTS := $(patsubst src/%.c,$(OUTPUT)/%.o,$(SOURCES))
DEPS	:= $(OBJECTS:.o=.d)

.PHONY: build clean sanity run_sanity_test

build: $(TOP)

#-------------------------------------------------
# Run sanity test
#-------------------------------------------------

run_sanity_test: sanity $(TOP)
	./rvemu tests/sanity/build/rv64i.bin

sanity:
	$(MAKE) -C tests/sanity all

#-------------------------------------------------
# Build emulator
#-------------------------------------------------

$(TOP): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OUTPUT)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(TOP) $(OUTPUT)
