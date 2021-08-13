
OUTPUT_FORMAT( elf64-x86-64 )
ENTRY( axk_x86_entry )

KERNEL_VA = 0xffffffff80000000;

SECTIONS
{
    . = 1M;
    axk_kernel_begin = .;

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

    axk_kernel_end = .;
}