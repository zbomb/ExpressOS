/*==============================================================
    T-0 Bootloader - Main Code File
    2021, Zachary Berry
    t-zero/source/main.c
==============================================================*/

#include "tzero.h"
#include "util.h"
#include "console.h"

/*
    Constants
*/
#define KERNEL_FILE_NAME        L"AXON.BIN"
#define KERNEL_VOFFSET          0xFFFFFFFF80000000
#define KERNEL_AP_INIT_ADDR     0x8000
#define KERNEL_AP_INIT_PAGES    1

/*
    Globals
*/
EFI_SYSTEM_TABLE* efi_table                         = NULL;
EFI_HANDLE efi_handle                               = NULL;
UINTN efi_memmap_key                                = 0;
struct tzero_payload_parameters_t* tzero_params     = NULL;
struct tzero_x86_payload_parameters_t* x86_params   = NULL;

/*
    Forward Function Declarations
*/
bool parse_framebuffer_info( struct tzero_payload_parameters_t* out_info );
bool finalize_memory_map( struct tzero_payload_parameters_t* out_info );
bool parse_acpi_info( struct tzero_x86_payload_parameters_t* out_arch_info );
__attribute__(( sysv_abi )) void on_success_callback( void );
__attribute__(( sysv_abi )) void on_failure_callback( const char* message );

