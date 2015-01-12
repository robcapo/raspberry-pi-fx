/*
 * system.h EXAMPLE FILE
 *
 *  Created on: Mar 12, 2014
 *      Author: Michael
 *
 *  Edited on: Apr 9, 2014
 *  	Author: Rob Capo
 *
 *  Added CAP_MIDTERM subsys to test and demo code
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_
#include <stdint.h>
#include <stdbool.h>

// change this to 1000000 for use with main_games_MSP430
#define PERIPHERAL_CLOCK 50000000

#define UART_TX_BUFFER_LENGTH 1024

#define USE_UART0
#define USE_UART1
// change this to 9600 for use with main_games_MSP430
#define UART_BAUD 115200


// subsytem enumeration needed for the subsys module
enum sys_index {
    SYSTEM = 0,
    // ADD SUBSYSTEMS BELOW //
    MUH_GAME1,
    MUH_GAME2,
    MUH_GAME3,
    CAP_GAME,
    CAP_GAME2,
    CAP_MIDTERM,
    // ADD SUBSYSTEMS ABOVE //
    UNKNOWN
};

#endif /* SYSTEM_H_ */
