/*==============================================================
    Axon Kernel - x86 ACPI PM Timer Driver
    2021, Zachary Berry
    axon/source/arch_x86/acpi_timer_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/acpi_timer_driver.h"
#include "stdlib.h"

/*
    Create Driver Function
*/
bool acpitimer_init( struct axk_timer_driver_t* self );
bool acpitimer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );


struct axk_timer_driver_t* axk_x86_create_acpi_timer_driver( void )
{
    struct axk_x86_acpi_timer_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_acpi_timer_driver_t, 1 );
    output->func_table.init = acpitimer_init;
    output->func_table.query_features = acpitimer_query_features;

    return (struct axk_timer_driver_t*)( output );
}


/*
    Driver Functions
*/
bool acpitimer_init( struct axk_timer_driver_t* self )
{
    return true;
}


bool acpitimer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return false;
}