
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

     class TKernelAgent;
     class TService;


    // TBaseProcess类 - 进程基类
    // 功能：实现所有应用进程的公共基础功能
    class TBaseProcess
    {
        // 友元声明，允许内核相关类和函数访问私有成员
        friend class TKernel;    // 内核类
        friend class TISRW;     // 中断服务例程包装器
        friend class TISRW_SS;  // 单次服务中断包装器
        friend class TKernelAgent; // 内核代理
        friend void run();       // 系统运行函数

    public:
    #if SEPARATE_RETURN_STACK == 0
        // 构造函数(不使用独立返回栈的版本)
        // 参数：
        // StackPoolEnd - 栈池结束地址(栈顶)
        // pr - 进程优先级
        // exec - 进程执行函数指针
        // (调试模式下可选参数)
        // aStackPool - 栈池起始地址(用于调试)
        // name - 进程名称(用于调试)
        TBaseProcess(stack_item_t* StackPoolEnd,
                TPriority pr,
                void (*exec)(),
                #if vortexRT_DEBUG_ENABLE == 1
                stack_item_t* aStackPool,
                const char* name = nullptr
                #endif
                );

    protected:
        // 设置进程为非就绪状态(内部使用)
        INLINE void set_unready() { Kernel.set_process_unready(this->Priority); }

        // 初始化栈帧(内部使用)
        // 参数：
        // StackPoolEnd - 栈池结束地址
        // exec - 进程执行函数指针
        // (调试模式下可选参数)
        // StackPool - 栈池起始地址
        void init_stack_frame(stack_item_t* StackPoolEnd,
                         void (*exec)(),
                         #if vortexRT_DEBUG_ENABLE == 1
                         stack_item_t* StackPool
                         #endif
                         );

    #else  // SEPARATE_RETURN_STACK == 1
        // 构造函数(使用独立返回栈的版本)
        // 额外参数：
        // RStack - 返回栈指针
        // (调试模式下可选参数)
        // aRStackPool - 返回栈池起始地址
        TBaseProcess(stack_item_t* StackPoolEnd,
                stack_item_t* RStack,
                TPriority pr,
                void (*exec)(),
                #if vortexRT_DEBUG_ENABLE == 1
                stack_item_t* aStackPool,
                stack_item_t* aRStackPool,
                const char* name = 0
                #endif
                );

    protected:
        // 初始化栈帧(独立返回栈版本)
        void init_stack_frame(stack_item_t* Stack,
                         stack_item_t* RStack,
                         void (*exec)(),
                         #if vortexRT_DEBUG_ENABLE == 1
                         stack_item_t* StackPool,
                         stack_item_t* RStackPool
                         #endif
                         );
    #endif // SEPARATE_RETURN_STACK

    public:
        // 获取进程优先级
        TPriority priority() const { return Priority; }

        // 进程控制函数
        static void sleep(timeout_t timeout = 0); // 进程休眠
        void wake_up();          // 唤醒进程
        void force_wake_up();    // 强制唤醒进程
        INLINE void start() { force_wake_up(); } // 启动进程(调用force_wake_up)

        // 进程状态查询
        INLINE bool is_sleeping() const;    // 检查进程是否在休眠
        INLINE bool is_suspended() const;   // 检查进程是否被挂起

    #if vortexRT_DEBUG_ENABLE == 1
        // 调试相关功能
        INLINE TService* waiting_for() const { return WaitingFor; } // 获取进程等待的服务
    public:
        size_t stack_size() const { return StackSize; }  // 获取栈大小
        size_t stack_slack() const;  // 获取栈剩余空间
        const char* name() const { return Name; } // 获取进程名称
        #if SEPARATE_RETURN_STACK == 1
        size_t rstack_size() const { return RStackSize; } // 获取返回栈大小
        size_t rstack_slack() const; // 获取返回栈剩余空间
        #endif
    #endif // vortexRT_DEBUG_ENABLE

    #if vortexRT_PROCESS_RESTART_ENABLE == 1
    protected:
        void reset_controls(); // 重置进程控制状态(重启功能)
    #endif
    protected:
        // 数据成员
        stack_item_t* StackPointer;    // 当前栈指针
        volatile timeout_t Timeout;    // 超时计数器(volatile用于多线程/中断环境)
        const TPriority Priority;      // 进程优先级(常量)

    #if vortexRT_DEBUG_ENABLE == 1
        // 调试相关数据成员
        TService* volatile WaitingFor; // 当前等待的服务(可能被中断修改)
        const stack_item_t* const StackPool; // 栈池起始地址(常量指针)
        const size_t StackSize;        // 栈大小(以stack_item_t为单位)
        const char* Name;              // 进程名称
        #if SEPARATE_RETURN_STACK == 1
        const stack_item_t* const RStackPool; // 返回栈池起始地址
        const size_t RStackSize;       // 返回栈大小
        #endif
    #endif // vortexRT_DEBUG_ENABLE

    #if vortexRT_PROCESS_RESTART_ENABLE == 1
        volatile TProcessMap* WaitingProcessMap; // 等待进程映射表(重启功能)
    #endif

    #if vortexRT_SUSPENDED_PROCESS_ENABLE != 0
        static TProcessMap SuspendedProcessMap; // 挂起进程映射表(静态成员)
    #endif

    };
}
#endif // OS_KERNEL_H
