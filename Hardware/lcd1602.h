#ifndef __LCD1602_H__
#define __LCD1602_H__

#include "SYSTEM/sys.h"

// ============================================================
// LCD1602 字符液晶驱动 (16×2)
//
// 接线：
//   RS  = P1^0    RW  = P1^1    EN  = P2^5
//   D0~D7 = P0    对比度 = 电位器
//
// 用法：
//   LCD_Init();
//   LCD_Clear();
//   LCD_WriteString(0, 0, "Dist: 12.3 cm");
//   LCD_WriteString(0, 1, "IR:5  Count:56");
// ============================================================

#define LCD_LINE1  0
#define LCD_LINE2  1

void LCD_Init(void);
void LCD_Clear(void);
void LCD_ClearLine(u8 y);
void LCD_WriteChar(u8 x, u8 y, u8 ch);
void LCD_WriteString(u8 x, u8 y, const u8 *str);
void LCD_SetCursor(u8 x, u8 y);

// 便捷函数 — 格式化显示
void LCD_ShowU16(u8 x, u8 y, u16 num, u8 digits);  // 显示无符号整数
void LCD_ShowDist(u8 x, u8 y, u16 mm);              // 距离 XX.X cm
void LCD_ShowCount(u8 x, u8 y, u8 count);           // 计数器 (2位)

// 光标位置读取
void lcd_get_cursor(u8 *x, u8 *y);

// 类 printf 格式化函数
// 格式符: %d(有符号), %u(无符号), %s(字符串), %c(字符), %%(%字面值)
// 控制符: %Px,y(光标跳到(x,y),不消耗参数)
//
// 用法:
//   lcdprintf(0, 0, "dis=%d.%u cm", cm, frac);
//   lcdprintf(0, 1, "ir=0x%X  spd=%d", ir, spd);
void lcdprintf(u8 x, u8 y, const char *fmt, ...);

#endif
