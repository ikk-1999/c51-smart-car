#include "Hardware/lcd1602.h"
#include <intrins.h>

// ============================================================
// LCD1602 驱动实现
//
// 参考 HL-1 开发板例程 32/33/34
//
// 注意事项：
//   1. LCD 是慢设备，操作后需等待 (busy flag 或延时)
//   2. P0 口被 LCD 独占使用，不可与数码管同时驱动
//   3. 上电后等 400ms 再初始化 (LCD 模块需要稳定时间)
//   4. 对比度电位器需手动调节
// ============================================================

// ---------- 内部函数 ----------
static void     LCD_DelayUs(unsigned char t);
static void     LCD_DelayMs(unsigned char t);
static bit      LCD_CheckBusy(void);
static void     LCD_WriteCom(unsigned char com);
static void     LCD_WriteData(unsigned char dat);

// ============================================================
// 微秒延时 (约 t×2+5 μs @ 12MHz, 11.0592MHz 相近)
// ============================================================
static void LCD_DelayUs(unsigned char t)
{
    while (--t);
}

// ============================================================
// 毫秒延时
// ============================================================
static void LCD_DelayMs(unsigned char t)
{
    while (t--)
    {
        LCD_DelayUs(245);
        LCD_DelayUs(245);
    }
}

// ============================================================
// 查忙 (BF=bit7, 0=空闲 1=忙)
//
// 注意: 必须把 EN 拉回低电平，否则后续写操作缺上升沿
//       仿真软件对此极其敏感。
// ============================================================
static bit LCD_CheckBusy(void)
{
    bit result;
    LCD_DATA = 0xFF;
    LCD_RS = 0;
    LCD_RW = 1;
    LCD_EN = 0;
    _nop_();
    LCD_EN = 1;
    _nop_();
    result = (bit)(LCD_DATA & 0x80);
    LCD_EN = 0;          // ← 关键：读完后 EN 必须回低
    return result;
}

// ============================================================
// 写命令
//
// 标准 EN 脉冲: LOW → 放数据 → HIGH → 等 → LOW
// 与 1602-miaobiao(实验36) 同一时序，仿真通过
// ============================================================
static void LCD_WriteCom(unsigned char com)
{
    while (LCD_CheckBusy());  // 等不忙 (如今 EN 会回到低电平)
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_EN = 0;              // ① 确保 EN 为低
    _nop_();
    _nop_();
    LCD_DATA = com;          // ② EN 低电平时放数据
    _nop_();
    _nop_();
    LCD_EN = 1;              // ③ 上升沿 → LCD 锁存命令
    _nop_();
    _nop_();
    LCD_EN = 0;              // ④ 下降沿 → 完成一个完整脉冲
    LCD_DelayUs(10);
}

// ============================================================
// 写数据
//
// 标准 EN 脉冲: LOW → 放数据 → HIGH → 等 → LOW
// ============================================================
static void LCD_WriteData(unsigned char dat)
{
    while (LCD_CheckBusy());  // 等不忙
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_EN = 0;              // ① 确保 EN 为低
    _nop_();
    _nop_();
    LCD_DATA = dat;          // ② EN 低电平时放数据
    _nop_();
    _nop_();
    LCD_EN = 1;              // ③ 上升沿 → LCD 锁存数据
    _nop_();
    _nop_();
    LCD_EN = 0;              // ④ 下降沿 → 完成一个完整脉冲
    LCD_DelayUs(10);
}

// ============================================================
// LCD_Init — 初始化 (上电后至少等 400ms 再调用)
// ============================================================
void LCD_Init(void)
{
    // 上电延时让 LCD 稳定
    LCD_DelayMs(15);

    LCD_WriteCom(0x38);   // 8位总线, 2行, 5×7点阵
    LCD_DelayMs(5);
    LCD_WriteCom(0x38);
    LCD_DelayMs(5);
    LCD_WriteCom(0x38);
    LCD_DelayMs(5);

    LCD_WriteCom(0x38);   // 最后确认一次
    LCD_WriteCom(0x08);   // 关显示
    LCD_WriteCom(0x01);   // 清屏
    LCD_WriteCom(0x06);   // 光标右移, 屏幕不滚动
    LCD_DelayMs(5);
    LCD_WriteCom(0x0C);   // 开显示, 无光标, 不闪烁
}

