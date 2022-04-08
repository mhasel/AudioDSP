// fx_lib.c: Michael Haselberger
// Description: Algorithm library for audio signal processing effects
#include "fx_lib.h"

// ---- Constants and Helpers ----

// sample frequency/sample rate Fs
static const uint16_t Fs = 48000;

/******************************************************************************
* Function Name: triangle_wave
*******************************************************************************
* Summary:
*  Calculate the amplitude of a non-phase-shifted triangle wave as a funtion of time.
*
* Parameters:
*  1. float32_t time - the x value/time at which to calculate the value.
* Return:
*  The value of the amplitude.
*
******************************************************************************/
static inline float32_t triangle_wave(float32_t time)
{	
	return asin(arm_cos_f32(time)) / 1.57079633;
}


/******************************************************************************
* Function Name: square_wave
*******************************************************************************
* Summary:
*  Calculate the amplitude of a non-phase-shifted square wave as a funtion of time.
*
* Parameters:
*  1. float32_t time - truct which contains information about the size of the buffer, 
*  its address in memory and which data it will hold.
* Return:
*  The value of the amplitude.
*
******************************************************************************/
static inline int8_t square_wave(float32_t time)
{
	return (arm_sin_f32(time) >= 0) ? 1 : -1;
}

// ---- Delay Section ----

// ring buffer handle for delay effect
static rb_handle_t _delay_rb = { 0 };
// 2^16 * 4 bytes = 262 144 B buffer size. this is pretty much an entire RAM region just delay buffer.	
// delay ring buffer needs to be > Fs. ring buffer implementation calls for buffers to be 2^X, so buffer needs to be 2^16 elements.
// this results in a ~27% memory inefficiency vs. a slight performance gain due to eliminating modulo calculations. if memory is tight,
// consider taking the performance hit (delay buffer for 48k samples would be around 192KB: (48000+1) * 4 bytes )
// might also be worth considering to dynamically assign this memory only when required. requires de-init function to avoid memory leaks
static volatile float32_t __attribute__((aligned(32))) __attribute__((section(".delay_buffer"))) delay_buffer[(1 << 16)] = { 0 };

// designator used by the ring buffer functions to identify the correct buffer
static rb_designator_t delay_rbd =  FXDELAY;

/******************************************************************************
* Function Name: delay_init
*******************************************************************************
* Summary:
*  Initialize delay handle struct with its parameters. This includes initialization of 
*  a circular buffer, which acts as the delay-line.
*
* Parameters:
*  1. delay_handle_t *handle	- Address pointer of delay handle struct.
*  2. float32_t *in_buffer		- Address pointer of the input buffer, which holds the blocked sample data.
*  3. float32_t *out_buffer		- Address pointer of the output buffer. This is where the modified samples are written to.
*  4. float32_t delay_ms		- The amount of time the delay line will delay the incoming samples. Delay-line buffer
*								  will be filled with the corresponding amount of samples. Maximum time depends on memory constraints.
*  5. float32_t blend			- Determines the ratio of dry and wet mix. Must be between 0 and 1
*  6. float32_t feedback		- Determines the amplitude of the wet sample which is fed back to the delay line.
* 
* Return:
*	255:						- Input or output buffers are NULL
*	254:						- Parameters out of range
*	253:						- Ring buffer error
*	  0:						- Successful initialization
*
******************************************************************************/
uint8_t delay_init(delay_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t delay_ms, float32_t blend, float32_t feedback)
{
	if ((in_buffer == NULL) || (out_buffer == NULL)) 
	{
		return 255;
	}
	
	if ((blend <= 0.0f) || (blend >= 1.0f) || (feedback <= 0.0f) || (feedback >= 0.99f) || (delay_ms > MAX_DELAY_TIME))
	{
		return 254;
	}
		
	_delay_rb.element_size = sizeof(delay_buffer[0]);
	_delay_rb.max_elements = ARRAY_SIZE(delay_buffer);
	_delay_rb.buffer = delay_buffer;	
	
	// rb init function increments designator. if designator is still FXDELAY, rb needs to be initialized.
	// otherwise, it's enough to clear the buffer
	// TODO: re-write check here. as it is, delay_rbd will always be 0
	if (delay_rbd == FXDELAY)
	{
		int8_t err = ring_buffer_init(&delay_rbd, &_delay_rb);
		if (err)
		{
			return 253;
		}
	}
	else
	{
		ring_buffer_clear(FXDELAY);
	}
	
	handle->src = in_buffer;
	handle->dst = out_buffer;
	handle->delay_ms = delay_ms;
	handle->blend = blend;	
	handle->feedback = feedback;
	// this struct member might be unnecessary. only useful if rb struct needs to be accessed outside of this file, which probably isn't the case
	handle->delay_rb = &_delay_rb;		
	
	// fill circular buffer with 0/increment head-pointer by amount of samples to delay
	uint16_t delay_in_samples = (uint16_t)(handle->delay_ms * (Fs / 1000.0f));
	for (int i = 0 ; i < delay_in_samples ; ++i)
	{
		static float32_t zero = 0.0f;
		ring_buffer_put(FXDELAY, &zero);
	}
	
	return 0;
}

