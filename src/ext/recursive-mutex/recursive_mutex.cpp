#include "recursive_mutex.h"

namespace OS
{


void TRecursiveMutex::lock()
{
    TCritSect cs;
    TProcessMap curr_tag = cur_proc_prio_tag();

    if ( 0 == ValueTag )
    {
        ValueTag = curr_tag;
        NestCount = 1;
    }
    else if ( ValueTag == curr_tag )
    {
        ++NestCount;
    }
    else
    {
        while ( ValueTag )
            suspend(ProcessMap);
        ValueTag = cur_proc_prio_tag();
        NestCount = 1;
    }
}



void TRecursiveMutex::unlock()
{
    TCritSect cs;

    if ( ValueTag != cur_proc_prio_tag() || 0 == NestCount )
        return;
    if ( --NestCount == 0 )
    {
        ValueTag = 0;
        resume_next_ready(ProcessMap);
    }
}



void TRecursiveMutex::force_unlock()
{
    TCritSect cs;

    if ( ValueTag != cur_proc_prio_tag() || 0 == NestCount )
        return;
    NestCount = 0;
    ValueTag = 0;
    resume_next_ready(ProcessMap);
}



bool TRecursiveMutex::try_lock()
{
    TCritSect cs;

    if ( ValueTag && ValueTag != cur_proc_prio_tag() ) 
        return false;
    else
        lock();
    return true; 
}


bool TRecursiveMutex::try_lock(timeout_t timeout)
{
    TCritSect cs;

    if ( ValueTag && ValueTag != cur_proc_prio_tag() ) 
    {
        // mutex already locked by another process, suspend current process
        while ( ValueTag ) 
        {
            cur_proc_timeout() = timeout;
            suspend(ProcessMap);
            if ( is_timeouted(ProcessMap) )
                return false;             // waked up by timeout or by externals
            cur_proc_timeout() = 0;
        }
    }
    lock();
    return true;
}

} // ns OS

