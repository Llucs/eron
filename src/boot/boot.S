# Constantes para o cabeçalho Multiboot
.set ALIGN,    1<<0             # Alinhar módulos carregados em limites de página
.set MEMINFO,  1<<1             # Fornecer mapa de memória
.set FLAGS,    ALIGN | MEMINFO  # Campo de 'flags' do Multiboot
.set MAGIC,    0x1BADB002       # Número mágico para o bootloader encontrar o cabeçalho
.set CHECKSUM, -(MAGIC + FLAGS) # Checksum para provar que é multiboot

# Seção Multiboot
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserva de pilha (stack) para o kernel
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# Ponto de entrada do kernel
.section .text
.global _start
.type _start, @function
_start:
        # Configura o ponteiro de pilha
        movl $stack_top, %esp
        andl $-16, %esp

        # Chama a função principal do kernel (em C)
        call kernel_main

        # Caso o kernel retorne, entra em loop infinito
        cli
1:        hlt
        jmp 1b

.size _start, . - _start

# Funções globais para IDT
.global idt_load
.extern idtp
idt_load:
    lidt (idtp)
    ret

# Handler de interrupção do teclado
.global keyboard_handler_asm
.extern teclado_handler
keyboard_handler_asm:
    pusha
    call teclado_handler
    popa
    iret