/*
    Entry Point
*/
EFI_STATUS efi_main( EFI_HANDLE in_handle, EFI_SYSTEM_TABLE* in_table )
{
    EFI_STATUS status;

    // Store the handle and table so all functions have access to them
    efi_table   = in_table;
    efi_handle  = in_handle;

    // Print header
    tzero_console_clear();
    tzero_console_prints16( L"T-0: Initializing bootloader...\r\n" );

    // Ensure the page required for processor initialization is reserved so it wont be allocated for any other purpose
    EFI_PHYSICAL_ADDRESS ap_init_addr = KERNEL_AP_INIT_ADDR;
    status = efi_table->BootServices->AllocatePages( AllocateAddress, EfiLoaderCode, (UINTN) KERNEL_AP_INIT_PAGES, &ap_init_addr );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints16( L"T-0 (Warning): Failed to reserve memory pages required for aux processor initialization by the kernel, continuing boot process but issues may arise \r\n" );

        // We need to pause for a short amount of time (3s) to ensure the user sees the message
        efi_table->BootServices->Stall( 3000000 );
    }

    // Attempt to load the kernel binary
    EFI_FILE_INFO kernel_info;
    EFI_FILE* kernel_file = tzero_open_file( NULL, KERNEL_FILE_NAME, &kernel_info );

    // Check if the file couldnt be found, or the file info couldnt be read
    if( kernel_file == NULL )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to open kernel binary (" );
        tzero_console_prints16( KERNEL_FILE_NAME );
        tzero_console_prints16( L"), entering halted state \r\n" );
        tzero_halt();
    }

    // TODO: We can read the file in, and validate a signature to ensure the binary hasnt been tampered with
    // Currently though, its pointless since the bootloader itself can just be tampered with instead

    // Read the ELF header from the binary
    struct elf_ehdr_t header;
    UINTN header_sz = (UINTN)( sizeof( struct elf_ehdr_t ) );
    memset( &header, 0, header_sz );

    status = kernel_file->SetPosition( kernel_file, 0UL );
    if( !EFI_ERROR( status ) )
    {
        status = kernel_file->Read( kernel_file, &header_sz, (void*) &header );
    }

    if( EFI_ERROR( status ) )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to read the kernel header (EFI Error: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_prints16( L"), entering halted state \r\n" );
        tzero_halt();
    }

    // Validate the header
    if( memcmp( header.ident, ELF_MAGIC, ELF_MAGIC_SZ ) != 0 ||
        header.ident[ EI_CLASS ] != ELF_CLASS_64 ||
        header.ident[ EI_DATA ] != ELF_DATA_2LSB ||
        header.type != ET_EXEC ||
        header.machine != EM_X86_64 ||
        header.version != EV_CURRENT )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to validate the kernel header, entering halted state \r\n" );
        tzero_halt();
    }

    // Now we need to read in the program headers
    UINTN phdrs_sz = header.phnum * header.phentsize;
    struct elf_phdr_t* phdr_list = (struct elf_phdr_t*) tzero_alloc( (uint64_t) phdrs_sz );

    status = kernel_file->SetPosition( kernel_file, header.phoff );
    if( !EFI_ERROR( status ) )
    {
        status = kernel_file->Read( kernel_file, &phdrs_sz, (void*) phdr_list );
    }

    if( EFI_ERROR( status ) )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to read kernel program headers! (EFI Error: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_prints16( L"), entering halted state \r\n" );
        tzero_halt();
    }

    // Now, we need to loop through and load the required sections of the program into memory
    bool b_program_loaded = false;
    for( uint32_t i = 0; i < header.phnum; i++ )
    {
        if( phdr_list[ i ].type == PT_LOAD )
        {
            uint64_t page_count = ( phdr_list[ i ].memsz + 0xFFFUL ) / 0x1000UL;
            uint64_t addr = (uint64_t) phdr_list[ i ].paddr;

            status = efi_table->BootServices->AllocatePages( AllocateAddress, EfiLoaderData, page_count, &addr );
            if( !EFI_ERROR( status ) )
            {
                status = kernel_file->SetPosition( kernel_file, phdr_list[ i ].offset );
            }

            if( !EFI_ERROR( status ) )
            {
                UINTN size = phdr_list[ i ].filesz;
                status = kernel_file->Read( kernel_file, &size, (void*) addr );
            }

            if( EFI_ERROR( status ) )
            {
                tzero_console_prints16( L"T-0 (ERROR): Failed to read kernel program section " );
                tzero_console_printu32( i );
                tzero_console_prints16( L" into system memory (EFI Error: " );
                tzero_console_printu32( (uint32_t) status );
                tzero_console_prints16( L"), entering halted state \r\n" );
                tzero_halt();
            }
            else
            {
                b_program_loaded = true;
            }
        }
    }

    // Ensure at least one program section was loaded into memory
    if( !b_program_loaded )
    {
        tzero_console_prints16( L"T-0 (ERROR): No program sections were loaded into memory from the kernel, entering halted state \r\n" );
        tzero_halt();
    }

    // Create the structure we will place all parameters into and pass along to the payload
    tzero_params    = (struct tzero_payload_parameters_t*) tzero_alloc( sizeof( struct tzero_payload_parameters_t ) );
    x86_params      = (struct tzero_x86_payload_parameters_t*) tzero_alloc( sizeof( struct tzero_x86_payload_parameters_t) );

    memset( (void*) tzero_params, 0, sizeof( struct tzero_payload_parameters_t ) );
    memset( (void*) x86_params, 0, sizeof( struct tzero_x86_payload_parameters_t ) );

    // Parse the current framebuffer information, along with all available resolutions
    if( !parse_framebuffer_info( tzero_params ) )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to parse graphics information, entering halted state \r\n" );
        tzero_halt();
    }

    // Set the final static values in the generic parameters structure
    tzero_params->magic_value       = TZERO_MAGIC_VALUE;
    tzero_params->fn_on_success     = on_success_callback;
    tzero_params->fn_on_error       = on_failure_callback;

    // Now, we need to build out architecture specific parameters
    if( !parse_acpi_info( x86_params ) )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to located ACPI table information, entering halted state \r\n" );
        tzero_halt();
    }

    // Ensure the magic value is set, so the payload can be sure both parameter lists are available
    x86_params->magic_value = TZERO_MAGIC_VALUE;
    x86_params->arch_code   = TZERO_ARCH_CODE_X86;
    
    // And finally, find the entry point, and launch the payload with the two parameter lists
    // Generic parameters will be placed into 'rdi', and the architecture specific parameters into 'rsi'
    tzero_launch( (uint64_t)( header.entry ) - KERNEL_VOFFSET, (void*) tzero_params, (void*) x86_params );
}

