
/**
  ******************************************************************************
  * @file           : os_services.cpp
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-19
  ******************************************************************************
*/

#include "vortexRT.h"

using namespace OS;


bool OS::TService::resume_all(TProcessMap volatile & waiters_map)
{
    TProcessMap Timeouted = ready_process_map();                       // Process has its tag set in ReadyProcessMap if timeout expired,
                                                                       // or it was waked up by OS::ForceWakeUpProcess()
                                                                       
    TProcessMap CachedMap = waiters_map;                               // cache volatile
    if( CachedMap & ~static_cast<unsigned>(Timeouted) )                // if any process has to be waked up
    {                                                                  
        set_prio_tag( ready_process_map(), CachedMap );                // place all waiting processes to the ready map
        clr_prio_tag( CachedMap, ~static_cast<unsigned>(Timeouted) );  // remove all non-timeouted processes from the waiting map.
        waiters_map = CachedMap;
        reschedule();
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------
bool OS::TService::resume_next_ready (TProcessMap volatile & waiters_map)
{
    TProcessMap Active = ready_process_map();                          // Cache volatile
    TProcessMap Waiters = waiters_map;                                 // Cache volatile
                                                                       
    TProcessMap Timeouted = Active;                                    // Process has its tag set in ReadyProcessMap if timeout expired,
                                                                       // or it was waked up by OS::ForceWakeUpProcess()

    TProcessMap Ready = Waiters & ~static_cast<unsigned>(Timeouted);
    if( Ready )                                                        // if any process has to be waked up
    {                                                                  
        TProcessMap PrioTag = highest_prio_tag(Ready);                 // get next ready process tag
        set_prio_tag(Active, PrioTag);                                 // place next ready process to the ready map
        clr_prio_tag(Waiters, PrioTag);                                // remove process from the waiting map.

        ready_process_map() = Active;
        waiters_map = Waiters;

        reschedule();
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
//
//      TEventFlag
//
//
bool OS::TEventFlag::wait(timeout_t timeout)
{
    TCritSect cs;

    if(Value)                                           // if flag already signaled
    {
        Value = efOff;                                  // clear flag
        return true;
    }
    else
    {
        cur_proc_timeout() = timeout;

        suspend(ProcessMap);

        if(is_timeouted(ProcessMap))
            return false;                               // waked up by timeout or by externals

        cur_proc_timeout() = 0;
        return true;                                    // otherwise waked up by signal() or signal_isr()
    }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
//
//      TMutex
//
//
void OS::TMutex::lock()
{
    TCritSect cs;

    while(ValueTag)
    {
        // mutex already locked by another process, suspend current process
        suspend(ProcessMap);
    }
    ValueTag = cur_proc_prio_tag();                     // mutex has been successfully locked
}
//------------------------------------------------------------------------------
bool OS::TMutex::try_lock(timeout_t timeout)
{
    TCritSect cs;

    while (ValueTag)
    {
        // mutex already locked by another process, suspend current process
        cur_proc_timeout() = timeout;
        suspend(ProcessMap);
        if(is_timeouted(ProcessMap))
            return false;             // waked up by timeout or by externals
        cur_proc_timeout() = 0;
    }
    ValueTag = cur_proc_prio_tag();   // mutex has been successfully locked
    return true;
}
//------------------------------------------------------------------------------
void OS::TMutex::unlock()
{
    TCritSect cs;

    if(ValueTag != cur_proc_prio_tag())
        return;                                         // the only process that had locked mutex can unlock the mutex
    ValueTag = 0;

    resume_next_ready(ProcessMap);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
//
//      TChannel
//
//
void OS::TChannel::push(uint8_t x)
{
    TCritSect cs;

    while (!Cbuf.get_free_size())
    {
        // channel is full, suspend current process
        suspend(ProducersProcessMap);
    }

    Cbuf.put(x);
    resume_all(ConsumersProcessMap);
}
//------------------------------------------------------------------------------
uint8_t OS::TChannel::pop()
{
    TCritSect cs;
    uint8_t x;

    while(!Cbuf.get_count())
    {
        // channel is empty, suspend current process
        suspend(ConsumersProcessMap);
    }
    x = Cbuf.get();
    resume_all(ProducersProcessMap);
    return x;
}
//------------------------------------------------------------------------------
void OS::TChannel::write(const uint8_t* data, const uint8_t count)
{
    TCritSect cs;

    while(Cbuf.get_free_size() < count)
    {
        // channel has not enough space, suspend current process
        suspend(ProducersProcessMap);
    }

    Cbuf.write(data, count);
    resume_all(ConsumersProcessMap);
}
//------------------------------------------------------------------------------
void OS::TChannel::read(uint8_t* const data, const uint8_t count)
{
    TCritSect cs;

    while(Cbuf.get_count() < count)
    {
        // channel doesn't contain enough data, suspend current process
        suspend(ConsumersProcessMap);
    }

    Cbuf.read(data, count);
    resume_all(ProducersProcessMap);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//              OS::message template
//
//          Function-members implementation
//
//
//------------------------------------------------------------------------------
bool OS::TBaseMessage::wait(timeout_t timeout)
{
    TCritSect cs;

    if(NonEmpty)                                                  // message already send
    {
        NonEmpty = false;
        return true;
    }
    else
    {
        cur_proc_timeout() = timeout;

        suspend(ProcessMap);

        if(is_timeouted(ProcessMap))
            return false;

        cur_proc_timeout() = 0;
        return true;
    }
}
//------------------------------------------------------------------------------

