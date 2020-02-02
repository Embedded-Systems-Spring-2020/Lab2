#include <esos.h>
#include "esos_pic24.h"
#include "esos_pic24_rs232.h"
#include <p33EP512GP806.h>
#include <pic24_all.h>

#include <stdio.h>
#include <stdlib.h>
#include "revF14.h"

#define rushHour ESOS_USER_FLAG_1
#define turnSignal ESOS_USER_FLAG_2
#define showEW ESOS_USER_FLAG_3

#define STATES (6)


//This is Andrew's idea.
//This array holds the color information for the NS and EW as well as the
//time information for each state. Bits 15-12 represent the NS light, bits
//11-8 represent the EW light, and bits 7-0 are the time for each state
//in ms. The bit encoding for a light is as follows, starting from left to right:
//{Red, Amber, Green, Turn}
//For example, the first element in the array encodes the following:
//8 -> 1000 -> The NS light is red in this state
//2 -> 0010 -> The EW light is green
//0A -> This state has a millisecond multiplier of 10
uint16_t au16_colorAndTime[2][STATES] ={
    {0x820A, 0x8403, 0x280A, 0x4803, 0x0000, 0x0000},
    {0x821E, 0x8403, 0x8801, 0x281E, 0x4803, 0x8801}
};

uint8_t u8_state = 0;

void reset_LEDs();

ESOS_USER_TASK(pollswitches) {
    ESOS_TASK_BEGIN();
    for (;;) {
        ESOS_TASK_WAIT_TICKS(25);
        SW3_PRESSED ? esos_SetUserFlag(showEW) : esos_ClearUserFlag(showEW);
        SW1_PRESSED ? esos_SetUserFlag(rushHour) : esos_ClearUserFlag(rushHour);
    }
    ESOS_TASK_END();
}

ESOS_USER_TASK(trackstate) {
    ESOS_TASK_BEGIN();

    for (;;) {

        uint8_t u8_msTime;
        if (esos_IsUserFlagSet(rushHour)) {
            u8_msTime = au16_colorAndTime[1][u8_state] & 0x00FF;
        } else {
            u8_msTime = au16_colorAndTime[0][u8_state] & 0x00FF;
        }
        
        ESOS_TASK_WAIT_TICKS(u8_msTime * 1000);
        u8_state++;
        if (u8_state >= STATES) u8_state = 0;
    }

    ESOS_TASK_END();
}

ESOS_USER_TASK(drawLEDs) {
    ESOS_TASK_BEGIN();

    for (;;) {
        reset_LEDs();
        uint16_t u16_color;
        uint8_t u8_mode = esos_IsUserFlagSet(rushHour) >= 1;
        
        if (esos_IsUserFlagSet(showEW)) {
            u16_color = (au16_colorAndTime[u8_mode][u8_state] & 0x0F00) >> 8;
        } else {
            u16_color = (au16_colorAndTime[u8_mode][u8_state] & 0xF000) >> 12;
        }

        if (u16_color == 8){
            LED1 = 1;
        } else if (u16_color == 4) {
            LED2 = 1;
        } else if (u16_color == 2) {
            LED3 = 0;
        }

        ESOS_TASK_WAIT_TICKS(25);
    }
    ESOS_TASK_END();
}

void user_init(void){
    CONFIG_LED1();
    CONFIG_LED2();
    CONFIG_LED3();

    CONFIG_SW1();
    CONFIG_SW2();
    CONFIG_SW3();
    
    esos_RegisterTask(trackstate);
    esos_RegisterTask(pollswitches);
    esos_RegisterTask(drawLEDs);
}

void reset_LEDs() {
    LED1 = 0;
    LED2 = 0;
    LED3 = 1;
}
