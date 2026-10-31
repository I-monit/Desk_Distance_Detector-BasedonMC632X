INCLUDE mem.ld
ENTRY(_start)
INCLUDE extern.ld
SECTIONS
{
    . = BASEADDR;

    . = ALIGN(4);
    __stage_10_start = .;
    .stage10_code :
    {
        INCLUDE baseos.ld
    }
    . = ALIGN(0x1000);
    __stage_10_end = .;

    __stage_20_start = .;
    .stage20_code :
    {
        /* empty for normal multi-stage-load */
    }
    __stage_20_end = .;

    . = ALIGN(0x1000);
    __stage_30_start = .;
    .rodata :
    {
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r*)
        *(.eh_frame)

        /* section information for finsh shell */
        /* really this section should be moved to link.ld */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        . = ALIGN(4);

        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        . = ALIGN(4);

        /* section information for modules */
        /* move to link.ld too. */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(RTMSymTab))
        __rtmsymtab_end = .;
    }

    . = ALIGN(4);
    .ctors :
    {
        PROVIDE(__ctors_start__ = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__ctors_end__ = .);
    }

    .dtors :
    {
        PROVIDE(__dtors_start__ = .);
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        PROVIDE(__dtors_end__ = .);
    }

    . = ALIGN(4);
    __exidx_start = .;
    ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        _sidata = .;
    }
    __exidx_end = .;

    . = ALIGN(4);
    .stage30_code :
    {
        *(.text .text.*)
    }

    . = ALIGN(4);
    .data :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
    }

    . = ALIGN(4);
    .end_mark :
    {
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
        LONG(0)
    }
    . = ALIGN(0x1000);
    __stage_30_end = .;      /* must be aligned to 4K boundary */

    . = ALIGN(4);
    .nobss :
    {
        *(.nobss)
    }
    /* svc stack here. */

    . = ALIGN(4);
    __bss_start = .;
    .bss : { *(.bss .bss*) }
    __bss_end = ALIGN(4);

    /* stabs debugging sections. */
    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_aranges 0 : { *(.debug_aranges) }

    . = ALIGN(0x1000);
    _end = .;
}
