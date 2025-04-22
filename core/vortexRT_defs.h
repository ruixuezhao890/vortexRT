#ifndef vortexRT_DEFS_H
#define vortexRT_DEFS_H


//    Check CONFIG Macro Definitions

#include "vortexRT_CONFIG.h"
#include "os_target.h"

//----------------- vortexRT_USER_DEFINED_CRITSECT_ENABLE -------------
#ifndef vortexRT_USER_DEFINED_CRITSECT_ENABLE
#define vortexRT_USER_DEFINED_CRITSECT_ENABLE  0
#endif

#if (vortexRT_USER_DEFINED_CRITSECT_ENABLE < 0) || (vortexRT_USER_DEFINED_CRITSECT_ENABLE > 1)
#error "Error: vortexRT_USER_DEFINED_CRITSECT_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_SYSTIMER_NEST_INTS_ENABLE --------------------------
#ifndef vortexRT_SYSTIMER_NEST_INTS_ENABLE
#error "Error: Config macro vortexRT_SYSTIMER_NEST_INTS_ENABLE must be defined!"
#endif

#if (vortexRT_SYSTIMER_NEST_INTS_ENABLE < 0) || (vortexRT_SYSTIMER_NEST_INTS_ENABLE > 1)
#error "Error: vortexRT_SYSTIMER_NEST_INTS_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_SYSTEM_TICKS_ENABLE --------------------------------
#ifndef vortexRT_SYSTEM_TICKS_ENABLE
#error "Error: Config macro vortexRT_SYSTEM_TICKS_ENABLE must be defined!"
#endif

#if (vortexRT_SYSTEM_TICKS_ENABLE < 0) || (vortexRT_SYSTEM_TICKS_ENABLE > 1)
#error "Error: vortexRT_SYSTEM_TICKS_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_SYSTEM_TICKS_ATOMIC --------------------------------
#ifndef vortexRT_SYSTEM_TICKS_ATOMIC
#define vortexRT_SYSTEM_TICKS_ATOMIC 0
#endif

#if (vortexRT_SYSTEM_TICKS_ATOMIC < 0) || (vortexRT_SYSTEM_TICKS_ATOMIC > 1)
#error "Error: vortexRT_SYSTEM_TICKS_ATOMIC must have values 0 or 1 only!"
#endif


//----------------- vortexRT_SYSTIMER_HOOK_ENABLE -------------------------------
#ifndef vortexRT_SYSTIMER_HOOK_ENABLE
#error "Error: Config macro vortexRT_SYSTIMER_HOOK_ENABLE must be defined!"
#endif

#if (vortexRT_SYSTIMER_HOOK_ENABLE < 0) || (vortexRT_SYSTIMER_HOOK_ENABLE > 1)
#error "Error: vortexRT_SYSTIMER_HOOK_ENABLE must have values 0 or 1 only!"
#endif

//-------------- vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE -----------------------
#ifndef vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE
#error "Error: Config macro vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE must be defined!"
#endif

#if (vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE < 0) || (vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE > 1)
#error "Error: vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_IDLE_HOOK_ENABLE -----------------------------------
#ifndef vortexRT_IDLE_HOOK_ENABLE
#error "Error: Config macro vortexRT_IDLE_HOOK_ENABLE must be defined!"
#endif

#if (vortexRT_IDLE_HOOK_ENABLE < 0) || (vortexRT_IDLE_HOOK_ENABLE > 1)
#error "Error: vortexRT_IDLE_HOOK_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_TARGET_IDLE_HOOK_ENABLE ----------------------------
#ifndef vortexRT_TARGET_IDLE_HOOK_ENABLE
#define vortexRT_TARGET_IDLE_HOOK_ENABLE 0
#endif

#if (vortexRT_TARGET_IDLE_HOOK_ENABLE < 0) || (vortexRT_TARGET_IDLE_HOOK_ENABLE > 1)
#error "Error: vortexRT_TARGET_IDLE_HOOK_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_CONTEXT_SWITCH_SCHEME ------------------------------
#ifndef vortexRT_CONTEXT_SWITCH_SCHEME
#error "Error: Config macro vortexRT_CONTEXT_SWITCH_SCHEME must be defined!"
#endif

#if (vortexRT_CONTEXT_SWITCH_SCHEME < 0) || (vortexRT_CONTEXT_SWITCH_SCHEME > 1)
#error "Error: vortexRT_CONTEXT_SWITCH_SCHEME must have values 0 or 1 only!"
#endif


//----------------- vortexRT_PRIORITY_ORDER -------------------------------------
#ifndef vortexRT_PRIORITY_ORDER
#error "Error: Config macro vortexRT_PRIORITY_ORDER must be defined!"
#endif

