# Makefile for baseos

# Tools (use the x86_64-elf cross-compiler)
AS = nasm
CC = /opt/homebrew/opt/x86_64-elf-gcc/bin/x86_64-elf-gcc
LD = /opt/homebrew/opt/x86_64-elf-binutils/bin/x86_64-elf-ld
OBJCOPY = /opt/homebrew/opt/x86_64-elf-binutils/bin/x86_64-elf-objcopy

# Flags
CFLAGS = -std=gnu99 -ffreestanding -O0 -g -Wall -Wextra -m32 -nostdlib -I.
ASFLAGS = -f elf       # Change to ELF format for better debugging
LDFLAGS = -T linker.ld -nostdlib -m elf_i386    # Linker flags for 32-bit output

# Files
BOOTLOADER_SRC = boot.asm
KERNEL_SRC = kernel.c
KERNEL_ENTRY_SRC = kernel_entry.asm
BOOTLOADER_BIN = boot.bin
KERNEL_OBJ = kernel.o
KERNEL_ENTRY_OBJ = kernel_entry.o
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
OUTPUT_IMG = baseos.img
FLOPPY_SIZE_KB = 1440 # Standard 1.44MB floppy

# Default target
all: $(OUTPUT_IMG)

# Build the bootloader
$(BOOTLOADER_BIN): $(BOOTLOADER_SRC)
	$(AS) -f bin $< -o $@

# Build the kernel entry stub
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	$(AS) $(ASFLAGS) $< -o $@

# Build the kernel C code
$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the kernel
$(KERNEL_ELF): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ)

# Extract the raw binary from the ELF kernel
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# Create the final floppy image
$(OUTPUT_IMG): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@echo "Creating floppy image..."
	cat $(BOOTLOADER_BIN) $(KERNEL_BIN) > $(OUTPUT_IMG).tmp
	dd if=/dev/zero bs=512 count=2880 >> $(OUTPUT_IMG).tmp 2>/dev/null || true
	dd if=$(OUTPUT_IMG).tmp of=$(OUTPUT_IMG) bs=512 count=2880
	rm $(OUTPUT_IMG).tmp

# Run with QEMU
run: $(OUTPUT_IMG)
	@echo "Checking for existing QEMU processes..."
	@pgrep qemu-system-i386 && echo "Killing existing QEMU instances..." && pkill qemu-system-i386 || true
	qemu-system-i386 -drive file=$(OUTPUT_IMG),format=raw,index=0,if=floppy -serial file:serial.out -no-reboot -display curses -d int,cpu_reset,guest_errors -D qemu_log.txt

# Clean up build files
clean:
	@echo "Cleaning up..."
	@pgrep qemu-system-i386 && echo "Killing QEMU processes..." && pkill qemu-system-i386 || true
	rm -f $(BOOTLOADER_BIN) $(KERNEL_OBJ) $(KERNEL_ENTRY_OBJ) $(KERNEL_ELF) $(KERNEL_BIN) $(OUTPUT_IMG) $(OUTPUT_IMG).tmp *.o serial.out qemu_log.txt

.PHONY: all run clean