/*==============================================================
    Axon Kernel - C Entry Point (x86)
    2021, Zachary Berry
    axon/source/arch_x86/entry.c
==============================================================*/
#ifdef _X86_64_

#include "axon/config.h"
#include "axon/arch_x86/util.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/system/interrupts.h"
#include "axon/debug_print.h"
#include "string.h"

/*
    Constants
*/
#define CACHE_TYPE_DATA             1
#define CACHE_TYPE_INSTRUCTIONS     2
#define CACHE_TYPE_UNIFIED          3
#define CACHE_TYPE_NULL             0

/*
    State
*/
static uint32_t smt_mask            = 0U;
static uint32_t package_mask        = 0U;
static uint32_t core_mask           = 0U;
static uint32_t package_mask_shift  = 0U;
static uint32_t smt_mask_width      = 0U;
static bool b_intel_leaf_b          = false;

static uint32_t l1_mask         = 0U;
static uint32_t l2_mask         = 0U;
static uint32_t l3_mask         = 0U;
static uint32_t l4_mask         = 0U;
static uint32_t cache_count     = 0U;

static bool parse_intel_topology( uint32_t max_cpuid );
static bool parse_intel_leaf_b( void );
static bool parse_non_intel_ext( uint32_t max_cpuid );
static bool parse_legacy( uint32_t max_cpuid );
static void parse_cache( void );
static uint32_t create_mask( uint32_t num_entries, uint32_t* out_width );


/*
    Function to read the local processor topology
*/
void axk_x86_get_core_topology( uint32_t* out_smt, uint32_t* out_core, uint32_t* out_package )
{
    uint32_t start_lapic;
    uint32_t eax, ebx, ecx, edx;
    eax = 0x00; ebx = 0x00; ecx = 0x00; edx = 0x00;

    if( b_intel_leaf_b )
    {
        eax = 0x0B;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        start_lapic = edx;
    }
    else
    {
        eax = 0x01;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        start_lapic = ( ( ebx & 0xFF000000 ) >> 24 );
    }

    *out_smt        = ( start_lapic & smt_mask );
    *out_core       = ( start_lapic & core_mask ) >> smt_mask_width;
    *out_package    = ( start_lapic & package_mask ) >> package_mask_shift;
}


uint32_t axk_x86_get_cache_topology(    uint32_t* out_l1_index, uint32_t* out_l2_index, uint32_t* out_l3_index, uint32_t* out_l4_index,
                                        uint32_t* out_l1_size, uint32_t* out_l2_size, uint32_t* out_l3_size, uint32_t* out_l4_size )
{
    uint32_t start_lapic;
    uint32_t eax, ebx, ecx, edx;
    eax = 0x00; ebx = 0x00; ecx = 0x00; edx = 0x00;

    if( b_intel_leaf_b )
    {
        eax = 0x0B;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        start_lapic = edx;
    }
    else
    {
        eax = 0x01;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        start_lapic = ( ( ebx & 0xFF000000 ) >> 24 );
    }

    // Use the masks to determine the indicies
    if( out_l1_index != NULL ) { *out_l1_index = ( start_lapic & l1_mask ); }
    if( out_l2_index != NULL ) { *out_l2_index = cache_count >= 2 ? ( start_lapic & l2_mask ) : 0xFFFFFFFFU; }
    if( out_l3_index != NULL ) { *out_l3_index = cache_count >= 3 ? ( start_lapic & l3_mask ) : 0xFFFFFFFFU; }
    if( out_l4_index != NULL ) { *out_l4_index = cache_count >= 4 ? ( start_lapic & l4_mask ) : 0xFFFFFFFFU; }

    // Next, we need to determine the size of each cache, and since this might be different for each logical processor in an unsymetric system, lets calculate it
    if( out_l1_size != NULL || out_l2_size != NULL || out_l3_size != NULL || out_l4_size != NULL )
    {
        uint32_t eax, ebx, ecx, edx;
        uint32_t index = 0U;

        while( true )
        {
            eax = 0x04; ebx = 0x00; ecx = index++; edx = 0x00;
            axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

            uint8_t cache_type      = (uint8_t) AXK_EXTRACT( eax, 0, 4 );
            uint8_t cache_level     = (uint8_t) AXK_EXTRACT( eax, 5, 7 );

            if( cache_type == 0 || cache_type > 3 || cache_level == 0 ) { break; }

            uint32_t cache_size     = ( AXK_EXTRACT( ebx, 0, 11 ) + 1 ) * ( AXK_EXTRACT( ebx, 12, 21 ) + 1 ) * ( AXK_EXTRACT( ebx, 22, 31 ) + 1 ) * ecx;

            switch( cache_level )
            {
                case 1:
                if( out_l1_size != NULL ) { *out_l1_size = cache_size; }
                break;

                case 2:
                if( out_l2_size != NULL ) { *out_l2_size = cache_size; }
                break;

                case 3:
                if( out_l3_size != NULL ) { *out_l3_size = cache_size; }
                break;

                case 4:
                if( out_l4_size != NULL ) { *out_l4_size = cache_size; }
                break;
            }
        }
    }

    // Return maximum cache level
    return cache_count;
}


