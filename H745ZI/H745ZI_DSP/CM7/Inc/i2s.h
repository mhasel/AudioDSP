// header guard
#ifndef __I2S_H__
#define __I2S_H__

// prevent C++ name mangling
#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern I2S_HandleTypeDef hi2s2;
extern DMA_HandleTypeDef hdma_i2s2_rx;
extern DMA_HandleTypeDef hdma_i2s2_tx;
void MX_I2S2_Init(void);
void MX_DMA_Init(void);

#ifdef __cplusplus
}
#endif

#endif // __I2S_H__
