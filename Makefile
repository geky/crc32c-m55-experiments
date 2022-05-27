
TARGET = main

CC = arm-none-eabi-gcc \
	-mthumb \
	-mcpu=cortex-m55 \
	--static \
	--specs=rdimon.specs \
	-mfloat-abi=softfp
QEMU = qemu-arm \
	-cpu cortex-m55
OBJDUMP = arm-none-eabi-objdump \
	-marmv8.1-m.main
SIZE = arm-none-eabi-size
GDB = arm-none-eabi-gdb

SRC ?= $(sort $(wildcard *.c))
OBJ := $(SRC:%.c=$(BUILDDIR)%.o) impls.py.o
DEP := $(SRC:%.c=$(BUILDDIR)%.d) impls.py.d

CRCS ?= $(sort $(wildcard crc32c_*.c))
TRACES ?= $(CRCS:%.c=%.trace)

override CFLAGS += -Os
override CFLAGS += -g3
override CFLAGS += -I.
override CFLAGS += -std=c99 -Wall -pedantic


# commands
.PHONY: all build
all build: $(TARGET)

.PHONY: run
run: build
	$(QEMU) ./main

.PHONY: disas
disas: $(OBJ)
	$(OBJDUMP) -d --visualize-jumps $^

.PHONY: size
size: $(OBJ)
	$(SIZE) $^

.PHONY: debug
debug: $(TARGET)
	$(QEMU) -g 8123 ./main &
	$(GDB) -ex "target remote :8123" $<

%.trace: PORT=$(shell \
	python -c "import sys; print(8123 + sys.argv.index('$(word 2,$^)'))" \
	$(CRCS))
%.trace: $(TARGET) %.c
	$(QEMU) -g $(PORT) ./main &
	$(GDB) -q -ex "target remote :$(PORT)" $< -x trace.gdb -ex "trace $* $@"

trace-%: %.trace
	grep '^=>' $<

count-%: %.trace
	./count.py $<

hist-%: %.trace
	./hist.py $<

count: $(TRACES)
	./count.py $^



# rules
-include $(DEP)
.SUFFIXES:

main: $(OBJ)
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@

impls.py.c: $(CRCS)
	./impls.py $(^:.c=) > impls.py.c

%.o: %.c
	$(CC) -c -MMD $(CFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@



# clean everything
.PHONY: clean
clean:
	rm -f $(TARGET)
	rm -f impls.py.c
	rm -f $(OBJ)
	rm -f $(DEP)
	rm -f $(TRACES)
