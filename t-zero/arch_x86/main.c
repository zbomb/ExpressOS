/*==============================================================
    T-0 Bootloader - Main Code File
    2021, Zachary Berry
    t-zero/source/main.c
==============================================================*/

#include "tzero.h"
#include "libc.h"

/*
    Globals
*/
static EFI_SYSTEM_TABLE* g_sys_table    = NULL;
static EFI_HANDLE g_img_handle          = NULL;
static struct efi_framebuffer_t g_framebuffer;
static struct efi_resolution_t* g_resolutions = NULL;
static uint32_t g_resolutions_count = 0;
/*
    GUIDS
*/
EFI_GUID LoadedImageProtocol    = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID FileSystemProtocol     = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID GenericFileInfo        = EFI_FILE_INFO_ID;


/* 
    Helper Functions
*/
void tzero_halt( void )
{
    while( true ) { __asm__( "pause" ); }
}


inline static void tzero_console_prints( CHAR16* in_str )
{
    g_sys_table->ConOut->OutputString( g_sys_table->ConOut, in_str );
}


inline static void tzero_console_clear( void )
{
    g_sys_table->ConOut->ClearScreen( g_sys_table->ConOut );
}


inline static void tzero_console_printnl( void )
{
    tzero_console_prints( L"\r\n" );
}

inline static void tzero_console_setcolor( uint8_t in_fg, uint8_t in_bg )
{
    g_sys_table->ConOut->SetAttribute( g_sys_table->ConOut, TZERO_FGBG_COLOR( in_fg, in_bg ) );
}

inline static void tzero_console_setfg( uint8_t in_fg )
{
    g_sys_table->ConOut->SetAttribute( g_sys_table->ConOut, in_fg | ( g_sys_table->ConOut->Mode->Attribute & 0b1110000 ) );
}

inline static void tzero_console_setbg( uint8_t in_bg )
{
    g_sys_table->ConOut->SetAttribute( g_sys_table->ConOut, ( g_sys_table->ConOut->Mode->Attribute & 0b1111 ) | ( in_bg << 4 ) );
}

void tzero_console_printu32( uint32_t num )
{
    if( num == 0U )
    {
        tzero_console_prints( L"0" );
    }
    else
    {
        // Loop through each digit, get its value and print the corresponding character
        // Max value of a uint32_t has 10 digits in base 10
        uint32_t place_value = 1000000000;
        bool b_zero = true;
        uint16_t str_data[ 11 ];
        uint8_t index = 0;

        for( int i = 9; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10;
                continue;
            }

            b_zero = false;
            uint32_t digit = num / place_value;

            str_data[ index++ ] = (uint16_t)( digit + 0x30 );

            num -= digit * place_value;
            place_value /= 10;
        }
        
        // Write null terminator character at the end of the string data
        str_data[ index ] = 0x00;
        tzero_console_prints( (CHAR16*) str_data );
    }
}


void tzero_console_printu64( uint64_t num )
{
    if( num == 0UL )
    {
        tzero_console_prints( L"0" );
    }
    else if( num <= 0xFFFFFFFF )
    {
        tzero_console_printu32( (uint32_t) num );
    }
    else
    {
        uint64_t place_value = 10000000000000000000UL;
        bool b_zero = true;
        uint16_t str_data[ 21 ];
        uint8_t index = 0;

        for( int i = 19; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10UL;
                continue;
            }

            b_zero = false;
            uint64_t digit = num / place_value;

            str_data[ index++ ] = (uint16_t)( digit + 0x30 );

            num -= digit * place_value;
            place_value /=  10UL;
        }

        // Write null terminator and print string
        str_data[ index ] = 0x00;
        tzero_console_prints( (CHAR16*) str_data );
    }
}


