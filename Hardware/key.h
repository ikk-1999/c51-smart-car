#ifndef __KEY_H__
#define __KEY_H__

#include "SYSTEM/sys.h"

// ============================================================
// 独立按键驱动（K1~K4，P3.4~P3.7，低电平有效）
//
// 用法示例：
//   KeyCode_t key = Key_Scan();
//   switch (key) {
//       case KEY_K1: ... break;
//       case KEY_K2: ... break;
//       ...
//   }
// ============================================================

// ---------- 按键键值枚举 ----------
// 使用 enum 而不是 #define，好处：
// 1. 类型安全：Key_Scan 返回 KeyCode_t，编译器帮你检查
// 2. main.c 不需要看到任何 KEY_NONE=0 这种宏定义
// 3. switch 语句 IDE 可以提示漏了哪个 case
typedef enum {
    KEY_NONE = 0,   // 无按键
    KEY_K1   = 1,   // P3.4
    KEY_K2   = 2,   // P3.5
    KEY_K3   = 3,   // P3.6
    KEY_K4   = 4,   // P3.7
} KeyCode_t;

// ---------- 函数接口 ----------
void      Key_Init(void);      // 初始化按键引脚
KeyCode_t Key_Scan(void);      // 非阻塞扫描（推荐主循环调用）

#endif
