

#include "vortexRT.h"

// 使用OS命名空间
using namespace OS;

// 定义全局内核对象实例
OS::TKernel OS::Kernel;

#if vortexRT_SUSPENDED_PROCESS_ENABLE != 0
// 挂起进程位图初始化
// 1左移PROCESS_COUNT位后减1，生成全1的位图，表示所有进程初始状态为挂起
OS::TProcessMap OS::TBaseProcess::SuspendedProcessMap = (1ul << (PROCESS_COUNT)) - 1; 
#endif

// 进程表数组定义
// 大小为最大进程数+1，索引0保留不使用
TBaseProcess * TKernel::ProcessTable[vortexRT_PROCESS_COUNT + 1];

//------------------------------------------------------------------------------
//
//    TKernel 调度函数
//
#if vortexRT_CONTEXT_SWITCH_SCHEME == 0
// 直接上下文切换实现方式
void TKernel::sched()
{
    // 从就绪进程位图中找出最高优先级进程
    uint_fast8_t NextPrty = highest_priority(ReadyProcessMap);
    
    // 如果新优先级与当前不同，则执行切换
    if(NextPrty != CurProcPriority)
    {
    #if vortexRT_CONTEXT_SWITCH_USER_HOOK_ENABLE == 1
        // 调用用户定义的上下文切换钩子函数
        context_switch_user_hook();
    #endif

        // 获取新进程的堆栈指针
        stack_item_t* Next_SP = ProcessTable[NextPrty]->StackPointer;
        // 获取当前进程堆栈指针的地址(用于保存)
        stack_item_t** Curr_SP_addr = &(ProcessTable[CurProcPriority]->StackPointer);
        // 更新当前优先级
        CurProcPriority = NextPrty;
        // 调用底层上下文切换函数
        os_context_switcher(Curr_SP_addr, Next_SP);
    }
}
#else
//------------------------------------------------------------------------------
// 软件中断切换实现方式
void TKernel::sched()
{
    // 从就绪进程位图中找出最高优先级进程
    uint_fast8_t NextPrty = highest_priority(ReadyProcessMap);
    
    // 如果新优先级与当前不同，则执行切换
    if(NextPrty != CurProcPriority)
    {
        // 设置待调度优先级
        SchedProcPriority = NextPrty;
    
        // 触发上下文切换中断
        raise_context_switch();
        
        // 等待上下文切换完成
        do
        {
            // 允许中断发生
            enable_context_switch();
            // 空指令，确保流水线刷新
            DUMMY_INSTR();
            // 禁止中断
            disable_context_switch();
        } 
        while(CurProcPriority != SchedProcPriority); // 直到上下文切换完成
    }
}
// 上下文切换钩子函数
// 参数: sp - 当前进程的堆栈指针
// 返回值: 处理后的堆栈指针
// 功能: 在上下文切换时被调用，允许内核对堆栈进行额外处理
stack_item_t* os_context_switch_hook(stack_item_t* sp) { return Kernel.context_switch_hook(sp); }

#endif // vortexRT_CONTEXT_SWITCH_SCHEME

//------------------------------------------------------------------------------
//
//       OS进程构造函数
//
//       功能:
//           * 初始化进程数据
//           * 在内核中注册进程
//           * 初始化堆栈帧
//                  
//
#if SEPARATE_RETURN_STACK == 0
// 单堆栈实现方式(数据和返回地址使用同一个堆栈)

TBaseProcess::TBaseProcess( stack_item_t * StackPoolEnd  // 堆栈结束地址
                          , TPriority pr                 // 进程优先级
                          , void (*exec)()               // 进程入口函数
                      #if vortexRT_DEBUG_ENABLE == 1
                          , stack_item_t * aStackPool    // 调试模式下堆池起始地址
                          , const char   * name_str      // 进程名称
                      #endif
                          ) : Timeout(0)                // 超时计数器初始化为0
                            , Priority(pr)              // 设置进程优先级
                      #if vortexRT_DEBUG_ENABLE == 1
                            , WaitingFor(0)            // 调试模式下等待对象初始化为0
                            , StackPool(aStackPool)    // 保存堆池起始地址
                            , StackSize(StackPoolEnd - aStackPool) // 计算堆栈大小
                            , Name(name_str)           // 保存进程名称
                      #endif 
                      #if vortexRT_PROCESS_RESTART_ENABLE == 1
                            , WaitingProcessMap(0)     // 进程重启相关标志初始化为0
                      #endif
{
    // 在内核中注册当前进程
    TKernel::register_process(this);
    // 初始化堆栈帧
    init_stack_frame( StackPoolEnd
                    , exec
                #if vortexRT_DEBUG_ENABLE == 1     
                    , aStackPool
                #endif  
                    );
}

#else  // SEPARATE_RETURN_STACK
// 双堆栈实现方式(数据堆栈和返回地址堆栈分离)

