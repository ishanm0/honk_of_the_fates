#include <math.h>
// #include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Board.h>

#include <ADC.h>
#include <buttons.h>
#include <pwm.h>
#include <QEI.h>
#include <timers.h>

#include "BT.h"
#include "IR.h"
#include "neopixel.h"

extern void map2color(int *red, int *green, int *blue, int degree);

#define NO_BUTTON 0x0 // no buttons pressed
#define BUTTON_1 0x1  // button 0 pressed
#define BUTTON_2 0x2  // button 1 pressed
#define BUTTON_3 0x4  // button 2 pressed
#define BUTTON_4 0x8  // button 3 pressed

// The player's ID number as will be assigned by the computer.
uint8_t playerID = 0;
uint8_t BT_sendable_byte[DATA_SIZE];
char flag;
int modulo;

int sentMessageLengths[256];               // save length of message in position i
uint8_t sendMessageGarage[256][DATA_SIZE]; // save message in position i
// i = (i + 1) % 256; // increment the message id
uint8_t activeID = 0;                       // ID to be assigned to the next sent message (cycles from 0->255)

uint8_t sendIRflag = FALSE;                 // flag controlling whether IR transmission is called in the main loop during game

typedef enum
{                 // see google doc stm32 state machine
    STATE_IDLE,   // initialize connection
    STATE_P_INIT, // choose color
    STATE_CON,    // Send color Pack
    STATE_WAIT,   // wait for game acknowledgement
    STATE_START,  // all the loops
} main_sm_t;

int initFunc(void) // initializes everything we need
{
    BOARD_Init();      // Board initialization
    QEI_Init();        // Encoder + encoder RGB initialization
    PWM_Init();        // PWM initialization
    BUTTONS_Init();    // Button initialization
    TIMER_Init();      // Timer initialization
    BT_Init();         // Bluetooth initialization
    WS2812_SPI_Init(); // RGB library initialization

    // IR transmission setup
    PWM_SetDutyCycle(PWM_5, 50);    // seems like a reasonable place to put it
    PWM_SetFrequency(30000);        // set to 30kHz
    PWM_Stop(PWM_5);                // stop the sound until it is activated in loop

    // All libraries should now be ready to go.
    return SUCCESS;
}

