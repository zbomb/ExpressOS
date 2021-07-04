/*==============================================================
    Axon Kernel - x86 TSC Driver
    2021, Zachary Berry
    axon/source/arch_x86/tsc_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/tsc_driver.h"
#include "stdlib.h"

/*
    Create Driver Function
*/
bool tsc_init( struct axk_timer_driver_t* self );
bool tsc_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );


struct axk_timer_driver_t* axk_x86_create_tsc_driver( void )
{
    struct axk_x86_tsc_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_tsc_driver_t, 1 );
    output->func_table.init = tsc_init;
    output->func_table.query_features = tsc_query_features;

    return (struct axk_timer_driver_t*)( output );
}

/*
    Calibrate Driver Function
*/
bool axk_x86_calibrate_tsc_driver( struct axk_timer_driver_t* self )
{
    struct axk_x86_tsc_driver_t* this = (struct axk_x86_tsc_driver_t*)( self );
    if( this == NULL ) { return false; }

    // TODO

    return true;
}


/*
    Driver Functions
*/
bool tsc_init( struct axk_timer_driver_t* self )
{
    return true;
}


bool tsc_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return false;
}