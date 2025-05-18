#ifndef vortexRT_CORTEXMX_H
#define vortexRT_CORTEXMX_H

// Cortex-M系列RTOS目标平台头文件
// 定义与Cortex-M处理器相关的RTOS配置和硬件抽象


//    编译器和目标检查
#ifndef __GNUC__
#error "This file should only be compiled with GNU C++ Compiler"
#endif // __GNUC__

// 处理器架构检查 - 仅支持Cortex-M系列
#if (!defined __ARM_ARCH_7M__) && (!defined __ARM_ARCH_7EM__) && (!defined __ARM_ARCH_6M__)
#error "This file must be compiled for ARMv6-M (Cortex-M0(+)), ARMv7-M (Cortex-M3) and ARMv7E-M (Cortex-M4(F)) processors only."
#endif

#if (__GNUC__ < 3)
#error "This file must be compiled by GCC C/C++ Compiler v3.0 or higher."
#endif


//   编译器特定的属性
// 编译器属性定义
#ifndef INLINE
#define INLINE      __attribute__((__always_inline__)) inline  // 强制内联函数
#endif

#ifndef NOINLINE
#define NOINLINE    __attribute__((__noinline__))
#endif

#ifndef NORETURN
#define NORETURN    __attribute__((__noreturn__))
#endif

// RTOS核心类型定义
typedef uint32_t stack_item_t; // 栈项类型，用于任务栈
typedef uint32_t status_reg_t; // 状态寄存器类型，用于保存中断状态


//    配置宏
// 进程和中断属性定义
#define OS_PROCESS __attribute__((__noreturn__))  // 进程函数不返回
#define OS_INTERRUPT extern "C"                   // 中断服务函数使用C链接

#define DUMMY_INSTR() __asm__ __volatile__ ("nop")
#define INLINE_PROCESS_CTOR INLINE

// 不需要单独的返回堆栈
#define SEPARATE_RETURN_STACK   0


// Cortex-M 端口不支持软件中断堆栈切换
// 因为处理器实现了硬件堆栈切换。
//因此，不能在项目级别选择系统计时器 ISR 包装器
#define vortexRT_ISRW_TYPE       TISRW


//    vortexRT 上下文切换方案

//  宏定义上下文切换方式。值 0 设置直接上下文
//    在计划程序和 OS ISR 中切换。这是主要方法。
//   值 1 设置第二种切换上下文的方式 - 通过使用软件
//   中断。有关详细信息，请参阅文档。
//   Cortex-M 端口仅支持软件中断切换方式。

#define  vortexRT_CONTEXT_SWITCH_SCHEME 1

//-----------------------------------------------------------------------------
//
//   vortexRT 优先级顺序
//
//    此宏定义进程优先级的顺序。违约
//   使用升序。或者，降序优先级
//   order 的在某些平台上，首选降序
//   因为性能。
//
//   宏的默认（对应于升序）值为
//   宏的替代项 （对应于降序） 值为 1。
//
//    在 Cortex-M3/M4 上，出于性能原因，使用降序。
//   Cortex-M0 缺少 CLZ 指令，因此仅实现升序。
//
#if (defined __ARM_ARCH_6M__)
#define  vortexRT_PRIORITY_ORDER             0
#else
#define  vortexRT_PRIORITY_ORDER             1
#endif

//-----------------------------------------------------------------------------
//
//    包括项目级配置
//    !!!includes 的顺序很重要!!
//
#include "vortexRT_CONFIG.h"
#include "vortexRT_TARGET_CFG.h"
#include <vortexRT_defs.h>

//-----------------------------------------------------------------------------
//
//    特定于目标的配置宏
//
#ifdef vortexRT_USER_DEFINED_STACK_PATTERN
#define vortexRT_STACK_PATTERN vortexRT_USER_DEFINED_STACK_PATTERN
#else
#define vortexRT_STACK_PATTERN 0xABBA
#endif


//-----------------------------------------------------------------------------
//
//     Interrupt and Interrupt Service Routines support
//
// 中断控制宏
#define enable_interrupts() __asm__ __volatile__ ("cpsie i")  // 开启全局中断
#define disable_interrupts() __asm__ __volatile__ ("cpsid i") // 关闭全局中断