/*
    Function to determine global parameters from the BSP
*/
bool axk_x86_parse_topology( void )
{
    struct axk_x86_acpi_info_t* ptr_acpi    = axk_x86_acpi_get();
    bool b_intel                            = ( memcmp( ptr_acpi->cpu_vendor, "GenuineIntel", 12 ) == 0 );
    uint32_t max_cpuid                      = 0U;

    // Determine max CPUID and if this is an Intel or non-Intel processor
    uint32_t eax, ebx, ecx, edx;
    eax = 0x00; ebx = 0x00; ecx = 0x00; edx = 0x00;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
    max_cpuid = eax;

    parse_cache();

    if( b_intel )
    {
        // Check if we can use the method for CPUID Leaf 0B
        if( max_cpuid >= 0x0B )
        {
            eax = 0x0B; ebx = 0x00; ecx = 0x00; edx = 0x00;
            axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
            b_intel_leaf_b = ( ebx != 0U );
        }

        eax = 0x01; ebx = 0x00; ecx = 0x00; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

        if( AXK_CHECK_FLAG( edx, ( 1U << 28 ) ) )
        {
            if( b_intel_leaf_b )
            {
                return parse_intel_leaf_b();
            }
            else
            {
                return parse_legacy( max_cpuid );
            }
        }
        else
        {
            // No multithreading support
            core_mask           = 0U;
            smt_mask            = 0U;
            package_mask        = 0xFFFFFFFFU;
            package_mask_shift  = 0U;
            smt_mask            = 0U;

            return true;
        }
    }
    else
    {
        // For non-intel processors, we need to query CPUID.80000008H
        eax = 0x80000008; ebx = 0x00; ecx = 0x00; edx = 0x00;
        if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
        {
            return parse_non_intel_ext( max_cpuid );
        }
        else
        {
            return parse_legacy( max_cpuid );
        }
    }

}

bool parse_intel_leaf_b( void )
{
    uint32_t eax, ebx, ecx, edx;
    bool b_thread_reported          = false;
    bool b_core_reported            = false;
    uint32_t sub_leaf                = 0;
    uint32_t level_type              = 0;
    uint32_t level_shift             = 0;
    uint64_t core_plus_smt_mask     = 0UL;

    do
    {
        eax = 0x0B; ebx = 0x00; ecx = sub_leaf; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        if( ebx == 0U )
        {
            break;
        }

        level_type      = ( ( ecx & 0xFF00U ) >> 8 );
        level_shift     = ( eax & 0b11111U );

        switch( level_type )
        {
            case 1:     // SMT Level
            {
                smt_mask            = ~( (-1) << level_shift );
                smt_mask_width      = level_shift;
                b_thread_reported   = true;

                break;
            }
            case 2:     // Core Reported
            {
                b_core_reported = true;
                break;
            }
            default:
            break;
        }
        sub_leaf++;

    } while( 1 );

    core_plus_smt_mask  = ~( (-1) << level_shift );
    package_mask_shift  = level_shift;
    package_mask        = (-1) ^ core_plus_smt_mask;

    if( b_thread_reported && b_core_reported )
    {
        core_mask = core_plus_smt_mask ^ smt_mask;
    }
    else
    {
        return false;
    }

    return true;
}


