#pragma once
#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

extern TIM_HandleTypeDef htim2;
void MX_TIM2_Init(void);

#ifdef __cplusplus
}
#endif

#endif //__TIMER_H__