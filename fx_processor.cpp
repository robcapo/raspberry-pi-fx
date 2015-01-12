/** @file
 * @addtogroup fx FX Processor
 *
 * @author Rob Capo
 *
 * @{
 * @brief This file contains the class interface and implementation for the FX processing object.
 * Details follow.
 */
#pragma once

#ifndef FX_PROCESSOR_CPP_
#define FX_PROCESSOR_CPP_

#include <iostream>
#include <unistd.h>
#include <jack/jack.h>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>


// 2 seconds * 44100 Hz
#define BUFFER_CAPACITY 88200 ///< Maximum size in samples of the delay buffer
#define SAMPLE_RATE 	44100 ///< Sampling rate of the sound card

enum FX_types { NONE, OVERDRIVE, DISTORTION, REVERB, TREMOLO, WAH }; ///< Different types of FX

typedef double fxparam; ///< Type to hold an FX parameter (i.e. distortion level, overdrive, tremolo rate, etc.

enum FX_param_types {
	NO_PARAM,
	// ADD FX PARAMS AFTER HERE
	
	OD_DRIVE, 					// OVERDRIVE
	TR_RATE, TR_OFF_VOLUME, 	// TREMOLO
	DS_DIST,					// DISTORTION
	RV_DECAY,					// REVERB
	WAH_DURATION,				// WAH
	
	// ADD FX BEFORE HERE
	LAST_PARAM
}; ///< The different paramters for each FX.


class FX_Processor
{
	public:
		/** Runs the current FX on a sample by sample basis.
		 *
		 * This function should be called for each sample in the frame. It will output
		 * the processed frame based on what the selected FX is.
		 *
		 * @param sample The sample to process
		 *
		 * @return Processed sample
		 */
		jack_default_audio_sample_t process(jack_default_audio_sample_t sample);
		
		/** Changes the FX to a type defined by the FX_types enum
		 *
		 * @param type An FX type defined in the FX_types enum
		 */
		void setFx(FX_types type);
		
		/** Sets a parameter value for any of the FX in the class.
		 *
		 * The FX does not have to be active in order to set its parameters. Therefore,
		 * default parameters can be set up upon initialization, and they will be
		 * remembered as the FX type is switched.
		 *
		 * @param param The parameter to change as defined in the FX_param_types enum.
		 * @param value The value of the parameter to set.
		 */
		void setParam(FX_param_types param, fxparam value);
		
		/** Switch to the next FX (goes in order of the FX_types enum). */
		void nextFx(void);
		
		/** Initialize the FX_Processor with a given type defined in FX_types enum. */
		FX_Processor(FX_types type);
	private:
		FX_types _fx_type;
		fxparam _fx_params[LAST_PARAM - NO_PARAM];
		
		// trem members
		uint32_t _trem_counter;
		uint32_t _max_trem_count;
		int _trem_state;
		
		// overdrive members
		double _od_k;
		
		// reverb members
		jack_default_audio_sample_t *_rv_buf;
		uint32_t _rv_counter;
		uint32_t _rv_max_ind;
		
		// wah members
		jack_default_audio_sample_t _wah_yh[BUFFER_CAPACITY];
		jack_default_audio_sample_t _wah_yb[BUFFER_CAPACITY];
		jack_default_audio_sample_t _wah_yl[BUFFER_CAPACITY];
		jack_default_audio_sample_t _wah_F1[BUFFER_CAPACITY];
		uint32_t _wah_max_ind;
		uint32_t _wah_counter;
};

FX_Processor::FX_Processor(FX_types type)
{
	setFx(type);
	_trem_counter = 0;
	_trem_state = 1;
	
	uint32_t rv_buf_size = SAMPLE_RATE * .2;
	_rv_buf = new jack_default_audio_sample_t[rv_buf_size]();
	_rv_counter = 0;
	_rv_max_ind = rv_buf_size - 1;
	
	_wah_counter = 0;
	
	setParam(OD_DRIVE, 1);
	setParam(TR_RATE, .2);
	setParam(TR_OFF_VOLUME, 0);
	setParam(DS_DIST, .8);
	setParam(RV_DECAY, .5);
	setParam(WAH_DURATION, 1.5);
}