// ============================================================
// LCD_Clear — 清屏
// ============================================================
void LCD_Clear(void)
{
    LCD_WriteCom(0x01);
    LCD_DelayMs(5);
}

// ============================================================
// LCD_ClearLine — 清空指定行 (填 16 个空格, 光标回行首)
//
// 用途: 模式切换时清除旧内容, 比全屏 Clear 更快
// ============================================================
void LCD_ClearLine(u8 y)
{
    u8 i;
    LCD_SetCursor(0, y);
    for (i = 0; i < 16; i++)
        LCD_WriteData(' ');
    LCD_SetCursor(0, y);
}

// ============================================================
// LCD_SetCursor — 设置光标位置
// x: 0~15 (列), y: 0=第一行, 1=第二行
// ============================================================
void LCD_SetCursor(u8 x, u8 y)
{
    u8 addr;

    x &= 0x0F;
    y &= 0x01;

    if (y == 0)
        addr = 0x80 + x;    // 第一行
    else
        addr = 0xC0 + x;    // 第二行

    LCD_WriteCom(addr);
}

// ============================================================
// LCD_WriteChar — 在指定位置写一个字符
// ============================================================
void LCD_WriteChar(u8 x, u8 y, u8 ch)
{
    LCD_SetCursor(x, y);
    LCD_WriteData(ch);
}

// ============================================================
// LCD_WriteString — 在指定位置写字符串 (以 '\0' 结尾)
// ============================================================
void LCD_WriteString(u8 x, u8 y, const u8 *str)
{
    LCD_SetCursor(x, y);

    while (*str)
    {
        if (x > 15) break;               // 超出屏幕宽度
        LCD_WriteData(*str);
        str++;
        x++;
    }
}

// ----------------------------------------------------------
// 以下函数当前未被 main.c 调用，用 #if 0 禁用以释放 DATA 空间
// 如需使用，去掉 #if 0 ... #endif 即可
// ----------------------------------------------------------
#if 0

// ============================================================
// LCD_ShowU16 — 显示无符号整数（右对齐, 不足补空格, 无前导零）
//
// 例: num=7, digits=2 → " 7"
//     num=42, digits=4 → "  42"
//     num=345, digits=2 → "45" (高位被截断!)
//
// ⚠️ digits 必须 ≤ 5，否则 buf[] 越界 (当前调用方都 ≤ 2, 安全)
// ============================================================
void LCD_ShowU16(u8 x, u8 y, u16 num, u8 digits)
{
    u8 buf[6] = "     ";     // 5 位空格 + 末尾 '\0'
    u8 i = digits;

    // 从右往左填数字 (个位在 buf[digits-1])
    while (i > 0)
    {
        i--;
        buf[i] = '0' + (num % 10);
        num /= 10;
    }

    // 去前导零 (保留最后一位, 使 "0" 而不是 "")
    i = 0;
    while (i < digits - 1)
    {
        if (buf[i] == '0')
            buf[i] = ' ';
        else
            break;
        i++;
    }

    buf[digits] = '\0';
    LCD_WriteString(x, y, buf);
}

// ============================================================
// LCD_ShowDist — 显示超声波距离 XX.X cm
// ============================================================
void LCD_ShowDist(u8 x, u8 y, u16 mm)
{
    u8 cm_tens, cm_ones, frac;

    if (mm == 0)
    {
        LCD_WriteString(x, y, " 0.0 cm");
        return;
    }

    if (mm > 999) mm = 999;

    cm_tens = (u8)((mm / 10) / 10);
    cm_ones = (u8)((mm / 10) % 10);
    frac    = (u8)(mm % 10);

    if (cm_tens == 0)
        LCD_WriteChar(x, y, ' ');
    else
        LCD_WriteChar(x, y, '0' + cm_tens);

    LCD_WriteChar(x + 1, y, '0' + cm_ones);
    LCD_WriteChar(x + 2, y, '.');
    LCD_WriteChar(x + 3, y, '0' + frac);
    LCD_WriteString(x + 4, y, " cm");
}

