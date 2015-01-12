/** @file
 *  @addtogroup button Register button callbacks
 *
 * @author Rob Capo
 *
 * @{
 * 
 * @brief Use this file to register a callback for either the press or release of a button
 * on the Tiva C.
 */
#include "subsys.h"
#include "drivers/buttons.h"

#ifndef BUTTON_H_
#define BUTTON_H_

/** Sets up buttons for callbacks */
void ButtonInit(void);

/** Poll buttons for changes and call functions if necessary
 *
 * Call this function as frequently as possible in main.
 */
void ButtonTick(void);

/** Register a callback for a given button
 *
 * Used to register a callback function for a given button on either press or release.
 *
 * @param btnMask The mask of the button as defined in buttons.h (LEFT_BUTTON or RIGHT_BUTTON on Tiva C)
 * @param upDown 1 if function should be called on press, 0 if funciton should be called on release
 * @param fn Pointer to the function that should be called.
 */
void ButtonRegisterCallback(uint8_t btnMask, uint8_t upDown, void (*fn)(void));

#endif /* BUTTON_H_ */
/** @} */