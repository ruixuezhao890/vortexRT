
/**
  ******************************************************************************
  * @file           : recursive_mutex.h
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-19
  ******************************************************************************
  */
#ifndef RECURSIVE_MUTEX_H
#define RECURSIVE_MUTEX_H
#include "vortexRT.h"
namespace OS
{

class TRecursiveMutex : protected TService
{
public:
    TRecursiveMutex(): ProcessMap(0), ValueTag(0), NestCount(0) { }

    void lock();
    void unlock();
    void unlock_isr();
    bool try_lock();
    bool try_lock(timeout_t timeout);
    bool is_locked() const { TCritSect cs; return (ValueTag != 0); }
    // Unlock the mutex despite of how many times it has been locked.
    void force_unlock();
    void force_unlock_isr();

protected:
    volatile TProcessMap ProcessMap;
    volatile TProcessMap ValueTag;
    volatile size_t NestCount;
}; 

//------------------------------------------------------------------------------
//
inline void TRecursiveMutex::unlock_isr()
{
    TCritSect cs;

    if ( NestCount && --NestCount == 0 )
    {
        ValueTag = 0;
        resume_next_ready_isr(ProcessMap);
    }
}

//------------------------------------------------------------------------------
//
inline void TRecursiveMutex::force_unlock_isr()
{
    TCritSect cs;

    if ( 0 == NestCount )
        return;
    NestCount = 0;
    ValueTag = 0;
    resume_next_ready_isr(ProcessMap);
}



} // ns OS

#endif /* RECURSIVE_MUTEX_H */

