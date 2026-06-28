#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "SYSTEM/sys.h"

// ============================================================
// 小车电机驱动 — 简洁版
//
// 引脚 (已在 sys.h 中定义, J3 排线):
//   左: EN1_M=P1.4  IN1_M=P1.2  IN2_M=P1.3
//   右: EN2_M=P1.5  IN3_M=P1.6  IN4_M=P1.7
//   右轮极性相反: IN3=1,IN4=0 才是正转
//
// 用法:
//   g_speed_left  =  50;   // 左轮前进 50% 占空比
//   g_speed_right = -30;   // 右轮后退 30% 占空比
//   Motor_Apply();          // 生效到硬件
//
// speed 语义: +前进 -后退 0停, 绝对值 0~100 = PWM 占空比 0%~100%
// ============================================================

#define MOTOR_SPEED_MAX   100
#define MOTOR_SPEED_STEP   10

// ---- 全局速度变量 (主循环直接读写) ----
extern s8 g_speed_left;
extern s8 g_speed_right;

// ---- API ----
void Motor_Init(void);           // 初始化引脚 + Timer2
void Motor_Apply(void);          // 把 g_speed_* 生效到硬件方向 + PWM
void Motor_PWM_Handler(void);    // Timer2 ISR (200μs, 100级PWM)

#endif