/*
    Bitmask Convert Helper
*/
bool parse_bitmask( uint32_t mask, uint8_t* out_width, uint8_t* out_shift )
{
    if( mask == 0U || out_width == NULL || out_shift == NULL ) { return false; }

    // Now, parse through the mask, if there are non-consecutive bits then we will return false
    uint8_t algo_mode = 0;
    uint8_t start_bit = 0;
    uint8_t end_bit = 0;

    for( uint8_t i = 0; i < 32; i++ )
    {
        // Check if bit 'i' is set
        bool set = ( ( mask & ( 1U << i ) ) != 0U );
        if( algo_mode == 0 && set )
        {
            start_bit = i;
            algo_mode = 1;
        }
        else if( algo_mode == 1 && !set )
        {
            end_bit = i;
            algo_mode = 2;
        }
        else if( algo_mode == 2 && set )
        {
            // Non-consecutive bits set!
            return false;
        }
    }

    if( algo_mode != 2 )
    {
        // Mask couldnt be parsed (no bits set? Shouldnt happen!)
        return false;
    }

    *out_width  = ( end_bit - start_bit );
    *out_shift  = start_bit;

    return true;
}


/*
    Framebuffer Parsing
*/
bool parse_framebuffer_info( struct tzero_payload_parameters_t* out_params )
{
    if( out_params == NULL ) { return false; }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop   = NULL;
    EFI_GUID gop_guid                       = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    if( EFI_ERROR( efi_table->BootServices->LocateProtocol( &gop_guid, NULL, (void**) &gop ) ) )
    {
        return false;
    }

    // Allocate space for all resolutions available in the output structure
    out_params->resolution_count        = gop->Mode->MaxMode;
    out_params->available_resolutions   = (struct tzero_resolution_t*) tzero_alloc( sizeof( struct tzero_resolution_t ) * gop->Mode->MaxMode );

    // Iterate through all available resolutions in GOP
    for( uint32_t i = 0; i < gop->Mode->MaxMode; i++ )
    {
        UINTN size;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*) tzero_alloc( sizeof( EFI_GRAPHICS_OUTPUT_MODE_INFORMATION ) );
        EFI_STATUS status = gop->QueryMode( gop, i, &size, &info );
        if( EFI_ERROR( status ) )
        {
            tzero_free( info );
            continue;
        }

        // Convert this to the tzero format
        struct tzero_resolution_t* ptr_res = out_params->available_resolutions + i;
        bool b_valid = false;

        switch( info->PixelFormat )
        {
            case PixelRedGreenBlueReserved8BitPerColor:

            ptr_res->red_bit_width      = 8;
            ptr_res->green_bit_width    = 8;
            ptr_res->blue_bit_width     = 8;
            ptr_res->red_shift          = 24;
            ptr_res->green_shift        = 16;
            ptr_res->blue_shift         = 8;
            ptr_res->mode               = (uint8_t) PIXEL_FORMAT_RGBX_32;
            b_valid                     = true;
            break;

            case PixelBlueGreenRedReserved8BitPerColor:

            ptr_res->red_bit_width      = 8;
            ptr_res->green_bit_width    = 8;
            ptr_res->blue_bit_width     = 8;
            ptr_res->red_shift          = 8;
            ptr_res->green_shift        = 16;
            ptr_res->blue_shift         = 24;
            ptr_res->mode               = (uint8_t) PIXEL_FORMAT_BGRX_32;
            b_valid                     = true;
            break;

            case PixelBitMask:

            if( !parse_bitmask( info->PixelInformation.RedMask, &( ptr_res->red_bit_width ), &( ptr_res->red_shift ) ) ||
                !parse_bitmask( info->PixelInformation.GreenMask, &( ptr_res->green_bit_width ), &( ptr_res->green_shift ) ) ||
                !parse_bitmask( info->PixelInformation.BlueMask, &( ptr_res->blue_bit_width ), &( ptr_res->blue_shift ) ) )
            {
                tzero_console_prints16( L"T-0 (Warning): Found a display mode that has an invalid pixel bitmask \r\n" );
                break;
            }

            ptr_res->mode   = (uint8_t) PIXEL_FORMAT_BITMASK;
            b_valid         = true;
            break;

            case PixelBltOnly:

            tzero_console_prints16( L"T-0 (Warning): Found a display mode that can only be used with 'block transfer', meaning, its not usable by the OS \r\n" );
            break;

            default:
            break;
        }

        // If the mode is not valid, then mark it as such
        if( !b_valid )
        {
            memset( (void*) ptr_res, 0, sizeof( struct tzero_resolution_t ) );
            ptr_res->mode = (uint8_t) PIXEL_FORMAT_INVALID;
        }
        else
        {
            // Fill out the rest of the structure since the mode is valid
            ptr_res->width                  = info->HorizontalResolution;
            ptr_res->height                 = info->VerticalResolution;
            ptr_res->pixels_per_scanline    = info->PixelsPerScanLine;
            ptr_res->index                  = i;
        }

        // Free the data allocated to hold GOP mode information
        tzero_free( info );
    }

    // Read current framebuffer information into the payload parameters
    out_params->framebuffer.phys_addr                       = (uint64_t) gop->Mode->FrameBufferBase;
    out_params->framebuffer.size                            = gop->Mode->FrameBufferSize;
    out_params->framebuffer.resolution.width                = gop->Mode->Info->HorizontalResolution;
    out_params->framebuffer.resolution.height               = gop->Mode->Info->VerticalResolution;
    out_params->framebuffer.resolution.pixels_per_scanline  = gop->Mode->Info->PixelsPerScanLine;
    out_params->framebuffer.resolution.index                = 0;

    switch( gop->Mode->Info->PixelFormat )
    {
        case PixelRedGreenBlueReserved8BitPerColor:

        out_params->framebuffer.resolution.red_bit_width    = 8;
        out_params->framebuffer.resolution.green_bit_width  = 8;
        out_params->framebuffer.resolution.blue_bit_width   = 8;
        out_params->framebuffer.resolution.red_shift        = 24;
        out_params->framebuffer.resolution.green_shift      = 16;
        out_params->framebuffer.resolution.blue_shift       = 8;
        out_params->framebuffer.resolution.mode             = (uint8_t) PIXEL_FORMAT_RGBX_32;
        break;

        case PixelBlueGreenRedReserved8BitPerColor:

        out_params->framebuffer.resolution.red_bit_width    = 8;
        out_params->framebuffer.resolution.green_bit_width  = 8;
        out_params->framebuffer.resolution.blue_bit_width   = 8;
        out_params->framebuffer.resolution.red_shift        = 8;
        out_params->framebuffer.resolution.green_shift      = 16;
        out_params->framebuffer.resolution.blue_shift       = 24;
        out_params->framebuffer.resolution.mode             = (uint8_t) PIXEL_FORMAT_BGRX_32;
        break;

        case PixelBitMask:

        if( !parse_bitmask( gop->Mode->Info->PixelInformation.RedMask, &( out_params->framebuffer.resolution.red_bit_width ), &( out_params->framebuffer.resolution.red_shift ) ) ||
            !parse_bitmask( gop->Mode->Info->PixelInformation.GreenMask, &( out_params->framebuffer.resolution.green_bit_width ), &( out_params->framebuffer.resolution.green_shift ) ) ||
            !parse_bitmask( gop->Mode->Info->PixelInformation.BlueMask, &( out_params->framebuffer.resolution.blue_bit_width ), &( out_params->framebuffer.resolution.blue_shift ) ) )
        {
            tzero_console_prints16( L"T-0 (ERROR): The current display mode has an invalid pixel bitmask! Entering halted state \r\n" );
            tzero_halt();
        }

        out_params->framebuffer.resolution.mode = (uint8_t) PIXEL_FORMAT_BITMASK;
        break;

        case PixelBltOnly:

        tzero_console_prints16( L"T-0 (ERROR): The current display mode only support 'block transfers', meaning it cannot be used by the OS, entering halted state \r\n" );
        tzero_halt();
        break;

        default:

        tzero_console_prints16( L"T-0 (ERROR): The current display mode is unknown! Entering halted state \r\n" );
        tzero_halt();
        break;
    }

    // Print debug message
    tzero_console_prints16( L"T-0: Parsed available resolutions and the current framebuffer info.. \r\n     Resolution: " );
    tzero_console_printu32( out_params->framebuffer.resolution.width );
    tzero_console_prints16( L" x " );
    tzero_console_printu32( out_params->framebuffer.resolution.height );
    tzero_console_prints16( L"  Format: " );

    switch( out_params->framebuffer.resolution.mode )
    {
        case PIXEL_FORMAT_RGBX_32:
        tzero_console_prints16( L"RGB  " );
        break;

        case PIXEL_FORMAT_BGRX_32:
        tzero_console_prints16( L"BGR  " );
        break;

        case PIXEL_FORMAT_BITMASK:
        tzero_console_prints16( L"BitMask  " );
        break;
    }

    tzero_console_prints16( L"and " );
    tzero_console_printu32( out_params->resolution_count );
    tzero_console_prints16( L" available resolutions\r\n" );

    return true;
}

