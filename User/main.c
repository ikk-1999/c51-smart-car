/********************** HL-1 智能小车 — 课程设计 ***********************
 *
 *  ==== 平台 ====
 *    MCU:  STC89C52RC (兼容 AT89C52)
 *    晶振: 11.0592MHz
 *    编译器: Keil C51
 *
 *  ==== 功能概述 ====
 *    [CHECK 模式] 手动调速, K1+ K2-
 *    [AVD  模式] 超声波自动避障: 遇障碍→倒车→右转→前进
 *    [DIST 模式] 定距巡航: 目标20cm, 死区1cm
 *    [BT   模式] 蓝牙遥控 (预留)
 *
 *  ==== 版本 ====
 *    2025-06  v2.1  DIST模式自动巡航
 *    2025-06  v2.2  引入 view_task + lcdprintf
 *    2025-06  v2.3  AVD避障模式 (替换红外遥控)
 **********************************************************************/

#include "SYSTEM/sys.h"
#include "SYSTEM/delay.h"
#include "Hardware/key.h"
#include "Hardware/beep.h"
#include "Hardware/hc_sr04.h"
#include "Hardware/lcd1602.h"
#include "Hardware/motor.h"
#include "Hardware/bt.h"
#include "User/config.h"

// ---- DIST 模式参数 ----
#define DIST_TARGET_MM     200
#define DIST_DEADZONE_MM    30
#define DIST_SPEED_MAX      40
#define DIST_SPEED_SLOW     30
#define DIST_NEAR_MM        60
#define DIST_SPEED_BAISE    3



// ---- AVD 避障参数 ----
#define AVD_SPEED_FAST      40    // 前进全速 35%
#define AVD_SPEED_SLOW      30    // 前进慢速 25% (距障碍<300mm)
#define AVD_SPEED_BACK      30    // 后退慢速 25%
#define AVD_SPEED_BAISE     5     //右轮要加的偏置速度
//#define avoid_slow_mm    300   // 慢速触发距离mm
#define AVD_BACK_TICKS      20    // 倒车拍数 (20×10ms=200ms)
#define AVD_TURN_TICKS      20    // 转弯拍数 (30×10ms=300ms)
#define AVD_AVOID_MIN       100   // 避障距离下限mm
#define AVD_AVOID_MAX       400   // 避障距离上限mm
#define AVD_AVOID_STEP      50   // 避障距离步进mm

// ---- 蓝牙控制参数 ----
#define BT_SPEED            40    // 蓝牙按键遥控速度

// AVD 状态枚举
#define AVD_ST_FORWARD  0
#define AVD_ST_BACK     1
#define AVD_ST_TURN     2
#define AVD_ST_ERROR    3

// AVD 显示用状态名
static const char code *avd_state_names[] = {
    "FWD", "BACK", "TURN", "ERR"
};

/* ========================================
 *  函数声明
 * ======================================== */
static void view_task(view_mode_e mode, u16 dist_mm, s8 speed, u16 avoid_mm);

/* ========================================
 *  avd_task — 超声波避障状态机
 *
 *  每 10ms 调用一次。
 *  FORWARD → 遇障 BACK → 倒完 TURN → 转完 FORWARD
 * ======================================== */
static void avd_task(u16 dist_mm, u16 avoid_slow_mm ,u16 avoid_mm)
{
    static u8 state = AVD_ST_FORWARD;
    static u8 count = 0;

    switch (state)
    {
        case AVD_ST_FORWARD:
            if (dist_mm == 0)
            {
                state = AVD_ST_ERROR;
            }
            else if (dist_mm <= avoid_mm)
            {
                Beep_On();

                state = AVD_ST_BACK;
                count = 0;
            }
            else if (dist_mm <= avoid_slow_mm)
            {
                g_speed_left  =  AVD_SPEED_SLOW;
                g_speed_right =  AVD_SPEED_SLOW;
            }
            else
            {
                g_speed_left  =  AVD_SPEED_FAST;
                g_speed_right =  AVD_SPEED_FAST + AVD_SPEED_BAISE;
            }
            break;

        case AVD_ST_BACK:
            g_speed_left  = -AVD_SPEED_BACK;
            g_speed_right = -AVD_SPEED_BACK;
            if (++count >= AVD_BACK_TICKS)
            {
                state = AVD_ST_TURN;
                count = 0;
            }
            break;

        case AVD_ST_TURN:
            // 原地右转
            g_speed_left  =  -AVD_SPEED_BACK;
            g_speed_right = AVD_SPEED_BACK;
            if (++count >= AVD_TURN_TICKS)
            {
                state = AVD_ST_FORWARD;
                count = 0;
            }
            break;

        case AVD_ST_ERROR:
        default:
            g_speed_left  = 0;
            g_speed_right = 0;
            if (dist_mm != 0)
                state = AVD_ST_FORWARD;
            break;
    }
}

