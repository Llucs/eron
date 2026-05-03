# Compiladores e Ferramentas
CC = gcc
AS = as
GRUB_MKRESCUE = grub-mkrescue

# Flags para 32-bit e freestanding
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m32 -ffreestanding -O2 -nostdlib -lgcc
ASFLAGS = --32

# Diretorios
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
INCLUDE_DIR = $(SRC_DIR)/include
ISODIR = isodir

# Arquivos
BOOT_OBJ = $(BOOT_DIR)/boot.o
KERNEL_OBJS = $(KERNEL_DIR)/kernel.o \
              $(KERNEL_DIR)/tty.o \
              $(KERNEL_DIR)/idt.o \
              $(KERNEL_DIR)/teclado.o \
              $(KERNEL_DIR)/shell.o

LINKER = $(BOOT_DIR)/linker.ld
GRUB_CONFIG = $(BOOT_DIR)/grub.cfg

# Alvos principais
.PHONY: all clean iso run

all: eron.bin

eron.bin: $(BOOT_OBJ) $(KERNEL_OBJS)
	$(CC) -T $(LINKER) -o eron.bin $(LDFLAGS) $(BOOT_OBJ) $(KERNEL_OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

# Geracao da ISO bootavel
iso: eron.iso

eron.iso: eron.bin $(GRUB_CONFIG)
	mkdir -p $(ISODIR)/boot/grub
	cp eron.bin $(ISODIR)/boot/eron.bin
	cp $(GRUB_CONFIG) $(ISODIR)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o eron.iso $(ISODIR)

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJS) eron.bin eron.iso
	rm -rf $(ISODIR)

run: eron.iso
	qemu-system-i386 -cdrom eron.iso