#if (vortexRT_PRIORITY_ORDER < 0) || (vortexRT_PRIORITY_ORDER > 1)
#error "Error: vortexRT_PRIORITY_ORDER must have values 0 or 1 only!"
#endif

//----------------- vortexRT_DEBUG ----------------------------------------------
#ifndef vortexRT_DEBUG_ENABLE
#define vortexRT_DEBUG_ENABLE  0
#endif

#if (vortexRT_DEBUG_ENABLE < 0) || (vortexRT_DEBUG_ENABLE > 1)
#error "Error: vortexRT_DEBUG must have values 0 or 1 only!"
#endif

//----------------- vortexRT_PROCESS_RESTART_ENABLE -----------------------------
#ifndef vortexRT_PROCESS_RESTART_ENABLE
#define vortexRT_PROCESS_RESTART_ENABLE  0
#endif

#if (vortexRT_PROCESS_RESTART_ENABLE < 0) || (vortexRT_PROCESS_RESTART_ENABLE > 1)
#error "Error: vortexRT_PROCESS_RESTART_ENABLE must have values 0 or 1 only!"
#endif

//----------------- vortexRT_SUSPENDED_PROCESS_ENABLE ---------------------------
#ifndef vortexRT_SUSPENDED_PROCESS_ENABLE
#define vortexRT_SUSPENDED_PROCESS_ENABLE  0
#endif

#if (vortexRT_SUSPENDED_PROCESS_ENABLE < 0) || (vortexRT_SUSPENDED_PROCESS_ENABLE > 1)
#error "Error: vortexRT_SUSPENDED_PROCESS_ENABLE must have values 0 or 1 only!"
#endif
     
//----------------- User Hooks inlining ----------------------------------------
#ifndef INLINE_SYS_TIMER_HOOK
#define INLINE_SYS_TIMER_HOOK
#endif

#ifndef INLINE_CONTEXT_SWITCH_HOOK
#define INLINE_CONTEXT_SWITCH_HOOK
#endif

//----------------- NORETURN Macro ---------------------------------------------
#ifndef NORETURN
#define NORETURN
#endif


//   Priority and process map type definitions
namespace OS
{
    constexpr uint_fast8_t MAX_PROCESS_COUNT = 32;

