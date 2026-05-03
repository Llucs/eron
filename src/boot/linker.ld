/* O ponto de entrada definido no boot.S */
ENTRY(_start)

SECTIONS
{
	/* Começamos em 1MiB, local padrão para kernels multiboot */
	. = 1M;

	/* Primeiro colocamos o cabeçalho multiboot, pois o bootloader precisa dele no início */
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	/* Dados somente leitura */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Dados lidos/escritos (inicializados) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Dados lidos/escritos (não inicializados) e pilha */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}