/*
    Memory Map Helpers
*/
enum tzero_memory_status_t convert_memory_type( uint32_t in_type )
{
    switch( in_type )
    {
        // These ranges might have been used by T-0, but should be usable after we parse this map
        case EfiLoaderCode:
        /* fall through */
        case EfiLoaderData:
        return TZERO_MEMORY_BOOTLOADER;

        // These ranges are good to use
        case EfiBootServicesCode:
        /* fall through */
        case EfiBootServicesData: 
        /* fall through */
        case EfiConventionalMemory:
        //case EfiPersistentMemory:
        return TZERO_MEMORY_AVAILABLE;

        // ACPI reclaimable memory
        case EfiACPIReclaimMemory:
        return TZERO_MEMORY_ACPI;

        case EfiMemoryMappedIO:
        /* fall through */
        case EfiMemoryMappedIOPortSpace:
        return TZERO_MEMORY_MAPPED_IO;

        // This is non-usable memory
        case EfiReservedMemoryType:
        /* fall through */
        case EfiRuntimeServicesCode:
        /* fall through */
        case EfiRuntimeServicesData:
        /* fall through */
        case EfiUnusableMemory:
        /* fall through */
        case EfiACPIMemoryNVS:
        /* fall through */
        case EfiPalCode:
        /* fall through */
        default:
        return TZERO_MEMORY_RESERVED;
    }
}


