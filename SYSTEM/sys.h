#ifndef __SYS_H__
#define __SYS_H__

#include <reg52.h>

// ============================================================
// 系统层：基础类型 + 时钟 + 位操作 + 引脚映射 + 中断封装
// 所有模块只需要 include 这一个头文件即可获得全部底层支撑
// ============================================================

// ---------- 1. 基础类型别名 ----------
typedef unsigned char  u8;
typedef unsigned int   u16;
typedef unsigned long  u32;
typedef signed   char  s8;
typedef signed   int   s16;
typedef signed   long  s32;

#define uchar unsigned char
#define uint  unsigned int
#define ulong unsigned long

// ---------- 2. 系统时钟 ----------
#define FOSC 11059200L    // 11.0592MHz

// ---------- 3. 通用位操作宏 ----------
#define SET_BIT(reg, bit)    ((reg) |=  (1 << (bit)))
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(1 << (bit)))
#define TOGGLE_BIT(reg, bit) ((reg) ^=  (1 << (bit)))
#define READ_BIT(reg, bit)   (((reg) >> (bit)) & 0x01)

// ---------- 4. 引脚映射 ----------
// P1 口：8 个 LED（低电平点亮）
#define LED_PORT  P1

// P3 口：独立按键 K1-K4（按下为低电平）
sbit K1_PIN = P3^4;
sbit K2_PIN = P3^5;
sbit K3_PIN = P3^6;
sbit K4_PIN = P3^7;

// P2^6 / P2^7：数码管锁存控制
sbit SMG_SEG_LATCH = P2^6;   // 段选锁存（原例程 dula）
sbit SMG_DIG_LATCH = P2^7;   // 位选锁存（原例程 wela）

// P2^3：蜂鸣器（可选）
sbit BEEP_PIN = P2^3;

// P1^0 / P1^1 / P2^5：LCD1602 液晶
sbit LCD_RS = P1^0;
sbit LCD_RW = P1^1;
sbit LCD_EN = P2^5;
#define LCD_DATA  P0

// P3^2：红外接收头（INT0）
sbit IR_PIN = P3^2;

// P2^1/P2^0：超声波 HC-SR04
sbit SR04_TRIG = P2^1;
sbit SR04_ECHO = P2^0;

// P1.2~P1.7：电机驱动 (J3 排线)
sbit IN1_M = P1^2;    // 右电机 IN1
sbit IN2_M = P1^3;    // 右电机 IN2
sbit EN1_M = P1^4;    // 右电机 EN1 (PWM)
sbit EN2_M = P1^5;    // 左电机 EN2 (PWM)
sbit IN3_M = P1^6;    // 左电机 IN3
sbit IN4_M = P1^7;    // 左电机 IN4

// ---------- 5. 中断控制宏 ----------
#define INT_ENABLE_GLOBAL()   (EA = 1)
#define INT_DISABLE_GLOBAL()  (EA = 0)

#define INT0_ENABLE()   (EX0 = 1)
#define INT0_DISABLE()  (EX0 = 0)
#define INT0_SET_EDGE()  (IT0 = 1)
#define INT0_SET_LEVEL() (IT0 = 0)

#define INT1_ENABLE()   (EX1 = 1)
#define INT1_DISABLE()  (EX1 = 0)
#define INT1_SET_EDGE()  (IT1 = 1)
#define INT1_SET_LEVEL() (IT1 = 0)

#define TIM0_INT_ENABLE()   (ET0 = 1)
#define TIM0_INT_DISABLE()  (ET0 = 0)
#define TIM1_INT_ENABLE()   (ET1 = 1)
#define TIM1_INT_DISABLE()  (ET1 = 0)

#define UART_INT_ENABLE()   (ES  = 1)
#define UART_INT_DISABLE()  (ES  = 0)
#define TIM2_INT_ENABLE()   (ET2 = 1)
#define TIM2_INT_DISABLE()  (ET2 = 0)

// ---------- 6. 函数声明 ----------
void Sys_Init(void);



// 变量声明
typedef enum{
    VIEW_MODE_MENU = 0,   // 主菜单模式
    VIEW_MODE_CHECK,      // 硬件检测模式
    VIEW_MODE_AVD,        // 超声波避障模式
    VIEW_MODE_DIST,       // 超声波定距离模式
    VIEW_MODE_LANYA,      // 蓝牙控制模式 (预留)
    VIEW_MODE_MAX

}view_mode_e;

#endif
