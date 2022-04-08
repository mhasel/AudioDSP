// fx_lib.h, Michael Haselberger
// Description: This file contains declarations for the effect library implemented in ringbuffer.c

#ifndef __FX_LIB_H__
#define __FX_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "defines_and_constants.h"
#include "ring_buffer.h"
	
#define MAX_DELAY_TIME 500
	
	
	enum fx_designator
	{
		FXNONE = 0,
		FXDELAY,
		FXOVERDRIVE,
		FXFUZZ,
		FXTREMOLO,
		FXRINGMOD,
		FXFILTER
	};
	
// DELAY
	typedef enum
	{
		DELAY = 0,
		FEEDBACK,
		BLEND
	} delay_parameter;

	typedef struct
	{
		volatile float32_t delay_ms;
		volatile float32_t blend;
		volatile float32_t feedback;
		volatile bool is_running;	
		float32_t level;
		float32_t *src;
		float32_t *dst;
		rb_handle_t *delay_rb;
	
	} delay_handle_t;

	uint8_t delay_init(delay_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t delay_ms, float32_t blend, float32_t feedback);
	uint8_t delay_update(delay_handle_t *handle, delay_parameter pm, float32_t value);
	void run_delay(delay_handle_t *handle);
	void init_fir_filter(float32_t *filter_taps);
	void run_fir_filter(float32_t *src, float32_t *dst);	
	
	// OVERDRIVE	
	typedef struct 
	{
		volatile float32_t threshold;
		volatile bool is_running;			
		float32_t *src;
		float32_t *dst;
		
	} overdrive_handle_t;
	
	uint8_t overdrive_init(overdrive_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t threshold);
	uint8_t overdrive_update(overdrive_handle_t *handle, float32_t threshold);
	void run_overdrive(overdrive_handle_t *handle);
	
	// FUZZ
	typedef enum
	{
		GAIN    = 0,
		MIX
	} fuzz_parameter;
	typedef struct 
	{
		volatile float32_t gain;
		volatile float32_t mix;
		volatile bool is_running;
		float32_t *src;
		float32_t *dst;
		
	} fuzz_handle_t;
	
	uint8_t fuzz_init(fuzz_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t gain, float32_t mix);
	uint8_t fuzz_update(fuzz_handle_t *handle, fuzz_parameter pm, float32_t value);
	void run_fuzz(fuzz_handle_t *handle);
	
	// TREMOLO
	typedef enum
	{
		RATE = 0,
		DEPTH
	} modulation_parameter;
	typedef struct 
	{
		volatile float32_t rate;
		volatile float32_t depth;
		volatile bool is_running;		
		float32_t time;
		float32_t *src;
		float32_t *dst;
		
	} tremolo_handle_t;
	
	uint8_t tremolo_init(tremolo_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t rate, float32_t depth);
	uint8_t tremolo_update(tremolo_handle_t *handle, modulation_parameter pm, float32_t value);
	void run_tremolo(tremolo_handle_t *handle);
	
	// RING MODULATOR
	typedef enum
	{
		SINE = 0,
		TRIANGLE,
		SQUARE,
		NO_CHANGE
	} modulator_type;
	typedef struct 
	{
		volatile float32_t rate;
		volatile float32_t blend;
		volatile modulator_type type;
		volatile bool is_running;			
		float32_t *src;
		float32_t *dst;
		float32_t time;
		
	} ring_mod_handle_t;
	
	uint8_t ring_mod_init(ring_mod_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t rate, float32_t blend, modulator_type type);
	uint8_t ring_mod_update(ring_mod_handle_t *handle, modulation_parameter pm, modulator_type type, float32_t value);
	void run_ring_mod(ring_mod_handle_t *handle);
	
	#ifdef __cplusplus
	}
#endif
#endif // _FX_LIB_H_