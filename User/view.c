// view.c — 已废弃 (v2.3+)
// 原独立显示模块，通过 View_SetData/View_Refresh 刷新 LCD。
// 现已被 main.c 中的 static view_task() 替代，直接调用 lcdprintf。
// 保留此文件但禁用编译以释放 DATA 空间。
#if 0

#include "Hardware/lcd1602.h"
#include "User/view.h"

// ---- 显示状态 ----
static view_mode_e mode        = 0xFF;
static u16         dist        = 0;
static u8          ir          = 0;
static s8          spd         = 0;
static u16         target      = 0;

// ---- 上一次显示的值 ----
static u16         last_dist   = 0xFFFF;
static u8          last_ir     = 0xFF;
static s8          last_spd    = 0x7F;
static u16         last_target = 0xFFFF;


void View_Init(void)
{
    mode        = 0xFF;
    last_dist   = 0xFFFF;
    last_ir     = 0xFF;
    last_spd    = 0x7F;
    last_target = 0xFFFF;
}


void View_SetMode(view_mode_e m)
{
    mode = m;
    LCD_Clear();

    switch (mode)
    {
        case VIEW_MODE_MENU:
            LCD_WriteString(0, 0, "  WHICH MODE ");
            LCD_WriteString(0, 1, "CH  IR  DST BT ");
            break;

        case VIEW_MODE_CHECK:
            LCD_WriteString(0, 0, "ds=  .  cm ");
            LCD_WriteString(0, 1, "ir=    spd=   ");
            break;

        case VIEW_MODE_IR:
            LCD_WriteString(0, 0, "    IR CMD    ");
            LCD_WriteString(0, 1, "Key:          ");
            break;

        case VIEW_MODE_DIST:
            LCD_WriteString(0, 0, "T=  .  cm     ");  // Target
            LCD_WriteString(0, 1, "D=  .  cm     ");  // Distance
            break;

        case VIEW_MODE_LANYA:
            LCD_WriteString(0, 0, "   BLUETOOTH  ");
            LCD_WriteString(0, 1, "Connecting... ");
            break;

        default:
            break;
    }

    last_dist   = 0xFFFF;
    last_ir     = 0xFF;
    last_spd    = 0x7F;
    last_target = 0xFFFF;
}


void View_SetData(u16 dist_mm, u8 ir_disp, s8 speed, u16 target_mm)
{
    dist   = dist_mm;
    ir     = ir_disp;
    spd    = speed;
    target = target_mm;
}


void View_Refresh(void)
{
    if (mode == VIEW_MODE_CHECK)
    {
        if (dist != last_dist)
        {
            LCD_Printf(3, 0, "%d.%u",
                       (int)(dist / 10),
                       (unsigned int)(dist % 10));
            last_dist = dist;
        }

        if (ir != last_ir)
        {
            LCD_Printf(3, 1, "0x%X", (unsigned int)ir);
            last_ir = ir;
        }

        if (spd != last_spd)
        {
            LCD_Printf(12, 1, "%d ", (int)spd);
            last_spd = spd;
        }
    }
    else if (mode == VIEW_MODE_DIST)
    {
        // 行0: T=XX.X cm (目标距离)
        if (target != last_target)
        {
            LCD_Printf(2, 0, "%d.%u",
                       (int)(target / 10),
                       (unsigned int)(target % 10));
            last_target = target;
        }

        // 行1: D=XX.X cm (当前距离)
        if (dist != last_dist)
        {
            LCD_Printf(2, 1, "%d.%u",
                       (int)(dist / 10),
                       (unsigned int)(dist % 10));
            last_dist = dist;
        }
    }
}

#endif  // #if 0 — 禁用 view.c
