/*==============================================================
    Axon Kernel - Global Scheduler (Process Scheduler)
    2021, Zachary Berry
    axon/source/scheduler/global_scheduler.c
==============================================================*/

#include "axon/scheduler/global_scheduler.h"
#include "axon/scheduler/local_scheduler.h"
#include "axon/arch.h"
#include "axon/system/sysinfo.h"
#include "axon/library/rbtree.h"
#include "axon/library/spinlock.h"
#include "axon/debug_print.h"

#include "stdlib.h"


/*
    State
*/
static bool g_global_schd_init                      = false;
static uint32_t g_local_schd_count                  = 0U;
static struct axk_local_scheduler_t** g_local_schd  = NULL;

static struct axk_rbtree_t g_process_list;
static struct axk_spinlock_t g_process_lock;
static struct axk_process_t g_kernel_process;

/*
    Functions
*/
bool axk_scheduler_init_global( void )
{
    if( g_global_schd_init ) { return false; }
    g_global_schd_init = true;

    // Get information required to initialize
    struct axk_sysinfo_general_t general_info;
    if( !axk_sysinfo_query( AXK_SYSINFO_GENERAL, 0, &general_info, sizeof( general_info ) ) ) { return false; }

    // Allocate array to store local scheduler pointers within
    g_local_schd_count = general_info.cpu_count + general_info.alt_cpu_count;
    g_local_schd = (struct axk_local_scheduler_t**) calloc( g_local_schd_count, sizeof( void* ) );

    // Now, we need to determine what type of local scheduler to create for each thread
    // TODO: For now, were going to use the 'default' local scheduler
    for( uint32_t i = 0; i < g_local_schd_count; i++ )
    {
        g_local_schd[ i ] = axk_create_smp_scheduler();
    }

    axk_rbtree_create( &g_process_list, sizeof( void* ), NULL, NULL );
    axk_rbtree_create( &( g_kernel_process.threads ), sizeof( void* ), NULL, NULL );
    axk_spinlock_init( &g_process_lock );

    // Create and insert the special 'kernel process'
    g_kernel_process.id         = AXK_PROCESS_KERNEL;
    g_kernel_process.type       = AXK_PROCESS_TYPE_KERNEL;
    g_kernel_process.priority   = AXK_PRIORITY_HIGH;
    g_kernel_process.name       = "axkernel";

    void* ptr_kproc = (void*) &g_kernel_process;
    axk_rbtree_insert( &g_process_list, AXK_PROCESS_KERNEL, (void*) &ptr_kproc );

    axk_terminal_lock();
    axk_terminal_prints( "Global Scheduler: Initialized! \n" );
    axk_terminal_unlock();

    return true;
}

bool axk_scheduler_init_local( void )
{
    uint32_t cpu_id = axk_get_cpu_id();

    // Get reference to the local scheduler
    struct axk_local_scheduler_t* ptr_schd = g_local_schd[ cpu_id ];
    if( ptr_schd == NULL ) { return false; }

    if( !ptr_schd->init( ptr_schd ) )
    {
        return false;
    } 

    axk_terminal_lock();
    axk_terminal_prints( "Local Scheduler: Initialized on processor " );
    axk_terminal_printu32( cpu_id );
    axk_terminal_prints( "\n" );
    axk_terminal_unlock();

    return true;
}
