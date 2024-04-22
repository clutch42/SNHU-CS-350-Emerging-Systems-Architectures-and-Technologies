/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */

/*
 * Problem Description:
 *
 * This code implements the Project as specified
 * in the document:
 * Milestone Three Timer Interrupt Lab Guide PDF
 * for the SNHU Course CS-350.
 *
 * Solution:
 *
 * I created two states, one for the which message to display and
 * the other for the light states. Then I created 2 arrays of light states for the
 * messages, and with a index variable cycled through one each phase of the timer.
 * I also checked a variable at the end of the array to determine if the interrupt
 * had switched the message to be displayed.
 *
 */

#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* macro for finding array size since we need it quite a bit */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* states for what message should be displayed now
 * and if an interrupt was hit what it should
 * change to  */
enum MESSAGES_STATES {SOS_MESSAGE, OK_MESSAGE} TIMER_MESSAGE, INTERRUPT_MESSAGE;

/* the different states for the dot (red light),
 * dash (green light), and off (no lights) */
enum LIGHT_STATES {DOT_STATE, DASH_STATE, OFF_STATE } LIGHT_STATE;

/* I tried a fancier way of building increasing size arrays
 * for s, o, and k but couldn't get it to work. The following arrays
 * are just all the building blocks from LIGHT_STATES */
enum LIGHT_STATES sos[] = {
    /* S  built with dot and off (red on for 500ms and off for 500ms
     * last line off for 500ms * 3 for character break*/
    DOT_STATE, OFF_STATE,
    DOT_STATE, OFF_STATE,
    DOT_STATE, OFF_STATE, OFF_STATE, OFF_STATE,
    /* O  built with dash and off (green on for 500ms * 3 and off for 500ms
     * last line off for 500ms * 3 for character break*/
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE,
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE,
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE, OFF_STATE, OFF_STATE,
    /* S built with dot and off (red on for 500ms and off for 500ms
     * last line off for 500ms * 7 for word break*/
    DOT_STATE, OFF_STATE,
    DOT_STATE, OFF_STATE,
    DOT_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE
};

enum LIGHT_STATES ok[] = {
    /* O  built with dash and off (green on for 500ms * 3 and off for 500ms
     * last line off for 500ms * 3 for character break*/
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE,
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE,
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE, OFF_STATE, OFF_STATE,
    /* K built with dot, dash, and off (dash on for 500ms * 3 or dot on for 500ms followed by off for 500ms
     * last line off for 500ms * 7 for word break*/
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE,
    DOT_STATE, OFF_STATE,
    DASH_STATE, DASH_STATE, DASH_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE, OFF_STATE
};

/* position in the message array (position of current state)*/
unsigned int messageArrayPosition = 0;

/*
 *  ======== setLights ========
 *  Function to turn lights on and off according to state
 *
 *  Function turns red on and ensures green is off if state is DOT_STATE
 *  Function turns green on and ensures red is off if state is DASH_STATE
 *  Function turns both lights off is state is OFF_STATE
 *  does not have any parameters and does not return anything
 */
void setLights() {
    switch(LIGHT_STATE) {
        case DOT_STATE:   // DOT (red on)
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;
        case DASH_STATE: // DASH (green on)
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            break;
        case OFF_STATE:   // OFF
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            break;
        default:
            break;
    }
}

/*
 *  ======== timerCallback ========
 *  Callback function for the timer interrupt.
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    /* Transition
     * changes the timer LIGHT_STATE to the state in the array
     * for the message at index messageArrayPosition
    */
    switch(TIMER_MESSAGE) {
        case SOS_MESSAGE:
            LIGHT_STATE = sos[messageArrayPosition];
            break;
        case OK_MESSAGE:
            LIGHT_STATE = ok[messageArrayPosition];
            break;
        default:
            break;
    }
    /* Actions
     * changes the lights to the current state, increments the position
     * of the index, and checks if the index is past the end of the array.
     * If it is sets the state to the interrupt's state and resets the index.
     */
    switch(TIMER_MESSAGE) {
        case SOS_MESSAGE:
            setLights();
            messageArrayPosition++;
            if(messageArrayPosition == ARRAY_SIZE(sos)) {
                TIMER_MESSAGE = INTERRUPT_MESSAGE;
                messageArrayPosition = 0;
            }
            break;
        case OK_MESSAGE:
            setLights();
            messageArrayPosition++;
            if(messageArrayPosition == ARRAY_SIZE(ok)) {
                TIMER_MESSAGE = INTERRUPT_MESSAGE;
                messageArrayPosition = 0;
            }
            break;
        default:
            break;
    }
}

/*
 *  ======== timerInit ========
 *  Initialization function for the timer interrupt on timer0.
 */
void initTimer(void)
{
    Timer_Handle timer0;
    Timer_Params params;

    Timer_init();
    Timer_Params_init(&params);
    params.period = 500000;             // set timer for 500ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if(timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }

    if(Timer_start(timer0) == Timer_STATUS_ERROR)
    {
        /* Failed to start timer */
        while (1) {}
    }
}

/*
 *  ======== gpioButtonCallback ========
 *  Callback function for the GPIO interrupt for either CONFIG_GPIO_BUTTON_0 or CONFIG_GPIO_BUTTON_1.
 */
void gpioButtonCallback(uint_least8_t index)
{
    /* if interrupt button is pressed, change to the other message */
    switch(INTERRUPT_MESSAGE) {
        case SOS_MESSAGE:
            INTERRUPT_MESSAGE = OK_MESSAGE;
            break;
        case OK_MESSAGE:
            INTERRUPT_MESSAGE = SOS_MESSAGE;
            break;
        default:
            break;
    }
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions for GPIO and timer */
    GPIO_init();
    initTimer();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Start with LEDs off */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);

    /* Set initial states to SOS_MESSAGE */
    INTERRUPT_MESSAGE = SOS_MESSAGE;
    TIMER_MESSAGE = SOS_MESSAGE;

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonCallback);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonCallback);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    return (NULL);
}
