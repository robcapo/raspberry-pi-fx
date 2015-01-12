/*
 * tap_tempo.c
 *
 *  Created on: May 6, 2014
 *      Author: Rob
 */
#include "tap_tempo.h"
#include "timing.h"
#include "subsys.h"
#include "task.h"
#include "buffer_printf.h"

tint_t last_taps[MAX_TAPS];

void RegisterTap(void) {
	tint_t last_tap;
	int i;

	// if buffer is empty, set to now
	if (last_taps[0] == 0) {
		last_taps[0] = TimeNow();
		return;
	}

	// find the last tap time in buffer
	for (i = 0; i < MAX_TAPS; i++) {
		if (last_taps[i] != 0) last_tap = last_taps[i];
		else break;
	}

	// if the last tap was over 2 seconds ago, clear the buffer and set first tap to now
	if (TimeSince(last_tap) > 1999) {
		for (i = 0; i < MAX_TAPS; i++) {
			last_taps[i] = 0;
		}

		last_taps[0] = TimeNow();
		return;
	}

	// if we have filled the buffer, shift all taps down and add TimeNow() to the last slot
	// This may not be the most efficient way to do this (especially if MAX_TAPS is big)
	if (i == MAX_TAPS) {
		int j;

		for (j = 0; j < MAX_TAPS - 1; j++) {
			last_taps[j] = last_taps[j + 1];
		}

		last_taps[--i] = TimeNow();
	} else { // if we haven't filled the buffer, add TimeNow() to the end
		last_taps[i] = TimeNow();
	}
}

tint_t GetTempo(void)
{
	tint_t last_tap;
	int i;
	for (i = 0; i < MAX_TAPS; i++) {
		if (last_taps[i] != 0) last_tap = last_taps[i];
		else break;
	}

	return (last_tap - last_taps[0]) / i;
}

tint_t GetTap(int i)
{
	return last_taps[i];
}
