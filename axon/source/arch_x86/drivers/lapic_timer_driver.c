/*==============================================================
    Axon Kernel - x86 LAPIC Timer Driver
    2021, Zachary Berry
    axon/source/arch_x86/lapic_timer_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/lapic_timer_driver.h"
#include "stdlib.h"


bool lapic_timer_init( struct axk_timer_driver_t* self );
bool lapic_timer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );

/*
    Create Driver Function
*/
struct axk_timer_driver_t* axk_x86_create_lapic_timer_driver( void )
{
    struct axk_x86_lapic_timer_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_lapic_timer_driver_t, 1 );
    output->func_table.init = lapic_timer_init;
    output->func_table.query_features = lapic_timer_query_features;

    return (struct axk_timer_driver_t*)( output );
}

bool axk_x86_calibrate_lapic_timer( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    if( this == NULL ) { return false; }

    // TODO

    return true;
}

/*
    Driver Functions
*/
bool lapic_timer_init( struct axk_timer_driver_t* self )
{
    return true;
}


bool lapic_timer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return false;
}