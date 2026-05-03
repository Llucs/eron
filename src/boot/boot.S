# Constantes para o cabeçalho Multiboot
.set ALIGN,    1<<0             
.set MEMINFO,  1<<1             
.set FLAGS,    ALIGN | MEMINFO  
.set MAGIC,    0x1BADB002       
.set CHECKSUM, -(MAGIC + FLAGS) 

# Ponto de entrada do kernel
.section .text
.global _start
.type _start, @function

# Cabeçalho Multiboot - nos primeiros 8KB
.align 4
_multiboot_header:
.long MAGIC
.long FLAGS
.long CHECKSUM

_start:
	movl $stack_top, %esp
	call kernel_main
	cli
1:	hlt
	jmp 1b

.size _start, . - _start

# Reserva de pilha - agora em .data
.section .data
.align 16
stack_bottom:
.skip 16384 
stack_top:

# Funções globais para IDT
.global idt_load
idt_load:
    lidt idtp
    ret

# Handler de interrupção do teclado
.global keyboard_handler_asm
keyboard_handler_asm:
    pusha
    call teclado_handler
    popa
    iret