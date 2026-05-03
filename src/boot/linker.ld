ENTRY(_start)

PHDRS
{
    text PT_LOAD FILEHDR PHDRS;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS
{
    . = 0x00100000;

    .multiboot ALIGN(0x1000) :
    {
        KEEP(*(.multiboot))
    } :text

    .text ALIGN(0x1000) :
    {
        *(.text)
        *(.text.*)
    } :text

    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
    } :text

    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.data.*)
    } :data

    .bss ALIGN(0x1000) :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } :bss

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu_debugdata)
    }
}
