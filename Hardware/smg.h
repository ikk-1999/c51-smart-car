#ifndef __SMG_H__
#define __SMG_H__

#include "SYSTEM/sys.h"

// ============================================================
// 数码管驱动 — 6位共阴（P0段选, P2.6段锁存, P2.7位锁存）
//
// 显示布局（位0~位5）：
//   [5]      [4]      [3]      [2]      [1]      [0]
//   超声波百位  超声波十位.  超声波个位  红外键值   计数十位   计数个位
//   (位4带小数点)
//
// 用法：
//   SMG_Init();
//   while(1) {
//       SMG_Refresh();   // 每轮刷一位（非阻塞，无需 delay）
//   }
// ============================================================

// ---------- 显示位编号 ----------
#define SMG_POS_CNT_ONES   0   // 计数器个位
#define SMG_POS_CNT_TENS   1   // 计数器十位
#define SMG_POS_IR         2   // 红外键值
#define SMG_POS_DIST_ONES  3   // 超声波距离个位
#define SMG_POS_DIST_TENS  4   // 超声波距离十位 (带小数点)
#define SMG_POS_DIST_HUND  5   // 超声波距离百位

// ---------- 段码索引 ----------
#define SEG_BLANK    16       // 空白 (全灭)
#define SEG_COUNT    17       // 段码表长度

// ---------- 函数接口 ----------
void SMG_Init(void);
void SMG_ClearBuf(void);          // 清空全部缓冲区
void SMG_SetNum(u8 pos, u8 num);  // 在指定位置显示数字 0-9
void SMG_SetDP(u8 pos);           // 给指定位置加小数点
void SMG_SetBlank(u8 pos);        // 指定位置不显示
void SMG_Refresh(void);           // 刷新一位（每轮主循环调一次）
void SMG_Off(void);               // 全部熄灯

// ---------- 便捷函数 ----------
void SMG_ShowDist(u16 mm);        // 超声波距离 (mm → 显示为 XX.X cm)
void SMG_ShowCount(u8 count);     // 计数器 (0-99)
void SMG_ShowIR(u8 cmd);          // 红外键值 (0-9)

#endif
