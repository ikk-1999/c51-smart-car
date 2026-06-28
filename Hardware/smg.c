#include "Hardware/smg.h"

// ============================================================
// 数码管驱动实现 — 6位共阴数码管 (2片 74HC573 锁存)
//
// 硬件 (HL-1 开发板)：
//   P0[7:0]  → 段选 → 74HC573(段) 的 D0-D7
//   P2.6     → 段锁存 (LE, 上升沿锁存)
//   P2.7     → 位锁存
//   P0[7:0]  → 位选 → 74HC573(位) 的 D0-D7 → 6 个共阴选通
//
// 段码约定（共阴）：
//   段顺序: a,b,c,d,e,f,g,dp  (dp=bit7, a=bit0)
//   1 → 段亮, 0 → 段灭
//   小数点由段码 | 0x80 实现
//
// 动态扫描：
//   每次 SMG_Refresh() 只显示 1 位，6次走完一整屏
//   刷新率 = 主循环频率 / 6，建议 ≥ 50Hz → 主循环 ≤ 3.3ms
//   当前主循环 ≈ 10ms → 刷新率 ≈ 16.7Hz → 略有闪烁但可接受
// ============================================================

// 位选表 — P0 输出值，共 6 位（低电平有效）
// pos_table[0]=0xFE → P0.0=0 → 位0选通，其余高阻
static u8 code pos_table[6] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF};

// 共阴段码表 — 前 16 个: 0~9, A~F, 第 17 个: 全灭
// 索引 = SEG_BLANK (16) → 0x00 (全灭)
static u8 code seg_table[17] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D,   // 0~6
    0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E,   // 7~D
    0x79, 0x71, 0x00                              // E, F, 全灭
};

// 显示缓冲区 — 存的是段码 (不是数字)，6 字节对应 6 个数码管位
static u8 smg_buf[6];

// 扫描指针 — 当前正在显示第几位 (0~5 循环)
static u8 smg_scan_pos = 0;

// ============================================================
// SMG_Init — 初始化锁存器 + 清缓冲区
//
// 74HC573: 锁存器 LE 上升沿 → 锁存 D 输入 → Q 输出
// 写 P0 → 锁存 → P0 可用于下一个功能 (段选/位选复用)
// ============================================================
void SMG_Init(void)
{
    SMG_SEG_LATCH = 0;   // LE=低 → 锁存器透明 (输入直接到输出)
    SMG_DIG_LATCH = 0;

    P0 = 0x00;           // 段选全灭 (共阴: 0=灭)
    SMG_SEG_LATCH = 1;   // 上升沿 → 锁存段选数据
    SMG_SEG_LATCH = 0;

    P0 = 0xFF;           // 位选全关 (1=不选通)
    SMG_DIG_LATCH = 1;   // 上升沿 → 锁存位选数据
    SMG_DIG_LATCH = 0;

    SMG_ClearBuf();      // 缓冲区全填空白
    smg_scan_pos = 0;    // 从第 0 位开始扫描
}

// ============================================================
// SMG_ClearBuf — 清空显示缓冲区 (全部空白)
// ============================================================
void SMG_ClearBuf(void)
{
    u8 i;
    for (i = 0; i < 6; i++)
        smg_buf[i] = seg_table[SEG_BLANK];
}

// ============================================================
// SMG_SetNum — 在指定位置显示数字 0~9
// ============================================================
void SMG_SetNum(u8 pos, u8 num)
{
    if (pos > 5 || num > 9) return;
    smg_buf[pos] = seg_table[num];
}

// ============================================================
// SMG_SetDP — 给指定位置加小数点 (共阴: |0x80)
// ============================================================
void SMG_SetDP(u8 pos)
{
    if (pos > 5) return;
    smg_buf[pos] |= 0x80;
}

// ============================================================
// SMG_SetBlank — 指定位置不显示
// ============================================================
void SMG_SetBlank(u8 pos)
{
    if (pos > 5) return;
    smg_buf[pos] = seg_table[SEG_BLANK];
}

