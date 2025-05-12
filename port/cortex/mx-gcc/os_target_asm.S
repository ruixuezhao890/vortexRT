
//    处理器: ARM Cortex-M0(+), Cortex-M1, Cortex-M3, Cortex-M4(F)

    .syntax unified    // 使用统一的ARM/Thumb汇编语法
    .text              // 开始代码段
    .align 2           // 按4字节对齐(2^2=4)

    // 声明外部符号(定义在其他文件中)
    .extern os_context_switch_hook  // 上下文切换钩子函数
    // 声明全局符号(可被其他文件引用)  
    .global PendSV_Handler          // PendSV异常处理入口


/*
 * PendSV用于执行上下文切换。这是Cortex-M推荐的方法。
 * 因为Cortex-M在异常发生时自动保存一半的处理器上下文，
 * 并在异常返回时恢复。所以只需要保存R4-R11寄存器
 * 并调整堆栈指针。使用PendSV异常意味着上下文保存和恢复
 * 无论是从线程发起还是由于中断或异常发生都是相同的。
 *
 * 由于PendSV中断在系统中优先级最低(由下面的os_start()设置)，
 * 我们可以确保它只在没有其他异常或中断活动时运行，
 * 因此可以安全地假设被切换出的上下文使用的是进程堆栈(PSP)。
 *
 * 定义用于确定架构:
 * M0(+)核心: 定义了__ARM_ARCH_6M__
 * M3核心: 定义了__SOFTFP__，未定义__ARM_ARCH_6M__
 * M4F核心: 未定义__SOFTFP__
 */

.thumb_func
PendSV_Handler:

#if (defined __ARM_ARCH_6M__)
    // Cortex-M0(+)/Cortex-M1
    CPSID   I                   // 在上下文切换期间防止中断
    MRS     R0, PSP             // 将进程堆栈指针加载到R0
    SUBS    R0, #32             // 调整R0指向堆栈中保存的上下文顶部
    MOV     R1, R0              // 保存R0(用于os_context_switch_hook()调用)
    STMIA   R1!, {R4-R7}        // 在进程堆栈上保存剩余寄存器的低部分(r4-7)
    MOV     R4, R8              // 将剩余寄存器的高部分(r8-11)移动到低寄存器
    MOV     R5, R9
    MOV     R6, R10
    MOV     R7, R11
    STMIA   R1!, {R4-R7}        // 在进程堆栈上保存剩余寄存器的高部分(r8-11)

// 此时，进程的整个上下文已保存
    PUSH    {LR}                // 必须保存LR(exc_return值)直到异常返回
    LDR     R1, =os_context_switch_hook   // 调用os_context_switch_hook();
    BLX     R1

// R0是新进程SP;
    ADDS    R0, #16            // 调整R0指向高寄存器(r8-11)
    LDMIA   R0!, {R4-R7}       // 从新进程堆栈恢复r8-11
    MOV     R8, R4             // 将恢复的值移动到高寄存器(r8-11)
    MOV     R9, R5
    MOV     R10, R6
    MOV     R11, R7
    MSR     PSP, R0            // 此时R0是新进程SP
    SUBS    R0, #32            // 调整R0指向低寄存器
    LDMIA   R0!, {R4-R7}       // 恢复r4-7
    CPSIE   I
    POP     {PC}               // 返回到保存的exc_return。异常返回将恢复剩余上下文


#elif (defined __SOFTFP__)
    // 不带FPU的M3/M4核心
    CPSID   I                  // 在上下文切换期间防止中断
    MRS     R0, PSP            // PSP是进程堆栈指针
    STMDB   R0!, {R4-R11}      // 在进程堆栈上保存剩余寄存器r4-11

// 此时，进程的整个上下文已保存
    PUSH    {LR}               // 必须保存LR(exc_return值)直到异常返回
    LDR     R1, =os_context_switch_hook     // 调用os_context_switch_hook();
    BLX     R1

// R0是新进程SP;
    LDMIA   R0!, {R4-R11}      // 从新进程堆栈恢复r4-11
    MSR     PSP, R0            // 用新进程SP加载PSP
    CPSIE   I
    POP     {PC}               // 返回到保存的exc_return。异常返回将恢复剩余上下文


#else
    // 带FPU的核心(cortex-M4F)
    CPSID     I                  // 在上下文切换期间防止中断
    MRS       R0, PSP            // PSP是进程堆栈指针
    TST       LR, #0x10          // exc_return[4]=0?(表示当前进程
    IT        EQ                 // 有活动的浮点上下文)
    VSTMDBEQ  R0!, {S16-S31}     // 如果是-保存它
    STMDB     R0!, {R4-R11, LR}  // 在进程堆栈上保存剩余寄存器r4-11和LR

// 此时，进程的整个上下文已保存
    LDR     R1, =os_context_switch_hook     // 调用os_context_switch_hook();
    BLX     R1

// R0是新进程SP;
    LDMIA     R0!, {R4-R11, LR}  // 从新进程堆栈恢复r4-11和LR
    TST       LR, #0x10          // exc_return[4]=0?(表示新进程
    IT        EQ                 // 有活动的浮点上下文)
    VLDMIAEQ  R0!, {S16-S31}     // 如果是-恢复它
    MSR       PSP, R0            // 用新进程SP加载PSP
    CPSIE     I
    BX        LR                 // 返回到保存的exc_return。异常返回将恢复剩余上下文
#endif

.end

