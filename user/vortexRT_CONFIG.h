

#ifndef  vortexRT_CONFIG_H
#define  vortexRT_CONFIG_H

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef uint16_t      timeout_t;
typedef uint_fast32_t tick_count_t;

#endif // __ASSEMBLER__

//------------------------------------------------------------------------------
//
//    Specify vortexRT Process Count. Must be less than 31
//
//
#define  vortexRT_PROCESS_COUNT                  31

//-----------------------------------------------------------------------------
//
//    vortexRT System Timer
//
//    Nested Interrupts enable macro. Value 1 means that interrupts are
//    globally enabled within System Timer ISR (this is default for Cortex-M3).
//
//
#define vortexRT_SYSTIMER_NEST_INTS_ENABLE   1

//-----------------------------------------------------------------------------
//
//    vortexRT System Timer Tick Counter Enable
//
//
#define  vortexRT_SYSTEM_TICKS_ENABLE        1


//-----------------------------------------------------------------------------
//
//    vortexRT System Timer Hook
//
//
#define  vortexRT_SYSTIMER_HOOK_ENABLE       1

//-----------------------------------------------------------------------------
//
//    vortexRT Idle Process Hook
//
//
#define  vortexRT_IDLE_HOOK_ENABLE           0

//-----------------------------------------------------------------------------
//
//    vortexRT Idle Process Stack size (in bytes)
//    (20 * sizeof(stack_item_t)) - it's a minimum allowed value.
//
#define vortexRT_IDLE_PROCESS_STACK_SIZE     (100 * sizeof(stack_item_t))

//-----------------------------------------------------------------------------
//
//    vortexRT Context Switch User Hook enable
//
//    The macro enables/disables user defined hook called from system
//    Context Switch Hook function.
//
//
#define  vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE  1

//-----------------------------------------------------------------------------
//
//    vortexRT Debug enable
//
//    The macro enables/disables acquisition of some debug info:
//    stack usage information and information about the object that process is waiting for.
//
//
#define vortexRT_DEBUG_ENABLE                1

//-----------------------------------------------------------------------------
//
//    vortexRT Process Restart enable
//
//    The macro enables/disables process restarting facility.
//
//
#define vortexRT_PROCESS_RESTART_ENABLE      0

//-----------------------------------------------------------------------------
//
//     PendSVC_ISR optimization:
//    0 - use near (+- 1Mb) call for os_context_switch_hook
//    1 - use far call
//
#define vortexRT_CONTEXT_SWITCH_HOOK_IS_FAR     0

//-----------------------------------------------------------------------------
//
//    vortexRT process with initial suspended state enable
//
//
#define vortexRT_SUSPENDED_PROCESS_ENABLE  0

#endif // vortexRT_CONFIG_H
//-----------------------------------------------------------------------------

