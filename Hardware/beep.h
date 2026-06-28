#ifndef __BEEP_H__
#define __BEEP_H__

#include "SYSTEM/sys.h"

// ============================================================
// 蜂鸣器驱动（P2.3，低电平响、高电平停）
// 极简版 — 只保留 On/Off
// ============================================================

void Beep_Init(void);   // 初始化
void Beep_On(void);     // 直接响（低电平）
void Beep_Off(void);    // 直接停

#endif
