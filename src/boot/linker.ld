ENTRY(_start)

PHDRS
{
    headers PT_PHDR PHDRS;
    text PT_LOAD;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS
{
    . = 1M;

    .multiboot ALIGN(4K) :
    {
        KEEP(*(.multiboot))
    } :text

    .text ALIGN(4K) :
    {
        *(.text*)
    } :text

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } :text

    .data ALIGN(4K) :
    {
        *(.data*)
    } :data

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
    } :bss
}