uint8_t _print_hex_byte( uint8_t in_byte, uint16_t* out_chars, bool* b_print_lz )
{
    static const char lookup_table[ 16 ] = 
    {
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F'
    };

    uint8_t high = ( in_byte & 0xF0 ) >> 4;
    uint8_t low = ( in_byte & 0x0F );

    if( *b_print_lz || high > 0 )
    {
        *b_print_lz = true;
        out_chars[ 0 ] = lookup_table[ high ];
        out_chars[ 1 ] = lookup_table[ low ];

        return 2;
    }
    else
    {
        if( *b_print_lz || low > 0 )
        {
            *b_print_lz = true;
            out_chars[ 0 ] = lookup_table[ low ];

            return 1;
        }
        else
        {
            return 0;
        }
    }
}


void tzero_console_printh32( uint32_t num, bool b_print_leading_zeros )
{
    uint16_t str_data[ 11 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = (uint16_t)( '0' );
    str_data[ index++ ] = (uint16_t)( 'x' );

    // Write each byte in hex
    index += _print_hex_byte( ( num & 0xFF000000 ) >> 24, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x00FF0000 ) >> 16, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000FF00 ) >> 8, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000FF ), str_data + index, &b_print_leading_zeros );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_print_leading_zeros )
    {
        str_data[ index++ ] = (uint16_t)( '0' );
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    tzero_console_prints( (CHAR16*) str_data );
}


void tzero_console_printh64( uint64_t num, bool b_print_leading_zeros )
{
    uint16_t str_data[ 19 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = (uint16_t)( '0' );
    str_data[ index++ ] = (uint16_t)( 'x' );

    // Write each byte in hex
    index += _print_hex_byte( ( num & 0xFF00000000000000 ) >> 56, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x00FF000000000000 ) >> 48, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000FF0000000000 ) >> 40, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000FF00000000 ) >> 32, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x00000000FF000000 ) >> 24, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000000000FF0000 ) >> 16, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000000000FF00 ) >> 8, str_data + index, &b_print_leading_zeros );
    index += _print_hex_byte( ( num & 0x00000000000000FF ), str_data + index, &b_print_leading_zeros );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_print_leading_zeros )
    {
        str_data[ index++ ] = (uint16_t)( '0' );
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    tzero_console_prints( (CHAR16*) str_data );
}


void* tzero_alloc_data( uint64_t size )
{
    void* output = NULL;
    EFI_STATUS status = g_sys_table->BootServices->AllocatePool( EfiLoaderData, size, &output );
    
    if( EFI_ERROR( status ) || output == NULL )
    {
        tzero_console_prints( L"===> T-0 (ERROR): Memory allocation failed! Error code: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_prints( L" \r\n" );
        tzero_halt();
    }

    return output;
}

void tzero_free_data( void* ptr )
{
    if( ptr == NULL ) { return; }

    EFI_STATUS status = g_sys_table->BootServices->FreePool( ptr );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"====> T-0 (WARNING): Failed to release allocated memory! Error code: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_printnl();
    }
}


EFI_FILE* tzero_load_file( EFI_FILE* in_dir, CHAR16* in_path )
{
    EFI_FILE* output                    = NULL;
    EFI_LOADED_IMAGE* image_info        = NULL;
    EFI_FILE_IO_INTERFACE* file_system  = NULL;

    g_sys_table->BootServices->HandleProtocol( g_img_handle, &LoadedImageProtocol, (void**) &image_info );
    g_sys_table->BootServices->HandleProtocol( image_info->DeviceHandle, &FileSystemProtocol, (void**) &file_system );

    if( in_dir == NULL )
    {
        file_system->OpenVolume( file_system, &in_dir );
    }

    return( in_dir->Open( in_dir, &output, in_path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY ) == EFI_SUCCESS ? output : NULL );
}

