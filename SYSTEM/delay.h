#ifndef __DELAY_H__
#define __DELAY_H__

#include "SYSTEM/sys.h"

// ============================================================
// 纯软件延时 (不占用定时器)
//
// 精度：
//   11.0592MHz / 12 = 921.6kHz 机器周期
//   1 机器周期 ≈ 1.085μs
//   delay_ms: 外层 do-while, 内层 while(--i) 共约 114 次
//             实际约 114 × 2 × 1.085μs ≈ 1ms
//   delay_us: while(--us) 每次 2 机器周期 ≈ 2.2μs
//
// ⚠️ 中断会延长延时，故不适合精确定时 (如 NEC 解码、串口波特率)
//   延时期间 EA 保持原状态，中断仍可响应
// ============================================================

void delay_ms(unsigned int ms);
void delay_us(unsigned char us);

#endif
