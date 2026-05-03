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

# Alvos principais
.PHONY: all clean iso run

all: eron.bin

eron.bin: $(BOOT_OBJ) $(KERNEL_OBJS)
	$(CC) -T $(LINKER) -o eron.bin $(LDFLAGS) $(BOOT_OBJ) $(KERNEL_OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

# Geracao da ISO bootavel (Dinamica)
iso: eron.iso

eron.iso: eron.bin
	@echo "Criando estrutura isodir..."
	mkdir -p $(ISODIR)/boot/grub
	@echo "Copiando kernel..."
	cp eron.bin $(ISODIR)/boot/eron.bin
	@echo "Gerando grub.cfg dinamicamente..."
	@echo 'set timeout=5' > $(ISODIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '' >> $(ISODIR)/boot/grub/grub.cfg
	@echo 'menuentry "Eron OS" {' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '	multiboot /boot/eron.bin' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '	boot' >> $(ISODIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	@echo "Executando grub-mkrescue..."
	$(GRUB_MKRESCUE) -o eron.iso $(ISODIR)
	@echo "ISO gerada com sucesso: eron.iso"

clean:
	@echo "Limpando arquivos de build..."
	rm -f $(BOOT_OBJ) $(KERNEL_OBJS) eron.bin eron.iso
	rm -rf $(ISODIR)

run: eron.iso
	qemu-system-i386 -cdrom eron.iso