int main(void)
{
    initFunc();
    main_sm_t state = STATE_IDLE;
    int red = 0;
    int green = 0;
    int blue = 0;
    // int color_select = FALSE; // <---- what is this for exactly? ???????
    char color[20];

    uint8_t BT_buffer[256];
    int chars_read = 0;
    int colorChosen = FALSE;
    // int activeID = (activeID + 1) % 256; // increment the message id

    while (TRUE)
    {
        uint8_t buttons = ~buttons_state() & 0xF;          // read the buttons and invert the bits
        map2color(&red, &green, &blue, QEI_GetPosition()); // map rotary count to RGB values
        // printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue); // print the RGB values (for debugging purposes
        //  set the duty cycle of the PWM to the RGB values

        // these pins are 100% mapped wrong but don't change them i have the colors where i want them
        PWM_SetDutyCycle(PWM_4, red);
        PWM_SetDutyCycle(PWM_0, green);
        PWM_SetDutyCycle(PWM_2, blue);

        chars_read = BT_Recv(BT_buffer); // read the bluetooth buffer
        BT_buffer[chars_read] = '\0';    // null terminate the buffer
        // store player id

        switch (state)
        {
        case STATE_IDLE:
            // wait for connection confirmation from uart initialization
            if (BT_buffer[0] == 0x08)  // if the bluetooth buffer matches 0x08 (color code assigned from doc)
            {                          // if the string is equal to "connected"
                BT_buffer[0] = 0x01;   // set the first byte of the buffer to 0x01 (ACK)
                BT_Send(BT_buffer, 2); // send ack to laptop
                // increment message id
                activeID = (activeID + 1) % 256;
                // if no ack back don't move on ?????????????????????
                state = STATE_P_INIT; // change the state to STATE_P_INIT
            }
            break;

        case STATE_P_INIT:
            // choose color
            if (buttons & (BUTTON_2 | BUTTON_3 | BUTTON_4))
            {
                // !!!!!!!!! might break but ez fix bc qei colors is supposed to be 1-100, just print them out and change to actual values
                if (red == 255 && green == 10 && blue == 255)
                {
                    strcpy(color, "navy");
                }
                else if (red == 0 && green == 100 && blue == 200)
                {
                    strcpy(color, "rose");
                }
                else if (red == 255 && green == 255 && blue == 0)
                {
                    strcpy(color, "green");
                }
                else if (red == 0 && green == 255 && blue == 255)
                {
                    strcpy(color, "red");
                }
                else if (red == 0 && green == 100 && blue == 255)
                {
                    strcpy(color, "cyan");
                }
                else if (red == 255 && green == 255 && blue == 255)
                {
                    strcpy(color, "yellow");
                }
                else if (red == 255 && green == 10 && blue == 255)
                {
                    strcpy(color, "purple");
                }
                else if (red == 255 && green == 255 && blue == 255)
                {
                    strcpy(color, "white");
                }
                state = STATE_CON; // change the state to STATE_WAIT
            }
            break;

        case STATE_CON:
            // wait for confirmation from stm32

            if (BT_buffer[0] == 0x08)
            {
                BT_buffer[0] = 0x01;
                BT_Send(BT_buffer, 2); // send ack to laptop
                activeID = (activeID + 1) % 256;
                // if no ack back don't move on ?????????????????????
            }
            else if (BT_buffer[0] == 0x01)
            {
                state = STATE_WAIT; // change the state to STATE_START
            }
            else
            { //!!!!!!!1 add timer stuff later
                BT_buffer[0] = 0x07;
                BT_buffer[1] = color;
                BT_Send(BT_buffer, 2 + strlen(color));
                activeID = (activeID + 1) % 256;
                // if no ack back don't move on ?????????????????????
            }
            break;

        case STATE_WAIT:
            if (BT_buffer[0] == 0x02)
            {
                playerID = BT_buffer[1];
                BT_buffer[0] = 0x01;
                BT_Send(BT_buffer, 2); // send ack to laptop
                activeID = (activeID + 1) % 256;
                state = STATE_START; // change the state to STATE_START
            }
            break;

        case STATE_START:

            if (BT_buffer[0] == 0x02)
            {
                BT_buffer[0] = 0x01;
                BT_Send(BT_buffer, 2); // send ack to laptop
                activeID = (activeID + 1) % 256;
            }

            // service button input
            else if (buttons & BUTTON_2)
            {
                BT_buffer[0] = 0x03;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 2;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256;
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE;                  // set flag so it sends IR
            }
            else if (buttons & BUTTON_3)
            {
                BT_buffer[0] = 0x03;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 3;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256;
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE;                  // set flag so it sends IR
            }
            else if (buttons & BUTTON_4)
            {
                BT_buffer[0] = 0x03;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 4;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256;
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE;                  // set flag so it sends IR
            }
            else
            {
                spellPulse(UNSPECIFIED, FALSE);
            }

            if(sendIRflag == TRUE){                 // send IR if needed
                if(sendIRsignal(playerID)==TRUE)    // run service and check if done sending
                    sendIRflag = FALSE;             // reset the flag if done sending
            }

            if (IR_Detect() == TRUE)
            {
                flag = TRUE;
            }
            if ((IR_timecheck() + 300 < TIMERS_GetMilliSeconds()) && (flag == TRUE))
            {

                if (!(abs(IR_Count() - playerID * 3) <= 1))
                {
                    modulo = IR_Count() % 3;
                    BT_buffer[0] = 0x04;
                    BT_buffer[1] = activeID;
                    BT_buffer[2] = playerID;
                    if (modulo == 0 || modulo == 1)
                    {
                        // return opp id IR_Count()/3
                        BT_buffer[3] = IR_Count() / 3;
                    }
                    else if (modulo == 2)
                    {
                        BT_buffer[3] = (IR_Count() + 1) / 3;
                    }
                    BT_Send(BT_buffer, 4);
                    // if no ack back don't move on ?????????????????????
                    activeID = (activeID + 1) % 256;
                    
                }
                flag = FALSE;
            }
            // check for bluetooth messages !!!!!!!! we need to implement the below:
            //  For this we need to:
            //      check for stray player ID assignment messages and re-give acknoledgement
            //      check for acknoledgement and clear the associated message from the sent message queue
            //      periodically check queue of sent messages to make sure it is empty
            //          if some not cleared then resend themc

            // service IR

            break;
        } // state machine initialization
    }
}

// // start the process
//                 // if IR receiver counts x amount of pulses
//                 if ((IR_Count() != playerID * 3) && (TIMERS_GetMicroSeconds() > 5000))
//                 { // if the IR receiver counts 10 pulses
//                     // send hit and time to stm32
//                     // char time = itoa(TIMERS_GetMicroSeconds(), time, 10); // convert the time to a string
//                     // char pkt = "hit " + time;                             // send the string "hit" and the time
//                     // uart_send(pkt);                                       // not a real function, just pseudo code

//                     // NEED TO CREATE AND SEND THE PACKET SHOWING A REGISTERED HIT !!!!!!!!!!!!!!!!!!!
//                     // 0x04 = shot received
//                     //     0x04 [msg id, 1 char] [receiving player’s id, 1 char] [sending player’s id, 1 char] (4 bytes)
//                     //     (STM → laptop)
//                     BT_sendable_byte[0] = 0x04;
//                     BT_sendable_byte[0] = 0x04;                   // !!!!!!!! ADD proper message ID
//                     BT_sendable_byte[2] = playerID;               // [receiving player’s id, 1 char]
//                     BT_sendable_byte[3] = (char)(IR_Count() / 3); // [sending player’s id, 1 char]
//                     BT_Send(*BT_sendable_byte, 4);                // Send the 4 char "shot received" message
//                 }
//                 else if (strcmp("miss", (char *)BT_buffer) == 0)
//                 { // if the string is equal to "missed"y
//                   // state = STATE_PROCESSING; // change the state to STATE_WAIT
//                 }