TBaseProcess::TBaseProcess( stack_item_t * Stack       // 数据堆栈指针
                          , stack_item_t * RStack      // 返回地址堆栈指针
                          , TPriority pr               // 进程优先级
                          , void (*exec)()             // 进程入口函数
                      #if vortexRT_DEBUG_ENABLE == 1
                          , stack_item_t * aStackPool   // 数据堆池起始地址
                          , stack_item_t * aRStackPool // 返回地址堆池起始地址
                          , const char   * name_str    // 进程名称
                      #endif
                          ) : StackPointer(Stack)     // 初始化数据堆栈指针
                            , Timeout(0)              // 超时计数器初始化为0
                            , Priority(pr)            // 设置进程优先级
                      #if vortexRT_DEBUG_ENABLE == 1
                            , WaitingFor(0)          // 调试模式下等待对象初始化为0
                            , StackPool(aStackPool)   // 保存数据堆池起始地址
                            , StackSize(Stack - aStackPool) // 计算数据堆栈大小
                            , Name(name_str)          // 保存进程名称
                            , RStackPool(aRStackPool) // 保存返回地址堆池起始地址
                            , RStackSize(RStack - aRStackPool) // 计算返回地址堆栈大小
                      #endif 
                      #if vortexRT_PROCESS_RESTART_ENABLE == 1
                            , WaitingProcessMap(0)    // 进程重启相关标志初始化为0
                      #endif
{
    // 在内核中注册当前进程
    TKernel::register_process(this);
    // 初始化双堆栈帧
    init_stack_frame( Stack
                    , RStack
                    , exec
                #if vortexRT_DEBUG_ENABLE == 1     
                    , aStackPool
                    , aRStackPool
                #endif  
                    );
}
#endif // SEPARATE_RETURN_STACK
//------------------------------------------------------------------------------
// 进程睡眠函数
// 参数: timeout - 睡眠超时时间(时钟节拍数)
void TBaseProcess::sleep(const timeout_t timeout)
{
    TCritSect cs;  // 临界区保护，防止并发访问

    // 设置当前进程的超时时间
    Kernel.ProcessTable[Kernel.CurProcPriority]->Timeout = timeout;
    // 将当前进程设置为非就绪状态
    Kernel.set_process_unready(Kernel.CurProcPriority);
    // 触发调度器重新调度
    Kernel.scheduler();
}

// 进程唤醒函数(条件唤醒)
void OS::TBaseProcess::wake_up()
{
    TCritSect cs;  // 临界区保护，防止并发访问

    // 如果进程设置了超时(处于睡眠状态)
    if(this->Timeout)
    {
        this->Timeout = 0;  // 清除超时标志
        // 将进程设置为就绪状态
        Kernel.set_process_ready(this->Priority);
        // 触发调度器重新调度
        Kernel.scheduler();
    }
}

// 进程强制唤醒函数(无条件唤醒)
void OS::TBaseProcess::force_wake_up()
{
    TCritSect cs;  // 临界区保护，防止并发访问

    this->Timeout = 0;  // 清除超时标志
    // 将进程设置为就绪状态
    Kernel.set_process_ready(this->Priority);
    // 触发调度器重新调度
    Kernel.scheduler();
}
//------------------------------------------------------------------------------
//
//
//   Idle Process
//
//
namespace OS
{
#ifndef __GNUC__  // 避免GCC编译器bug(http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867)
    template<> void TIdleProc::exec();  // 显式实例化声明
#endif

#if vortexRT_DEBUG_ENABLE == 1
    TIdleProc IdleProc("Idle");  // 调试模式下创建带名称的空闲进程实例
#else
    TIdleProc IdleProc;          // 非调试模式下创建空闲进程实例
#endif
}

namespace OS
{
    // 空闲进程执行函数
    template<> void TIdleProc::exec()
    {
        for(;;)  // 无限循环
        {
        #if vortexRT_IDLE_HOOK_ENABLE == 1
            idle_process_user_hook();  // 调用用户定义的空闲钩子函数
        #endif

        #if vortexRT_TARGET_IDLE_HOOK_ENABLE == 1
            idle_process_target_hook();  // 调用目标平台特定的空闲钩子函数
        #endif
        }
    }
}
//------------------------------------------------------------------------------
#if vortexRT_DEBUG_ENABLE == 1
#if SEPARATE_RETURN_STACK == 0
// 计算单堆栈模式下堆栈空闲空间
size_t TBaseProcess::stack_slack() const
{
     size_t slack = 0;
     const stack_item_t * Stack = StackPool;  // 从堆栈底部开始检查
     // 统计连续匹配堆栈模式(vortexRT_STACK_PATTERN)的数量
     while (*Stack++ == vortexRT_STACK_PATTERN)
         slack++;
     return slack;  // 返回空闲堆栈空间大小
}
#else  // SEPARATE_RETURN_STACK
// 计算堆栈空闲空间的辅助函数(用于双堆栈模式)
static size_t calc_stack_slack(const stack_item_t * Stack)
{
     size_t slack = 0;
     // 统计连续匹配堆栈模式(vortexRT_STACK_PATTERN)的数量
     while (*Stack++ == vortexRT_STACK_PATTERN)
         slack++;
     return slack;
}
// 计算数据堆栈空闲空间
size_t TBaseProcess::stack_slack() const
{
     return calc_stack_slack(StackPool);
}
// 计算返回地址堆栈空闲空间
size_t TBaseProcess::rstack_slack() const
{
     return calc_stack_slack(RStackPool);
}
#endif // SEPARATE_RETURN_STACK
#endif // vortexRT_DEBUG_ENABLE
//------------------------------------------------------------------------------
#if vortexRT_PROCESS_RESTART_ENABLE == 1
// 重置进程控制状态(用于进程重启)
void TBaseProcess::reset_controls()
{
    // 将进程设置为非就绪状态
    Kernel.set_process_unready(this->Priority);
    // 如果进程正在等待服务
    if(WaitingProcessMap)
    {
        // 从服务进程映射中移除当前进程
        clr_prio_tag( *WaitingProcessMap, get_prio_tag(Priority) );
        WaitingProcessMap = 0;
    }
    // 重置超时计数器
    Timeout = 0;
#if vortexRT_DEBUG_ENABLE == 1
    // 调试模式下重置等待对象
    WaitingFor = 0;
#endif
}
#endif  // vortexRT_PROCESS_RESTART_ENABLE


