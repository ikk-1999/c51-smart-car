#include "Hardware/led.h"
#include "SYSTEM/delay.h"

// ============================================================
// LED 驱动实现 (8 个 LED 接 P1 口, 低电平点亮)
//
// 原理：
//   P1.n = 0 → LED 亮 (VCC → 限流电阻 → LED → P1.n → GND)
//   P1.n = 1 → LED 灭 (两端无压差)
//
// LED 编号约定：LED1=P1.0, LED2=P1.1, ... LED8=P1.7
//   参数 led_num ∈ [1, 8], 非法值会被静默忽略
// ============================================================

void LED_Init(void)
{
    LED_PORT = 0xFF;    // 全灭 (所有位拉高)
}

void LED_On(u8 led_num)
{
    if (led_num < 1 || led_num > 8) return;   // 边界保护
    LED_PORT &= ~(1 << (led_num - 1));         // 对应位清零 → 亮
}

void LED_Off(u8 led_num)
{
    if (led_num < 1 || led_num > 8) return;
    LED_PORT |= (1 << (led_num - 1));          // 对应位置1 → 灭
}

void LED_Toggle(u8 led_num)
{
    if (led_num < 1 || led_num > 8) return;
    LED_PORT ^= (1 << (led_num - 1));          // 翻转对应位
}

// ============================================================
// LED_Set — 直接写 P1 口 (0=亮, 1=灭)
// 例: LED_Set(0xF0) → LED1~4 亮, LED5~8 灭
// ============================================================
void LED_Set(u8 val)
{
    LED_PORT = val;
}

// ============================================================
// LED_Flow — 跑马灯效果 (从左到右, 每个 150ms)
// 阻塞 8×150ms = 1.2s，仅用于调试/演示
// ============================================================
void LED_Flow(void)
{
    u8 i;
    for (i = 0; i < 8; i++)
    {
        LED_PORT = ~(1 << i);    // 逐个点亮
        delay_ms(150);
    }
}

// ============================================================
// LED_Blink — 单灯闪烁一次 (亮→灭)
// 阻塞 2×ms，仅用于调试/演示
// ============================================================
void LED_Blink(u8 led_num, u16 ms)
{
    LED_On(led_num);
    delay_ms(ms);
    LED_Off(led_num);
    delay_ms(ms);
}
