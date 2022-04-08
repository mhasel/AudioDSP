// main.h, Michael Haselberger
// Description: This file bundles .h files used for this project

// header/recursive inclusion guard
#ifndef __MAIN_H__
#define __MAIN_H__
// prevent C++ name mangling
#ifdef __cplusplus
extern "C" {
#endif

// include external dependy headers from board support package
#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"
#include "stm32h7xx_hal_conf.h"
#include <arm_math.h>

// include project internal header files	
#include "defines_and_constants.h"
#include "rcc.h"
#include "i2s.h"
#include "timer.h"
#include "i2c.h"
#include "i2c_lcd.h"
#include "fx_lib.h"
#include "user_interface.h"

void Error_Handler(void);

#ifdef __cplusplus
}
#endif
#endif // _MAIN_H__

