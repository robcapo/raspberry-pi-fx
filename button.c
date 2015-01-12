/*
 * button.c
 *
 *  Created on: May 6, 2014
 *      Author: Rob
 */
#include "button.h"

#define MAX_BTN_CALLBACKS 8

uint8_t btnDelta, btnRaw, btnData;

typedef struct {
	uint8_t mask;
	void (*callback_fn)(void);
	uint8_t upDown;
} btn_callback_t;

btn_callback_t btnCallbacks[MAX_BTN_CALLBACKS];

void ButtonInit(void)
{
    ButtonsInit();
}

void ButtonTick(void)
{
	btnData = ButtonsPoll(&btnDelta, &btnRaw);
	//*
	if (btnDelta != 0) {
		int i;
		for (i = 0; i < MAX_BTN_CALLBACKS; i++) {
			if ((btnCallbacks[i].mask & btnDelta) != 0) {
				if ( ((btnCallbacks[i].mask & btnData) != 0 && btnCallbacks[i].upDown == 1) ||
						((btnCallbacks[i].mask & btnData) == 0 && btnCallbacks[i].upDown == 0)) {

					btnCallbacks[i].callback_fn();

				}
			}
		}
	}
	//*/
}

void ButtonRegisterCallback(uint8_t btnMask, uint8_t upDown, void (*fn)(void)) {
	int i;
	for (i = 0; i < MAX_BTN_CALLBACKS; i++) {
		if (btnCallbacks[i].mask == 0) {
			btnCallbacks[i].mask = btnMask;
			btnCallbacks[i].upDown = upDown;
			btnCallbacks[i].callback_fn = fn;
			return;
		}
	}
}