// ============================================================
// LCD_ShowCount — 计数器 0~99 (2位, 不补前导零)
// ============================================================
void LCD_ShowCount(u8 x, u8 y, u8 count)
{
    LCD_ShowU16(x, y, count, 2);
}

#endif  // #if 0 — 禁用未使用的显示函数

// ============================================================
// lcd_read_addr — 读取 LCD 地址计数器 (内部函数)
//
// HD44780 读操作: RS=0, RW=1 时读出 busy flag (bit7) +
// 当前 DDRAM/CGRAM 地址 (bit6~bit0)
// ============================================================
static u8 lcd_read_addr(void)
{
    u8 addr;
    LCD_RS = 0;
    LCD_RW = 1;
    LCD_EN = 0;
    _nop_();
    LCD_EN = 1;
    _nop_();
    addr = LCD_DATA & 0x7F;   // bit7=busy, bit6~0=address
    LCD_EN = 0;
    return addr;
}

// ============================================================
// lcd_get_cursor — 读取当前光标位置 (行列坐标)
//
// 用法:
//   u8 x, y;
//   lcd_get_cursor(&x, &y);
//   lcdprintf("当前位置: (%d, %d)", x, y);
// ============================================================
void lcd_get_cursor(u8 *x, u8 *y)
{
    u8 addr;

    while (LCD_CheckBusy());    // 等 LCD 空闲
    addr = lcd_read_addr();     // 读地址计数器

    if (addr >= 0x40)           // 第二行 (DDRAM 0x40~0x4F)
    {
        *y = 1;
        *x = addr - 0x40;
    }
    else                        // 第一行 (DDRAM 0x00~0x0F)
    {
        *y = 0;
        *x = addr;
    }

    if (*x > 15) *x = 15;       // 安全截断
}

// ============================================================
// lcd_vprintf / lcdprintf — 类 printf 格式化输出到 LCD
//
// 支持格式符:
//   %d — 有符号整数 (int)
//   %u — 无符号整数 (unsigned int)
//   %X — 十六进制大写 (u8/u16)
//   %s — 字符串 (const u8 *)
//   %c — 单个字符
//   %% — 输出百分号本身
//   %Px,y — 光标跳到 (x,y), 不消耗参数
//
// 不支持:
//   宽度控制 (如 %04d)、long 类型、浮点数
//   — 这些在 8051 上代价太大，不需要
//
// 实现原理:
//   Keil C51 的可变参数通过 ... 传递，用 va_list/va_arg 宏访问。
//   参数在 C51 的 data/idata/XDATA 段中按 1 字节对齐排列。
//   int = 2 字节，u8/char = 1 字节，指针 = 1~2 字节 (取决于模式)。
//
// 注意:
//   - ⚠️ 只输出到行尾 (x>15 停止)，不自动换行
//   - ⚠️ 此函数使用可变参数，在 Keil C51 中需要 small 或 large
//     内存模型支持。如果编译报错，检查项目选项中 memory model
//     是否设为 SMALL 以上。
// ============================================================
#include <stdarg.h>

