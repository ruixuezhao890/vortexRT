/**
  ******************************************************************************
  * @file           : os_target.cpp
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-19
  ******************************************************************************
  */
#include <vortexRT.h>

namespace OS
{

#if vortexRT_DEBUG_ENABLE == 1

// 调试支持信息结构体
struct TDebugSupportInfo
{
    uint8_t PROCESS_COUNT;  // 进程数量
    uint8_t TIMEOUT_SIZE;   // 超时值大小(字节)
    uint8_t NAME_OFFSET;    // 进程名称在结构体中的偏移量
};

// 调试信息常量定义
// __attribute__确保该变量被保留在二进制文件中且2字节对齐
__attribute__((used, aligned(2)))
extern const TDebugSupportInfo DebugInfo =
{
    PROCESS_COUNT,          // 填充进程数量
    sizeof(timeout_t),      // 填充超时值大小
    sizeof(timeout_t) == 2 ? 20 : 24  // 根据超时值大小计算名称偏移量
};
#endif // vortexRT_DEBUG_ENABLE

} // namespace OS

// 初始化进程堆栈帧
void OS::TBaseProcess::init_stack_frame(stack_item_t * Stack
                                   , void (*exec)()
                                #if vortexRT_DEBUG_ENABLE == 1
                                   , stack_item_t * StackBegin
                                #endif
                                   )
{
    // 根据AAPCS标准要求8字节对齐堆栈指针
    StackPointer = (stack_item_t*)((uintptr_t)Stack & 0xFFFFFFF8UL);

    *(--StackPointer)  = 0x01000000UL;      // 初始xPSR寄存器值(Thumb状态)
    *(--StackPointer)  = reinterpret_cast<stack_item_t>(exec); // 进程入口点(PC指针)
    
#if (defined __SOFTFP__)    // 无FPU内核
    StackPointer -= 14;     // 模拟压入LR,R12,R3,R2,R1,R0,R11-R4寄存器
#else                       // 有FPU内核
    StackPointer -= 6;      // 模拟压入LR,R12,R3,R2,R1,R0寄存器
    *(--StackPointer)  = 0xFFFFFFFDUL;  // exc_return值: 返回线程模式,禁用FPU上下文,使用PSP
    StackPointer -= 8;      // 模拟压入R4-R11寄存器
#endif

#if vortexRT_DEBUG_ENABLE == 1
    // 调试模式下初始化堆栈模式
    *(StackPointer)  = reinterpret_cast<stack_item_t>(&DebugInfo); // 确保DebugInfo保留在二进制中
    
    // 用默认模式填充堆栈空间
    for (stack_item_t *pDst = StackBegin; pDst < StackPointer; pDst++)
        *pDst = STACK_DEFAULT_PATTERN;
#endif
}

/*
 * 默认使用SysTick定时器作为系统定时器
 * 如果没有定义vortexRT_USE_CUSTOM_TIMER，则默认设为0(使用SysTick)
 */
#if (! defined vortexRT_USE_CUSTOM_TIMER)
#define vortexRT_USE_CUSTOM_TIMER 0
#endif

/*
 * Cortex-MX寄存器定义和常量
 */
namespace {
    // 硬件寄存器模板类
    // addr: 寄存器地址, type: 寄存器类型(默认为uint32_t)
    template<uintptr_t addr, typename type = uint32_t>
    struct ioregister_t
    {
        // 重载赋值运算符
        type operator=(type value) { *(volatile type*)addr = value; return value; }
        // 重载或等于运算符
        void operator|=(type value) { *(volatile type*)addr |= value; }
        // 重载与等于运算符
        void operator&=(type value) { *(volatile type*)addr &= value; }
        // 类型转换运算符
        operator type() { return *(volatile type*)addr; }
    };

    // 硬件结构体模板类
    // addr: 结构体基地址, T: 结构体类型
    template<uintptr_t addr, class T>
    struct iostruct_t
    {
        // 重载箭头运算符
        volatile T* operator->() { return (volatile T*)addr; }
    };

    // PendSV和SysTick优先级寄存器定义
#if (defined __ARM_ARCH_6M__)
    // Cortex-M0系统控制寄存器(仅支持字访问)
    static ioregister_t<0xE000ED20UL, uint32_t> SHPR3;
#define SHP3_WORD_ACCESS
#else
    // Cortex-M3/M4系统控制寄存器(支持字节访问)
    static ioregister_t<0xE000ED22UL, uint8_t> PendSvPriority;  // PendSV优先级寄存器
#if (vortexRT_USE_CUSTOM_TIMER == 0)
    static ioregister_t<0xE000ED23UL, uint8_t> SysTickPriority; // SysTick优先级寄存器
#endif
#endif

#if (vortexRT_USE_CUSTOM_TIMER == 0)
    // SysTick定时器结构体定义
    struct systick_t
    {
        uint32_t       CTRL;   // 控制状态寄存器
        uint32_t       LOAD;   // 重装载值寄存器
        uint32_t       VAL;    // 当前值寄存器
        uint32_t const CALIB;  // 校准值寄存器(只读)
    };

