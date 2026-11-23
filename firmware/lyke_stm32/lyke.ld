ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = LYKE_ORIGIN, LENGTH = LYKE_LENGTH
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 0x57f0
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH
    _sidata = LOADADDR(.data);
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    end = _ebss;

    ASSERT(_ebss <= _estack - 2048, "Sem espaco para a pilha")
}