bool tzero_init_framebuffer( struct efi_framebuffer_t* out_buffer )
{
    if( out_buffer == NULL ) { return false; }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* ptr_gop   = NULL;
    EFI_GUID gop_guid                       = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    
    // Locate the GOP protocol
    //if( EFI_ERROR( uefi_call_wrapper(
    //    g_sys_table->BootServices->LocateProtocol, 3, &gop_guid, NULL, (void**) &ptr_gop
    //) ) )
    if( EFI_ERROR( g_sys_table->BootServices->LocateProtocol( &gop_guid, NULL, (void**) &ptr_gop ) ) )
    {
        return false;
    }

    // Query for all available modes
    EFI_STATUS status;
    uint32_t max_mode       = ptr_gop->Mode->MaxMode;
    g_resolutions           = (struct efi_resolution_t*) tzero_alloc_data( sizeof( struct efi_resolution_t ) * max_mode );
    g_resolutions_count     = 0;

    for( uint32_t i = 0; i < max_mode; i++ )
    {
        // Get the next available graphics mode
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
        UINTN sz = 0;
        info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*) tzero_alloc_data( sizeof( EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) );
        status = ptr_gop->QueryMode( ptr_gop, i, &sz, &info );
        if( EFI_ERROR( status ) ) { continue; }

        switch( info->PixelFormat )
        {
            case PixelRedGreenBlueReserved8BitPerColor:

            g_resolutions[ g_resolutions_count ].width                  = info->HorizontalResolution;
            g_resolutions[ g_resolutions_count ].height                 = info->VerticalResolution;
            g_resolutions[ g_resolutions_count ].pixels_per_scanline    = info->PixelsPerScanLine;
            g_resolutions[ g_resolutions_count ].index                  = i;
            g_resolutions[ g_resolutions_count++ ].format               = efi_pixel_rgbx_32;
            break;

            case PixelBlueGreenRedReserved8BitPerColor:

            g_resolutions[ g_resolutions_count ].width                  = info->HorizontalResolution;
            g_resolutions[ g_resolutions_count ].height                 = info->VerticalResolution;
            g_resolutions[ g_resolutions_count ].pixels_per_scanline    = info->PixelsPerScanLine;
            g_resolutions[ g_resolutions_count ].index                  = i;
            g_resolutions[ g_resolutions_count++ ].format               = efi_pixel_bgrx_32;
            break;

            case PixelBitMask:
            /* fall through */
            case PixelBltOnly:
            /* fall through */
            default:
            break;
        }
        
        tzero_free_data( (void*) info );
    }

    // Now, we need to determine what mode to select, were simply going to stick with the current mode for now
    // TODO: Change to user desired mode

    // Setup the framebuffer
    out_buffer->address                 = (void*) ptr_gop->Mode->FrameBufferBase;
    out_buffer->size                    = ptr_gop->Mode->FrameBufferSize;

    out_buffer->res.width                   = ptr_gop->Mode->Info->HorizontalResolution;
    out_buffer->res.height                  = ptr_gop->Mode->Info->VerticalResolution;
    out_buffer->res.pixels_per_scanline     = ptr_gop->Mode->Info->PixelsPerScanLine;
    out_buffer->res.index                   = ptr_gop->Mode->Mode;
    
    if( ptr_gop->Mode->Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor )
    {
        out_buffer->res.format = efi_pixel_rgbx_32;
    }
    else if( ptr_gop->Mode->Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor )
    {
        out_buffer->res.format = efi_pixel_bgrx_32;
    }
    else
    {
        tzero_console_prints( L"==> T-0 (ERROR): Current video mode is unsupported by the payload! \r\n" );
        return false;
    }

    return true;
}


