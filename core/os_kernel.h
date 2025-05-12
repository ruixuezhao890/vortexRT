

#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <cstddef>
#include <cstdint>
//todo #include <usrlib.h>

//------------------------------------------------------------------------------

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
    

    
//==============================================================================

//------------------------------------------------------------------------------
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
    constexpr uint_fast8_t PROCESS_COUNT = vortexRT_PROCESS_COUNT + 1;
    
    // 定义栈默认填充模式，用于检测栈溢出
    // vortexRT_STACK_PATTERN是用户定义的栈填充值
    constexpr stack_item_t STACK_DEFAULT_PATTERN = vortexRT_STACK_PATTERN;
    
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
        volatile uint_fast8_t SchedProcPriority{}; // 调度进程优先级(方案1专用)
        #endif
    
        #if vortexRT_SYSTEM_TICKS_ENABLE == 1
        volatile tick_count_t SysTickCount{};     // 系统滴答计数器
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
            , ReadyProcessMap((1ull << PROCESS_COUNT) - 1)
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

    //--------------------------------------------------------------------------
    //
    //  BaseProcess
    // 
    //  Implements base class-type for application processes
    //
    //      DESCRIPTION:
    //
    //
    class TKernelAgent;
    class TService;

    //--------------------------------------------------------------------------
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
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //
    //   process
    // 
    //   Implements template for application processes instantiation
    //
    //      DESCRIPTION:
    //
    //
    #if SEPARATE_RETURN_STACK == 0
    // 进程模板类定义(单栈版本)
    // 模板参数：
    // pr       - 进程优先级(TPriority枚举类型)
    // stk_size - 栈空间大小(字节数)
    // pss      - 进程启动状态(默认pssRunning运行状态)
        template<TPriority pr, size_t stk_size, TProcessStartState pss = pssRunning>
        class process : public TBaseProcess
        {
        public:

            // 构造函数声明(使用INLINE_PROCESS_CTOR宏定义内联)
            // 参数：
            // name_str - 进程名称字符串指针(调试模式下使用)
            INLINE_PROCESS_CTOR process(const char* name_str = nullptr);

            // 进程执行函数声明(必须由派生类实现)
            // 使用OS_PROCESS宏定义特殊属性
            OS_PROCESS static void exec();

        #if vortexRT_PROCESS_RESTART_ENABLE == 1
            // 进程终止函数(当启用进程重启功能时有效)
            // 用于安全终止并重置进程状态
            INLINE void terminate();
        #endif
        
        private:
            // 进程栈空间数组
            // 大小根据模板参数stk_size计算(stack_item_t单位)
            stack_item_t Stack[stk_size/sizeof(stack_item_t)];
        };

        // 进程模板类构造函数实现
        template<TPriority pr, size_t stk_size, TProcessStartState pss>
        OS::process<pr, stk_size, pss>::process(const char*
            #if vortexRT_DEBUG_ENABLE == 1// 调试模式下使用进程名称
            name_str
            #endif
            ) : TBaseProcess(&Stack[stk_size / sizeof(stack_item_t)]// 栈顶地址
                             , pr// 进程优先级
                             , reinterpret_cast<void (*)()>(exec)// 执行函数指针转换
                          #if vortexRT_DEBUG_ENABLE == 1// 调试模式下使用进程名称
                             , Stack// 栈起始地址(调试)
                             , name_str// 进程名称(调试)
                          #endif
                             )
            
        {
            #if vortexRT_SUSPENDED_PROCESS_ENABLE != 0
            // 如果进程启动状态为挂起(pssSuspended)
            if ( pss == pssSuspended )
                // 从全局挂起进程映射表中清除当前优先级标记
                clr_prio_tag(SuspendedProcessMap, get_prio_tag(pr));
            #endif
        }

        #if vortexRT_PROCESS_RESTART_ENABLE == 1
        // 进程终止函数实现
        template<TPriority pr, size_t stk_size, TProcessStartState pss>
        void OS::process<pr, stk_size, pss>::terminate()
        {
            TCritSect cs;// 临界区保护(防止中断干扰)
            // 重置进程控制状态(清除等待状态等)
            reset_controls();
            // 重新初始化栈帧(保持原有执行函数)
            init_stack_frame( &Stack[stk_size/sizeof(stack_item_t)]
                             , reinterpret_cast<void (*)()>(exec)
                          #if vortexRT_DEBUG_ENABLE == 1
                              , Stack
                          #endif
                            );
        }
        #endif // vortexRT_RESTART_ENABLE
    // 定义系统空闲进程类型别名
    // 使用最低优先级(prIDLE)和预定义的栈大小
        typedef OS::process<OS::prIDLE, vortexRT_IDLE_PROCESS_STACK_SIZE> TIdleProc;

    #else  // SEPARATE_RETURN_STACK

        // 进程模板类定义(双栈版本)
        // 模板参数：
        // pr       - 进程优先级(TPriority枚举类型)
        // stk_size - 数据栈空间大小(字节数)
        // rstk_size - 返回栈空间大小(字节数)
        // pss      - 进程启动状态(默认pssRunning运行状态)
        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss = pssRunning>
        class process : public TBaseProcess {
        public:
            // 构造函数声明(使用INLINE_PROCESS_CTOR宏定义内联)
            // 参数：
            // name_str - 进程名称字符串指针(调试模式下使用)
            INLINE_PROCESS_CTOR process(const char* name_str = 0);
        
            // 进程执行函数声明(必须由派生类实现)
            // 使用OS_PROCESS宏定义特殊属性
            OS_PROCESS static void exec();
        
        #if vortexRT_PROCESS_RESTART_ENABLE == 1
            // 进程终止函数(当启用进程重启功能时有效)
            // 用于安全终止并重置进程状态
            INLINE void terminate();
        #endif
        
        private:
            // 进程数据栈空间数组
            stack_item_t Stack[stk_size/sizeof(stack_item_t)];
            // 进程返回栈空间数组
            stack_item_t RStack[rstk_size/sizeof(stack_item_t)];
        };
        
        // 进程模板类构造函数实现
        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss>
        process<pr, stk_size, rstk_size, pss>::process(const char*
            #if vortexRT_DEBUG_ENABLE == 1  // 调试模式下使用进程名称
            name_str
            #endif
            ): TBaseProcess(&Stack[stk_size / sizeof(stack_item_t)],  // 数据栈顶地址
                           &RStack[rstk_size/sizeof(stack_item_t)],  // 返回栈顶地址
                           pr,                                       // 进程优先级
                           reinterpret_cast<void (*)()>(exec)         // 执行函数指针转换
                        #if vortexRT_DEBUG_ENABLE == 1               // 调试模式参数
                           , Stack                                   // 数据栈起始地址
                           , RStack                                  // 返回栈起始地址
                           , name_str                                // 进程名称
                        #endif
                           )
        {
            #if vortexRT_SUSPENDED_PROCESS_ENABLE != 0
            // 如果进程启动状态为挂起(pssSuspended)
            if (pss == pssSuspended) {
                // 从全局挂起进程映射表中清除当前优先级标记
                clr_prio_tag(SuspendedProcessMap, get_prio_tag(pr));
            }
            #endif
        }
        
        #if vortexRT_PROCESS_RESTART_ENABLE == 1
        // 进程终止函数实现
        template<TPriority pr, size_t stk_size, size_t rstk_size, TProcessStartState pss>
        void OS::process<pr, stk_size, rstk_size, pss>::terminate()
        {
            TCritSect cs;  // 临界区保护(防止中断干扰)
            
            // 重置进程控制状态(清除等待状态等)
            reset_controls();
            // 重新初始化双栈帧(保持原有执行函数)
            init_stack_frame(&Stack[stk_size/sizeof(stack_item_t)],
                            &RStack[rstk_size/sizeof(stack_item_t)],
                            reinterpret_cast<void (*)()>(exec)
                        #if vortexRT_DEBUG_ENABLE == 1
                            , Stack    // 调试模式下传递数据栈起始地址
                            , RStack   // 调试模式下传递返回栈起始地址
                        #endif
                            );
        }
        #endif
        
        // 定义系统空闲进程类型别名(双栈版本)
        // 使用最低优先级(prIDLE)和预定义的双栈大小
        typedef OS::process<OS::prIDLE, 
                           vortexRT_IDLE_PROCESS_DATA_STACK_SIZE,
                           vortexRT_IDLE_PROCESS_RETURN_STACK_SIZE> TIdleProc;

    #endif    // SEPARATE_RETURN_STACK
    //--------------------------------------------------------------------------


    extern TIdleProc IdleProc;
        
    //--------------------------------------------------------------------------
    //
    //  TKernel代理
    //
    //授予对某些 RTOS 内核内部结构的访问权限，以便实现服务
    //
    //描述：
    //
    //
    class TKernelAgent
    {
        // 获取当前运行进程对象指针
        // 通过查询内核的进程表(ProcessTable)获取
        INLINE static TBaseProcess * cur_proc()                        { return Kernel.ProcessTable[cur_proc_priority()]; }

    protected:
        // 构造函数设为protected，限制只有派生类可以实例化
        TKernelAgent() { }
        // 获取当前运行进程的优先级(只读引用)
        INLINE static uint_fast8_t const   & cur_proc_priority()       { return Kernel.CurProcPriority;  }
        // 获取就绪进程位图(volatile引用，可能被中断修改)
        INLINE static volatile TProcessMap & ready_process_map()       { return Kernel.ReadyProcessMap;  }
        // 获取当前进程的超时计数器(volatile引用)
        INLINE static volatile timeout_t   & cur_proc_timeout()        { return cur_proc()->Timeout;     }
        // 触发内核重新调度
        INLINE static void reschedule()                                { Kernel.scheduler();             }

        // 设置指定优先级进程为就绪状态
        INLINE static void set_process_ready   (const uint_fast8_t pr) { Kernel.set_process_ready(pr);   }
        // 设置指定优先级进程为非就绪状态
        INLINE static void set_process_unready (const uint_fast8_t pr) { Kernel.set_process_unready(pr); }

    #if vortexRT_DEBUG_ENABLE == 1
        // 调试模式下获取当前进程等待的服务对象
        INLINE static TService * volatile & cur_proc_waiting_for()     { return cur_proc()->WaitingFor;  }
    #endif
    
    #if vortexRT_PROCESS_RESTART_ENABLE == 1
        // 进程重启功能启用时，获取当前进程的等待映射表
        INLINE static volatile TProcessMap * & cur_proc_waiting_map()  { return cur_proc()->WaitingProcessMap; }
    #endif
    };
    

    //杂项
    // 系统运行函数（永不返回）
    INLINE NORETURN void run();

    // 检查操作系统是否正在运行
    INLINE bool os_running();

    // 锁定系统定时器（临界区保护）
    INLINE void lock_system_timer()    { TCritSect cs; LOCK_SYSTEM_TIMER();   }

    // 解锁系统定时器（临界区保护）
    INLINE void unlock_system_timer()  { TCritSect cs; UNLOCK_SYSTEM_TIMER(); }

    // 进程休眠函数（默认参数0表示无限期休眠）
    INLINE void sleep(timeout_t t = 0) { TBaseProcess::sleep(t); }

    // 根据优先级获取进程控制块指针
    INLINE const TBaseProcess * get_proc(uint_fast8_t Prio) { return Kernel.ProcessTable[Prio]; }

