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
 *  ======== uartecho.c ========
 */

// Problem Description:
//
// This code implements the Project as specified
// in the document:
// Milestone Two UART GPIO Lab Guide PDF
// for the SNHU Course CS-350.

// Solution:
//
// First I enumerated the different states.
// Then I created a state machine to change between states and act on those states appropriately
// This is then called in main() to turn on and off the light according to the input


#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Driver configuration */
#include "ti_drivers_config.h"

// defined the enum list of states, but didn't create the variable yet
enum LIGHT_STATES {LIGHT_START, LIGHT_INPUT_CHAR_1, LIGHT_INPUT_CHAR_2, LIGHT_INPUT_CHAR_3, LIGHT_ON, LIGHT_OFF};

// created variable here because requirements were for one byte of RAM for state and
// enum would have used an integer. this way I can use an unsigned char
unsigned char LIGHT_STATE;

// had to switch these to global variables since I put the state machine in a function
char input;
UART_Handle uart;

/**
 * Function for state machine for turning on and off a light
 *
 * The function will turn the light on when on is input and
 * turn the light off when off is input. Added a few if else statements
 * to handle human error in typing on and off. Light remains unchanged if
 * on or off are not typed. Does not take any parameters or return anything.
 * Only changes global variable LIGHT_STATE.
 *
**/
void Light_Toggle() {
    // Transitions
    switch (LIGHT_STATE) {
        // Initial state
        case LIGHT_START:
            LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            break;
        // Determines next state depending on first char input
        case LIGHT_INPUT_CHAR_1:
            if (input == 'o' || input == 'O') {
                LIGHT_STATE = LIGHT_INPUT_CHAR_2;
            } else {
                LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            }
            break;
        // Determines next state depending on second char input
        case LIGHT_INPUT_CHAR_2:
            if (input == 'f' || input == 'F') {
                LIGHT_STATE = LIGHT_INPUT_CHAR_3;
            } else if (input == 'n' || input == 'N') {
                LIGHT_STATE = LIGHT_ON;
            } else if (input == 'o' || input == 'O') {
                LIGHT_STATE = LIGHT_INPUT_CHAR_2;
            } else {
                LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            }
            break;
        // Determines next state depending on third char input
        case LIGHT_INPUT_CHAR_3:
            if (input == 'f' || input == 'F') {
                LIGHT_STATE = LIGHT_OFF;
            } else if (input == 'o' || input == 'O') {
                LIGHT_STATE = LIGHT_INPUT_CHAR_2;
            } else {
                LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            }
            break;
        // State for turning of the light and returning to first char input
        case LIGHT_ON:
            LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            break;
        // State for turning off the light and returning to first char input
        case LIGHT_OFF:
            LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            break;
        // Default state of first char input
        default:
            LIGHT_STATE = LIGHT_INPUT_CHAR_1;
            break;
    }
    // Actions
    switch (LIGHT_STATE) {
        // defined but should never come up
        case LIGHT_START:
            break;
        // Get first char input
        case LIGHT_INPUT_CHAR_1:
            UART_read(uart, &input, 1);
            UART_write(uart, &input, 1);
            break;
        // Get second char input
        case LIGHT_INPUT_CHAR_2:
            UART_read(uart, &input, 1);
            UART_write(uart, &input, 1);
            break;
        // Get third char input
        case LIGHT_INPUT_CHAR_3:
            UART_read(uart, &input, 1);
            UART_write(uart, &input, 1);
            break;
        // Turn light on
        case LIGHT_ON:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            break;
        // Turn light off
        case LIGHT_OFF:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            break;
        // default defined but should never come up
        default:
            break;
    }
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    const char  echoPrompt[] = "\r\nEchoing characters:\r\n";
    UART_Params uartParams;
    // initialize LIGHT_STATE. Is it supposed to be done here or could I have done it back when I declared the global variable?
    // Read it is good practice to initialize in main function.
    LIGHT_STATE = LIGHT_START;

    /* Call driver init functions */
    GPIO_init();
    UART_init();

    /* Configure the LED pin */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;

    uart = UART_open(CONFIG_UART_0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    /* Turn on user LED to indicate successful initialization */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    UART_write(uart, echoPrompt, sizeof(echoPrompt));

    /* Loop forever echoing */
    while (1) {
        Light_Toggle();
    }
}
