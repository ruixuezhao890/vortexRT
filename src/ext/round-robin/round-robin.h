#ifndef  ROUND_ROBIN_H
#define  ROUND_ROBIN_H

#include <vortexRT.h>

namespace os = OS;

//------------------------------------------------------------------------------
//
//      
// 
//
template <uint_fast8_t proc_count>
class round_robin_mgr : public OS::TKernelAgent
{
public:
    round_robin_mgr() : cur_idx(0), reg_idx(0) 
    { 
        for(int i = 0; i < proc_count; ++i)
        {
            put_off[i]  = true;
        }
    }

    void register_process(const OS::TBaseProcess &proc, timeout_t tslot = 1)
    {
        table[reg_idx].p = &proc;
        table[reg_idx].t = tslot;
        timers[reg_idx]  = tslot;
        if(reg_idx++)
        {
            set_process_unready( proc.priority() );
        }
    }

    INLINE void run();

private:
    struct TItem
    {
        const OS::TBaseProcess *p;
        timeout_t               t;
    };

    INLINE void next();

private:                
   ::uint_fast8_t cur_idx;
   ::uint_fast8_t reg_idx;
    TItem        table[proc_count];
    timeout_t    timers[proc_count];
    bool         put_off[proc_count];      // set the flag when process is suspended by round-robin manager

};
//------------------------------------------------------------------------------
template <uint_fast8_t proc_count>
void round_robin_mgr<proc_count>::next()
{
    for(int i = 0; i < reg_idx; ++i)
    {
        if( ++cur_idx == reg_idx )
        {
            cur_idx = 0;
        }

        if( put_off[cur_idx] )
        {
            put_off[cur_idx]  = false;
            timers[cur_idx]   = table[cur_idx].t;
            set_process_ready( table[cur_idx].p->priority() );
            return;
        }
    }
}
//------------------------------------------------------------------------------
template <uint_fast8_t proc_count>
void round_robin_mgr<proc_count>::run()
{

    if( table[cur_idx].p->is_suspended() )
    {
        next();
        return;
    }


    for(int i = 0; i < reg_idx; ++i)
    {
        if( cur_idx != i && !table[i].p->is_suspended() )
        {
            put_off[i]  = true;
            set_process_unready( table[i].p->priority() );
        }
    }


    if( --timers[cur_idx] == 0  )
    {
        put_off[cur_idx] = true;
        set_process_unready( table[cur_idx].p->priority() );
        next();
    }
}
#endif // ROUND_ROBIN_H
