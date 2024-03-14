#include <stdio.h>
#include <stdlib.h>
#include <Board.h>
#include <ADC.h>
#include <pwm.h>
#include <QEI.h>
#include <stdbool.h>
#include <math.h>

int main(void) {
    BOARD_Init();
    QEI_Init();
    PWM_Init();

    while(TRUE) {
        int rotary_count = QEI_GetPosition();
        printf(">rotary_count: %d\n", rotary_count);
        int red = 0;
        int green = 0;
        int blue = 0;
        map2color(&red, &green, &blue, rotary_count); // map rotary count to RGB values
        // set the duty cycle of the PWM to the RGB values
        PWM_SetDutyCycle(PWM_2, red);
        PWM_SetDutyCycle(PWM_1, green);
        PWM_SetDutyCycle(PWM_0, blue);
    }
}