// ============================================================
// lcd_vprintf — 格式化输出引擎 (内部函数)
//
// 支持格式符:
//   %d   有符号整数   %u   无符号整数
//   %X   十六进制2位  %s   字符串
//   %c   单个字符     %%   百分号本身
//   %Px,y  光标跳到(x,y), 不消耗参数
//
// 例:
//   lcdprintf("dis=%d.%u cm", cm, frac);      // 一行一把梭
//   lcdprintf("%P5,0Hi %P0,1World");          // %P 跳位置
// ============================================================
static void lcd_vprintf(u8 x, u8 y, const char *fmt, va_list ap)
{
    u8       ch;
    u8      *pstr;
    int      i;
    idata u8 num_buf[7];   // 放 IDATA 省 DATA 空间
    u8       pos;
    u8       is_neg;

    LCD_SetCursor(x, y);

    while ((ch = (u8)*fmt) != '\0' && x <= 15)
    {
        if (ch != '%')
        {
            // 普通字符 (含中文 GB2312 字节) → 直接显示
            LCD_WriteData(ch);
            fmt++;
            x++;
            continue;
        }

        // 遇到 % → 解析格式符
        fmt++;
        ch = (u8)*fmt;

        switch (ch)
        {
            case 'd':
                // 有符号整数
                i = (int)va_arg(ap, int);
                is_neg = 0;
                if (i < 0)
                {
                    is_neg = 1;
                    i = -i;
                }

                // 整数转字符串 (从个位向高位填)
                pos = 6;
                num_buf[pos] = '\0';
                if (i == 0)
                {
                    pos--;
                    num_buf[pos] = '0';
                }
                else
                {
                    while (i > 0 && pos > 0)
                    {
                        pos--;
                        num_buf[pos] = '0' + (i % 10);
                        i /= 10;
                    }
                }
                if (is_neg)
                {
                    pos--;
                    num_buf[pos] = '-';
                }

                // 逐字符输出
                pstr = &num_buf[pos];
                while (*pstr && x <= 15)
                {
                    LCD_WriteData(*pstr);
                    pstr++;
                    x++;
                }
                break;

            case 'u':
                // 无符号整数
                {
                    unsigned int u = va_arg(ap, unsigned int);
                    pos = 6;
                    num_buf[pos] = '\0';
                    if (u == 0)
                    {
                        pos--;
                        num_buf[pos] = '0';
                    }
                    else
                    {
                        while (u > 0 && pos > 0)
                        {
                            pos--;
                            num_buf[pos] = '0' + (u % 10);
                            u /= 10;
                        }
                    }
                    pstr = &num_buf[pos];
                    while (*pstr && x <= 15)
                    {
                        LCD_WriteData(*pstr);
                        pstr++;
                        x++;
                    }
                }
                break;

            case 'X':
                // 十六进制大写 (2位)
                {
                    unsigned int u = va_arg(ap, unsigned int);
                    u8 hi = (u >> 4) & 0x0F;
                    u8 lo = u & 0x0F;
                    LCD_WriteData(hi < 10 ? '0' + hi : 'A' + hi - 10);
                    x++;
                    if (x <= 15)
                    {
                        LCD_WriteData(lo < 10 ? '0' + lo : 'A' + lo - 10);
                        x++;
                    }
                }
                break;

            case 's':
                // 字符串
                pstr = va_arg(ap, u8 *);
                while (*pstr && x <= 15)
                {
                    LCD_WriteData(*pstr);
                    pstr++;
                    x++;
                }
                break;

            case 'c':
                // 单个字符
                {
                    char c = (char)va_arg(ap, int);   // char 在 VA 中被提升为 int
                    LCD_WriteData((u8)c);
                    x++;
                }
                break;

            case '%':
                LCD_WriteData('%');
                x++;
                break;

            case 'P':
                // %Px,y — 光标跳到 (x,y)，不消耗可变参数
                // 例: lcdprintf("%P5,0Hello") → 在(5,0)输出 Hello
                {
                    u8 px = 0, py = 0;
                    fmt++;
                    while (*fmt >= '0' && *fmt <= '9') { px = px * 10 + (*fmt - '0'); fmt++; }
                    if (*fmt == ',')                     fmt++;
                    while (*fmt >= '0' && *fmt <= '9') { py = py * 10 + (*fmt - '0'); fmt++; }
                    if (px > 15) px = 15;
                    if (py > 1)  py = 1;
                    x = px; y = py;
                    LCD_SetCursor(px, py);
                    fmt--;  // 回退: 循环末尾的 fmt++ 会使指针正确前进
                }
                break;

            default:
                // 不识别的格式符 → 原样输出
                LCD_WriteData('%');
                x++;
                if (x <= 15)
                {
                    LCD_WriteData(ch);
                    x++;
                }
                break;
        }
        fmt++;
    }
}

// ============================================================
// lcdprintf — 格式化输出到 LCD
//
// 用法:
//   lcdprintf(0, 0, "dis=%d.%u cm", cm, frac);
//   lcdprintf(0, 1, "ir=0x%X  spd=%d", ir, spd);
//   lcdprintf(5, 0, "%P3,1Hi");               // %Px,y 可在串内跳位置
//
// 格式符: %d, %u, %X, %s, %c, %%
// 控制符: %Px,y 定位 (不消耗参数, 可多次使用)
// ============================================================
void lcdprintf(u8 x, u8 y, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    lcd_vprintf(x, y, fmt, ap);
    va_end(ap);
}
