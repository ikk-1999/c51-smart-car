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
 *    [BT   模式] 蓝牙遥控
 *
 *  ==== 版本 ====
 *    2025-06  v2.1  DIST模式自动巡航
 *    2025-06  v2.2  引入 view_task + lcdprintf
 *    2025-06  v2.3  AVD避障模式 (替换红外遥控)
 *    2025-06  v2.4  重构: 业务逻辑抽取为独立函数, 参数移至 config.h
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

// AVD 显示用状态名
static const char code *avd_state_names[] = {
    "FWD", "BACK", "TURN", "ERR"
};

/* ========================================
 *  函数声明
 * ======================================== */
static void avd_task(u16 dist_mm, u16 avoid_slow_mm, u16 avoid_mm);
static void dist_task(u16 dist_mm);
static void key_handler(KeyCode_t key, view_mode_e *mode, s8 *speed,
                        u16 *avoid_mm, u16 *avoid_slow_mm);
static void bt_handler(view_mode_e *mode, s8 *speed);
static void view_task(view_mode_e mode, u16 dist_mm, s8 speed, u16 avoid_mm);

/* ========================================
 *  avd_task — 超声波避障状态机
 *
 *  每 10ms 调用一次。
 *  FORWARD → 遇障 BACK → 倒完 TURN → 转完 FORWARD
 * ======================================== */
static void avd_task(u16 dist_mm, u16 avoid_slow_mm, u16 avoid_mm)
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
 *  dist_task — DIST 定距巡航控制
 *
 *  比例控制: 距离偏大→前进, 偏小→后退
 *  |diff|≥60mm 快速, <60mm 慢速, ±15mm 死区停车
 * ======================================== */
static void dist_task(u16 dist_mm)
{
    s16 diff;
    s8  spd;

    if (dist_mm == 0)
    {
        g_speed_left  = 0;
        g_speed_right = 0;
        return;
    }

    diff = (s16)dist_mm - DIST_TARGET_MM;

    if      (diff > 0)  spd =  1;
    else if (diff < 0)  spd = -1;
    else                spd =  0;

    if (spd != 0)
    {
        if (diff < 0) diff = -diff;
        spd *= (diff < DIST_NEAR_MM) ? DIST_SPEED_SLOW : DIST_SPEED_MAX;
    }

    g_speed_left  = spd;
    g_speed_right = spd + DIST_SPEED_BAISE;

    // 死区检查: 距离已在目标 ±15mm 内则停车
    if (dist_mm >= DIST_TARGET_MM - DIST_DEADZONE_MM / 2 &&
        dist_mm <= DIST_TARGET_MM + DIST_DEADZONE_MM / 2)
    {
        g_speed_left  = 0;
        g_speed_right = 0;
    }
}

/* ========================================
 *  key_handler — 按键事件分发
 *
 *  根据当前模式将 K1~K4 映射到不同动作。
 *  mode/speed/avoid_mm/avoid_slow_mm 为 in/out 参数。
 * ======================================== */
