/*==============================================================
    T-0 Bootloader - Utility Functions
    2021, Zachary Berry
    t-zero/source/util.c
==============================================================*/

#include "util.h"
#include "tzero.h"
#include "console.h"

/*
    Constants
*/
EFI_GUID LoadedImageProtocol    = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID FileSystemProtocol     = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID GenericFileInfo        = EFI_FILE_INFO_ID;

/*
    Globals
*/
extern EFI_SYSTEM_TABLE* efi_table;
extern EFI_HANDLE efi_handle;

/*
    Function Implementations
*/
int memcmp( const void* ptr_a, const void* ptr_b, size_t count )
{
    if( ptr_a == NULL && ptr_b != NULL ) { return -1; }
    if( ptr_b == NULL && ptr_a != NULL ) { return 1; }
    if( ptr_a == NULL && ptr_b == NULL ) { return 0; }
    
    const uint8_t* a = ptr_a;
    const uint8_t* b = ptr_b;

    for( size_t i = 0; i < count; i++ )
    {
        if( a[ i ] < b[ i ] ) { return -1; }
        else if( a[ i ] > b[ i ] ) { return 1; }
    }

    return 0;
}


int strcmp( const char* str_a, const char* str_b )
{
    if( str_a == NULL && str_b != NULL ) { return -1; }
    if( str_b == NULL && str_a != NULL ) { return 1; }
    if( str_a == NULL && str_b == NULL ) { return 0; }

    size_t index = 0;
    while( 1 )
    {
        if( str_a[ index ] != str_b[ index ] )
        {
            return( (int)str_a[ index ] - (int)str_b[ index ] );
        }

        if( str_a[ index ] == '\0' )
        {
            return 0;
        }

        index++;
    }

    return 0; // Uncreachable
}

 
int strcmp_u16( const uint16_t* str_a, const uint16_t* str_b )
{
    if( str_a == NULL && str_b != NULL ) { return -1; }
    if( str_b == NULL && str_a != NULL ) { return 1; }
    if( str_a == NULL && str_b == NULL ) { return 0; }

    size_t index = 0;
    while( 1 )
    {
        if( str_a[ index ] != str_b[ index ] )
        {
            return( (int)str_a[ index ] - (int)str_b[ index ] );
        }

        if( str_a[ index ] == '\0' )
        {
            return 0;
        }

        index++;
    }

    return 0; // Uncreachable
}


void* memset( void* dst, int val, size_t count )
{
    if( dst == NULL ) { return NULL; }

    uint8_t* p = (uint8_t*)( dst );
    for( size_t i = 0; i < count; i++ )
    {
        p[ i ] = (uint8_t)( val );
    }

    return dst;
}


size_t strlen( const char* str )
{
    size_t count = 0UL;
    if( str == NULL ) { return count; }
    while( str[ count ] != '\0' ) { count++; }
    return count;
}


void* memcpy( void* restrict dst, const void* restrict src, size_t count )
{
    if( dst == NULL || src == NULL || count == 0UL ) { return dst; }
    for( size_t i = 0; i < count; i++ ) { ( (uint8_t*)dst )[ i ] = ( (uint8_t*)src )[ i ]; }
    return dst;
}


__attribute__((__noreturn__)) void tzero_halt( void )
{
    while( true ) { __asm__( "hlt" ); }
}


void* tzero_alloc( uint64_t sz )
{
    void* output = NULL;
    EFI_STATUS status = efi_table->BootServices->AllocatePool( EfiLoaderData, sz, &output );
    
    if( EFI_ERROR( status ) || output == NULL )
    {
        tzero_console_prints16( L"T-0 (ERROR): Memory allocation failed! Error code: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_prints16( L" \r\n" );
        tzero_halt();
    }

    return output;
}


void tzero_free( void* ptr )
{
    if( ptr == NULL ) { return; }

    EFI_STATUS status = efi_table->BootServices->FreePool( ptr );
    if( EFI_ERROR( status ) )
    {
        tzero_console_prints16( L"T-0 (WARNING): Failed to release allocated memory! Error code: " );
        tzero_console_printu32( (uint32_t) status );
        tzero_console_printnl();
    }
}


__attribute__((__noreturn__)) void tzero_launch( uint64_t entry_point, void* param_one, void* param_two )
{
    // TODO: Does the noreturn attribute affect the lifetime of locals in the block that calls this function?
    // i.e. If 'param_one' or 'param_two' is a pointer to a local in the function that calls 'tzero_launch', will they be popped from the stack before this function is called?

    if( entry_point == 0UL )
    {
        tzero_console_prints16( L"T-0 (WARNING): Attempt to launch payload with NULL entry point! Bootloader will now halt \r\n" );
        tzero_halt();
    }

    tzero_console_prints16( L"T-0: Launching payload.. entry point at " );
    tzero_console_printh64( entry_point, true );
    tzero_console_printnl();

    __asm__( 
        "movq %2, %%rdi\n"
        "movq %1, %%rsi\n"
        "movq %0, %%rax\n"
        "jmp *%%rax"
        : : "r"( (uint64_t)( entry_point ) ), "r"( param_two ), "r"( param_one )
        :
    );

    while( 1 ) { __asm__( "hlt" ); }
}


EFI_FILE* tzero_open_file( EFI_FILE* directory, CHAR16* path, EFI_FILE_INFO* out_info )
{
    EFI_FILE* output                    = NULL;
    EFI_LOADED_IMAGE* image_info        = NULL;
    EFI_FILE_IO_INTERFACE* file_system  = NULL;

    efi_table->BootServices->HandleProtocol( efi_handle, &LoadedImageProtocol, (void**) &image_info );
    efi_table->BootServices->HandleProtocol( image_info->DeviceHandle, &FileSystemProtocol, (void**) &file_system );

    if( directory == NULL )
    {
        file_system->OpenVolume( file_system, &directory );
    }

    // Open the file
    if( EFI_ERROR( directory->Open( directory, &output, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY ) ) )
    {
        return NULL;
    }
    
    // Get file info (optional)
    if( out_info != NULL )
    {
        UINTN finfo_sz;
        if( EFI_ERROR( output->GetInfo( output, &GenericFileInfo, &finfo_sz, NULL ) ) ) { return NULL; }
        void* info_buffer = tzero_alloc( finfo_sz );
        if( EFI_ERROR( output->GetInfo( output, &GenericFileInfo, &finfo_sz, info_buffer ) ) )
        {
            tzero_free( info_buffer );
            return NULL;
        }

        memcpy( (void*) out_info, info_buffer, sizeof( EFI_FILE_INFO ) );
        tzero_free( info_buffer );
    }

    return output;
}