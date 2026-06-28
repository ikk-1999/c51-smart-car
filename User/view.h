#ifndef __VIEW_H__
#define __VIEW_H__

#include "SYSTEM/sys.h"

// ============================================================
// 此文件已废弃，不再参与编译
//
// 原 View 模块的独立 API：View_Init / View_SetMode /
// View_SetData / View_Refresh。
// 现已被 main.c 中的 static view_task() 替代。
//
// 保留此文件仅作为架构演进参考。
// ============================================================

#if 0
void View_Init(void);
void View_SetMode(view_mode_e mode);
void View_SetData(u16 dist_mm, u8 ir_disp, s8 speed, u16 target_mm);
void View_Refresh(void);
#endif

#endif
