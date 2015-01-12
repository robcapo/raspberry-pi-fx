/*
 * Author: Rob Capo
 */
#include "subsys.h"
#include "task.h"
#include "buffer_printf.h"
#include "button.h"
#include "tap_tempo.h"
#include "inc/tm4c123gh6pm.h"

/** Communication is very simple. We are only sending two types of messages,
 * one to cycle through the effect, and one to change the tempo. Therefore,
 * our message can simply be two bytes. If the byte is 0xFFFF, assume change FX,
 * otherwise assume it is the number of milliseconds to set the delay to.
 */

/** This function will tell the Raspberry Pi to cycle to the next effect. */
void cycle_fx(void);

/** This function will keep track of how quickly the user presses a button and set the tempo of the delay
 *
 * The user should push this button as quickly as he/she wants the echo to repeat. The last 4 taps are
 * considered and averaged.
 */
void tap_tempo_pushed(void);

void blink_led(void);
void turn_off_led(void);


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Main 'C' Language entry point.
// See http://www.ti.com/tm4c123g-launchpad/project0 for more information and
// tutorial videos.
//
//*****************************************************************************
int main(void)
{

	volatile char cThisChar, c;

    //
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|
                    SYSCTL_OSC_MAIN);



    // Initialize system (including UART0 and UART1
    TimingInit();
    TaskInit();
    SystemInit();


    volatile uint32_t ui32Loop;
//*
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;
//*
    //
    // Do a dummy read to insert a few cycles after enabling the peripheral.
    //
    ui32Loop = SYSCTL_RCGC2_R;

    //
    // Enable the GPIO pin for the LED (PF3).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIO_PORTF_DIR_R = 0x08;
    GPIO_PORTF_DEN_R = 0x08;
/*
    //
    // Turn on the LED.
    //
    //GPIO_PORTF_DATA_R |= 0x08;
    //GPIO_PORTF_DATA_R &= ~(0x08);
*/

    ButtonInit();
    ButtonRegisterCallback(LEFT_BUTTON, 0, cycle_fx);
    ButtonRegisterCallback(RIGHT_BUTTON, 1, tap_tempo_pushed);

    // print a message to each terminal
    Push_printf(&tx0, "Hello terminal 0\r\n");
    Push_printf(&tx1, "Hello terminal 1\r\n");

    while(1)
    {
    	// SystemTick will call our receivers when there is data in the
    	// buffers. We do it this way so that we aren't calling the
    	// receivers from within the interrupt.
    	SystemTick();

    	// Button tick will call our button callbacks when the buttons are
    	// pressed.
    	ButtonTick();

    }
}

void cycle_fx(void)
{
	Push_printf(&tx0, "Left button down\r\n");
	Push_printf(&tx1, "a\r\n");
}

void tap_tempo_pushed(void)
{
	RegisterTap();
	tint_t tempo = GetTempo();
/*
	char msg[3];

	msg[0] = tempo / 0xF;
	msg[1] = tempo % msg[0];
	msg[2] = 0;
*/
	if (tempo > 0) {
		Push_printf(&tx0, "%d\r\n", tempo);
		Push_printf(&tx1, "%d\r\n", tempo);
		TaskQueueAdd(blink_led, 10);
	}
}

void blink_led(void)
{
	GPIO_PORTF_DATA_R |= 0x08;
	RemoveTaskFromSchedule(turn_off_led);
	RemoveTaskFromSchedule(blink_led);
	TaskScheduleAdd(turn_off_led, 10, 100, 0);
	TaskScheduleAdd(blink_led, 10, GetTempo(), 0);
}

void turn_off_led(void)
{
	GPIO_PORTF_DATA_R &= ~0x08;
}
