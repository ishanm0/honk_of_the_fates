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

#define ACK_ID 0x1
#define ASSIGN_ID 0x2
#define PP_SENT_ID 0x3
#define PP_RECV_ID 0x4
#define WIN_ID 0x5
#define LOSS_ID 0x6
#define COLOR_ID 0x7
#define CONN_ID 0x8

#define RESENDS_CHECK_TIMEOUT 10   // ms between checks for whether messages need to be resent
#define RESEND_TIMEOUT 500         // ms for resending messages
#define SHOT_PAIRING_TIMEDELTA 500 // ms for shot pairings (max time diff between one player's shot & another's receive)
#define RPS_TIMEOUT 1000           // ms for RPS pairings (max time diff between shots between two players)

// The player's ID number as will be assigned by the computer.
uint8_t playerID = 0;
uint8_t BT_sendable_byte[DATA_SIZE];
char flag;
int modulo;

int sentMessageLengths[256];               // save length of message in position i
uint8_t sentMessageGarage[256][DATA_SIZE]; // save message in position i
uint32_t sentMessageTimes[256];
uint8_t alreadyAcked[256];
// i = (i + 1) % 256; // increment the message id
uint8_t activeID = 0; // ID to be assigned to the next sent message (cycles from 0->255)

uint8_t sendIRflag = FALSE; // flag controlling whether IR transmission is called in the main loop during game

uint32_t lastResend = 0;

typedef enum
{                       // see google doc stm32 state machine
    STATE_CON_WAIT,     // initialize connection
    STATE_CHOOSE_COLOR, // choose color
    STATE_COLOR_ACK,    // Send color Pack
    STATE_WAIT_ASSIGN,  // wait for game acknowledgement
    STATE_START,        // all the loops
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
    PWM_SetDutyCycle(PWM_5, 50); // seems like a reasonable place to put it
    PWM_SetFrequency(30000);     // set to 30kHz
    PWM_Stop(PWM_5);             // stop the sound until it is activated in loop

    // All libraries should now be ready to go.
    return SUCCESS;
}

