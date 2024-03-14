#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <QEI.h>
#include <stdbool.h>
#include <math.h>
#include <buttons.h>

#define NO_BUTTON 0x0 // no buttons pressed
#define BUTTON_1 0x1 // button 0 pressed
#define BUTTON_2 0x2 // button 1 pressed
#define BUTTON_3 0x4 // button 2 pressed
#define BUTTON_4 0x8 // button 3 pressed

int main(void) {
    BOARD_Init();
    QEI_Init();
    PWM_Init();
    BUTTONS_Init();

    while(TRUE) {
        int red = 0;
        int green = 0;
        int blue = 0;
        uint8_t buttons = ~buttons_state() & 0xF; // read the buttons and invert the bits
        map2color(&red, &green, &blue, QEI_GetPosition()); // map rotary count to RGB values
        // set the duty cycle of the PWM to the RGB values
        // PWM_SetDutyCycle(PWM_2, red);
        // PWM_SetDutyCycle(PWM_1, green);
        // PWM_SetDutyCycle(PWM_0, blue);
        
        if (buttons & BUTTON_1) {
            printf("Button 1 pressed\n");
        } else if (buttons & BUTTON_2) {
            printf("Button 2 pressed\n"); // pin 26
        } else if (buttons & BUTTON_3) {
            printf("Button 3 pressed\n"); // pin 27
        } else if (buttons & BUTTON_4) {
            printf("Button 4 pressed\n"); // pin 30
        }
    }
}
