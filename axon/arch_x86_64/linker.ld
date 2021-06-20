
OUTPUT_FORMAT( elf64-x86-64 )
ENTRY( ax_x86_bsp_entry )

KERNEL_VA = 0xffffffff80000000;

SECTIONS
{
    . = 1M;
    ax_kernel_begin = .;
    .ax_multiboot_header :
    {
        KEEP(*(ax_multiboot_header))
    }

    /* Add our higher-half offset, so everything is linked properly */
    . += KERNEL_VA;

    .text ALIGN( 0x1000 ) : AT( ADDR( .text ) - KERNEL_VA )
    {
        *(.text)
    }

    .data ALIGN( 0x1000 ) : AT( ADDR( .data ) - KERNEL_VA )
    {
        *(.data)
    }

    .rodata ALIGN( 0x1000 ) : AT( ADDR( .rodata ) - KERNEL_VA )
    {
        *(.rodata)
    }

    .bss ALIGN( 0x1000 ) : AT( ADDR( .bss ) - KERNEL_VA )
    {
        *(.bss)
    }

    ax_kernel_end = .;
}