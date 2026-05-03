# Compiladores
CC = gcc
AS = as
# Flags para 32-bit e freestanding
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m32 -ffreestanding -O2 -nostdlib -lgcc
ASFLAGS = --32

# Arquivos
BOOT_OBJ = src/boot/boot.o
KERNEL_OBJS = src/kernel/kernel.o \
              src/kernel/tty.o \
              src/kernel/idt.o \
              src/kernel/teclado.o \
              src/kernel/shell.o

LINKER = src/boot/linker.ld

all: eron.bin

eron.bin: $(BOOT_OBJ) $(KERNEL_OBJS)
	$(CC) -T $(LINKER) -o eron.bin $(LDFLAGS) $(BOOT_OBJ) $(KERNEL_OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJS) eron.bin eron.iso
	rm -rf isodir

iso: eron.bin
	mkdir -p isodir/boot/grub
	cp eron.bin isodir/boot/eron.bin
	echo 'menuentry "Eron OS v0.1.0" {' > isodir/boot/grub/grub.cfg
	echo '	multiboot /boot/eron.bin' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o eron.iso isodir
