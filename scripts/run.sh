#!/bin/bash
set -e

echo "Compilando Eron OS..."
make clean
make iso

echo "Iniciando QEMU..."
qemu-system-i386 -cdrom eron.iso -m 128M -serial stdio