jack_default_audio_sample_t FX_Processor::process(jack_default_audio_sample_t sample)
{
	switch (_fx_type) {
		case NONE:
			return sample;
			break;
		case OVERDRIVE:
			sample = (1 + _od_k) * sample / (1 + _od_k * abs(sample));
			
			return sample;
			break;
		case DISTORTION:
			sample = 5 * _fx_params[DS_DIST] * sample;
			if (sample > 0.2) {
				sample = 0.2;
			} else if (sample < -0.2) {
				sample = -0.2;
			}
			
			return sample;
			break;
		case REVERB:
			_rv_buf[_rv_counter] = _fx_params[RV_DECAY]*_rv_buf[_rv_counter] + sample;
			
			sample = _rv_buf[_rv_counter];
			
			if (++_rv_counter > _rv_max_ind) _rv_counter = 0;
			
			return sample;
			break;
		case TREMOLO:
			_trem_counter++;
			if (_trem_counter == _max_trem_count) {
				_trem_state = !_trem_state;
				_trem_counter = 0;
			}
			
			if (_trem_state == 1) {
				return sample;
			} else {
				return _fx_params[TR_OFF_VOLUME] * sample;
			}
			break;
		case WAH:
			if (_wah_counter == 0) {
				_wah_yh[_wah_counter] = sample;
				_wah_yb[_wah_counter] = 0;
				_wah_yl[_wah_counter] = 0;
			} else {
				_wah_yh[_wah_counter] = sample - _wah_yl[_wah_counter - 1] - 0.1 * _wah_yb[_wah_counter - 1];
				_wah_yb[_wah_counter] = _wah_F1[_wah_counter] * _wah_yh[_wah_counter] + _wah_yb[_wah_counter - 1];
				_wah_yl[_wah_counter] = _wah_F1[_wah_counter] * _wah_yb[_wah_counter] + _wah_yl[_wah_counter - 1];
			}
			
			sample = _wah_yb[_wah_counter];
			
			if (++_wah_counter > _wah_max_ind) _wah_counter = 0;
			return sample;
			break;
		
		default:
			return sample;
	}
}

void FX_Processor::nextFx(void)
{
	if (_fx_type == WAH) {
		setFx(static_cast<FX_types>(0));
	} else {
		setFx(static_cast<FX_types>(static_cast<int>(_fx_type)+1) );
	}
}

void FX_Processor::setFx(FX_types type)
{
	_fx_type = type;
	
	if (type == WAH) {
		_wah_counter = 0;
		printf("Now WAHing\n");
	} else if (type == REVERB) {
		_rv_counter = 0;
		printf("Now VERBing\n");
	} else if (type == TREMOLO) {
		_trem_counter = 0;
		printf("Now TREMing\n");
	} else if (type == OVERDRIVE) {
		printf("Now ODing\n");
	} else if (type == DISTORTION) {
		printf("Now DISTing\n");
	} else if (type == NONE) {
		printf("NOFXing\n");
	}
}

void FX_Processor::setParam(FX_param_types param, fxparam value)
{
	if (param == TR_RATE) {
		_max_trem_count = SAMPLE_RATE * value;
		printf("Set tremolo frame length to %d\n", _max_trem_count);
	} else if (param == OD_DRIVE) {
		_od_k = 2 * sin(((value * 100 + 1) / 101) * 3.14159/2);
		printf("Set overdrive to %f\n", _od_k);
	} else if (param == WAH_DURATION) {
		uint32_t samples = value * SAMPLE_RATE;
		_wah_max_ind = samples;
		for (uint32_t i = 0; i < samples; i = i + 1) {
			_wah_F1[i] = 2*sin(3.14159*(((jack_default_audio_sample_t)i/(samples-1)) * 3000 + 500)/SAMPLE_RATE);
		}
	}
	_fx_params[param] = value;
}

#endif

/** @} */