// 设置中断状态（PRIMASK寄存器）
// 参数：status - 要设置的状态值（0启用中断，1禁用中断）
INLINE void set_interrupt_state(status_reg_t status)
{
    __asm__ __volatile__ (
        "MSR PRIMASK, %0\n"  // 内联汇编：将参数值写入PRIMASK寄存器
        : : "r"(status)      // 输入操作数：将status变量放入通用寄存器
        :"memory"            // 破坏描述：指示内存可能被修改
    );
}

// 获取当前中断状态（PRIMASK寄存器值）
// 返回值：当前中断状态（0表示中断启用，1表示中断禁用）
INLINE status_reg_t get_interrupt_state()
{
    status_reg_t sr;
    __asm__ __volatile__ (
        "MRS %0, PRIMASK"    // 内联汇编：读取PRIMASK寄存器值
        : "=r"(sr)           // 输出操作数：将结果存入sr变量
    );
    return sr;
}

//-----------------------------------------------------------------------------
//
//    关键部分包装器
//
//
#if vortexRT_USER_DEFINED_CRITSECT_ENABLE == 0
// 临界区保护类 - RAII模式
class TCritSect {
public:
    INLINE TCritSect() : StatusReg(get_interrupt_state()) { disable_interrupts(); } // 进入临界区
    INLINE ~TCritSect() { set_interrupt_state(StatusReg); } // 退出临界区

private:
    status_reg_t StatusReg; // 保存原始中断状态
};
#endif // vortexRT_USER_DEFINED_CRITSECT_ENABLE

//   请取消下方宏定义的注释，使 system_timer() 和 context_switch_hook() 在临界区中运行。
//   这在目标处理器硬件支持嵌套中断时非常有用（且必要）。用户可以使用自定义的 TCritSect 功能来定义自己的宏。
//   Cortex-M 处理器支持嵌套中断，但在上下文切换 ISR 期间会禁用中断。因此：.
//   系统定时器例程需要临界区保护
//   上下文切换器则不需要临界区保护
#define SYS_TIMER_CRIT_SECT() TCritSect cs
#define CONTEXT_SWITCH_HOOK_CRIT_SECT()



//    锁定/解锁系统计时器。
void LOCK_SYSTEM_TIMER();

void UNLOCK_SYSTEM_TIMER();

//    优先内容
namespace OS {
    // 根据优先级值生成对应的优先级标记(位图)
    INLINE OS::TProcessMap get_prio_tag(const uint_fast8_t pr) { return static_cast<OS::TProcessMap>(1 << pr); }

#if vortexRT_PRIORITY_ORDER == 0
    // 升序优先级顺序实现(用于Cortex-M0等没有CLZ指令的处理器)
    // 参数pm: 优先级位图
    // 返回值: 最高优先级值(0为最低优先级)
    INLINE uint_fast8_t highest_priority(TProcessMap pm)
    {
        extern TPriority const PriorityTable[];

#if vortexRT_PROCESS_COUNT < 6
             // 进程数较少时直接查表
            return PriorityTable[pm];
#else

   // 进程数较多时使用位操作优化
            uint32_t x = pm;
            x = x & -x;                            // 提取最低位的1(最右边的1)

            // 以下位操作等效于乘以0x450FBAF(用于快速计算优先级索引)
                                                // x = x * 0x450FBAF
            x = (x << 4) | x;                       // x = x*17.
            x = (x << 6) | x;                       // x = x*65.
            x = (x << 16) - x;                      // x = x*65535.
 // 使用查找表返回最终优先级
            return PriorityTable[x >> 26];
#endif  // vortexRT_PROCESS_COUNT < 6
    }
#else
// 降序优先级顺序实现(用于Cortex-M3/M4等有CLZ指令的处理器)
    // 使用内置指令__builtin_clz计算前导零数量
    // 参数pm: 优先级位图
    // 返回值: 最高优先级值(31为最高优先级)
    INLINE uint_fast8_t highest_priority(TProcessMap pm) {
        return 31 - __builtin_clz(pm);
        //前导零数量 = 最高优先级位位置
    }
#endif // vortexRT_PRIORITY_ORDER
}

