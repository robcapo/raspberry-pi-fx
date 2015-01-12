/*
 * tap_tempo.h
 *
 *  Created on: May 6, 2014
 *      Author: Rob
 */
 
/** @file
 *
 * @author Rob Capo
 *
 * This file includes the interface for registering taps of a button and outputting the 
 * tempo at which they were tapped.
 */

#ifndef TAP_TEMPO_H_
#define TAP_TEMPO_H_

#include "timing.h"

/** MAX_TAPS can be any integer, specifying the maximum number of taps to
 * record in order to calculate the tempo
 */
#define MAX_TAPS 4

/** Call this when the user presses the button */
void RegisterTap(void);

/** Returns the average time between taps */
tint_t GetTempo(void);

tint_t GetTap(int i);

#endif /* TAP_TEMPO_H_ */
