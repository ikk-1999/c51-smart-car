#ifndef __HC_SR04_H__
#define __HC_SR04_H__

#include "SYSTEM/sys.h"

// ============================================================
// HC-SR04 超声波测距驱动 (简化版 — 无状态机)
//
// 每 20ms 采样一次，3 次滑动平均出结果
//
// 硬件：
//   SR04_TRIG = P2^1 — 触发引脚
//   SR04_ECHO = P2^0 — 回波引脚
//
// 定时器：
//   Timer0 模式1 (16位) — 测量 ECHO 高电平脉宽 + 超时保护
//   11.0592MHz: 1 tick ≈ 1.085μs, 最大 65535 ticks ≈ 71ms ≈ 12m
//
// 用法：
//   SR04_Poll();           // 主循环每轮调一次 (~10ms), 内部 20ms 采样一次
//   dist = SR04_GetFiltered();  // 取 3 次滑动平均 (mm)
// ============================================================

#define SR04_WIN_SIZE      2    // 滑动平均窗口 (采样3次)
#define SR04_TICK_PERIOD   2    // 采样间隔: 2 × ~10ms = ~20ms

void SR04_Init(void);
void SR04_Poll(void);
u16  SR04_GetFiltered(void);
u16  SR04_GetDistance(void);

#endif
