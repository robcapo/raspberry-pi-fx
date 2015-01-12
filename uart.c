/*
 * uart.c
 *
 *  Created on: Mar 12, 2014
 *      Author: Michael
 */
#include "uart.h"

struct {
	buffer_t * rx;
	buffer_t * tx;
} uart[NUM_UARTS];

#if NUM_UARTS == 8
void UART0_ISR(void);
void UART1_ISR(void);
void UART2_ISR(void);
void UART3_ISR(void);
void UART4_ISR(void);
void UART5_ISR(void);
void UART6_ISR(void);
void UART7_ISR(void);
void UART_ISR(uint32_t base, int index);
void UART0_TxCallback(buffer_t * buf);
void UART1_TxCallback(buffer_t * buf);
void UART2_TxCallback(buffer_t * buf);
void UART3_TxCallback(buffer_t * buf);
void UART4_TxCallback(buffer_t * buf);
void UART5_TxCallback(buffer_t * buf);
void UART6_TxCallback(buffer_t * buf);
void UART7_TxCallback(buffer_t * buf);
#endif

void TxCallback(buffer_t * buf);

void UART_Init(uint32_t base, buffer_t * rx, buffer_t * tx, uint32_t baud) {
	UART_InitHardware(base);
	UART_Disable(base);
	UART_SetBaud(base,baud);
	UART_Configure(base);
#if NUM_UARTS == 8
	switch(base) {
	case UART0_BASE:
		uart[0].rx = rx;
		uart[0].tx = tx;
		UARTIntRegister(UART0_BASE, UART0_ISR);
		BufferSetCallback(tx, UART0_TxCallback);
	    break;
	case UART1_BASE:
		uart[1].rx = rx;
		uart[1].tx = tx;
		UARTIntRegister(UART1_BASE, UART1_ISR);
		BufferSetCallback(tx, UART1_TxCallback);
		break;
	case UART2_BASE:
		uart[2].rx = rx;
		uart[2].tx = tx;
		UARTIntRegister(UART2_BASE, UART2_ISR);
		BufferSetCallback(tx, UART2_TxCallback);
		break;
	case UART3_BASE:
		uart[3].rx = rx;
		uart[3].tx = tx;
		UARTIntRegister(UART3_BASE, UART3_ISR);
		BufferSetCallback(tx, UART3_TxCallback);
		break;
	case UART4_BASE:
		uart[4].rx = rx;
		uart[4].tx = tx;
		UARTIntRegister(UART4_BASE, UART4_ISR);
		BufferSetCallback(tx, UART4_TxCallback);
		break;
	case UART5_BASE:
		uart[5].rx = rx;
		uart[5].tx = tx;
		UARTIntRegister(UART5_BASE, UART5_ISR);
		BufferSetCallback(tx, UART5_TxCallback);
		break;
	case UART6_BASE:
		uart[6].rx = rx;
		uart[6].tx = tx;
		UARTIntRegister(UART6_BASE, UART6_ISR);
		BufferSetCallback(tx, UART6_TxCallback);
		break;
	case UART7_BASE:
		uart[7].rx = rx;
		uart[7].tx = tx;
		UARTIntRegister(UART7_BASE, UART7_ISR);
		BufferSetCallback(tx, UART7_TxCallback);
		break;
	default:

		break;
	}
#else
	uart[0].rx = rx;
	uart[0].tx = tx;
	BufferSetCallback(tx, TxCallback);
#endif
	UART_EnableInterrupts(base);
}

void TxCallback(buffer_t * buf) {
	// make sure the interrupt is enabled
	SET_UART_TX_IE(0);
}

#ifdef UART_TX_ISR
UART_TX_ISR(void) {
	while(UART_CAN_TRANSMIT(0))
	{
		if(GetSize(uart[0].tx)){
			UART_TX_Char(0,Pop(uart[0].tx));
		}
		else {
			CLEAR_UART_TX_IE(0);
			break;
		}
	}
}
#endif
#ifdef UART_RX_ISR
UART_RX_ISR(void) {
	while(UART_DATA_AVAILABLE(0)) {
		Push(uart[0].rx,UART_RX_Char(0));
	}
}
#endif