namespace OS {
    // 使能上下文切换 - 实际上是开启全局中断
    // 当RTOS需要允许任务切换时调用此函数
    INLINE void enable_context_switch() { enable_interrupts(); }
    
    // 禁用上下文切换 - 实际上是关闭全局中断  
    // 当RTOS需要保护关键代码段不被任务切换打断时调用此函数
    INLINE void disable_context_switch() { disable_interrupts(); }
}

//------------------------------------------------------------------------------
//
//       Context Switch ISR stuff
//
//
namespace OS {
#if vortexRT_CONTEXT_SWITCH_SCHEME == 1

    // 触发上下文切换中断(PendSV)
    // 通过设置中断控制状态寄存器(0xE000ED04)的PENDSVSET位来触发软件中断
    // Cortex-M处理器使用PendSV异常来实现延迟的上下文切换
    INLINE void raise_context_switch() { 
        *((volatile uint32_t *) 0xE000ED04) = 0x10000000; // 设置PENDSVSET位
    }

    // 允许嵌套中断的宏定义
    // 在Cortex-M上默认允许嵌套中断
#define ENABLE_NESTED_INTERRUPTS()

#if vortexRT_SYSTIMER_NEST_INTS_ENABLE == 0
    // 禁止嵌套中断的宏定义
    // 当系统定时器不支持嵌套中断时，使用临界区保护
#define DISABLE_NESTED_INTERRUPTS() TCritSect cs
#else
    // 系统定时器支持嵌套中断时，不需要额外保护
#define DISABLE_NESTED_INTERRUPTS()
#endif

#else
    // Cortex-M端口仅支持软件中断切换方式
    // 如果配置为其他方式则报错
#error "Cortex-M3 port supports software interrupt switch method only!"

#endif // vortexRT_CONTEXT_SWITCH_SCHEME
}

#include <os_kernel.h>

namespace OS {
    //--------------------------------------------------------------------------
    //
    //      NAME       :   OS ISR support
    //
    //      PURPOSE    :   实现RTOS下中断进入和退出的通用操作
    //
    //      DESCRIPTION:
    //      使用RAII模式管理中断嵌套计数，确保中断退出时正确调度
    //
    class TISRW {
    public:
        // 构造函数 - 进入ISR时调用
        INLINE TISRW() { ISR_Enter(); }
        // 析构函数 - 退出ISR时调用
        INLINE ~TISRW() { ISR_Exit(); }

    private:
        //-----------------------------------------------------
        // ISR进入处理
        // 1. 进入临界区保护
        // 2. 增加中断嵌套计数
        INLINE void ISR_Enter() {
            TCritSect cs;  // 自动进入临界区
            Kernel.ISR_NestCount++;  // 增加嵌套计数
        }

        //-----------------------------------------------------
        // ISR退出处理
        // 1. 进入临界区保护
        // 2. 减少中断嵌套计数
        // 3. 如果是最外层中断，触发调度
        INLINE void ISR_Exit() {
            TCritSect cs;  // 自动进入临界区
            if (--Kernel.ISR_NestCount) return;  // 如果不是最外层中断则直接返回
            Kernel.sched_isr();  // 触发中断级调度
        }
        //-----------------------------------------------------
    };

    // 定义软件中断堆栈切换类型(兼容性定义)
    // Cortex-M使用硬件堆栈切换，所以直接使用TISRW
#define TISRW_SS    TISRW

    /*
     * 系统定时器中断处理函数
     * 1. 创建TISRW对象管理ISR生命周期
     * 2. 根据配置决定是否禁用嵌套中断
     * 3. 调用用户钩子函数(如果启用)
     * 4. 处理系统定时器事件
     */
    INLINE void system_timer_isr() {
        OS::TISRW ISR;  // 自动管理ISR嵌套计数

#if vortexRT_SYSTIMER_NEST_INTS_ENABLE == 0
    DISABLE_NESTED_INTERRUPTS();  // 禁用嵌套中断(如果需要)
#endif

#if vortexRT_SYSTIMER_HOOK_ENABLE == 1
        system_timer_user_hook();  // 调用用户钩子函数
#endif

        Kernel.system_timer();  // 处理系统定时器事件
    }
} // namespace OS
#endif // vortexRT_CORTEXMX_H