/*
    Memory Map Parsing
*/
bool finalize_memory_map( struct tzero_payload_parameters_t* out_params )
{
    if( out_params == NULL || out_params->memory_map.list != NULL ) { return false; }

    // First, determine the total number of EFI memory map entries and allocate storage for both the EFI map, and the T-0 map
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR* mmap_efi = NULL;
    UINTN mmap_entry_size, mmap_size;
    UINT32 mmap_entry_ver;

    // Print out message, in-case of an error after exiting boot services, and pause for 1s incase any bootloader output is important to developer
    tzero_console_prints16( L"T-0: Finalizing memory map and exiting EFI boot services..\r\nIf the kernel does not continue to boot, then there was an error during this process...\r\n" );
    efi_table->BootServices->Stall( 1000000 );

    // We are going to have 5 attempts at reading the memory map
    for( uint32_t i = 0; i < 5; i++ )
    {
        // First call to this function will give us the total number of entries in the memory map so we can allocate storage
        efi_table->BootServices->GetMemoryMap( &mmap_size, mmap_efi, &efi_memmap_key, &mmap_entry_size, &mmap_entry_ver );

        // Allocate space to write the EFI memory map to, and also space to parse that map into a more generic structure
        // Were going to allocate space for 5 extra entries, because we are performing two pool allocations here (plus a few extra just incase)
        // The 'tzero memory map' most definately wont have the same numebr of entries as the EFI version, since we combine adjacent entries of similar type
        // BUT, we need to perform the allocation before we have a chance to parse! So, we allocate space for a 'worst case scenario'
        uint32_t mmap_efi_entries = mmap_size / mmap_entry_size;
        mmap_efi = (EFI_MEMORY_DESCRIPTOR*) tzero_alloc( mmap_size + ( mmap_entry_size * 5UL ) );
        out_params->memory_map.list = (struct tzero_memory_entry_t*) tzero_alloc( sizeof( struct tzero_memory_entry_t ) * ( mmap_efi_entries + 5UL ) );

        // Read the EFI memory map..
        status = efi_table->BootServices->GetMemoryMap( &mmap_size, mmap_efi, &efi_memmap_key, &mmap_entry_size, &mmap_entry_ver );
        if( EFI_ERROR( status ) )
        {
            // Free the storage allocated, and try again
            tzero_free( mmap_efi );
            tzero_free( out_params->memory_map.list );
        }
        else
        {
            // We need to exit boot services since we have the most up-to-date memory map
            status = efi_table->BootServices->ExitBootServices( efi_handle, efi_memmap_key );
            if( EFI_ERROR( status ) )
            {
                tzero_free( mmap_efi );
                tzero_free( out_params->memory_map.list );
            }
            else
            {
                break;
            }
        }
    }

    // Check if we were able to read the memory map
    if( mmap_efi == NULL || out_params->memory_map.list == NULL )
    {
        tzero_console_prints16( L"T-0 (ERROR): Failed to read the EFI memory map or hand system control over to the kernel! Entering halted state.. \r\n" );
        tzero_halt();
    }

    // First, lets go through the EFI memory map and determine the total number of unique sections we need to write into the T-0 memory map
    uint32_t mmap_entries_efi                           = mmap_size / mmap_entry_size;
    enum tzero_memory_status_t last_entry_status_efi    = TZERO_MEMORY_RESERVED;
    uint64_t mmap_entries_tzero                          = 0UL;
    uint64_t total_pages                                = 0UL;

    for( uint32_t i = 0; i < mmap_entries_efi; i++ )
    {
        EFI_MEMORY_DESCRIPTOR* entry = (EFI_MEMORY_DESCRIPTOR*)( (uint8_t*)( mmap_efi ) + ( (uint64_t)( i ) * (uint64_t)( mmap_entry_size ) ) );
        // We want to determine the 'converted' type, and increment the counter when the last section type doesnt match the current
        // section type, this way we end up with a count of the total number of unique memory sections
        if( i == 0 )
        {
            last_entry_status_efi = convert_memory_type( entry->Type );
            mmap_entries_tzero++;
        }
        else
        {
            enum tzero_memory_status_t this_status = convert_memory_type( entry->Type );
            if( this_status != last_entry_status_efi )
            {
                // Found the start of a new memory range, increment the counter
                last_entry_status_efi = this_status;
                mmap_entries_tzero++;
            }
        }

        total_pages += entry->NumberOfPages;
    }

    if( mmap_entries_tzero == 0UL )
    {
        tzero_halt();
    }

    // Now, we need to actually parse the map
    uint32_t last_entry_efi         = 0U;
    out_params->memory_map.count    = mmap_entries_tzero;

    for( uint32_t i = 0; i < mmap_entries_tzero; i++ )
    {
        struct tzero_memory_entry_t* dest_entry = out_params->memory_map.list + i;
        EFI_MEMORY_DESCRIPTOR* last_entry = (EFI_MEMORY_DESCRIPTOR*)( (uint8_t*)( mmap_efi ) + ( (uint64_t)( last_entry_efi ) * (uint64_t)( mmap_entry_size ) ) );

        dest_entry->type            = (uint32_t) convert_memory_type( last_entry->Type );
        dest_entry->base_address    = last_entry->PhysicalStart;

        for( uint32_t j = last_entry_efi; j < mmap_entries_efi; j++ )
        {
            EFI_MEMORY_DESCRIPTOR* j_entry = (EFI_MEMORY_DESCRIPTOR*)( (uint8_t*)( mmap_efi ) + ( (uint64_t)( j ) * (uint64_t)( mmap_entry_size ) ) );

            // Check if we hit the start of the next section
            if( (uint32_t)( convert_memory_type( j_entry->Type ) ) != dest_entry->type )
            {
                last_entry_efi = j;
                break;
            }

            dest_entry->page_count += j_entry->NumberOfPages;
        }
    }

    // And finally, we just want to ensure this map is sorted by physical start address
    struct tzero_memory_entry_t temp_entry;
    for( uint32_t i = 0; i < mmap_entries_tzero - 1U; i++ )
    {
        // Find the lowest physical starting address for any entry starting at index (i)
        uint32_t lowest_index = i;
        for( uint32_t j = i; j < mmap_entries_tzero; j++ )
        {
            if( out_params->memory_map.list[ j ].base_address < out_params->memory_map.list[ lowest_index ].base_address )
            {
                lowest_index = j;
            }
        }

        // And now, we want to swap the entry at index 'i' with index 'lowest_index'
        if( i != lowest_index )
        {
            memcpy( &temp_entry, out_params->memory_map.list + i, sizeof( struct tzero_memory_entry_t ) );
            memcpy( out_params->memory_map.list + i, out_params->memory_map.list + lowest_index, sizeof( struct tzero_memory_entry_t ) );
            memcpy( out_params->memory_map.list + lowest_index, &temp_entry, sizeof( struct tzero_memory_entry_t ) );
        }
    }

    return true;
}


