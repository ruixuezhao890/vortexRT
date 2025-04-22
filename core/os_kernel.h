
#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <stddef.h>
#include <stdint.h>
// #include <usrlib.h> 



//==============================================================================
// 声明操作系统启动函数，使用C语言链接规范
// NORETURN 表示该函数不会返回（通常进入无限循环）
// sp: 初始进程的栈指针
extern "C" NORETURN void os_start(stack_item_t* sp);

// 根据上下文切换方案选择不同的上下文切换函数声明
#if vortexRT_CONTEXT_SWITCH_SCHEME == 0
    // 方案0：直接上下文切换
    // 声明上下文切换函数，使用C语言链接规范
    // Curr_SP: 指向当前进程栈指针的指针（用于保存当前上下文）
    // Next_SP: 下一个要切换进程的栈指针（用于恢复新上下文）
    extern "C" void os_context_switcher(stack_item_t** Curr_SP, stack_item_t* Next_SP);
#else
    // 方案1：通过钩子函数进行上下文切换
    // 声明上下文切换钩子函数，使用C语言链接规范
    // sp: 当前栈指针
    // 返回: 下一个要切换进程的栈指针
    extern "C" stack_item_t* os_context_switch_hook(stack_item_t* sp);
#endif

//
//
//     NAME       :   OS
//
//     PURPOSE    :   Namespace for all OS stuff
//
//     DESCRIPTION:   Includes:  Kernel,
//                               Processes,
//                               Mutexes,
//                               Event Flags,
//                               Arbitrary-type Channels,
//                               Messages
//
namespace OS
{
    
    // 定义系统中进程的总数，包括空闲进程(idle process)
    // vortexRT_PROCESS_COUNT是用户配置的进程数，+1是包含系统空闲进程
    const uint_fast8_t PROCESS_COUNT = vortexRT_PROCESS_COUNT + 1;
    
    // 定义栈默认填充模式，用于检测栈溢出
    // vortexRT_STACK_PATTERN是用户定义的栈填充值
    const stack_item_t STACK_DEFAULT_PATTERN = vortexRT_STACK_PATTERN;
    
    // 前向声明TBaseProcess类，因为后续函数声明中需要使用
    class TBaseProcess;
    
    // 设置优先级标记(volatile版本)
    // 用于在中断服务程序(ISR)中修改进程就绪映射表
    // pm: 进程优先级映射表引用
    // PrioTag: 要设置的优先级标记
    INLINE void set_prio_tag(volatile TProcessMap & pm, const TProcessMap PrioTag) { pm |= PrioTag; }
    
    // 清除优先级标记(volatile版本)
    // 用于在中断服务程序(ISR)中修改进程就绪映射表
    // pm: 进程优先级映射表引用
    // PrioTag: 要清除的优先级标记
    INLINE void clr_prio_tag(volatile TProcessMap & pm, const TProcessMap PrioTag) { pm &= ~static_cast<unsigned>(PrioTag); }
    
    // 设置优先级标记(非volatile版本)
    // 用于在普通代码中修改进程就绪映射表
    // pm: 进程优先级映射表引用
    // PrioTag: 要设置的优先级标记
    INLINE void set_prio_tag(TProcessMap & pm, const TProcessMap PrioTag) { pm |= PrioTag; }
    
    // 清除优先级标记(非volatile版本)
    // 用于在普通代码中修改进程就绪映射表
    // pm: 进程优先级映射表引用
    // PrioTag: 要清除的优先级标记
    INLINE void clr_prio_tag(TProcessMap & pm, const TProcessMap PrioTag) { pm &= ~static_cast<unsigned>(PrioTag); }
    
    //--------------------------------------------------------------------------
    //
    //     NAME       :   TKernel
    //
    // TKernel类 - 操作系统内核核心类
    // 功能：实现内核级操作，包括进程管理、调度、系统计时等
    class TKernel
    {
        //-----------------------------------------------------------
        // 友元声明部分
        // 允许特定类和函数访问TKernel的私有成员
        friend class TISRW;        // 中断服务例程包装器
        friend class TISRW_SS;     // 单次服务中断包装器
        friend class TBaseProcess; // 基础进程类
        friend class TKernelAgent; // 内核代理
        
        // 友元函数
        friend void run();                      // 系统运行函数
        friend bool os_running();               // 系统运行状态检查
        friend const TBaseProcess* get_proc(uint_fast8_t Prio); // 获取进程
        
        #if vortexRT_SYSTEM_TICKS_ENABLE == 1
        friend inline tick_count_t get_tick_count(); // 获取系统滴答计数
        #endif
    
        //-----------------------------------------------------------
        // 数据成员
    private:
        uint_fast8_t CurProcPriority;          // 当前运行进程的优先级
        volatile TProcessMap ReadyProcessMap;  // 就绪进程位图(volatile用于多线程/中断环境)
        volatile uint_fast8_t ISR_NestCount;   // 中断嵌套计数器
        
    private:
        static TBaseProcess* ProcessTable[PROCESS_COUNT]; // 进程表，按优先级索引
        
        #if vortexRT_CONTEXT_SWITCH_SCHEME == 1
        volatile uint_fast8_t SchedProcPriority; // 调度进程优先级(方案1专用)
        #endif
    
        #if vortexRT_SYSTEM_TICKS_ENABLE == 1
        volatile tick_count_t SysTickCount;     // 系统滴答计数器
        #endif
    
        //-----------------------------------------------------------
        // 成员函数
    public:
        // 构造函数
        // 初始化内核状态：
        // - CurProcPriority设为MAX_PROCESS_COUNT表示OS尚未运行
        // - ReadyProcessMap初始化为所有进程就绪状态
        // - ISR_NestCount初始为0(无中断嵌套)
        INLINE TKernel() 
            : CurProcPriority(MAX_PROCESS_COUNT)  
            , ReadyProcessMap((1ul << PROCESS_COUNT) - 1) 
            , ISR_NestCount(0) 
        {}
        
    private:
        // 注册进程到进程表
        INLINE static void register_process(TBaseProcess* const p);
    
        // 调度器核心实现
        void sched();
        // 调度器入口，检查中断嵌套情况
        INLINE void scheduler() { if(ISR_NestCount) return; else sched(); }
        // 中断服务例程专用的调度器
        INLINE void sched_isr();
    
        #if vortexRT_CONTEXT_SWITCH_SCHEME == 1
        // 上下文切换完成检查(方案1专用)
        INLINE bool is_context_switch_done();
        // 触发上下文切换(方案1专用)
        INLINE void raise_context_switch() { OS::raise_context_switch(); }
        #endif
        
        // 设置进程为就绪状态
        INLINE void set_process_ready(const uint_fast8_t pr) { 
            TProcessMap PrioTag = get_prio_tag(pr); 
            set_prio_tag(ReadyProcessMap, PrioTag); 
        }
        
        // 设置进程为非就绪状态
        INLINE void set_process_unready(const uint_fast8_t pr) { 
            TProcessMap PrioTag = get_prio_tag(pr); 
            clr_prio_tag(ReadyProcessMap, PrioTag); 
        }
    
    public:
        // 系统定时器处理函数
        INLINE void system_timer();
        
        #if vortexRT_CONTEXT_SWITCH_SCHEME == 1
        // 上下文切换钩子函数(方案1专用)
        INLINE stack_item_t* context_switch_hook(stack_item_t* sp);
        #endif
    };  // TKernel类定义结束
    
    // 全局内核实例声明
    extern TKernel Kernel;

}
#endif // OS_KERNEL_H