/******************************************************************************
* Function Name: delay_update
*******************************************************************************
* Summary:
*  Allows updating of delay parameters during runtime.
*
* Parameters:
*  1. delay_handle_t *handle	- Address pointer of delay handle struct.
*  2. delay_parameter pm		- The parameter to change.
*  3. float32_t value			- The value to update it to.
* 
* Return:
*  255:							- Value out of range.
*	 0:							- Success.
******************************************************************************/
uint8_t delay_update(delay_handle_t *handle, delay_parameter pm, float32_t value)
{
	switch (pm)
	{
	case DELAY:
		if (value > MAX_DELAY_TIME)
			return 255;
		handle->delay_ms = value;
		break;
	case FEEDBACK:
		if ((value < 0.0f) || (value > 1.0f))
			return 255;
		handle->feedback = value;
	case BLEND:
		if ((value < 0.0f) || (value > 1.0f))
			return 255;
		handle->blend = value;
	}
		
	return 0;
}

/******************************************************************************
* Function Name: run_delay
*******************************************************************************
* Summary:
*  Processes a sample block. Writes the current sample with adjusted feedback amplitude into a buffer.
*  
*
* Parameters:
*  1. delay_handle_t *handle	- Address pointer of delay handle struct.
* 
* Return:
*  None.
*
******************************************************************************/
#pragma optimize_for_speed
void run_delay(delay_handle_t *delay)
{	
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		float32_t delayed = 0.0f;
		float32_t current = delay->src[i] * delay->feedback;
		// save input sample to delay buffer
		ring_buffer_put(FXDELAY, &current);	
		// get sample at max delay depth
		ring_buffer_get(FXDELAY, &delayed);
		// sum input with delay
		delay->dst[i] = (1.0f - delay->blend) * delay->src[i] + delay->blend * delayed;
	}
}

// ---- Filter ----
static arm_fir_instance_f32 fir_filter;

// filter taps/coeffs
// Filter designed with http://t-filter.engineerjs.com/
const float32_t filter_taps[NUM_TAPS] = 
{
	-0.00038320543575594507f,
	-0.001377178701148151f,
	-0.0025366259116399122f,
	-0.004432549591717381f,
	-0.006494295696777184f,
	-0.008515660530043372f,
	-0.009767438023472977f,
	-0.009526244099262525f,
	-0.006932364763420581f,
	-0.0012788243688729513f,
	0.007887516146031764f,
	0.020575396949645768f,
	0.036269525391131f,
	0.05390782359810524f,
	0.07197526362835165f,
	0.08868444305715538f,
	0.10222851805387685f,
	0.11105647594211442f,
	0.11412216765453903f,
	0.11105647594211442f,
	0.10222851805387685f,
	0.08868444305715538f,
	0.07197526362835165f,
	0.05390782359810524f,
	0.036269525391131f,
	0.020575396949645768f,
	0.007887516146031764f,
	-0.0012788243688729513f,
	-0.006932364763420581f,
	-0.009526244099262525f,
	-0.009767438023472977f,
	-0.008515660530043372f,
	-0.006494295696777184f,
	-0.004432549591717381f,
	-0.0025366259116399122f,
	-0.001377178701148151f,
	-0.00038320543575594507f
 };
// fir state size is (number of samples + number of fir tabs - 1)

// filter state
static float32_t fir_state[SAMPLES + NUM_TAPS - 1];

/******************************************************************************
* Function Name: init_fir_filter
*******************************************************************************
* Summary:
*  Initialize arm-CMSIS filter struct.
*
* Parameters:
*  1. float32_t *filter_taps		- Filter coefficients. More taps make the filter more accurate, but introduce
*									  more delay into the system.
* 
* Return:
*  None.
*
******************************************************************************/
void init_fir_filter(float32_t  *filter_taps)
{
	arm_fir_init_f32(&fir_filter, NUM_TAPS, (float32_t *)&filter_taps[0], &fir_state[0], SAMPLES);
}

