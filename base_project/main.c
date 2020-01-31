/* 
 * File:   stoplightMain.c
 * Author: Andrew Yingst
 *
 * Created on January 28, 2020, 8:08 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <p33EP512GP806.h>
#include <esos.h>
#include <pic24_all.h>

#define rushHour ESOS_USER_FLAG_1
#define turnSignal ESOS_USER_FLAG_2
#define showEW ESOS_USER_FLAG_3


/*u16_stateAndTime is a 2 dimensional array of control information
 *There are 4 operating modes and 8 light-on times or states.
 *First mode is Rush Hour + Turn Signal (SW1 and SW2), Second is Turn Signal only (SW2),
 *Third is Rush Hour only (SW1), and Fourth is the normal mode when no buttons are pressed.
 *Each element in the array has two bytes of information. The MSB is the light on-off info
 *Each bit represents a light following the following convention--
 *(Rns, Ans, Gns, Tns, Rew, Aew, Gew, Tew)where R=Red light on, A=Amber, G=Green, T=Turn Signal,
 *ns and ew are the views controlled by SW3.
 *The LSB is the time for that state in seconds*/
const int u16_lightsAndTime = 
    {{0x810A, 0x821E, 0x8403, 0x8801, 0x180A, 0x281E, 0x4803, 0x8801},
     {0x810A, 0x8210, 0x8403, 0x8800, 0x180A, 0x2810, 0x4803, 0x8800},
     {0x8100, 0x821E, 0x8403, 0x8801, 0x1800, 0x281E, 0x4803, 0x8801},
     {0x8100, 0x820A, 0x8403, 0x8800, 0x1800, 0x280A, 0x4803, 0x8800}};
static unsigned short u8_mode; //this is the row of the array and is changed by SW1 and SW2
static unsigned short u8_state = 0; //this is the column of the array and represents the sequence of the stoplight

void user_init(void){
    CONFIG_LED1();
    CONFIG_LED2();
    CONFIG_LED3();
    
    esos_RegisterTask(loadStopLightInfo);
    //esos_RegisterTask(turnSignalBlink)
    //esos_RegisterTimer(turnSignalTimer, 250);
}
ESOS_USER_TASK(loadStopLightInfo){
    ESOS_TASK_BEGIN();
    while(1){
        if (esos_IsUserFlagSet(rushHour) && esos_IsUserFlagSet(turnSignal)){ //rush hour and turn signal
            u8_mode = 0;
        }
        else if (esos_IsUserFlagClear(rushHour) && esos_IsUserFlagSet(turnSignal)){ //turn signal but not rush hour
            u8_mode = 1;
        }
        else if (esos_IsUserFlagSet(rushHour) && esos_IsUserFlagClear(turnSignal))){ //rush hour but not turn signal
            u8_mode = 2;
        }
        else {
            u8_mode = 3;
        }
        ESOS_ALLOCATE_CHILD_TASK(th_child);
        ESOS_TASK_SPAWN_AND_WAIT(th_child, turnOnLights, u16_lightsAndTime[u8_mode][u8_state]);
        u8_state +=1; //after time spent in child task, move to next state
        if (u8_state > 7){ //start over from beginning use len command later
            u8_state = 0;
        }
    }
    ESOS_TASK_END();
}

ESOS_CHILD_TASK(turnOnLights, u16_lightsAndTime){
    EOS_TASK_BEGIN();
    LED1 = (u16_lightsAndTime[u8_mode][u8_state] & 0x8000 && !esos_IsUserFlagSet(showEW)); //red light North/South view
    LED2 = (u16_lightsAndTime[u8_mode][u8_state] & 0x4000 && !esos_IsUserFlagSet(showEW)); //light NS
    LED3 = (u16_lightsAndTime[u8_mode][u8_state] & 0x2000 && !esos_IsUserFlagSet(showEW)); //green NS
    while(u16_lightsAndTime[u8_mode][u8_state] & 0x1000 && !esos_IsUserFlagSet(showEW)){ //blinking green NS
        LED3 = !LED3;
        ESOS_TASK_WAIT_TICKS(250);
    }
    LED1 = (u16_lightsAndTime[u8_mode][u8_state] & 0x0800 && esos_IsUserFlagSet(showEW)); //red light East/West view
    LED2 = (u16_lightsAndTime[u8_mode][u8_state] & 0x0400 && esos_IsUserFlagSet(showEW)); //amber EW
    LED3 = (u16_lightsAndTime[u8_mode][u8_state] & 0x0200 && esos_IsUserFlagSet(showEW)); //green EW
    while(u16_lightsAndTime[u8_mode][u8_state] & 0x0100 && esos_IsUserFlagSet(showEW)){ //blinking green EW
        LED3 = !LED3;
        ESOS_TASK_WAIT_TICKS(250);
    }
    unsigned short u8_time = (u16_lightsAndTime[u8_mode][u8_state] && 0x00FF); //strips off mode information leaving time
    ESOS_TASK_WAIT_TICKS(u8_time * 1000);
    ESOS_TASK_END();
}
