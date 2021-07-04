/*==============================================================
    Axon Kernel - x86 HPET Driver
    2021, Zachary Berry
    axon/source/arch_x86/hpet_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/hpet_driver.h"
#include "axon/arch_x86/acpi_info.h"
#include "stdlib.h"

/*
    Create Driver Function
*/
bool hpet_init( struct axk_timer_driver_t* self );
bool hpet_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );


struct axk_timer_driver_t* axk_x86_create_hpet_driver( struct axk_x86_hpet_info_t* ptr_info )
{
    if( ptr_info == NULL ) { return NULL; }

    struct axk_x86_hpet_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_hpet_driver_t, 1 );
    output->func_table.init = hpet_init;
    output->func_table.query_features = hpet_query_features;
    output->info = ptr_info;

    return (struct axk_timer_driver_t*)( output );
}


/*
    Driver Functions
*/
bool hpet_init( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    if( this == NULL || this->info == NULL ) { return false; }

    
    return true;
}


bool hpet_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return false;
}