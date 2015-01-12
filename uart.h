/**  * @file
 * @addtogroup uart UART Module
 * @{
 * 	@brief Contains the functionality for UART on the Tiva C and MSP430
 *  @created Mar 12, 2014
 *  @author Michael
 */

#ifndef UART_H_
#define UART_H_
#include "buffer.h"
#include "system.h"

/** Initialize UART module
 *
 * Example usage:
 * @code
 * UART_Init(UART0_BASE, &rx0, &tx0, 115200);
 * @endcode
 *
 * @param base - Memory base of UART module if system supports multiple modules, input 0 otherwise
 * @param rx - pointer to receive buffer of type #buffer_t
 * @param tx - pointer to transmit buffer of type #buffer_t
 * @param baud - desired baud rate
 */
void UART_Init(uint32_t base, buffer_t * rx, buffer_t * tx, uint32_t baud);

// processor specific defines
#ifdef __MSP430G2553__
#include <msp430.h>
#define NUM_UARTS 1
#define UART0_BASE 0 // there is only 1 UART so just set base to 0, this allows
					// compatibility with multi UART processors
#define UART_InitHardware(base) P1SEL = BIT1 + BIT2 ; P1SEL2 = BIT1 + BIT2 // P1.1 = RXD, P1.2=TXD
#define UART_Disable(base) UCA0CTL1 = 0
#define UART_SetBaud(base,baud) UCA0BR0 = PERIPHERAL_CLOCK / baud; \
	UCA0BR1 = PERIPHERAL_CLOCK / baud / 256; \
	UCA0MCTL = ((PERIPHERAL_CLOCK*8) / baud - (PERIPHERAL_CLOCK / baud) * 8) << 1
#define UART_Configure(base) UCA0CTL1 |= UCSSEL_2
#define UART_EnableInterrupts(base) IE2 |= UCA0RXIE
#define UART_TX_ISR(void) _Pragma("vector=USCIAB0TX_VECTOR") \
		__interrupt void _UART_TX_ISR(void)
#define UART_RX_ISR(void) _Pragma("vector=USCIAB0RX_VECTOR") \
		__interrupt void _UART_RX_ISR(void)
#define UART_TX_Char(base,c) UCA0TXBUF = c
#define UART_RX_Char(base) UCA0RXBUF
#define READ_UART_TX_IF(base) ((IFG2 & 0x02) >> 1)
#define CLEAR_UART_TX_IF(base) IFG2 &= ~UCA0TXIFG
#define CLEAR_UART_RX_IF(base) IFG2 &= ~UCA0RXIFG
#define CLEAR_UART_TX_IE(base) IE2 &= ~UCA0TXIE
#define SET_UART_TX_IE(base) IE2 |= UCA0TXIE
#define UART_CAN_TRANSMIT(base) (IFG2 & UCA0TXIFG)
#define UART_DATA_AVAILABLE(base) (IFG2 & UCA0RXIFG)

#elif defined PART_TM4C123GH6PM
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#define NUM_UARTS 8
void UART_InitHardware(uint32_t base);
#define UART_Disable(base) //UARTDisable(base)
#define UART_SetBaud(base,baud) UARTConfigSetExpClk(base, PERIPHERAL_CLOCK, baud, UART_CONFIG_WLEN_8)
#define UART_Configure(base) UARTFIFODisable(base); UARTRxErrorClear(base) //UARTFIFOEnable(base); UARTFIFOLevelSet(base, UART_FIFO_TX1_8, UART_FIFO_RX1_8)
#define UART_EnableInterrupts(base) UARTIntEnable(base, UART_INT_RX | UART_INT_TX);

#define UART_TX_Char(base,c) UARTCharPutNonBlocking(base, c)
#define UART_RX_Char(base) UARTCharGetNonBlocking(base)
//#define READ_UART_TX_IF(base) UARTIntStatus(base, UART_INT_TX)
//#define CLEAR_UART_TX_IF(base) UARTIntClear(base, UART_INT_RX)
//#define CLEAR_UART_RX_IF(base) UARTIntClear(base, UART_INT_TX)
//#define CLEAR_UART_TX_IE(base) UARTIntDisable(base, UART_INT_TX)
#define SET_UART_TX_IE(base) UARTIntEnable(base, UART_INT_TX)
#define UART_CAN_TRANSMIT 0
#define UART_DATA_AVAILABLE 0

#else
#error "Processor not supported"
#endif

/** @}*/
#endif /* UART_H_ */