    // SysTick控制寄存器位定义
    enum
    {
        NVIC_ST_CTRL_CLK_SRC = 0x00000004,  // 时钟源选择(1=处理器时钟,0=外部时钟)
        NVIC_ST_CTRL_INTEN   = 0x00000002,  // 中断使能
        NVIC_ST_CTRL_ENABLE  = 0x00000001    // 定时器使能
    };

    // SysTick寄存器组实例化
    static iostruct_t<0xE000E010UL, systick_t> SysTickRegisters;
#endif

#if (!defined __SOFTFP__)
    // 浮点上下文控制寄存器定义
    // FPCCR: 浮点上下文控制寄存器地址(0xE000EF34)
    static ioregister_t<0xE000EF34UL> FPCCR;
    // 浮点上下文控制寄存器位定义
    enum
    {
        ASPEN =   (0x1UL << 31),  // 总是保存使能位(自动保存FPU上下文)
        LSPEN =   (0x1UL << 30)   // 延迟保存使能位(惰性保存FPU上下文)
    };
#endif

    // 系统定时器中断处理函数(SysTick中断)
#if (vortexRT_USE_CUSTOM_TIMER == 0)
    OS_INTERRUPT void SysTick_Handler()
    {
        OS::system_timer_isr();  // 调用系统定时器中断服务例程
    }
#endif

    // 默认系统定时器初始化配置
    // 优先级设置略高于最低优先级
    // 配置SysTick定时器以SYSTICKINTRATE频率中断
#if (!defined CORE_PRIORITY_BITS)
#    define CORE_PRIORITY_BITS        8  // 默认优先级位数为8
#endif

    namespace
    {
        // 系统定时器优先级计算(0xFE左移调整)
        enum { SYS_TIMER_PRIORITY = ((0xFEUL << (8-(CORE_PRIORITY_BITS))) & 0xFF) };
    }

#if (vortexRT_USE_CUSTOM_TIMER == 0)
    // 系统定时器初始化函数
    extern "C" __attribute__((used)) void __init_system_timer()
    {
#if (defined SHP3_WORD_ACCESS)   // Cortex-M0字访问方式
        SHPR3 = (SHPR3 & ~(0xFF << 24)) | (SYS_TIMER_PRIORITY << 24);
#else  // Cortex-M3/M4字节访问方式
        SysTickPriority = SYS_TIMER_PRIORITY;  // 设置SysTick优先级
#endif
        // 配置SysTick定时器
        SysTickRegisters->LOAD = SYSTICKFREQ/SYSTICKINTRATE-1;  // 设置重装载值
        SysTickRegisters->VAL = 0;         // 清除当前计数器值
        // 使能SysTick定时器(使用处理器时钟,使能中断,启动计数器)
        SysTickRegisters->CTRL = NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
    }

    // 系统定时器锁定函数(禁用中断)
    void LOCK_SYSTEM_TIMER()   { SysTickRegisters->CTRL &= ~NVIC_ST_CTRL_INTEN; }
    // 系统定时器解锁函数(使能中断)
    void UNLOCK_SYSTEM_TIMER() { SysTickRegisters->CTRL |= NVIC_ST_CTRL_INTEN; }

#endif  // #if (vortexRT_USE_CUSTOM_TIMER == 0)



    /*
     * 启动多任务调度
     * 注意事项:
     * 1) os_start()必须完成以下操作:
     *    a) 设置PendSV和SysTick中断优先级为最低
     *    b) 调用系统定时器初始化函数
     *    c) 启用中断(任务将在中断启用状态下运行)
     *    d) 跳转到最高优先级进程的入口点
     */
    extern "C" NORETURN void os_start(stack_item_t *sp)
    {
        // 设置PendSV最低优先级
#if (defined SHP3_WORD_ACCESS)  // Cortex-M0字访问方式
        SHPR3 |= (0xFF << 16);     // 设置PendSV优先级为最低(0xFF)
#else  // Cortex-M3/M4字节访问方式
        PendSvPriority = 0xFF;     // 设置PendSV优先级为最低
#endif

#if (!defined __SOFTFP__)
        // 启用FPU上下文自动保存和延迟保存
        FPCCR |= ASPEN | LSPEN;
#endif

        // 内联汇编实现上下文切换和任务启动
        asm volatile (
    #if (defined __SOFTFP__)  // 无FPU内核代码
            "    LDR     R4, [%[stack], #(4 * 14)] \n" // 从堆栈加载进程入口点到R4(14个寄存器偏移)这里是从低地址加上14个4字节地址偏移
            "    ADD     %[stack], #(4 * 16)       \n" // 调整堆栈指针模拟上下文恢复 这里的意思是恢复到栈顶地址方便下次寄存器进行保存
    #else  // 有FPU内核代码
            "    LDR     R4, [%[stack], #(4 * 15)] \n" // 从堆栈加载进程入口点到R4(15个寄存器偏移)
            "    ADD     %[stack], #(4 * 17)       \n" // 调整堆栈指针模拟上下文恢复
    #endif
            "    MSR     PSP, %[stack]             \n" // 设置进程堆栈指针到PSP
            "    MOV     R0, #2                    \n" // 配置当前模式:使用PSP作为堆栈指针,特权级
            "    MSR     CONTROL, R0               \n"
            "    ISB                               \n" // 插入内存屏障
            "    LDR     R1, =__init_system_timer  \n" // 加载系统定时器初始化函数地址
            "    BLX     R1                        \n" // 调用系统定时器初始化
            "    CPSIE   I                         \n" // 启用处理器级中断
            "    BX      R4                        \n" // 跳转到进程exec()函数
            : [stack]"+r" (sp)  // 输出操作数:堆栈指针
            :                   // 无输入操作数
            : "r0", "r1", "r4"  // 被修改的寄存器
        );

        __builtin_unreachable(); // 抑制编译器"'noreturn'函数可能返回"警告
    }
}

