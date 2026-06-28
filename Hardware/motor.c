#include "Hardware/motor.h"

// ============================================================
// 小车电机驱动 — 直接用 sbit 引脚, 不用结构体
//
// 设计原则:
//   1. 引脚是硬件常量 → 用 sbit, 不存 RAM
//   2. 速度是唯一可变状态 → 两个全局 s8
//   3. duty/pwm_cnt 是 PWM 运行时 → static 局部
//   4. main 直接读写 g_speed_*, 不用传指针
// ============================================================

// ---- 全局速度 ----
s8 g_speed_left  = 0;
s8 g_speed_right = 0;

// ---- PWM 运行时 (ISR 使用) ----
static u8 duty_left  = 0;
static u8 duty_right = 0;
static u8 pwm_cnt    = 0;

// ============================================================
// Motor_Init — 引脚拉低 + Timer2 启动
// ============================================================
void Motor_Init(void)
{
    IN1_M = 0; IN2_M = 0; EN1_M = 0;
    IN3_M = 0; IN4_M = 0; EN2_M = 0;

    // Timer2: 16位自动重装, 200μs 溢出 → 50Hz PWM (100级)
    // 65536 - 184 = 0xFF48 @ 11.0592MHz/12T
    T2CON  = 0x00;
    TH2    = 0xFF;
    TL2    = 0x48;
    RCAP2H = 0xFF;
    RCAP2L = 0x48;
    ET2    = 1;
    TR2    = 1;
    duty_left  = 0;
    duty_right = 0;
    pwm_cnt    = 0;
}


// ============================================================
// Motor_Apply — 把 g_speed_* → 方向引脚 + duty
//
// speed ±0~100 → duty 0~100 (占空比 %), 方向由符号决定
// 在 while(1) 中每轮调一次 (main.c 中 10ms 一次)
// ============================================================
void Motor_Apply(void)
{
    s8 s;

    EA = 0;  // 关总中断 — 防止 PWM ISR 在更新中途改 EN 引脚

    // ---- 右轮 ----
    s = g_speed_right;
    duty_right = (s >= 0) ? (u8)s : (u8)(-s);
    if (s > 0)           // 前进: I3=1 IN4=0
    {
        IN1_M = 0; IN2_M = 1;
    }
    else if (s < 0)      // 后退: IN1=1 IN2=0
    {
        IN1_M = 1; IN2_M = 0;
    }
    else                 // 停: 刹车 (EN=1, IN1=IN2=0)
    {
        IN1_M = 0; IN2_M = 0; EN1_M = 1;
    }

    // ---- 左轮 (极性与右轮相反!) ----
    s = g_speed_left;
    duty_left = (s >= 0) ? (u8)s : (u8)(-s);
    if (s > 0)           // 前进: IN1=1 IN2=0
    {
        IN3_M = 1; IN4_M = 0;
    }
    else if (s < 0)      // 后退: IN3=0 IN4=1
    {
        IN3_M = 0; IN4_M = 1;
    }
    else                 // 停: 刹车 (EN=1, IN3=IN4=0)
    {
        IN3_M = 0; IN4_M = 0; EN2_M = 1;
    }

    EA = 1;  // 开总中断
}


// ============================================================
// Motor_PWM_Handler — Timer2 ISR 调用, 每 200μs
//
// 100 级 PWM: pwm_cnt 0→99 循环
//   pwm_cnt < duty → EN=1 (通电)
//   pwm_cnt >= duty → EN=0 (断电)
//   duty = 0~100, 直接对应 0%~100% 占空比
// ============================================================
void Motor_PWM_Handler(void)
{
    TF2 = 0;

    if (++pwm_cnt >= 100)
        pwm_cnt = 0;
    EN1_M = (duty_right != 0 && pwm_cnt < duty_right);
    EN2_M = (duty_left  != 0 && pwm_cnt < duty_left);

}
