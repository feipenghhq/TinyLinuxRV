
OUTPUT = build

CC=gcc

SRCS := plic_unit_test.c
SRCS += ../../../src/device/plic.c

CPPFLAGS += -I../../../include
CPPFLAGS += -I../../../include/device

target = $(OUTPUT)/plic_unit_test

run: $(target)
	$<

$(target): $(SRCS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(target)