static void key_handler(KeyCode_t key, view_mode_e *mode, s8 *speed,
                        u16 *avoid_mm, u16 *avoid_slow_mm)
{
    switch (key)
    {
        case KEY_K1:
            if (*mode == VIEW_MODE_MENU)
            {
                *mode = VIEW_MODE_CHECK;
            }
            else if (*mode == VIEW_MODE_CHECK)
            {
                // 加速
                *speed += MOTOR_SPEED_STEP;
                if (*speed > MOTOR_SPEED_MAX) *speed = MOTOR_SPEED_MAX;
                g_speed_left  = *speed;
                g_speed_right = *speed;
            }
            else if (*mode == VIEW_MODE_AVD)
            {
                // 增大避障阈值
                *avoid_mm      += AVD_AVOID_STEP;
                *avoid_slow_mm += AVD_AVOID_STEP;
                if (*avoid_mm > AVD_AVOID_MAX) *avoid_mm = AVD_AVOID_MAX;
            }
            break;

        case KEY_K2:
            if (*mode == VIEW_MODE_MENU)
            {
                *mode = VIEW_MODE_AVD;
            }
            else if (*mode == VIEW_MODE_CHECK)
            {
                // 减速
                *speed -= MOTOR_SPEED_STEP;
                *avoid_slow_mm += AVD_AVOID_STEP;
                if (*speed < -MOTOR_SPEED_MAX) *speed = -MOTOR_SPEED_MAX;
                g_speed_left  = *speed;
                g_speed_right = *speed;
            }
            else if (*mode == VIEW_MODE_AVD)
            {
                // 减小避障阈值
                *avoid_mm -= AVD_AVOID_STEP;
                if (*avoid_mm < AVD_AVOID_MIN) *avoid_mm = AVD_AVOID_MIN;
            }
            break;

        case KEY_K3:
            if (*mode == VIEW_MODE_MENU)
            {
                *mode = VIEW_MODE_DIST;
            }
            break;

        case KEY_K4:
            if (*mode == VIEW_MODE_MENU)
            {
                // 菜单下按 K4 → 进入蓝牙模式
                *mode = VIEW_MODE_LANYA;
                g_speed_left  = 0;
                g_speed_right = 0;
                *speed = 0;
            }
            else if (*mode != VIEW_MODE_LANYA)
            {
                // 其他模式(非蓝牙) → 返回菜单
                *mode = VIEW_MODE_MENU;
                *speed = 0;
                g_speed_left  = 0;
                g_speed_right = 0;
            }
            // 蓝牙模式下 K4 无效 (一次性不可关闭)
            break;

        default:
            break;
    }
}

/* ========================================
 *  bt_handler — 蓝牙按键事件处理
 *
 *  仅在有新蓝牙事件时调用, is_down=1 按下 0 松开。
 *  前进/后退/左转/右转为点动控制。
 * ======================================== */
static void bt_handler(view_mode_e *mode, s8 *speed)
{
    u8  bt_key;
    u8  is_down;

    if (!BT_GetKey(&bt_key, &is_down))
        return;

    if (is_down)
    {
        switch (bt_key)
        {
            case 1: *mode = VIEW_MODE_CHECK; break;
            case 2: *mode = VIEW_MODE_AVD;   break;
            case 3: *mode = VIEW_MODE_DIST;  break;
            case 4:  // 前进
                g_speed_left  =  BT_SPEED;
                g_speed_right =  BT_SPEED;
                break;
            case 5:  // 停车 / 返回菜单
                *mode = VIEW_MODE_MENU;
                g_speed_left  = 0;
                g_speed_right = 0;
                *speed = 0;
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
                      (int)(DIST_TARGET_MM / 10),
                      (unsigned int)(DIST_TARGET_MM % 10));
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
    u16         avoid_mm  = 200;      // 默认避障 20cm
    u16         avoid_slow_mm = 400;  // 慢速触发距离 40cm
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

        /* ---- 1. 传感器 ---- */
        BT_Poll();
        SR04_Poll();
        dist_mm = SR04_GetFiltered();

        /* ---- 2. 实体按键 ---- */
        key = Key_Scan();
        if (key != KEY_NONE)
        {
            Beep_On();
            key_handler(key, &view_mode, &speed, &avoid_mm, &avoid_slow_mm);
        }

        /* ---- 3. 蓝牙按键 ---- */
        bt_handler(&view_mode, &speed);

        /* ---- 4. AVD 避障状态机 ---- */
        if (view_mode == VIEW_MODE_AVD)
            avd_task(dist_mm, avoid_slow_mm, avoid_mm);

        /* ---- 5. DIST 定距巡航 ---- */
        if (view_mode == VIEW_MODE_DIST)
            dist_task(dist_mm);

        /* ---- 6. LCD 界面 ---- */
        view_task(view_mode, dist_mm, speed, avoid_mm);

        /* ---- 7. 电机 PWM 输出 ---- */
        Motor_Apply();

        delay_ms(10);
    }
}
