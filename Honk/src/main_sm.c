#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <QEI.h>
#include <stdbool.h>
#include <math.h>
#include <buttons.h>
#include <timers.h>
#include "IR.h"

#define NO_BUTTON 0x0 // no buttons pressed
#define BUTTON_1 0x1 // button 0 pressed
#define BUTTON_2 0x2 // button 1 pressed
#define BUTTON_3 0x4 // button 2 pressed
#define BUTTON_4 0x8 // button 3 pressed

typedef enum { // see google doc stm32 state machine
    STATE_IDLE,
    STATE_P_INIT,
    STATE_WAIT,
    STATE_START,
    STATE_PROCESSING,
} main_sm_t;

void uart_receive() {
    // receive packet from stm32
    // return packet
    
}

void uart_send() {
    // send packet to stm32
}

int main(void) {
    BOARD_Init();
    QEI_Init();
    PWM_Init();
    BUTTONS_Init();
    main_sm_t state = STATE_IDLE;

    while(TRUE) {
        int red = 0;
        int green = 0;
        int blue = 0;
        uint8_t buttons = ~buttons_state() & 0xF; // read the buttons and invert the bits
        map2color(&red, &green, &blue, QEI_GetPosition()); // map rotary count to RGB values
        printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue); // print the RGB values (for debugging purposes
        // set the duty cycle of the PWM to the RGB values

        // these pins are 100% mapped wrong but don't change them i have the colors where i want them
        PWM_SetDutyCycle(PWM_4, red); 
        PWM_SetDutyCycle(PWM_0, green);
        PWM_SetDutyCycle(PWM_2, blue);
        
        // if (buttons & BUTTON_1) {
        //     printf("Button 1 pressed\n");
        // } else if (buttons & BUTTON_2) {
        //     printf("Button 2 pressed\n"); // pin 26
        // } else if (buttons & BUTTON_3) {
        //     printf("Button 3 pressed\n"); // pin 27
        // } else if (buttons & BUTTON_4) {
        //     printf("Button 4 pressed\n"); // pin 30
        // }

        // spell 1 > spell 2
        // spell 2 > spell 3
        // spell 3 > spell 1

        switch (state) {
            case STATE_IDLE:
                // wait for connection confirmation from uart initialization
                if (strcmp("connected", uart_receive()) == 0) { // if the string is equal to "connected"
                    // uart_receive is undeclared, will be packet handler on stm32
                    state = STATE_P_INIT; // change the state to STATE_P_INIT
                }
                break;
            case STATE_P_INIT:
                // choose color
                // if button 1 is pressed and 1 second has passed
                if ((buttons & BUTTON_1) && (TIMERS_GetMicroSeconds() > 1000)) {
                    // save color in string, add color to packet
                    if (red == 255 && green == 10 && blue == 255) {
                        char *color = "orange";
                    } else if (red == 0 && green == 100 && blue == 255) {
                        char *color = "yellow";
                    } else if (red == 255 && green == 255 && blue == 255) {
                        char *color = "white";
                    } else if (red == 255 && green == 10 && blue == 255) {
                        char *color = "orange";
                    } else if (red == 0 && green == 100 && blue == 255) {
                        char *color = "yellow";
                    } else if (red == 255 && green == 255 && blue == 255) {
                        char *color = "white";
                    } else if (red == 255 && green == 10 && blue == 255) {
                        char *color = "orange";
                    } else if (red == 0 && green == 100 && blue == 255) {
                        char *color = "yellow";
                    } else if (red == 255 && green == 255 && blue == 255) {
                        char *color = "white";
                    }
                    // send packet
                    uart_send(color); // not a real function, just pseudo code
                    state = STATE_WAIT; // change the state to STATE_START
                }
                break;
            case STATE_WAIT:
                // wait for confirmation from stm32
                if (strcmp("ack", uart_receive()) == 0) { // if the string is equal to "confirmed"
                    state = STATE_START; // change the state to STATE_START
                }
                break;
            case STATE_START:
                // start the process
                // if IR receiver counts x amount of pulses
                if ((IR_receiver_count() == 10) && (TIMERS_GetMicroSeconds() > 5000)) { // if the IR receiver counts 10 pulses
                    // send hit and time to stm32
                    char time = itoa(TIMERS_GetMicroSeconds(), time, 10); // convert the time to a string
                    char pkt = "hit " + time; // send the string "hit" and the time
                    uart_send(pkt); // not a real function, just pseudo code
                    state = STATE_PROCESSING; // change the state to STATE_PROCESSING
                } else if (strcmp("miss", uart_receive()) == 0) { // if the string is equal to "missed"
                    state = STATE_PROCESSING; // change the state to STATE_WAIT
                }
                break;
            case STATE_PROCESSING:
                // disable buttons until acknolwedgement from stm32
                // wait for confirmation from stm32
                if (strcmp("ack", uart_receive()) == 0) { // if the string is equal to "confirmed"
                    state = STATE_START; // change the state to STATE_IDLE
                }
                break;

        } // state machine initialization
    }
}