/* ========================================
 *  view_task — LCD 界面调度
 * ======================================== */
static void view_task(view_mode_e mode, u16 dist_mm, s8 speed, u16 avoid_mm)
{
    static view_mode_e last_mode = 0xFF;

    if (mode != last_mode)
    {
        last_mode = mode;
        LCD_Clear();
    }

    switch (mode)
    {
        case VIEW_MODE_MENU:
            lcdprintf(0, 0, "  WHICH MODE ");
            lcdprintf(0, 1, "CH  AVD DST BT ");
            break;

        case VIEW_MODE_CHECK:
            lcdprintf(0, 0, "ds=%d.%u cm  ",
                      (int)(dist_mm / 10), (unsigned int)(dist_mm % 10));
            lcdprintf(0, 1, "Spd=%d          ", (int)speed);
            break;

        case VIEW_MODE_AVD:
            lcdprintf(0, 0, "D=%d.%u Av=%d.%u",
                      (int)(dist_mm / 10), (unsigned int)(dist_mm % 10),
                      (int)(avoid_mm / 10), (unsigned int)(avoid_mm % 10));
            lcdprintf(0, 1, "L=%d R=%d       ",
                      (int)g_speed_left, (int)g_speed_right);
            break;

        case VIEW_MODE_DIST:
            lcdprintf(0, 0, "T=%d.%u cm ",
                      (int)(DIST_TARGET_MM / 10), (unsigned int)(DIST_TARGET_MM % 10));
            lcdprintf(0, 1, "D=%d.%u cm ",
                      (int)(dist_mm / 10), (unsigned int)(dist_mm % 10));
            break;
        case VIEW_MODE_LANYA:
            lcdprintf(0, 0, "L=%d R=%d       ",
                      (int)g_speed_left, (int)g_speed_right);
            lcdprintf(0, 1, "rx=%d            ", (int)bt_rx_cnt);
            break;

        default:
            break;
    }
}

/* ========================================
 *  main — 主函数
 * ======================================== */
