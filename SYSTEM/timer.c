#include "SYSTEM/timer.h"

// ============================================================
// Timer0 / Timer1 — 通用配置封装
//
// 默认配置：
//   - 模式1 (16位定时器, 需手动重装)
//   - 重装值 0xFC67 → 溢出时间 = (0x10000-0xFC67) × 1.085μs
//     = 977 × 1.085μs ≈ 1.06ms (接近 1ms)
//   - TRx=0 (默认停止), 由调用方按需启动
//
// 注意：这些函数仅设置 TMOD、THx/TLx、ETx、TFx、TRx。
// 不处理 GPIO 复用（不需要），不配中断优先级（用默认）。
//
// 使用模式：
//   Timer0_Init();        // 设为模式1, 1ms 溢出, 未启动
//   Timer0_SetReload(N);  // 可选：改重装值
//   Timer0_Start();       // 启动
//   ...
//   Timer0_Stop();        // 暂停
// ============================================================

void Timer0_Init(void)
{
    TMOD &= 0xF0;        // 清 Timer0 字段 (bit0~3)
    TMOD |= 0x01;        // Timer0 模式1 (16位, 不自动重装)
    TH0 = 0xFC;          // 高字节: ~1ms 溢出
    TL0 = 0x67;          // 低字节
    ET0 = 0;             // 默认关中断（调用方按需开）
    TF0 = 0;             // 清溢出标志
    TR0 = 0;             // 默认不运行
}

void Timer0_Start(void)  { TR0 = 1; }
void Timer0_Stop(void)   { TR0 = 0; }

void Timer0_SetReload(u16 reload)
{
    TH0 = reload >> 8;       // 高 8 位
    TL0 = reload & 0xFF;     // 低 8 位
}

// ============================================================
// Timer1 — 同 Timer0，用于 Timer1 的基础配置
// ============================================================

void Timer1_Init(void)
{
    TMOD &= 0x0F;        // 清 Timer1 字段 (bit4~7)
    TMOD |= 0x10;        // Timer1 模式1 (16位, 不自动重装)
    TH1 = 0xFC;
    TL1 = 0x67;
    ET1 = 0;
    TF1 = 0;
    TR1 = 0;
}

void Timer1_Start(void)  { TR1 = 1; }
void Timer1_Stop(void)   { TR1 = 0; }

void Timer1_SetReload(u16 reload)
{
    TH1 = reload >> 8;
    TL1 = reload & 0xFF;
}