#if vortexRT_SYSTEM_TICKS_ENABLE == 1
#if vortexRT_SYSTEM_TICKS_ATOMIC == 1
    // 原子方式获取系统滴答计数（无锁版本）
    INLINE tick_count_t get_tick_count() { return Kernel.SysTickCount; }
#else
    // 获取系统滴答计数（带临界区保护）
    INLINE tick_count_t get_tick_count() { TCritSect cs; return Kernel.SysTickCount; }
#endif
#endif // vortexRT_SYSTEM_TICKS_ENABLE

#if vortexRT_TARGET_IDLE_HOOK_ENABLE == 1
    // 目标平台特定的空闲进程钩子函数
    void idle_process_target_hook();
#endif // vortexRT_TARGET_IDLE_HOOK_ENABLE

#if vortexRT_SYSTIMER_HOOK_ENABLE == 1
    // 系统定时器钩子函数（必须启用才能使用时间片轮转）
    INLINE_SYS_TIMER_HOOK void system_timer_user_hook();
#endif // vortexRT_SYSTIMER_HOOK_ENABLE

#if vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE == 1
    // 上下文切换用户钩子函数（用于自定义上下文切换逻辑）
    INLINE_CONTEXT_SWITCH_HOOK void context_switch_user_hook();
#endif // vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE

