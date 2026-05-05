# Eron OS - Build System
CC = gcc
AS = as
GRUB_MKRESCUE = grub-mkrescue

# Security-focused compiler flags (compatible with freestanding)
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra

# Additional security linking flags - basic
LDFLAGS = -m32 -ffreestanding -O2 -nostdlib -no-pie
ASFLAGS = --32

SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
INCLUDE_DIR = $(SRC_DIR)/include
ISODIR = isodir

BOOT_OBJ = $(BOOT_DIR)/boot.o
KERNEL_OBJS = $(KERNEL_DIR)/kernel.o \
              $(KERNEL_DIR)/tty.o \
              $(KERNEL_DIR)/idt.o \
              $(KERNEL_DIR)/gdt.o \
              $(KERNEL_DIR)/tss.o \
              $(KERNEL_DIR)/teclado.o \
              $(KERNEL_DIR)/shell.o \
              $(KERNEL_DIR)/mm.o \
              $(KERNEL_DIR)/timer.o \
              $(KERNEL_DIR)/vfs.o \
              $(KERNEL_DIR)/task.o \
              $(KERNEL_DIR)/syscall.o \
              $(KERNEL_DIR)/display.o \
              $(KERNEL_DIR)/process.o \
              $(KERNEL_DIR)/elf.o \
              $(KERNEL_DIR)/interrupt.o

LINKER = $(BOOT_DIR)/linker.ld

.PHONY: all clean iso run

all: iso

eron.bin: $(BOOT_OBJ) $(KERNEL_OBJS)
	$(CC) -T $(LINKER) -o eron.bin $(LDFLAGS) $(BOOT_OBJ) $(KERNEL_OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

iso: eron.iso

eron.iso: eron.bin
	@echo "Creating isodir..."
	mkdir -p $(ISODIR)/boot/grub
	cp eron.bin $(ISODIR)/boot/eron.bin
	@echo 'set timeout=3' > $(ISODIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '' >> $(ISODIR)/boot/grub/grub.cfg
	@echo 'menuentry "Eron OS" {' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '	multiboot /boot/eron.bin' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '	boot' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o eron.iso $(ISODIR)

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJS) eron.bin eron.iso
	rm -rf $(ISODIR)

run: eron.iso
	qemu-system-i386 -cdrom eron.iso -m 128M