/******************************************************************************
* Function Name: run_fir_filter
*******************************************************************************
* Summary:
*  Run filter function on sample block.
*
* Parameters:
*  1. float32_t *src		- Sample in-buffer
*  2. float32_t *dst		- Sample out-buffer
* Return:
*  None.
*
******************************************************************************/
void run_fir_filter(float32_t *src, float32_t *dst)
{
	arm_fir_f32(&fir_filter, &src[0], &dst[0], SAMPLES);
}

// ---- Overdrive ----

/******************************************************************************
* Function Name: overdrive_init
*******************************************************************************
* Summary:
*  Initializes the overdrive struct.
*
* Parameters:
*  1. overdrive_handle_t *handle		- Address pointer of overdrive handle struct.
*  2. float32_t *in_buffer				- Address pointer of the sample in-buffer.
*  3. float32_t *out_buffer				- Address pointer of the sample out-buffer.
*  4. float32_t threshold				- Threshold value for the overdrive algorithm. Must be between 0 and 0.4
* Return:
*  255:									- Sample buffers point to NULL
*  254:									- Threshold value out of range.
*    0:									- Success.
*
******************************************************************************/
uint8_t overdrive_init(overdrive_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t threshold)
{
	if ((in_buffer == NULL) || (out_buffer == NULL)) 
	{
		return 255;
	}
	if ((threshold <= 0) || (threshold > 0.4f))
	{
		return 254;
	}
	
	handle->src = in_buffer;
	handle->dst = out_buffer;
	handle->threshold = threshold;
	return 0;
}

/******************************************************************************
* Function Name: overdrive_update
*******************************************************************************
* Summary:
*  Updates overdrive threshold parameter.
*
* Parameters:
*  1. overdrive_handle_t *handle				- Address pointer of overdrive handle struct.
*  2. float32_t threshold						- The new threshold value. Needs to be between 0 and 0.4.
* Return:
*  255:											- Threshold value out of range.
*    0:											- Success.
*
******************************************************************************/
uint8_t overdrive_update(overdrive_handle_t *handle, float32_t threshold)
{
	if ((threshold <= 0) || (threshold > 0.4f))
	{		
		return 255;
	}
	handle->threshold = threshold;
	return 0;
}

/******************************************************************************
* Function Name: run_delay
*******************************************************************************
* Summary:
*  Add delay to the sample block.
*
* Parameters:
*  1. overdrive_handle_t *handle			- Address pointer of overdrive handle struct.
* Return:
*  None.
*
******************************************************************************/
#pragma optimize_for_speed
void run_overdrive(overdrive_handle_t *handle)
{
	float32_t abs[PING_PONG_BUFFER_SIZE];
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		if (abs[i] == 0)
			handle->dst[i] = 0;
		else if (abs[i] < handle->threshold)
			handle->dst[i] = 2 * handle->src[i];		
		else if (abs[i] > 2 * handle->threshold)
		{
			if (handle->src[i] > 0)
				handle->dst[i] = 1;
			else
				handle->dst[i] = -1;	
		}
		else if (abs[i] >= handle->threshold)
		{
			if (handle->src[i] > 0)
				handle->dst[i] = (3 - ((2 - abs[i] * 3) * (2 - abs[i] * 3)) / 3);
			else
				handle->dst[i] = - (3 - ((2 - abs[i] * 3) * (2 - abs[i] * 3)) / 3);	
		}		
	}
}

// ---- Fuzz ----

/******************************************************************************
* Function Name: fuzz_init
*******************************************************************************
* Summary:
*  Initizalize the fuzz struct.
*
* Parameters:
*  1. fuzz_handle_t *handle				- Address pointer of fuzz handle struct.
*  2. float32_t *in_buffer				- Address pointer of sample in-buffer.
*  3. float32_t *out_buffer				- Address pointer of sample out-buffer.
*  4. float32_t gain					- Amplitification gain value of the fuzz algorithm. Range: 0 < gain <= 18.
*  5. float32_t mix						- Relative ratio of distorted (wet) signal to dry signal. Range: 0 < mix < 1.
* Return:
*  255:									- Sample buffers point to NULL.
*  254:									- Gain or mix values are out of range.
*    0:									- Success.
*
******************************************************************************/
uint8_t fuzz_init(fuzz_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t gain, float32_t mix)
{
	if ((in_buffer == NULL) || (out_buffer == NULL)) 
	{
		return 255;
	}
	if ((gain < 0) || (gain > 18) || (mix < 0) || (mix >= 1))
	{
		return 254;
	}
	
	handle->src = in_buffer;
	handle->dst = out_buffer;
	handle->gain = gain;
	handle->mix = gain;
	
	return 0;
}