    #if vortexRT_PROCESS_COUNT < 8
        typedef uint_fast8_t TProcessMap;
    #elif vortexRT_PROCESS_COUNT < 16
        typedef uint_fast16_t TProcessMap;
    #else
        typedef uint_fast32_t TProcessMap;
    #endif
    //------------------------------------------------------
#if vortexRT_PRIORITY_ORDER == 0
    enum TPriority {
        #if vortexRT_PROCESS_COUNT   > 0
            pr0,
        #endif
        #if vortexRT_PROCESS_COUNT   > 1
            pr1,
        #endif
        #if vortexRT_PROCESS_COUNT   > 2
            pr2,
        #endif
        #if vortexRT_PROCESS_COUNT   > 3
            pr3,
        #endif
        #if vortexRT_PROCESS_COUNT   > 4
            pr4,
        #endif
        #if vortexRT_PROCESS_COUNT   > 5
            pr5,
        #endif
        #if vortexRT_PROCESS_COUNT   > 6
            pr6,
        #endif
        #if vortexRT_PROCESS_COUNT   > 7
            pr7,
        #endif
        #if vortexRT_PROCESS_COUNT   > 8
            pr8,
        #endif
        #if vortexRT_PROCESS_COUNT   > 9
            pr9,
        #endif
        #if vortexRT_PROCESS_COUNT   > 10
            pr10,
        #endif
        #if vortexRT_PROCESS_COUNT   > 11
            pr11,
        #endif
        #if vortexRT_PROCESS_COUNT   > 12
            pr12,
        #endif
        #if vortexRT_PROCESS_COUNT   > 13
            pr13,
        #endif
        #if vortexRT_PROCESS_COUNT   > 14
            pr14,
        #endif
        #if vortexRT_PROCESS_COUNT   > 15
            pr15,
        #endif
        #if vortexRT_PROCESS_COUNT   > 16
            pr16,
        #endif
        #if vortexRT_PROCESS_COUNT   > 17
            pr17,
        #endif
        #if vortexRT_PROCESS_COUNT   > 18
            pr18,
        #endif
        #if vortexRT_PROCESS_COUNT   > 19
            pr19,
        #endif
        #if vortexRT_PROCESS_COUNT   > 20
            pr20,
        #endif
        #if vortexRT_PROCESS_COUNT   > 21
            pr21,
        #endif
        #if vortexRT_PROCESS_COUNT   > 22
            pr22,
        #endif
        #if vortexRT_PROCESS_COUNT   > 23
            pr23,
        #endif
        #if vortexRT_PROCESS_COUNT   > 24
            pr24,
        #endif
        #if vortexRT_PROCESS_COUNT   > 25
            pr25,
        #endif
        #if vortexRT_PROCESS_COUNT   > 26
            pr26,
        #endif
        #if vortexRT_PROCESS_COUNT   > 27
            pr27,
        #endif
        #if vortexRT_PROCESS_COUNT   > 28
            pr28,
        #endif
        #if vortexRT_PROCESS_COUNT   > 29
            pr29,
        #endif
        #if vortexRT_PROCESS_COUNT   > 30
            pr30,
        #endif
        #if (vortexRT_PROCESS_COUNT   > 31) || (vortexRT_PROCESS_COUNT   < 1)
            #error "Invalid Process Count specification! Must be from 1 to 31."
        #endif
            prIDLE
    };
#else   // vortexRT_PRIORITY_ORDER == 1
    enum TPriority {
            prIDLE,
        #if vortexRT_PROCESS_COUNT   > 30
            pr30,
        #endif
        #if vortexRT_PROCESS_COUNT   > 29
            pr29,
        #endif
        #if vortexRT_PROCESS_COUNT   > 28
            pr28,
        #endif
        #if vortexRT_PROCESS_COUNT   > 27
            pr27,
        #endif
        #if vortexRT_PROCESS_COUNT   > 26
            pr26,
        #endif
        #if vortexRT_PROCESS_COUNT   > 25
            pr25,
        #endif
        #if vortexRT_PROCESS_COUNT   > 24
            pr24,
        #endif
        #if vortexRT_PROCESS_COUNT   > 23
            pr23,
        #endif
        #if vortexRT_PROCESS_COUNT   > 22
            pr22,
        #endif
        #if vortexRT_PROCESS_COUNT   > 21
            pr21,
        #endif
        #if vortexRT_PROCESS_COUNT   > 20
            pr20,
        #endif
        #if vortexRT_PROCESS_COUNT   > 19
            pr19,
        #endif
        #if vortexRT_PROCESS_COUNT   > 18
            pr18,
        #endif
        #if vortexRT_PROCESS_COUNT   > 17
            pr17,
        #endif
        #if vortexRT_PROCESS_COUNT   > 16
            pr16,
        #endif
        #if vortexRT_PROCESS_COUNT   > 15
            pr15,
        #endif
        #if vortexRT_PROCESS_COUNT   > 14
            pr14,
        #endif
        #if vortexRT_PROCESS_COUNT   > 13
            pr13,
        #endif
        #if vortexRT_PROCESS_COUNT   > 12
            pr12,
        #endif
        #if vortexRT_PROCESS_COUNT   > 11
            pr11,
        #endif
        #if vortexRT_PROCESS_COUNT   > 10
            pr10,
        #endif
        #if vortexRT_PROCESS_COUNT   > 9
            pr9,
        #endif
        #if vortexRT_PROCESS_COUNT   > 8
            pr8,
        #endif
        #if vortexRT_PROCESS_COUNT   > 7
            pr7,
        #endif
        #if vortexRT_PROCESS_COUNT   > 6
            pr6,
        #endif
        #if vortexRT_PROCESS_COUNT   > 5
            pr5,
        #endif
        #if vortexRT_PROCESS_COUNT   > 4
            pr4,
        #endif
        #if vortexRT_PROCESS_COUNT   > 3
            pr3,
        #endif
        #if vortexRT_PROCESS_COUNT   > 2
            pr2,
        #endif
        #if vortexRT_PROCESS_COUNT   > 1
            pr1,
        #endif
        #if vortexRT_PROCESS_COUNT   > 0
            pr0
        #endif
        #if (vortexRT_PROCESS_COUNT   > 31) || (vortexRT_PROCESS_COUNT   < 1)
            #error "Invalid Process Count specification! Must be from 1 to 31."
        #endif
    };
#endif //vortexRT_PRIORITY_ORDER
}

//     Process's constructor inlining control: default behaviour

#ifndef INLINE_PROCESS_CTOR
#define INLINE_PROCESS_CTOR
#endif


//   Initial process state.
namespace OS
{
    enum TProcessStartState
    {
        pssRunning,
        pssSuspended
    };
}
//-----------------------------------------------------------------------------

#endif // vortexRT_DEFS_H
