#ifndef __VIEW_H__
#define __VIEW_H__

#include "SYSTEM/sys.h"

void View_Init(void);
void View_SetMode(view_mode_e mode);
void View_SetData(u16 dist_mm, u8 ir_disp, s8 speed, u16 target_mm);
void View_Refresh(void);

#endif