/*  
    ACPI Table Location Function
*/
bool parse_acpi_info( struct tzero_x86_payload_parameters_t* out_arch_info )
{
    if( out_arch_info == NULL ) { return false; }

    // We need to iterate through system tables looking for RSDP, either the new or old version, preferring the new version
    EFI_CONFIGURATION_TABLE* ptr_config     = efi_table->ConfigurationTable;
    void* ptr_rsdp                          = NULL;
    EFI_GUID acpi_v2_guid                   = ACPI_20_TABLE_GUID;
    EFI_GUID acpi_v1_guid                   = ACPI_TABLE_GUID;
    bool b_new_rsdp                         = false;

    for( UINTN i = 0; i < efi_table->NumberOfTableEntries; i++ )
    {
        if( memcmp( &( ptr_config[ i ].VendorGuid ), &acpi_v1_guid, sizeof( EFI_GUID ) ) == 0 &&
            memcmp( ptr_config[ i ].VendorTable, (void*) "RSD PTR ", 8 ) == 0 )
        {
            // Found an RSDP Version 1 
            ptr_rsdp = ptr_config[ i ].VendorTable;
        }
        else if( memcmp( &( ptr_config[ i ].VendorGuid ), &acpi_v2_guid, sizeof( EFI_GUID ) ) == 0 &&
            memcmp( ptr_config[ i ].VendorTable, (void*) "RSD PTR ", 8 ) == 0 )
        {
            // Found an RSDP Version 2, so we dont need to keep searching
            ptr_rsdp = ptr_config[ i ].VendorTable;
            b_new_rsdp = true;
            break;
        }
    }

    if( ptr_rsdp == NULL ) { return false; }
    out_arch_info->acpi.rsdp_phys_addr      = (uint64_t) ptr_rsdp;
    out_arch_info->acpi.b_rsdp_new_version  = b_new_rsdp;

    // Print debug message
    tzero_console_prints16( L"T-0: Found ACPI table information!  Version: " );
    tzero_console_prints16( b_new_rsdp ? L"2.0" : L"1.0" );
    tzero_console_prints16( L"  Address: " );
    tzero_console_printh64( (uint64_t) ptr_rsdp, true );
    tzero_console_printnl();

    return true;
}



/*
    Launch Success Callback
*/
__attribute__(( sysv_abi )) void on_success_callback( void )
{
    // Parse the memory map and exit EFI boot services
    if( !finalize_memory_map( tzero_params ) )
    {
        tzero_console_printnl();
        tzero_console_prints16( L"T-0 (ERROR): Failed to handover system control to the kernel, entering halted state\r\n" );
        tzero_halt();
    }
}


/*
    Launch Failure Callback
*/
__attribute__(( sysv_abi )) void on_failure_callback( const char* message )
{
    tzero_console_printnl();
    tzero_console_printnl();
    tzero_console_prints16( L"T-0 (ERROR): The kernel was unabled to be launched! Error: " );
    tzero_console_prints8( message == NULL ? "No message provided, the installation may be corrupt" : message );
    tzero_console_printnl();
    tzero_halt();
}

