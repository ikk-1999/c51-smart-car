#ifndef __LED_H__
#define __LED_H__

#include "SYSTEM/sys.h"

void LED_Init(void);
void LED_On(u8 led_num);
void LED_Off(u8 led_num);
void LED_Toggle(u8 led_num);
void LED_Set(u8 val);
void LED_Flow(void);
void LED_Blink(u8 led_num, u16 ms);

#endif