#if vortexRT_IDLE_HOOK_ENABLE == 1
    // 空闲进程用户钩子函数（用于自定义空闲任务处理）
    void idle_process_user_hook();
#endif // vortexRT_IDLE_HOOK_ENABLE
    
}   // namespace OS
//------------------------------------------------------------------------------

//todo #include <os_services.h>
#include "vortexRT_extensions.h"


//  注册流程
//   在内核的进程表中放置指向 process 的指针
void OS::TKernel::register_process(OS::TBaseProcess* const p)
{
    OS::TKernel::ProcessTable[p->Priority] = p;
}

//   系统定时器实现
//   执行进程的超时检查和
//   将流程移动到 Ready-to-Run 状态
void OS::TKernel::system_timer()
{
    SYS_TIMER_CRIT_SECT();
#if vortexRT_SYSTEM_TICKS_ENABLE == 1
    SysTickCount++;
#endif

#if vortexRT_PRIORITY_ORDER == 0
    const uint_fast8_t BaseIndex = 0;
#else
    constexpr uint_fast8_t BaseIndex = 1;
#endif

    for(uint_fast8_t i = BaseIndex; i < (PROCESS_COUNT - 1 + BaseIndex); i++)
    {
        if(TBaseProcess* p = ProcessTable[i]; p->Timeout > 0)
        {
            if(--p->Timeout == 0)
            {
                set_process_ready(p->Priority);
            }
        }
    }
}

