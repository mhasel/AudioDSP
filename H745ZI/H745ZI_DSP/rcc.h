// rcc.h, Michael Haselberger
// Description: This file contains function prototypes for sysclock and peripheral clock configuration.

// header guard
#ifndef __RCC_H__
#define __RCC_H__

// prevent C++ name mangling
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

extern RCC_ClkInitTypeDef RCC_ClkInitStruct;

void SystemClock_Config(void);
void PeriphCommonClock_Config(void);

#ifdef __cplusplus
}
#endif
#endif // __RCC_H__