// ============================================================
// SMG_Refresh — 动态扫描一位（非阻塞，每次点亮 1 位）
//
// 调用频率决定刷新率：
//   主循环 ~10ms → 刷新率 = 1000/(10×6) ≈ 16.7Hz
//   人眼可察觉 16Hz 的闪烁 → 建议加 1-2ms 的延时改善
//   或者将主循环优化到 < 5ms
//
// 时序：
//   1. 段选打开 → P0=段码 → 段锁存 → 段选关闭
//   2. 位选打开 → P0=位码 → 位锁存 → 位选关闭
//   3. 指针 +1，指向下一位
// ============================================================
void SMG_Refresh(void)
{
    // --- 消隐：先关段选（防"鬼影"）---
    P0 = 0x00;
    SMG_SEG_LATCH = 1; SMG_SEG_LATCH = 0;

    // --- 位选：选通第 smg_scan_pos 位 ---
    P0 = pos_table[smg_scan_pos];
    SMG_DIG_LATCH = 1; SMG_DIG_LATCH = 0;

    // --- 段选：输出当前位的段码 ---
    P0 = smg_buf[smg_scan_pos];
    SMG_SEG_LATCH = 1; SMG_SEG_LATCH = 0;

    // --- 指针移到下一位 ---
    smg_scan_pos++;
    if (smg_scan_pos >= 6)
        smg_scan_pos = 0;
}

// ============================================================
// SMG_Off — 全部熄灯
// ============================================================
void SMG_Off(void)
{
    P0 = 0x00;
    SMG_SEG_LATCH = 1; SMG_SEG_LATCH = 0;

    P0 = 0xFF;
    SMG_DIG_LATCH = 1; SMG_DIG_LATCH = 0;
}

// ============================================================
// SMG_ShowDist — 超声波距离 (mm → XX.X cm 格式)
// 显示在位5(十位)、位4(个位+DP)、位3(小数位)
// 例: 115mm → "11.5"  (位5=1, 位4=1(DP), 位3=5)
//      85mm → " 8.5"  (位5消隐, 位4=8(DP), 位3=5)
//       0mm → " 0.0"
// 最大显示: "99.9" (即 999mm, 约 1m)
// ============================================================
void SMG_ShowDist(u16 mm)
{
    u8 tens, ones, frac;

    if (mm == 0)
    {
        SMG_SetBlank(SMG_POS_DIST_HUND);
        SMG_SetNum(SMG_POS_DIST_TENS, 0);
        SMG_SetDP(SMG_POS_DIST_TENS);
        SMG_SetNum(SMG_POS_DIST_ONES, 0);
        return;
    }

    if (mm > 999) mm = 999;

    tens = (u8)((mm / 10) / 10);     // cm 十位 (115mm→11cm→1)
    ones = (u8)((mm / 10) % 10);     // cm 个位 (115mm→11cm→1)
    frac = (u8)(mm % 10);            // mm 个位 = 小数位 (115mm→5)

    if (tens == 0)
        SMG_SetBlank(SMG_POS_DIST_HUND);
    else
        SMG_SetNum(SMG_POS_DIST_HUND, tens);

    SMG_SetNum(SMG_POS_DIST_TENS, ones);
    SMG_SetDP(SMG_POS_DIST_TENS);

    SMG_SetNum(SMG_POS_DIST_ONES, frac);
}

// ============================================================
// SMG_ShowCount — 计数器 (0~99), 显示在位0/位1
// - 十位 0 时消隐 (如 5 → " 5" 而不是 "05")
// ============================================================
void SMG_ShowCount(u8 count)
{
    u8 tens = count / 10;
    u8 ones = count % 10;

    if (tens == 0)
        SMG_SetBlank(SMG_POS_CNT_TENS);
    else
        SMG_SetNum(SMG_POS_CNT_TENS, tens);

    SMG_SetNum(SMG_POS_CNT_ONES, ones);
}

// ============================================================
// SMG_ShowIR — 红外键值 (0~9), 显示在位2
// - 无效值 (>9) → 消隐
// ============================================================
void SMG_ShowIR(u8 cmd)
{
    if (cmd > 9)
        SMG_SetBlank(SMG_POS_IR);
    else
        SMG_SetNum(SMG_POS_IR, cmd);
}
