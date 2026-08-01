
LOG_LEVEL ?= LOG_LEVEL_DEBUG

INCLUDE_DIRS := $(shell find . -type d)
CFLAGS := $(addprefix -I,$(INCLUDE_DIRS))
CFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
CFLAGS += -Wall -Wextra -Wpedantic


all: rvemu

rvemu: src/main.c src/cpu/cpu.c src/memory/memory.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -rf rvemu