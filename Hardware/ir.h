#ifndef __IR_H__
#define __IR_H__

#include "SYSTEM/sys.h"

// ============================================================
// 红外遥控驱动（NEC 协议 / TC9012 兼容）
//
// 硬件连接：
//   IR_PIN = P3^2 (INT0) — 红外接收头 VS1838 输出
//     接收头三脚: VCC(5V), GND, OUT(→P3.2)
//     平时高电平，收到红外信号时输出低电平脉冲
//
// 使用的定时器：
//   Timer0 模式2 (8位自动重装, TH0=TL0=0)
//   → 每 256 个机器周期 ≈ 278μs 溢出一次
//   → 这就是 irtime 的时间单位
//
// NEC 协议时间基准（供参考，不直接参与计算）：
//   引导码：9ms 低 + 4.5ms 高 = 33+16 ≈ 49 个 irtime 单位
//   逻辑 0：560μs 低 + 560μs 高  ≈ 2+2 = 4 个单位
//   逻辑 1：560μs 低 + 1690μs 高 ≈ 2+6 = 8 个单位
//   重复码：9ms 低 + 2.25ms 高 + 560μs 停（当前未处理）
//
// 解码得到的 4 字节（小端序传输）：
//   IRcord[0]  = 地址码       (例如 0x00)
//   IRcord[1]  = 地址反码     (例如 0xFF)
//   IRcord[2]  = 命令码       ← 按键标识，我们用的就是这个
//   IRcord[3]  = 命令反码
//
// 返回值语义 (Ir_GetCmd)：
//   - 返回 0：无新数据（可能没收到、校验失败、或还没收齐）
//   - 返回非 0：上次成功解码的 NEC 命令码
//
// 已知局限：
//   - 重复码（长按）未处理 → 需要松手再按才能再次响应
//   - Ircordpro 与 ISR 共享数组 → 有数据竞争风险（见 ir.c 注释）
// ============================================================

// ---------- NEC 引导码判断阈值 (基于 ~278μs 的 irtime 单位) ----------
// 引导码前半段 9ms  ≈ 33 个单位
// 引导码全长    13.5ms ≈ 49 个单位
// 这里用 [33, 63) 作为有效范围，排除杂波和超长脉冲
#define IR_LEAD_MIN  33
#define IR_LEAD_MAX  63

// ---------- 位值判断阈值 ----------
// 逻辑 0 的 pulse 约 2 个单位，逻辑 1 约 6 个单位 → 选中间值 7
#define IR_BIT_THRESHOLD  7    // cord > 7 则判决为 '1'，≤7 为 '0'

// ---------- Ir_GetCmd 返回值语义 ----------
// 所有已知 NEC 命令码均为非零值，所以用 0 表示"无数据"
// 用常量名显式表达这个约定，避免与"数字键 0"混淆
#define IR_CMD_NONE  0

// ============================================================
// 红外遥控器键码表 (HL-1 配套 NEC 遥控器)
//
// 注意：不同批次的遥控器键码可能略有差异，如有按键不响应，
// 请用串口打印 IRcord[2] 确认实际键码并修改下表。
// ============================================================

// --- 数字键 ---
#define IR_KEY_0       0x16
#define IR_KEY_1       0x0C
#define IR_KEY_2       0x18
#define IR_KEY_3       0x5E
#define IR_KEY_4       0x08
#define IR_KEY_5       0x1C
#define IR_KEY_6       0x5A
#define IR_KEY_7       0x42
#define IR_KEY_8       0x52
#define IR_KEY_9       0x4A

// --- 功能键 ---
#define IR_KEY_POWER     0x45   // 电源 (红色)
#define IR_KEY_MENU      0x46   // 菜单
#define IR_KEY_MUTE      0x47   // 静音
#define IR_KEY_MODE      0x44   // 模式
#define IR_KEY_FORWARD   0x40   // ▲ 前进 (方向键上)
#define IR_KEY_BACK      0x43   // 返回
#define IR_KEY_LEFT      0x07   // ◄ 左转 (方向键左)
#define IR_KEY_PAUSE     0x15   // ⏸ 暂停 (||)
#define IR_KEY_BACKWARD  0x19   // ▼ 后退 (方向键下)
#define IR_KEY_RIGHT     0xD9   // ► 右转 (方向键右)
#define IR_KEY_OK        0x0D   // OK (中心键)

// ---------- 函数接口 ----------
void Ir_Init(void);         // 初始化 (配置 INT0 + Timer0 模式2)
u8   Ir_GetCmd(void);       // 非阻塞读最新命令码 (0=无数据)
u8   Ir_CmdToNum(u8 cmd);   // 命令码 → 数字 0-9 (非法返回 0xFF)

#endif
