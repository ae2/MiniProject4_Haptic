/*
	Elecanisms Mini-Project IV using a PIC24F
	
	Shivam Desai and Asa Eckert-Erdheim
	October 28, 2013
	
	Questions? 		   [Shivam@students.olin.edu; Asa@students.olin.edu]
*/

/***************************************************** 
		Includes
**************************************************** */

// Include files
#include <p24FJ128GB206.h> // PIC
#include "config.h"
#include "common.h"
#include "oc.h"			   // output compare
#include "pin.h"
#include "timer.h"
#include "uart.h"
#include "ui.h"
#include "usb.h"
#include <stdio.h>

// Define vendor requests
#define SET_VALS            1   // Vendor request that receives 2 unsigned integer values
#define GET_VALS            2   // Vendor request that returns  2 unsigned integer values
#define PRINT_VALS          3   // Vendor request that prints   2 unsigned integer values 
#define PING_ULTRASONIC     4   // Vendor request that prints 	1 unsigned integer value

// Define names for pins
#define ENCODER         &D[0] // Encoder pin
#define nSF     		&D[1] // Status flag pin
#define nD2	        	&D[2] // Disable IN2 pin
#define D1         	    &D[3] // Disable IN1 pin
#define ENA             &D[4] // Enable pin
#define IN2             &D[5] // Input 2 pin
#define IN1             &D[6] // Input 1 pin
#define SLEW            &D[7] // Slew rate pin
#define INV             &D[8] // Inverting pin

#define CURRENT         &A[0] // Current pin
#define EMF             &A[1] // Back EMF pin
#define FB              &A[2] // Load current feedback pin

// Define names for timers
#define BLINKY_TIMER	&timer1 // blinky light
#define PWM_TIMER		&timer2 // motor

// Define constants
#define freq		250 // run the motor at 250Hz
#define duty_init	0

/***************************************************** 
		Function Prototypes & Variables
**************************************************** */ 

void initChip(void);
void initInt(void);

void __attribute__((interrupt)) _CNInterrupt(void); 

uint16_t LOW  = 0;
uint16_t HIGH = 1;

uint16_t CURRENT_VAL;
uint16_t EMF_VAL;
uint16_t FB_VAL;
uint16_t ENC_COUNT_VAL;

uint16_t DUTY_VAL = 65536*2/5; // 40% duty cycle
uint16_t EMF_MID = 32768;     // middle point for EMF ADC

// uint16_t LED_VAL  = 0;

// uint16_t DUTY_VAL = 65536/2; // 100% duty cycle

// uint16_t PULSE_TICK_COUNT = 0;
// uint16_t OVERFLOW_COUNT = 0;

// uint16_t TOF_VAL = 0;

// uint16_t TIMEOUT_FLAG = 0;


/*************************************************
			Initialize the PIC24F
**************************************************/

void initChip(){
	
    init_clock();
    init_uart();
    init_pin(); 	// initialize the pins for HAPTIC
    init_ui();		// initialize the user interface for BLINKY LIGHT
    init_timer();	// initialize the timer for BLINKY LIGHT
    init_oc(); 		// initialize the output compare module for MOTOR

	pin_digitalIn(ENCODER);     // configure digital inputs
    pin_digitalIn(nSF);

	pin_digitalOut(nD2);        // configure digital outputs
	pin_digitalOut(D1);
    pin_digitalOut(ENA);
    pin_digitalOut(IN2);
    pin_digitalOut(IN1);
    pin_digitalOut(SLEW);
    pin_digitalOut(INV);

    pin_analogIn(CURRENT);      // configure analog inputs
    pin_analogIn(EMF); 
    pin_analogIn(FB); 

	oc_pwm  (&oc1, nD2, PWM_TIMER, freq, duty_init);	// configure motor PWM

}

/*************************************************
            Initialize Interrupts 
**************************************************/