int main(void)
{
    initFunc();
    main_sm_t state = STATE_CON_WAIT;
    int red = 0;
    int green = 0;
    int blue = 0;
    // int color_select = FALSE; // <---- what is this for exactly? ???????
    char color[20];

    uint8_t BT_buffer[256];
    int chars_read = 0;
    // int colorChosen = FALSE;
    // int activeID = (activeID + 1) % 256; // increment the message id
    int inputHandled = 0;
    uint8_t colorMsgID = 0;
    uint8_t ackID = 0;

    while (TRUE)
    {
        uint8_t buttons = ~buttons_state() & 0xF; // read the buttons and invert the bits

        chars_read = BT_Recv(BT_buffer); // read the bluetooth buffer
        BT_buffer[chars_read] = '\0';    // null terminate the buffer
        // store player id
        inputHandled = 0;
        ackID = 0;

        if (BT_buffer[0] == CONN_ID || alreadyAcked[BT_buffer[1]])
        {                                      // if the bluetooth buffer matches CONN_ID (color code assigned from doc), or has already been acknowledged/handled
            inputHandled = BT_buffer[0];       // save the msg type
            BT_buffer[0] = ACK_ID;             // set the first byte of the buffer to ACK_ID (ACK)
            alreadyAcked[BT_buffer[1]] = TRUE; // mark the message as having been handled
            BT_Send(BT_buffer, 2);             // send ack to laptop
        }
        else if (BT_buffer[0] == ACK_ID)
        {                                  // if message is an acknowledge
            ackID = BT_buffer[1];          // save the message ID being acked in case we need it
            sentMessageLengths[ackID] = 0; // clear the length (used to track whether a message needs to be re-sent)
            inputHandled = ACK_ID;         // save the msg type
        }

        switch (state)
        {
        case STATE_CON_WAIT:
            // wait for connection confirmation from uart initialization
            // if (BT_buffer[0] == CONN_ID)
            if (inputHandled)
            {
                // BT_buffer[0] = ACK_ID;
                // BT_Send(BT_buffer, 2);

                state = STATE_CHOOSE_COLOR; // change the state to STATE_CHOOSE_COLOR
            }
            break;

        case STATE_CHOOSE_COLOR:
            map2color(&red, &green, &blue, QEI_GetPosition()); // map rotary count to RGB values
            // printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue); // print the RGB values (for debugging purposes
            //  set the duty cycle of the PWM to the RGB values

            // these pins are 100% mapped wrong but don't change them i have the colors where i want them
            // map values to values of 100
            PWM_SetDutyCycle(PWM_4, (red * 100) / 255);
            PWM_SetDutyCycle(PWM_0, (green * 100) / 255);
            PWM_SetDutyCycle(PWM_2, (blue * 100) / 255);
            // choose color
            if (buttons & (BUTTON_2 | BUTTON_3 | BUTTON_4))
            {
                // !!!!!!!!! might break but ez fix bc qei colors is supposed to be 1-100, just print them out and change to actual values
                {
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
                    else if (red == 255 && green == 0 && blue == 0)
                    {
                        strcpy(color, "cyan");
                    }
                    else if (red == 0 && green == 255 && blue == 0)
                    {
                        strcpy(color, "yellow");
                    }
                    else if (red == 150 && green == 0 && blue == 255)
                    {
                        strcpy(color, "purple");
                    }
                    else if (red == 0 && green == 0 && blue == 0)
                    {
                        strcpy(color, "white");
                    }
                }
                sentMessageGarage[activeID][0] = COLOR_ID;
                sentMessageGarage[activeID][1] = activeID;
                for (int i = 0; i < strlen(color); i++)
                {
                    sentMessageGarage[activeID][i + 2] = color[i];
                }
                // strcpy(sentMessageGarage[activeID][2], color);
                BT_Send(sentMessageGarage[activeID], 2 + strlen(color));
                sentMessageLengths[activeID] = 2 + strlen(color);
                sentMessageTimes[activeID] = TIMERS_GetMilliSeconds();
                colorMsgID = activeID;
                activeID = (activeID + 1) % 256;

                state = STATE_COLOR_ACK; // change the state to STATE_WAIT_ASSIGN
            }
            break;

        case STATE_COLOR_ACK:
            // wait for ack of color message from stm32

            /* if (BT_buffer[0] == CONN_ID)
            {
                BT_buffer[0] = ACK_ID;
                BT_Send(BT_buffer, 2); // send ack to laptop
                activeID = (activeID + 1) % 256;
            }
            else  */
            if (inputHandled == ACK_ID && ackID == colorMsgID)
            {
                state = STATE_WAIT_ASSIGN; // change the state to STATE_START
            }
            // else
            // {
            //     HAL_Delay(10000); // :)
            //     // BT_buffer[0] = COLOR_ID;
            //     // BT_buffer[1] = color;
            //     // BT_Send(BT_buffer, 2 + strlen(color));
            //     // activeID = (activeID + 1) % 256;
            //     // if (BT_buffer[0] == ACK_ID)
            //     // {
            //     //     state = STATE_WAIT_ASSIGN; // change the state to STATE_START
            //     // }
            // }
            break;

        case STATE_WAIT_ASSIGN:
            if (!inputHandled && BT_buffer[0] == ASSIGN_ID)
            {
                playerID = BT_buffer[2];
                BT_buffer[0] = ACK_ID;
                alreadyAcked[BT_buffer[1]] = TRUE;
                BT_Send(BT_buffer, 2); // send ack to laptop
                inputHandled = TRUE;
                state = STATE_START; // change the state to STATE_START
            }
            break;

        case STATE_START:

            // shouldn't need to handle this, the assign id message has already been handled & acked so it should be caught by the "if alreadyAcked[BT_Buffer[1]]" check before the switch/case
            // if (!inputHandled && BT_buffer[0] == ASSIGN_ID) // if the BT_buffer item is an ID assignment
            // {                                               //
            //     BT_buffer[0] = ACK_ID;                      // store the id
            //     BT_Send(BT_buffer, 2);                      // send ack to laptop
            //     activeID = (activeID + 1) % 256;            // update the active ID for next transmit
            // }

            // service button input
            if (buttons & BUTTON_2)
            {
                BT_buffer[0] = PP_SENT_ID;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 2;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256; // update the active ID for next transmit
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE; // set flag so it sends IR
            }
            else if (buttons & BUTTON_3)
            {
                BT_buffer[0] = PP_SENT_ID;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 3;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256; // update the active ID for next transmit
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE; // set flag so it sends IR
            }
            else if (buttons & BUTTON_4)
            {
                BT_buffer[0] = PP_SENT_ID;
                BT_buffer[1] = playerID;
                BT_buffer[2] = 4;
                BT_Send(BT_buffer, 4);
                // if no ack back don't move on ?????????????????????
                activeID = (activeID + 1) % 256; // update the active ID for next transmit
                spellPulse(UNSPECIFIED, TRUE);
                sendIRflag = TRUE; // set flag so it sends IR
            }
            else
            {
                spellPulse(UNSPECIFIED, FALSE);
            }

            // service IR transmission
            if (sendIRflag == TRUE)
            {                                       // send IR if needed
                if (sendIRsignal(playerID) == TRUE) // run service and check if done sending
                    sendIRflag = FALSE;             // reset the flag if done sending
            }

            // service IR reception
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

            break;
        } // state machine end

        // send any messages that need to be resent
        if (TIMERS_GetMilliSeconds() - lastResend > RESENDS_CHECK_TIMEOUT) // If it has been more than RESENDS_CHECK_TIMEOUT since last garage maintinence
        {
            lastResend = TIMERS_GetMilliSeconds();                                                                 // record maintininence time
            for (int i = 0; i < 256; i++)                                                                          // loop through all garage spaces
            {                                                                                                      //
                if (sentMessageLengths[i] != 0 && TIMERS_GetMilliSeconds() - sentMessageTimes[i] > RESEND_TIMEOUT) // if there is an active message and it was sent too long ago
                {                                                                                                  //
                    BT_Send(sentMessageGarage[i], sentMessageLengths[i]);                                          // resend the active message
                    sentMessageTimes[i] = TIMERS_GetMilliSeconds();                                                // update it's recorded send time.
                }
            }
        }
        /* if time.time() - waiting_to_recv_ack[i][j][1] > 1:
            waiting_to_recv_ack[i][j][1] = time.time()
            send(
                i,
                waiting_to_recv_ack[i][j][2],  // msg_type
                waiting_to_recv_ack[i][j][3],  // msg_str
                waiting_to_recv_ack[i][j][0],  // msg_id
            ) */
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
//                   // state = STATE_PROCESSING; // change the state to STATE_WAIT_ASSIGN
//                 }