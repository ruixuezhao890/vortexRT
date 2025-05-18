
#ifndef PROFILER_H
#define PROFILER_H

#include <vortexRT.h>

//------------------------------------------------------------------------------
template < uint_fast8_t sum_shift_bits = 0 >
class TProfiler : public OS::TKernelAgent
{
    uint32_t time_interval();
public:
    INLINE TProfiler();

    INLINE void advance_counters()
    {
        uint32_t Elapsed = time_interval();
        Counter[ cur_proc_priority() ] += Elapsed;
    }

    INLINE uint16_t get_result(uint_fast8_t index) { return Result[index]; }
    INLINE void     process_data();

protected:
    volatile uint32_t  Counter[OS::PROCESS_COUNT];
             uint16_t  Result [OS::PROCESS_COUNT];
};
//------------------------------------------------------------------------------
template < uint_fast8_t sum_shift_bits >
TProfiler<sum_shift_bits>::TProfiler()
    : Counter (   )
    , Result  (   )
{
}

template < uint_fast8_t sum_shift_bits >
void TProfiler<sum_shift_bits>::process_data()
{
    // Use cache to make critical section fast as possible
    uint32_t CounterCache[OS::PROCESS_COUNT];
    {
        TCritSect cs;
        for(uint_fast8_t i = 0; i < OS::PROCESS_COUNT; ++i)
        {
            CounterCache[i] = Counter[i];
            Counter[i]       = 0;
        }
    }

    uint32_t Sum = 0;
    for(uint_fast8_t i = 0; i < OS::PROCESS_COUNT; ++i)
    {
        Sum += CounterCache[i];
    }
    Sum >>=  sum_shift_bits;

    const uint32_t K = 10000;
    for(uint_fast8_t i = 0; i < OS::PROCESS_COUNT; ++i)
    {
        Result[i]  = (CounterCache[i] >> sum_shift_bits) * K / Sum;
    }
}
//------------------------------------------------------------------------------

#endif  // PROFILER_H