/******************************************************************************
* Function Name: fuzz_update
*******************************************************************************
* Summary:
*  Update fuzz parameters.
*
* Parameters:
*  1. fuzz_handle_t *handle				- Address pointer of fuzz handle struct.
*  2. fuzz_parameter pm					- Enum of fuzz parameters.
*  3. float32_t value					- The new value of the parameter.
* 
* Return:
*  255:									- Parameter value are out of range.
*    0:									- Success.
*
******************************************************************************/
uint8_t fuzz_update(fuzz_handle_t *handle, fuzz_parameter pm, float32_t value)
{
	switch (pm)
	{
		case GAIN:
			if ((value < 0) || (value > 18))
				return 255;
			handle->gain = value;
			break;
		case MIX:
			if ((value < 0) || (value >= 1))
				return 255;
			handle->mix = value;
			break;		
	}
	
	return 0;
}


/******************************************************************************
* Function Name: run_fuzz
*******************************************************************************
* Summary:
*  Run fuzz algorithm on sample block.
*
* Parameters:
*  1. fuzz_handle_t *handle				- Address pointer of fuzz handle struct.
* Return:
*  None.
*
******************************************************************************/
#pragma optimize_for_speed
void run_fuzz(fuzz_handle_t *handle)
{
	float32_t z[PING_PONG_BUFFER_SIZE];
	float32_t max_z = 0;
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		// multiply sample with gain and then normalize by dividing by positive max-value
		float32_t q = handle->src[i] * handle->gain / (1 << 24);
		if (q != 0)
		{
			z[i] = -q / fabs(q) * (1 - expf(-q / fabs(q) * q));

			if (z[i] > max_z)
				max_z = z[i];
		}
		else 
			z[i] = 0;
	}
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		handle->dst[i] = handle->mix * z[i] * (1 << 24) / max_z + (1 - handle->mix) * handle->src[i];
	}
}

// ---- Tremolo ----

/******************************************************************************
* Function Name: tremolo_init
*******************************************************************************
* Summary:
*  Initialize tremolo handle struct.
*
* Parameters:
*  1. tremolo_handle_t *handle				- Address pointer of tremolo handle struct.
*  2. float32_t *in_buffer					- Address pointer of sample block in-buffer.
*  3. float32_t *out_buffer					- Address pointer of sample block out-buffer.
*  4. float32_t rate						- Allows modifying the LFO frequency. Range: 0 < rate < 1.
*  5. float32_t depth						- Modulation depth. Range: 0 <= depth <= 1.
* Return:
*  255:										- Sample buffers point to NULL.
*  254:										- Rate or depth values are out of range.
*    0:										- Success.
*
******************************************************************************/
uint8_t tremolo_init(tremolo_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t rate, float32_t depth)
{
	if ((in_buffer == NULL) || (out_buffer == NULL)) 
	{
		return 255;
	}
	if ((rate < 0) || (rate > 1) || (depth < 0) || (depth > 1))
	{
		return 254;
	}

	handle->src = in_buffer;
	handle->dst = out_buffer;
	handle->rate = rate;
	handle->depth = depth;
	handle->time = 0;

	return 0;
}

/******************************************************************************
* Function Name: tremolo_update
*******************************************************************************
* Summary:
*  Update tremolo parameters.
*
* Parameters:
*  1. tremolo_handle_t *handle				- Address pointer of tremolo handle struct.
*  2. modulation_parameter pm				- Enum of modulation parameters.
*  3. float32_t value						- The new value of the parameter.
* 
* Return:
*  255:										- Parameter value are out of range.
*    0:										- Success.
*
******************************************************************************/
uint8_t tremolo_update(tremolo_handle_t *handle, modulation_parameter pm, float32_t value)
{
	switch (pm)
	{
	case RATE:
		if ((value < 0) || (value > 1.0f))
			return 255;
		handle->rate = value;
		break;
	case DEPTH:
		if ((value < 0) || (value > 1.0f))
			return 254;
		handle->depth = value;
		break;		
	}
	
	return 0;
}
	