////////////////// uC Specific
#if NUM_UARTS == 8
void UART0_ISR(void) { UART_ISR(UART0_BASE, 0); }
void UART1_ISR(void) { UART_ISR(UART1_BASE, 1); }
void UART2_ISR(void) { UART_ISR(UART2_BASE, 2); }
void UART3_ISR(void) { UART_ISR(UART3_BASE, 3); }
void UART4_ISR(void) { UART_ISR(UART4_BASE, 4); }
void UART5_ISR(void) { UART_ISR(UART5_BASE, 5); }
void UART6_ISR(void) { UART_ISR(UART6_BASE, 6); }
void UART7_ISR(void) { UART_ISR(UART7_BASE, 7); }

void UART0_TxCallback(buffer_t * buf) {
	if(UARTSpaceAvail(UART0_BASE)) {
	if(GetSize(uart[0].tx)){
			UART_TX_Char(UART0_BASE, Pop(uart[0].tx));
		}
	} else SET_UART_TX_IE(UART0_BASE);
}
void UART1_TxCallback(buffer_t * buf) {
	if(UARTSpaceAvail(UART1_BASE)) {
	if(GetSize(uart[1].tx)){
			UART_TX_Char(UART1_BASE, Pop(uart[1].tx));
		}
	} else SET_UART_TX_IE(UART1_BASE);
}
/*
	if(UARTIntStatus(UART1_BASE, 1) & UART_INT_TX == 0) {
		if(UARTSpaceAvail(UART1_BASE)) {
			if(GetSize(uart[1].tx)){
				UART_TX_Char(UART1_BASE, Pop(uart[1].tx));
			}
		}
	}
	SET_UART_TX_IE(UART1_BASE);
}*/
void UART2_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART2_BASE); }
void UART3_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART3_BASE); }
void UART4_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART4_BASE); }
void UART5_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART5_BASE); }
void UART6_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART6_BASE); }
void UART7_TxCallback(buffer_t * buf) {	SET_UART_TX_IE(UART7_BASE); }
void UART_ISR(uint32_t base, int index) {
	while(UARTCharsAvail(base)) {
		Push(uart[index].rx,UART_RX_Char(base));
	}
	if(UARTIntStatus(base, 1) & UART_INT_TX) {
		while(UARTSpaceAvail(base)) {
			if(GetSize(uart[index].tx)){
				UART_TX_Char(base, Pop(uart[index].tx));
			}
			else {
				UARTIntDisable(base, UART_INT_TX);
				break;
			}
		}
	}
}
#endif

#ifdef PART_TM4C123GH6PM
void UART_InitHardware(uint32_t base) {
	switch(base) {
	case UART0_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	    GPIOPinConfigure(GPIO_PA0_U0RX);
	    GPIOPinConfigure(GPIO_PA1_U0TX);
	    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	    break;
	case UART1_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
		GPIOPinConfigure(GPIO_PB0_U1RX);
		GPIOPinConfigure(GPIO_PB1_U1TX);
	    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
		break;
	case UART2_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
		GPIOPinConfigure(GPIO_PD6_U2RX);
		GPIOPinConfigure(GPIO_PD7_U2TX);
		GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
		break;
	case UART3_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
		GPIOPinConfigure(GPIO_PC6_U3RX);
		GPIOPinConfigure(GPIO_PC7_U3TX);
		GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
		break;
	case UART4_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
		GPIOPinConfigure(GPIO_PC4_U4RX);
		GPIOPinConfigure(GPIO_PC5_U4TX);
		GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
		break;
	case UART5_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
		GPIOPinConfigure(GPIO_PE4_U5RX);
		GPIOPinConfigure(GPIO_PE5_U5TX);
		GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);
		break;
	case UART6_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
		GPIOPinConfigure(GPIO_PD4_U6RX);
		GPIOPinConfigure(GPIO_PD5_U6TX);
		GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
		break;
	case UART7_BASE:
		SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
		GPIOPinConfigure(GPIO_PE0_U7RX);
		GPIOPinConfigure(GPIO_PE1_U7TX);
		GPIOPinTypeUART(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1);
		break;
	default:
#ifdef __DEBUG__
		while(1);
#endif
		break;
	}
}
#endif // PART_TM4C123GH6PM
