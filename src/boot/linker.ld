OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

PHDRS {
    text PT_LOAD FILEHDR PHDRS;
}

SECTIONS {
    . = 0x1000;
    .text : {
        _start = .;
        *(.text)
        *(.text.*)
    } :text
    .rodata : { *(.rodata) } :text
    .data : { *(.data) } :text
    .bss : { *(.bss) } :text
}