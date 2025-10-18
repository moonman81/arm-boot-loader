# Minimal Raspberry Pi Bootloader Makefile

CROSS_COMPILE ?= aarch64-elf-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

CFLAGS = -Wall -Wextra -Os -ffreestanding -nostdlib -nostartfiles
ASFLAGS = -march=armv8-a
LDFLAGS = -T linker.ld

# Minimal source files
SRC_C = main.c uart.c timer.c gpio.c memory.c mailbox.c sd.c
SRC_S = start.S
OBJ = $(SRC_C:.c=.o) $(SRC_S:.S=.o)

TARGET = bootloader.bin

.PHONY: all clean qemu-test

all: $(TARGET)

$(TARGET): bootloader.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(TARGET) ($$(stat -f%z $(TARGET) 2>/dev/null || stat -c%s $(TARGET)) bytes)"

bootloader.elf: $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ start.o main.o uart.o timer.o gpio.o memory.o mailbox.o sd.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(OBJ) bootloader.elf $(TARGET)
	@echo "Cleaned build artifacts"

# QEMU test
qemu-test: $(TARGET)
	@echo "Running bootloader in QEMU (raspi3b)..."
	qemu-system-aarch64 -M raspi3b -kernel $(TARGET) -serial stdio -nographic