#if vortexRT_PRIORITY_ORDER == 0
namespace OS
{
    // 优先级查找表定义
    // 该表用于快速确定下一个要运行的进程，采用位图算法优化调度效率
    extern TPriority const PriorityTable[] =
    {
        #if vortexRT_PROCESS_COUNT == 1// 1个进程的情况
            static_cast<TPriority>(0xFF), // 无效优先级标记(0xFF表示空位)
            pr0,// 优先级0进程
            prIDLE, pr0// 空闲进程和优先级0进程交替
        #elif vortexRT_PROCESS_COUNT == 2// 2个进程的情况
            static_cast<TPriority>(0xFF),
            pr0, 
            pr1, pr0, // 优先级1和优先级0进程交替
            prIDLE, pr0, pr1, pr0// 空闲进程和优先级0、1进程交
        #elif vortexRT_PROCESS_COUNT == 3
            static_cast<TPriority>(0xFF),
            pr0,
            pr1, pr0,
            pr2, pr0, pr1, pr0, // 优先级2、0、1、0交替
            prIDLE, pr0, pr1, pr0, pr2, pr0, pr1, pr0
        #elif vortexRT_PROCESS_COUNT == 4
            static_cast<TPriority>(0xFF),
            pr0,
            pr1, pr0,
            pr2, pr0, pr1, pr0,
            pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0,
            prIDLE, pr0, pr1, pr0, pr2, pr0, pr1, pr0, pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0
        #elif vortexRT_PROCESS_COUNT == 5
            static_cast<TPriority>(0xFF),
            pr0,
            pr1, pr0,
            pr2, pr0, pr1, pr0,
            pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0,
            pr4, pr0, pr1, pr0, pr2, pr0, pr1, pr0, pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0,
            prIDLE, pr0, pr1, pr0, pr2, pr0, pr1, pr0, pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0, pr4, pr0, pr1, pr0, pr2, pr0, pr1, pr0, pr3, pr0, pr1, pr0, pr2, pr0, pr1, pr0
        #else // vortexRT_PROCESS_COUNT > 5// vortexRT_PROCESS_COUNT > 5  // 超过5个进程的情况
        // 使用更复杂的优先级映射表(基于位图算法)
            static_cast<TPriority>(32),      static_cast<TPriority>(0),       static_cast<TPriority>(1),       static_cast<TPriority>(12),
            static_cast<TPriority>(2),       static_cast<TPriority>(6),       static_cast<TPriority>(0xFF),    static_cast<TPriority>(13),
            static_cast<TPriority>(3),       static_cast<TPriority>(0xFF),    static_cast<TPriority>(7),       static_cast<TPriority>(0xFF),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(14),
            static_cast<TPriority>(10),      static_cast<TPriority>(4),       static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),
            static_cast<TPriority>(8),       static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(25),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(21),      static_cast<TPriority>(27),      static_cast<TPriority>(15),
            static_cast<TPriority>(31),      static_cast<TPriority>(11),      static_cast<TPriority>(5),       static_cast<TPriority>(0xFF),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),
            static_cast<TPriority>(9),       static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(24),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(20),      static_cast<TPriority>(26),
            static_cast<TPriority>(30),      static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),    static_cast<TPriority>(0xFF),
            static_cast<TPriority>(0xFF),    static_cast<TPriority>(23),      static_cast<TPriority>(0xFF),    static_cast<TPriority>(19),
            static_cast<TPriority>(29),      static_cast<TPriority>(0xFF),    static_cast<TPriority>(22),      static_cast<TPriority>(18),
            static_cast<TPriority>(28),      static_cast<TPriority>(17),      static_cast<TPriority>(16),      static_cast<TPriority>(0xFF)
        #endif  // vortexRT_PROCESS_COUNT
    };
}   //namespace
#endif

