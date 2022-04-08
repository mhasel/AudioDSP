#ifndef __DEFINES_AND_CONSTANTS_H__
#define __DEFINES_AND_CONSTANTS_H__
#ifdef __cplusplus
{
	extern "C" {
#endif
		
// ------------ DEFINES -----------------
#define DMA
// amount of samples processed at once (left + right) (block-processing. bigger blocks allow for more efficient processing, but increase latency)
// Buffer needs to be 4-byte (DMA) or 32-byte (cache) aligned
#define SAMPLES 128
// 2 channels: buffer size needs to be twice the sample count
#define DMA_BUFFER_SIZE (SAMPLES << 1)
#define PING_PONG_BUFFER_SIZE (SAMPLES >> 1)
#define NUM_TAPS 37
		
// allows using boolean type without including bool.h
typedef enum { false, true } bool;
	
enum ping_pong
{
	PING = 0,
	PONG
};
		
#ifdef __cpluplus
}
#endif
#endif // __DEFINES_AND_CONSTANTS_H__