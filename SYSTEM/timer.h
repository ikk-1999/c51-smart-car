#ifndef __TIMER_H__
#define __TIMER_H__

#include "SYSTEM/sys.h"

// ============================================================
// 通用 Timer0 / Timer1 初始化封装
//
// 注意：这些函数只做 GPIO 无关的定时器配置。
// 实际用途由各模块自行决定：
//   - ir.c 会覆盖 Timer0 为模式2 (8位自动重装)
//   - hc_sr04.c 会覆盖 Timer1 为模式1 (16位, 不自动重装)
//   - motor.c 使用 Timer2 独立配置
//
// 因此 Timer0_Init/Timer1_Init 仅用于需要"标准 1ms 溢出"
// 的场景（如下文默认的 0xFC67 重装值），其他模块可自行
// 改写 TMOD 和重装值。
// ============================================================

void Timer0_Init(void);
void Timer0_Start(void);
void Timer0_Stop(void);
void Timer0_SetReload(u16 reload);

void Timer1_Init(void);
void Timer1_Start(void);
void Timer1_Stop(void);
void Timer1_SetReload(u16 reload);

#endif