bool parse_non_intel_ext( uint32_t max_cpuid )
{
    uint32_t eax, ebx, ecx, edx;
    eax = 0x80000008; ebx = 0x00; ecx = 0x00; edx = 0x00;
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        uint8_t cbits = ( ecx & 0b1111000000000000U ) >> 12;

        if( cbits != 0 )
        {
            core_mask       = create_mask( cbits, &package_mask_shift );
            package_mask    = (-1) ^ core_mask;
            smt_mask        = 0U;
            smt_mask_width  = 0U;
        }
        else
        {
            eax = 0x01; ebx = 0x00; ecx = 0x00; edx = 0x00;
            axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
            uint32_t total_count = ( ( ebx & 0x00FF0000U ) >> 16 );

            // Get the number of max cores in ECX, round to next highest power of two
            uint32_t j = ( ecx & 0xFFU );
            uint32_t rounded_count = 1U;
            while( rounded_count < j ) { rounded_count *= 2U; }
            core_mask = create_mask( rounded_count, &package_mask_shift );

            // Now determine the SMT bits
            uint32_t k = total_count >> package_mask_shift;
            uint32_t rounded_k = 1U;
            while( rounded_k < k ) { rounded_k *= 2U; }
            smt_mask = create_mask( rounded_k, &smt_mask_width );

            package_mask_shift  += smt_mask_width;
            core_mask           <<= smt_mask_width;
            package_mask        = (-1) ^ ( core_mask | smt_mask );
        }

        return true;
    }
    else
    {
        return parse_legacy( max_cpuid );
    }
}


bool parse_legacy( uint32_t max_cpuid )
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t core_plus_smt_id_max = 1U;
    uint32_t core_id_max = 1U;
    uint32_t smt_per_core_max = 1U;

    eax = 0x01; ebx = 0x00; ecx = 0x00; edx = 0x00;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
    core_plus_smt_id_max = ( ( ebx & 0x00FF0000U ) >> 16 );

    if( max_cpuid >= 0x04 )
    {
        eax = 0x04; ebx = 0x00; ecx = 0x00; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        core_id_max = ( ( eax & 0b111111U ) >> 26 ) + 1U;
        smt_per_core_max = core_plus_smt_id_max / core_id_max;
    }
    else
    {
        smt_per_core_max = core_plus_smt_id_max / core_id_max;
    }

    smt_mask            = create_mask( smt_per_core_max, &smt_mask_width );
    core_mask           = create_mask( core_id_max, &package_mask_shift );
    package_mask_shift  += smt_mask_width;
    core_mask           <<= smt_mask_width;
    package_mask        = (-1) ^ ( core_mask | smt_mask );

    return true;
}


/*
    Function to generate bitmasks from parameters
*/
static uint8_t reverse_bitmask( uint32_t* index, uint32_t mask )
{
    uint32_t i;
    for( i = ( 8 * sizeof( uint32_t ) ); i > 0; i-- )
    {
        if( ( mask & ( 1 << ( i - 1 ) ) ) != 0 )
        {
            *index = ( i - 1 );
            break;
        }
    }

    return( mask != 0UL );
}


static uint32_t create_mask( uint32_t num_entries, uint32_t* out_width )
{
    uint32_t i;
    uint32_t k;

    k = (uint32_t)( num_entries ) * 2 - 1;
    if( reverse_bitmask( &i, k ) == 0U )
    {
        if( out_width ) { *out_width = 0U; }
        return 0U;
    }

    if( out_width ) { *out_width = i; }
    if( i == 31 ) { return 0xFFFFFFFFU; }

    return( ( 1 << i ) - 1 );
}


static void parse_cache( void )
{
    // First, we want to read parameters for the L1 (or L1(d/i)) cache
    uint32_t eax, ebx, ecx, edx;

    uint32_t i = 0;
    while( true )
    {
        eax = 0x04; ebx = 0x00; ecx = i++; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        uint8_t cache_type = (uint8_t) AXK_EXTRACT( eax, 0, 4 );
        uint8_t cache_level = (uint8_t) AXK_EXTRACT( eax, 5, 7 );

        if( cache_type == CACHE_TYPE_NULL || cache_type > CACHE_TYPE_UNIFIED || cache_level == 0 ) { break; }

        uint32_t max_logical_cores = AXK_EXTRACT( eax, 14, 25 ) + 1;
        uint32_t new_value = 1;
        while( new_value < max_logical_cores ) { new_value *= 2U; }
        max_logical_cores = new_value;
        
        uint32_t max_package_cores = AXK_EXTRACT( eax, 26, 31 ) + 1;
        new_value = 1;
        while( new_value < max_package_cores ) { new_value *= 2U; }
        max_package_cores = new_value;

        uint32_t mask = ~( create_mask( max_logical_cores, NULL ) );

        // Now, lets store this information so each processor can figure out what caches its connected to
        if( cache_count < cache_level ) { cache_count = cache_level; }

        switch( cache_level )
        {
            case 1:
            // There might be multiple entries for L1, because data and instructions are generally split
            // Were going to assume they are about the same in terms of size and how theyre shared across logical processors
            l1_mask = mask;
            break;

            case 2:
            l2_mask = mask;
            break;

            case 3:
            l3_mask = mask;
            break;

            case 4:
            l4_mask = mask;
            break;
        }
    }
}

#endif
