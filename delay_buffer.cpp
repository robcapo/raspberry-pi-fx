/** @file
 * @addtogroup delay Delay Buffer
 * 
 * @author Rob Capo
 *
 * @{
 *
 * @brief This file contains the class interface and implementation for the delay buffer object.
 * Details follow.
 */
#pragma once

#ifndef DELAY_BUFFER_CPP_
#define DELAY_BUFFER_CPP_

#include <iostream>
#include <unistd.h>
#include <jack/jack.h>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "fx_processor.cpp"

// 2 seconds * 44100 Hz
#define BUFFER_CAPACITY 88200
#define SAMPLE_RATE 	44100

class Delay_Buffer
{
	public:
		/** Add a frame of samples to the buffer and set the output buffer to the mixture
		 * of the delay and the dry signal.
		 *
		 * All samples will be added to the buffer, mixed with the existing samples, and
		 * the `_output_buffer` will be set to the mixture of the dry signal and the effect.
		 *
		 * @param sample Pointer to the frame of samples to add to the buffer
		 */
		void newFrame(jack_default_audio_sample_t *sample);
		
		/** Set the duration of the delay in seconds.
		 *
		 * The size of the buffer will automatically be calculated and matched as closely
		 * as possible to achieve the desired duration.
		 *
		 * @param seconds Number of seconds before echo is heard
		 */
		void setDelayLength(double seconds);
		
		/** Set the rate of decay of the delay effect.
		 *
		 * This controls how long the echoing signal will be heard after it occurs. If the
		 * decay rate is 0, no echo will be heard. If the decay rate is 1 (not recommended),
		 * the echo will be infinite. Otherwise, the number will be multiplied by each frame
		 * during each pass of the buffer.
		 *
		 * @param decay Rate of decay (0 <= decay <= 1) for the signal
		 */
		void setDecay(double decay);
		
		/** Set the volume of the echo effect.
		 *
		 * Controls how loud the echo is heard when it is added to the dry signal.
		 *
		 * @param level Volume of the echo (0 <= level <= 1).
		 */
		void setLevel(double level);
		
		/** Initialize a delay buffer
		 *
		 * @param decay The rate of decay for the buffer (see `setDecay`)
		 * @param level The level of the delay (see `setLevel`)
		 * @param duration The duration of the delay (see `setDelayLength`)
		 * @param frame_size The number of samples in each frame when `process` is called
		 */
		Delay_Buffer(double, double, double, jack_nframes_t);
		Delay_Buffer();
		
		jack_default_audio_sample_t *_output_buffer; ///< Holds one frame of data to be output each time the `newFrame` function is called
		
		FX_Processor *_fx_processor; ///< Holds a pointer to the FX Processor object that will process each sample as it's echoed
		
	private:
		uint32_t _buffer_ind; // location of the current index of the delay buffer
		jack_default_audio_sample_t _buffer[BUFFER_CAPACITY]; // delay buffer
		jack_nframes_t _frame_size; // number of samples per frame received
		
		int _active;
		
		// user settings
		uint32_t _max_buffer_ind; // ending point of buffer (i.e. duration)
		double _decay; // decay factor multiplied at each pass
		double _level; // level of effect
	
};
//*

Delay_Buffer::Delay_Buffer()
{
	_active = 1;
	
	_buffer_ind = 0;
	_frame_size = 0;
	_output_buffer = new jack_default_audio_sample_t[512];
	
	_max_buffer_ind = BUFFER_CAPACITY - 1;
	_decay = .5;
	_level = 1;
}
Delay_Buffer::Delay_Buffer(double decay, double level, double duration, jack_nframes_t frame_size)
{
	_buffer_ind = 0;
	_frame_size = frame_size;
	_output_buffer = new jack_default_audio_sample_t[frame_size];
	
	setDelayLength(duration);
	setDecay(decay);
	setLevel(level);
}

void Delay_Buffer::setDelayLength(double seconds)
{
	if (seconds == 0) _active = 0;
	else _active = 1;
	
	uint32_t samples_per_frame = (seconds * SAMPLE_RATE) / _frame_size;
	_max_buffer_ind = samples_per_frame * _frame_size - 1;
	if (_max_buffer_ind == -1) _max_buffer_ind = _frame_size - 1;
	printf("Max buffer ind: %d\n", _max_buffer_ind);
}

void Delay_Buffer::setDecay(double decay)
{
	if (decay > 1.0)		_decay = 1.0;
	else if (decay < 0.0) 	_decay = 0.0;
	else 					_decay = decay;
}

void Delay_Buffer::setLevel(double level)
{
	if (level > 1.0) 		_level = 1.0;
	else if (level < 0.0) 	_level = 0.0;
	else 					_level = level;
}

void Delay_Buffer::newFrame(jack_default_audio_sample_t *sample)
{
	if (_buffer_ind + _frame_size > _max_buffer_ind) {
		_buffer_ind = 0;
	}
	
	for (uint16_t i = 0; i < _frame_size; i++) {
		_buffer[_buffer_ind + i] = _buffer[_buffer_ind + i] * _decay;
		*(_output_buffer + i) = (_active == 1 ? _fx_processor->process(_buffer[_buffer_ind + i]) * _level : 0) + *(sample + i); // add processFX to _buffer[_buffer_ind+i]
		
		
		_buffer[_buffer_ind + i] = _buffer[_buffer_ind + i] + *(sample + i); 
	}
	
	_buffer_ind += _frame_size;
}
//*/

#endif