#include "Hardware/bt.h"

// STC89C52 辅助寄存器 (reg52.h 未包含)
sfr AUXR = 0x8E;  // AUXR.6=T1x12: Timer1 1T/12T 模式; AUXR.0=S1BRS

// ============================================================
// 蓝牙串口驱动
//
// 环形缓冲 + 帧解析:
//   收到 [key,N,down] → 按键 N 按下
//   收到 [key,N,up]   → 按键 N 松开
// ============================================================

// ---- 环形缓冲 ----
#define RX_BUF_SIZE  64
static idata volatile u8  rx_buf[RX_BUF_SIZE];
static volatile u8  rx_wr;      // ISR 写指针 — volatile: ISR/主循环共享
static volatile u8  rx_rd;      // 主循环读指针 — volatile: ISR/主循环共享
volatile u8  bt_rx_cnt;          // 调试: 收到的字节总数 (extern 到 main)

// ---- 最新解析结果 ----
static u8   key_num;             // 按键编号
static bit  key_new;             // 有新按键事件
static bit  key_down;            // 1=按下, 0=松开

// ============================================================
// BT_Init — 初始化 UART + Timer1 波特率
//
// SCON=0x50: 模式1 (8位UART), REN=1 (允许接收)
// Timer1 模式2 (8位自动重装), TH1=0xFD → 9600bps @11.0592MHz
// ============================================================
void BT_Init(void)
{
    PCON  = 0x00;          // 确保 SMOD=0, 波特率不倍频
    AUXR  = 0x00;          // 确保 T1x12=0 (12T模式), S1BRS=0 (Timer1做波特率源)
    SCON  = 0x50;          // 模式1, 8位UART, 允许接收
    TMOD &= 0x0F;          // 清 Timer1 (bit4~7)
    TMOD |= 0x20;          // 模式2: 8位自动重装
    TH1   = 0xFD;          // 9600 @ 11.0592MHz (SMOD=0)
    TL1   = 0xFD;
    TR1   = 1;             // 启动 Timer1
    ES    = 1;             // 开串口中断

    rx_wr   = 0;
    rx_rd   = 0;
    key_new = 0;
}

// ============================================================
// BT_RX_Handler — UART ISR 调用, 收字节入环形缓冲
// ============================================================
void BT_RX_Handler(void)
{
    u8 next;

    bt_rx_cnt++;                           // 调试计数
    rx_buf[rx_wr] = SBUF;
    next = rx_wr + 1;
    if (next >= RX_BUF_SIZE) next = 0;
    if (next != rx_rd)
        rx_wr = next;
}

// ============================================================
// BT_Poll — 主循环调用, 从环形缓冲读字节并解析帧
//
// 帧格式:
//   [key,N,down] → 按键按下
//   [key,N,up]   → 按键松开
// ============================================================
void BT_Poll(void)
{
    u8  ch;
    static u8  i;           // static! 跨 BT_Poll 调用保持帧位置
    u8  buf[32];            // 当前帧缓冲区
    u8  k_num;
    bit is_down;

    while (rx_wr != rx_rd)
    {
        ch = rx_buf[rx_rd];
        rx_rd++;
        if (rx_rd >= RX_BUF_SIZE) rx_rd = 0;

        if (ch == '[')
        {
            i = 0;
        }
        else if (ch == ']')
        {
            buf[i] = '\0';

            // 匹配 [key,N,down] 或 [key,N,up]
            if (buf[0] == 'k' && buf[1] == 'e' && buf[2] == 'y')
            {
                k_num = 0;
                i = 4;    // 跳过 "key,"
                while (buf[i] >= '0' && buf[i] <= '9')
                {
                    k_num = k_num * 10 + (buf[i] - '0');
                    i++;
                }

                is_down = (buf[i+1] == 'd');  // 跳过逗号, 看 'd'own 还是 'u'p

                key_num  = k_num;
                key_down = is_down;
                key_new  = 1;
            }
        }
        else if (i < 31)
        {
            buf[i++] = ch;
        }
    }
}

// ============================================================
// BT_GetKey — 取最新按键事件, 返回 1=有新事件
//
// 参数:
//   num     — [out] 按键编号
//   is_down — [out] 1=按下, 0=松开
// ============================================================
bit BT_GetKey(u8 *num, u8 *is_down)
{
    if (key_new)
    {
        *num     = key_num;
        *is_down = key_down;
        key_new  = 0;
        return 1;
    }
    return 0;
}