void main(void)
{
    KeyCode_t   key;
    s8          speed     = 0;
    u16         dist_mm   = 0;
    
    
    //AVG模式下
    u16         avoid_mm  = 200;         // 默认避障 20cm
    u16         avoid_slow_mm  = 400;    // 慢速触发距离 300cm
    view_mode_e view_mode = VIEW_MODE_MENU;

    Sys_Init();
    Key_Init();
    Beep_Init();
    SR04_Init();
    Motor_Init();
    BT_Init();

    delay_ms(400);
    LCD_Init();

    INT_ENABLE_GLOBAL();

    while (1)
    {
        Beep_Off();

        /* ---- 0. 蓝牙帧解析 ---- */
        BT_Poll();

        /* ---- 1. 传感器 ---- */
        SR04_Poll();
        dist_mm = SR04_GetFiltered();

        /* ---- 2. 按键 ---- */
        key = Key_Scan();
        if (key != KEY_NONE)
        {
            Beep_On();

            switch (key)
            {
                case KEY_K1:
                    if (view_mode == VIEW_MODE_MENU)
                    {
                        view_mode = VIEW_MODE_CHECK;
                    }
                    else if (view_mode == VIEW_MODE_CHECK)
                    {
                        speed += MOTOR_SPEED_STEP;
                        if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;
                        g_speed_left  = speed;
                        g_speed_right = speed;
                    }
                    else if (view_mode == VIEW_MODE_AVD)
                    {
                        avoid_mm += AVD_AVOID_STEP;
                        avoid_slow_mm += AVD_AVOID_STEP;
                        if (avoid_mm > AVD_AVOID_MAX) avoid_mm = AVD_AVOID_MAX;
                    }
                    break;

                case KEY_K2:
                    if (view_mode == VIEW_MODE_MENU)
                    {
                        view_mode = VIEW_MODE_AVD;
                    }
                    else if (view_mode == VIEW_MODE_CHECK)
                    {
                        speed -= MOTOR_SPEED_STEP;
                        avoid_slow_mm += AVD_AVOID_STEP;
                        if (speed < -MOTOR_SPEED_MAX) speed = -MOTOR_SPEED_MAX;
                        g_speed_left  = speed;
                        g_speed_right = speed;
                    }
                    else if (view_mode == VIEW_MODE_AVD)
                    {
                        avoid_mm -= AVD_AVOID_STEP;
                        if (avoid_mm < AVD_AVOID_MIN) avoid_mm = AVD_AVOID_MIN;
                    }
                    break;

                case KEY_K3:
                    if (view_mode == VIEW_MODE_MENU)
                    {
                        view_mode = VIEW_MODE_DIST;
                    }
                    break;

                case KEY_K4:
                    if (view_mode == VIEW_MODE_MENU)
                    {
                        // 菜单下按 K4 → 进入蓝牙模式
                        view_mode = VIEW_MODE_LANYA;
                        g_speed_left  = 0;
                        g_speed_right = 0;
                        speed = 0;
                    }
                    else if (view_mode != VIEW_MODE_LANYA)
                    {
                        // 其他模式(非蓝牙) → 返回菜单
                        view_mode = VIEW_MODE_MENU;
                        speed = 0;
                        g_speed_left  = 0;
                        g_speed_right = 0;
                    }
                    // 蓝牙模式下 K4 无效 (一次性不可关闭)
                    break;

                default:
                    break;
            }
        }

        /* ---- 3. 蓝牙按键 (LANYA 模式) ---- */
        {
            u8  bt_key;
            u8  is_down;

            if (BT_GetKey(&bt_key, &is_down))
            {
                // 按下 → 执行动作, 松开 → 停止
                if (is_down)
                {
                    switch (bt_key)
                    {
                        case 1: view_mode = VIEW_MODE_CHECK; break;
                        case 2: view_mode = VIEW_MODE_AVD;   break;
                        case 3: view_mode = VIEW_MODE_DIST;  break;
                        case 4:  // 前进
                            g_speed_left  =  BT_SPEED;
                            g_speed_right =  BT_SPEED;
                            break;
                        case 5:
                            view_mode = VIEW_MODE_MENU;
                            g_speed_left  = 0;
                            g_speed_right = 0;
                            speed = 0;
                            break;
                        case 7:  // 后退
                            g_speed_left  = -BT_SPEED;
                            g_speed_right = -BT_SPEED;
                            break;
                        case 8:  // 原地左转
                            g_speed_left  =  BT_SPEED;
                            g_speed_right = -BT_SPEED;
                            break;
                        case 9:  // 原地右转
                            g_speed_left  = -BT_SPEED;
                            g_speed_right =  BT_SPEED;
                            break;
                        default: break;
                    }
                }
                else  // 松开 → 停止
                {
                    g_speed_left  = 0;
                    g_speed_right = 0;
                }
            }
        }

        /* ---- 4. AVD 避障 ---- */
        if (view_mode == VIEW_MODE_AVD)
        {
            avd_task(dist_mm, avoid_slow_mm,avoid_mm);
        }

        /* ---- 4. DIST 模式: 自动巡航 ---- */
        if (view_mode == VIEW_MODE_DIST)
        {
            if (dist_mm != 0)
            {
                s16 diff   = (s16)dist_mm - DIST_TARGET_MM;
                s8  spd;

                if      (diff > 0)  spd =  1;
                else if (diff < 0)  spd = -1;
                else                spd =  0;

                if (spd != 0)
                {
                    if (diff < 0) diff = -diff;
                    if (diff < DIST_NEAR_MM)
                        spd *= DIST_SPEED_SLOW;
                    else
                        spd *= DIST_SPEED_MAX;
                }

                g_speed_left  = spd;
                g_speed_right = spd + DIST_SPEED_BAISE;

                if (dist_mm >= DIST_TARGET_MM - DIST_DEADZONE_MM/2 &&
                    dist_mm <= DIST_TARGET_MM + DIST_DEADZONE_MM/2)
                {
                    g_speed_left  = 0;
                    g_speed_right = 0;
                }
            }
            else
            {
                g_speed_left  = 0;
                g_speed_right = 0;
            }
        }

        /* ---- 5. LCD 界面 ---- */
        view_task(view_mode, dist_mm, speed, avoid_mm);

        
        /* ---- 6. 电机 ---- */
        Motor_Apply();

        delay_ms(10);
    }
}