/*
    Entry Point
*/
EFI_STATUS efi_main( EFI_HANDLE in_handle, EFI_SYSTEM_TABLE* in_table )
{
    // Store the globals so other functions can access them
    g_sys_table     = in_table;
    g_img_handle    = in_handle;

    // Clear the screen and write our 'initializing message'
    tzero_console_clear();
    tzero_console_prints( L"==> T-0 Bootloader Initializing...\r\n" );

    // Attempt to load the target binary
    EFI_STATUS status;
    EFI_FILE* bin_file = tzero_load_file( NULL, L"AXON.BIN" );

    if( bin_file == NULL )
    {
        tzero_console_prints( L"====> T-0 (ERROR): Failed to find the payload file! \r\n" );
        tzero_halt();
    }

    // Get some basic info about the binary, we have to call 'GetInfo' twice, first to get the size, so we can allocate storage, then to fill 
    UINTN finfo_size            = 0U;
    EFI_FILE_INFO* ptr_finfo    = NULL;

    bin_file->GetInfo( bin_file, &GenericFileInfo, &finfo_size, NULL );
    ptr_finfo = (EFI_FILE_INFO*) tzero_alloc_data( finfo_size );
    status = bin_file->GetInfo( bin_file, &GenericFileInfo, &finfo_size, (void*) ptr_finfo );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"===> T-0 (ERROR): Failed to find target payload to launch!\r\n" );
        tzero_halt();
    }

    // Now, were going to read the file into a buffer
    // This will be temporary, we need to validate the file before actually properly loading it in and executing it
    UINTN file_size     = ptr_finfo->FileSize;
    void* file_buffer   = tzero_alloc_data( file_size );

    status = bin_file->Read( bin_file, &file_size, file_buffer );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"====> T-0 (ERROR): Failed to read the target payload file\r\n" );
        tzero_halt();
    }

    // Compute CRC32
    UINT32 payload_hash = 0U;
    status = g_sys_table->BootServices->CalculateCrc32( file_buffer, file_size, &payload_hash );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"====> T-0 (ERROR): Failed to compute the hash of the payload \r\n" );
        tzero_halt();
    }

    // TODO: Some form of signature to somewhat validate the kernel is as we expect and hasnt been modified by another party since build
    tzero_free_data( file_buffer );

    // Now, were going to copy the header data to a local structure
    struct elf_ehdr_t header;
    UINTN header_size = (UINTN) sizeof( struct elf_ehdr_t );

    status = bin_file->SetPosition( bin_file, 0UL );
    status = EFI_ERROR( status ) ? status : bin_file->Read( bin_file, &header_size, (void*) &header );

    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"====> T-0 (ERROR): Failed to read payload header \r\n" );
        tzero_halt();
    }

    // Validate the ELF header
    if( memcmp( header.ident, ELF_MAGIC, ELF_MAGIC_SZ ) != 0 ||
        header.ident[ EI_CLASS ] != ELF_CLASS_64 ||
        header.ident[ EI_DATA ] != ELF_DATA_2LSB ||
        header.type != ET_EXEC ||
        header.machine != EM_X86_64 ||
        header.version != EV_CURRENT )
    {
        tzero_console_prints( L"\r\n=====> T-0 (ERROR): The payload ELF had an invalid format and couldnt be loaded!\r\n" );
        tzero_halt();
    }

    UINTN psz = header.phnum * header.phentsize;
    struct elf_phdr_t* phdr_list = (struct elf_phdr_t*) tzero_alloc_data( psz );
    status = bin_file->SetPosition( bin_file, header.phoff );
    status = EFI_ERROR( status ) ? status : bin_file->Read( bin_file, &psz, (void*) phdr_list );
    
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints( L"=====> T-0 (ERROR): Failed to read payload ELF file\r\n" );
        tzero_halt();
    }

    bool b_loaded = false;
    for( uint32_t i = 0; i < header.phnum; i++ )
    {
        struct elf_phdr_t* ptr = phdr_list + i;
        if( ptr->type == PT_LOAD )
        {
            uint64_t page_count = ( ptr->memsz + 0x1000UL - 1UL ) / 0x1000UL;
            elf_addr_t seg = ptr->paddr;

            status = g_sys_table->BootServices->AllocatePages( AllocateAddress, EfiLoaderData, page_count, &seg );
            status = EFI_ERROR( status ) ? status : bin_file->SetPosition( bin_file, ptr->offset );
            
            UINTN sz = ptr->filesz;
            status = EFI_ERROR( status ) ? status : bin_file->Read( bin_file, &sz, (void*)seg );

            if( EFI_ERROR( status ) )
            {
                tzero_console_prints( L"======> T-0 (ERROR): Failed to move the payload into system memory. Error Code: " );
                tzero_console_printu32( (uint32_t) status );
                tzero_console_printnl();
                tzero_halt();
            }
            
            b_loaded = true;
            //break;
        }
    }

    if( !b_loaded )
    {
        tzero_console_prints( L"=====> T-0 (ERROR): The target payload wasnt able to be copied into system memory\r\n" );
        tzero_halt();
    }

    // Next, we want to setup the framebuffer
    if( !tzero_init_framebuffer( &g_framebuffer ) )
    {
        tzero_console_prints( L"====> T-0 (ERROR): Failed to initialize framebuffer!\r\n" );
        tzero_halt();
    }

    /*
        Get a copy of the physical memory map
    */
    EFI_MEMORY_DESCRIPTOR* ptr_memdesc = NULL;
    UINTN memdesc_size, map_size, map_key;
    UINT32 memdesc_ver;

    g_sys_table->BootServices->GetMemoryMap( &map_size, ptr_memdesc, &map_key, &memdesc_size, &memdesc_ver );
    g_sys_table->BootServices->AllocatePool( EfiLoaderData, map_size, (void**) &ptr_memdesc );
    g_sys_table->BootServices->GetMemoryMap( &map_size, ptr_memdesc, &map_key, &memdesc_size, &memdesc_ver );

    /*
        Search for the ACPI table
    */
    EFI_CONFIGURATION_TABLE* ptr_config     = g_sys_table->ConfigurationTable;
    void* ptr_rsdp                          = NULL;
    EFI_GUID acpi_v2_guid                   = ACPI_20_TABLE_GUID;
    EFI_GUID acpi_v1_guid                   = ACPI_TABLE_GUID;
    bool b_new_rsdp                         = false;

    for( UINTN i = 0; i < g_sys_table->NumberOfTableEntries; i++ )
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

    if( ptr_rsdp == NULL )
    {
        tzero_console_prints( L"===> T-0 (ERROR): Failed to locate ACPI RSDP table!\r\n" );
        tzero_halt();
    }

    // Locate the entry point for the kernel
    void( *entry_fn )( struct tzero_payload_params_t*, uint64_t ) = ( (__attribute__((sysv_abi)) void(*)( struct tzero_payload_params_t*, uint64_t ) ) (uint64_t)( header.entry ) - TZERO_KVA_OFFSET );

    // Build the parameters to pass the payload, exit boot services and launch the payload
    struct tzero_payload_params_t params;

    params.magic_value              = TZERO_MAGIC_VALUE;
    params.framebuffer_ptr          = g_framebuffer.address;
    params.framebuffer_size         = g_framebuffer.size;
    params.framebuffer_res          = g_framebuffer.res;
    params.memmap_ptr               = ptr_memdesc;
    params.memmap_size              = map_size;
    params.memmap_desc_size         = memdesc_size;
    params.rsdp_ptr                 = ptr_rsdp;
    params.b_new_rsdp               = b_new_rsdp;
    params.res_list                 = g_resolutions;
    params.res_count                = g_resolutions_count;
    
    /*
    tzero_console_printnl();
    tzero_console_prints( L"===> Mode Count: " );
    tzero_console_printu32( params.res_count );
    tzero_console_prints( L"\r\n" );

    for( uint32_t i = 0; i < params.res_count; i++ )
    {
        tzero_console_prints( L"===> " );
        tzero_console_printu32( i );
        tzero_console_prints( L":  Width=" );
        tzero_console_printu32( params.res_list[ i ].width );
        tzero_console_prints( L"  Height=" );
        tzero_console_printu32( params.res_list[ i ].height );
        tzero_console_prints( params.res_list[ i ].format == efi_pixel_rgbx_32 ? L"  RGB\r\n" : L"  BGR\r\n" );
    }
    */

    tzero_console_prints( L"===> Launching payload...\r\n" );
    g_sys_table->BootServices->ExitBootServices( in_handle, map_key );

    // Directly jump to the code, we dont bother using a function call
    __asm__( 
        "movq %2, %%rdi\n"
        "movabsq $0x4C4946544F464621ul, %%rsi\n"
        "movq %0, %%rax\n"
        "jmp %%rax"
        : : "r"( (uint64_t)( entry_fn ) ), "r"( TZERO_MAGIC_VALUE ), "r"( &params )
        :
    );

    return EFI_SUCCESS;
}