void initInt(void) {

	// Enable encoder change interrupt
	CNEN1bits.CN14IE = 1; 	// configure change notification interrupt D[0]
	IFS1bits.CNIF = 0;		// clear change notification flag D[0]	
	IEC1bits.CNIE = 1;		// enable notification interrupt D[0]

}

/*************************************************
            Initialize Motor 
**************************************************/

void initMotor(void) {

    pin_write(ENA, HIGH);  // Enable motor driver
    pin_write(SLEW, HIGH); // slew rate is fast
    pin_write(INV, LOW);   // invert    OFF
    pin_write(D1, LOW);    // disable D1 OFF
    pin_write(nD2, HIGH);  // disable D2 OFF
    
}

/*************************************************
			Vendor Requests
**************************************************/

void VendorRequests(void) {
    WORD temp;

    switch (USB_setup.bRequest) {
        // case SET_VALS:
        //     PAN_VAL = USB_setup.wValue.w;
        //     TILT_VAL = USB_setup.wIndex.w;
        //     BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
        //     BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
        //     break;
        case GET_VALS:
            temp.w = CURRENT_VAL;
            BD[EP0IN].address[0] = temp.b[0];
            BD[EP0IN].address[1] = temp.b[1];
            temp.w = EMF_VAL;
            BD[EP0IN].address[2] = temp.b[0];
            BD[EP0IN].address[3] = temp.b[1];
            temp.w = FB_VAL;
            BD[EP0IN].address[4] = temp.b[0];
            BD[EP0IN].address[5] = temp.b[1];
            temp.w = ENC_COUNT_VAL;
            BD[EP0IN].address[6] = temp.b[0];
            BD[EP0IN].address[7] = temp.b[1];

            BD[EP0IN].bytecount = 8;    // set EP0 IN byte count to 4
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;            
        default:
            USB_error_flags |= 0x01;    // set Request Error Flag
    }
}

void VendorRequestsIn(void) {
    switch (USB_request.setup.bRequest) {
        default:
            USB_error_flags |= 0x01;                    // set Request Error Flag
    }
}

void VendorRequestsOut(void) {
    switch (USB_request.setup.bRequest) {
        default:
            USB_error_flags |= 0x01;                    // set Request Error Flag
    }
}

/*************************************************
            Interrupt Service Routines
**************************************************/

void encoder_serviceInterrupt() {
    IFS1bits.CNIF = 0; // clear change notification flag D[0]	
    ENC_COUNT_VAL ++;  // increment the encoder
}

/*************************************************
            Interrupt Declarations
**************************************************/

void __attribute__((interrupt, auto_psv)) _CNInterrupt(void) {
    encoder_serviceInterrupt();
}                   

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int16_t main(void) {
	
	initChip();						// initialize the PIC pins etc.
    InitUSB();                      // initialize the USB registers and serial interface engine
    initInt();						// initialize the interrupt pins
    initMotor();					// initialize the motor pins

    led_on(&led1);					// initial state for BLINKY LIGHT
    timer_setPeriod(BLINKY_TIMER, 1);	// timer for BLINKY LIGHT
    timer_start(BLINKY_TIMER);
	
	// Motor commands
    pin_write(IN1, HIGH);  // keep one input high
    pin_write(IN2, LOW);   // keep one input low
	pin_write(nD2, DUTY_VAL);  // invert D2 ON using dutycycle

    while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
        
        ServiceUSB();                       // ...service USB requests
        
    }

    while (1) {

        ServiceUSB(); 

        if (timer_flag(BLINKY_TIMER)) {	// when the timer trips
            timer_lower(BLINKY_TIMER);
            led_toggle(&led1);			// toggle the BLINKY LIGHT
        }
			
        CURRENT_VAL = pin_read(CURRENT);
        EMF_VAL = pin_read(EMF);
        FB_VAL = pin_read(FB);
        
        //ENC_COUNT_VAL = pin_read(ENCODER);
        
    }
}