//     ISR 优化调度程序
//    !!!重要说明：此函数只能从 ISR 服务调用!!

#if vortexRT_CONTEXT_SWITCH_SCHEME == 0
// 方案0的调度器中断服务例程实现
// 特点：直接上下文切换，无延迟
void OS::TKernel::sched_isr()
{
    // 从就绪进程映射表中找出最高优先级进程
    uint_fast8_t NextPrty = highest_priority(ReadyProcessMap);
    
    // 只有当新优先级与当前优先级不同时才执行切换
    if(NextPrty != CurProcPriority)
    {
    #if vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE == 1
        // 如果启用了用户钩子，调用用户定义的上下文切换前处理函数
        context_switch_user_hook();
    #endif
    
        // 准备切换参数：
        // Next_SP - 目标进程的栈指针
        // Curr_SP_addr - 当前进程栈指针的地址(用于保存上下文)
        stack_item_t*  Next_SP = ProcessTable[NextPrty]->StackPointer;
        stack_item_t** Curr_SP_addr = &(ProcessTable[CurProcPriority]->StackPointer);
        
        // 更新当前优先级
        CurProcPriority = NextPrty;
        
        // 调用底层上下文切换函数
        os_context_switcher(Curr_SP_addr, Next_SP);
    }
}
#else
// 方案1的调度器中断服务例程实现
// 特点：通过钩子函数延迟上下文切换
void OS::TKernel::sched_isr()
{
    // 从就绪进程映射表中找出最高优先级进程
    const uint_fast8_t NextPrty = highest_priority(ReadyProcessMap);
    
    // 保存目标优先级到成员变量
    SchedProcPriority = NextPrty;
    
    // 只有当新优先级与当前优先级不同时才触发切换
    if(NextPrty != CurProcPriority)
        raise_context_switch(); // 触发上下文切换请求
}
//------------------------------------------------------------------------------
#ifndef CONTEXT_SWITCH_HOOK_CRIT_SECT
#define CONTEXT_SWITCH_HOOK_CRIT_SECT()
#endif

// 上下文切换钩子函数实现
stack_item_t* OS::TKernel::context_switch_hook(stack_item_t* sp)
{
    // 执行临界区保护(如果定义了)
    CONTEXT_SWITCH_HOOK_CRIT_SECT();

    // 保存当前进程的栈指针
    ProcessTable[CurProcPriority]->StackPointer = sp;
    
    // 加载目标进程的栈指针
    sp = ProcessTable[SchedProcPriority]->StackPointer;
    
#if vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE == 1
    // 如果启用了用户钩子，调用用户定义的上下文切换前处理函数
    context_switch_user_hook();
#endif

    // 更新当前优先级
    CurProcPriority = SchedProcPriority;
    
    // 返回新进程的栈指针
    return sp;
}
#endif // vortexRT_CONTEXT_SWITCH_SCHEME

// 检查进程是否处于休眠状态
bool OS::TBaseProcess::is_sleeping() const
{
    TCritSect cs; // 临界区保护
    return this->Timeout != 0; // 超时计数器非0表示休眠
}

// 检查进程是否被挂起
bool OS::TBaseProcess::is_suspended() const
{
    TCritSect cs; // 临界区保护
    // 检查就绪映射表中是否没有当前优先级标记
    return (Kernel.ReadyProcessMap & get_prio_tag(this->Priority)) == 0;
}

// 系统启动函数
INLINE void OS::run()
{
#if vortexRT_SUSPENDED_PROCESS_ENABLE != 0
    // 如果启用了进程挂起功能，从挂起映射表初始化就绪映射表
    Kernel.ReadyProcessMap = TBaseProcess::SuspendedProcessMap;
    
    // 找出最高优先级进程
    uint_fast8_t p = highest_priority(Kernel.ReadyProcessMap); 
#else 
    // 否则默认从优先级0进程开始
    uint_fast8_t p = pr0;
#endif

    // 设置当前优先级
    Kernel.CurProcPriority = p;
    
    // 获取初始进程的栈指针
    stack_item_t *sp = Kernel.ProcessTable[p]->StackPointer;
    
    // 调用底层启动函数
    os_start(sp);
}

// 检查系统是否正在运行
INLINE bool OS::os_running()
{
    // 当前优先级小于MAX_PROCESS_COUNT表示系统已启动
    return Kernel.CurProcPriority < MAX_PROCESS_COUNT;
}
//-----------------------------------------------------------------------------

//todo #include <os_services.h>

#endif // OS_KERNEL_H
