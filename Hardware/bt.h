#ifndef __BT_H__
#define __BT_H__

#include "SYSTEM/sys.h"

// ============================================================
// 蓝牙串口驱动 (HC-05/HC-06 兼容)
//
// 硬件:
//   蓝牙 TXD → MCU P3.0 (RXD)
//   蓝牙 RXD → MCU P3.1 (TXD)
//
// 定时器:
//   Timer1 模式2 (8位自动重装) — UART 波特率 9600
//   TH1 = 0xFD @ 11.0592MHz, SMOD=0
//
// 用法:
//   BT_Init();
//   BT_Poll();                               // 主循环每轮调一次
//   if (BT_GetKey(&num, &is_down)) { ... }   // 按键事件
// ============================================================

void BT_Init(void);                            // 初始化 UART + Timer1 波特率
void BT_Poll(void);                            // 主循环调用, 解析帧
void BT_RX_Handler(void);                      // UART ISR 调用, 收字节

bit  BT_GetKey(u8 *key_num, u8 *is_down);      // 取最新按键事件, 返回 1=有新事件; is_down: 1=按下 0=松开

extern volatile u8 bt_rx_cnt;                  // 调试: 收到字节计数 (volatile: ISR 写入)

#endif