/******************************************************************************
* Function Name: run_tremolo
*******************************************************************************
* Summary:
*  Run tremolo algorithm on sample block.
*
* Parameters:
*  1. tremolo_handle_t *handle				- Address pointer of tremolo handle struct.
* 
* Return:
*  None.
*
******************************************************************************/

// https://wiki.analog.com/resources/tools-software/sharc-audio-module/baremetal/tremelo-effect-tutorial
void run_tremolo(tremolo_handle_t *handle)
{
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		// get moculation factor for this sample
		float32_t factor = 1 - (handle->depth * 0.5f * arm_sin_f32(handle->time) + 0.5f);
		handle->time += handle->rate * 0.002;
		if (handle->time > 2 * PI)
			handle->time -= 2 * PI;
		handle->dst[i] = factor * handle->src[i];	
	}
}
// ---- Ring Modulator ----

/******************************************************************************
* Function Name: ring_mod_init
*******************************************************************************
* Summary:
*  Initialize ring modulator struct.
*
* Parameters:
*  1. ring_mod_handle_t *handle				- Address pointer of ring modulator handle struct.
*  2. float32_t *in_buffer					- Address pointer of sample block in-buffer.
*  3. float32_t *out_buffer					- Address pointer of sample block out-buffer.
*  4. float32_t rate						- Rate of change of the modulator; frequency. Range: 0 < rate <= 1.
*  5. float32_t blend						- Ratio of mix between dry signal and wet signal. Range: 0 < blend < 1.
*  6. modulator_type type					- The modulating signal wave form. Sine, Triangle or Square wave.
* 
* Return:
*  255:										- Sample buffers point to NULL.
*  254:										- Rate or depth values are out of range.
*    0:										- Success. 
*
******************************************************************************/
uint8_t ring_mod_init(ring_mod_handle_t *handle, float32_t *in_buffer, float32_t *out_buffer, float32_t rate, float32_t blend, modulator_type type)
{
	if ((in_buffer == NULL) || (out_buffer == NULL)) 
	{
		return 255;
	}
	if ((rate >= 0) || (rate <= 1) || (blend < 0) || (blend > 0.99))
	{
		return 254;
	}
	
	handle->src = in_buffer;
	handle->dst = out_buffer;
	handle->rate = rate;
	handle->blend = blend;
	handle->type = type;
	handle->time = 0;
	
	return 0;
}

/******************************************************************************
* Function Name: ring_mod_update
*******************************************************************************
* Summary:
*  Update ring modulation parameters.
*
* Parameters:
*  1. ring_mod_handle_t *handle				- Address pointer of ring modulator handle struct.
*  2. modulation_parameter pm				- Enum of modulation parameters.
*  3. modulator_type type					- Enum of modulating signal wave form.
*  4. float32_t value						- The new value of the parameter.
* 
* Return:
*  255:										- Parameter value are out of range.
*    0:										- Success.
*
******************************************************************************/
uint8_t ring_mod_update(ring_mod_handle_t *handle, modulation_parameter pm, modulator_type type, float32_t value)
{
	switch (pm)
	{
	case RATE:
		if ((value < 0) || (value > 1.0f))
			return 255;
		handle->rate = value;
		break;
	case DEPTH:
		if ((value < 0) || (value > 1.0f))
			return 254;
		handle->blend = value;
		break;		
	default:
		break;
	}
	
	if (type != NO_CHANGE)
		handle->type = type;
	
	return 0;
}


/******************************************************************************
* Function Name: run_ring_mod
*******************************************************************************
* Summary:
*  Run ring modulation algorithm on sample block.
*
* Parameters:
*  1. ring_mod_handle_t *handle				- Address pointer of tremolo handle struct.
* 
* Return:
*  None.
*
******************************************************************************/
#pragma optimize_for_speed
void run_ring_mod(ring_mod_handle_t *handle)
{
	for (int i = 0; i < PING_PONG_BUFFER_SIZE; ++i)
	{
		float32_t factor;
		switch (handle->type)
		{
			case SINE:
				factor = arm_sin_f32(handle->time);
				break;
			case TRIANGLE:
				factor = triangle_wave(handle->time);
				break;
			case SQUARE:
				factor = square_wave(handle->time);
				break;
			default:
				return;
		}
		
		handle->time += handle->rate * 0.02;
		if (handle->time > 2 * PI)
			handle->time -= 2 * PI;
		
		handle->dst[i] = (1 - handle->blend) * handle->src[i] + handle->blend * factor * handle->src[i];
	}
}