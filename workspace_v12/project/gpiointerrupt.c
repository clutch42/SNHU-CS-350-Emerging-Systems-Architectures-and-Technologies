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

// Problem Description:
//
// This code implements the Project as specified
// in the document:
// Project Thermostat Lab Guide PDF
// for the SNHU Course CS-350.

// Solution:
//
// First I implemented a task scheduler to determine when to run a task
// Then I created state machine for the different tasks where appropriate
// and ran them according to the task scheduler



#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#define DISPLAY(x) UART_write(uart, &output, x);
#define TRUE 1
#define FALSE 0
#define NULL 0
#define NUMBER_OF_TASKS 3
#define GLOBAL_PERIOD 100
#define INTERRUPT_PERIOD 200
#define TEMP_PERIOD 500
#define UART_PERIOD 1000
#define START_TEMP 25
#define MIN_SETPOINT 0
#define MAX_SETPOINT 99
#define INITIAL_TEMP 0
#define INITIAL_SECONDS 0

// define the states for the state machines and set the initial state
enum BUTTON_STATES {NONE, BUTTON_0, BUTTON_1} BUTTON_STATE = NONE;
/*
 * since enum sets HEAT_OFF to 0 and HEAT_ON to 1 I can use HEAT_STATE
 * directly in the output to the UART
 */
enum HEAT_STATES {HEAT_OFF, HEAT_ON} HEAT_STATE = HEAT_OFF;


// UART Global Variables
char output[64];
int bytesToSend;

// I2C Global Variables
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[3] = {
    { 0x48, 0x0000, "11X" },
    { 0x49, 0x0000, "116" },
    { 0x41, 0x0001, "006" }
};
uint8_t txBuffer[1];
uint8_t rxBuffer[2];
I2C_Transaction i2cTransaction;

// Driver Handles - Global variables
I2C_Handle i2c;
UART_Handle uart;
Timer_Handle timer0;

// global variables
int temperature = INITIAL_TEMP;
int setPoint = START_TEMP;
int seconds = INITIAL_SECONDS;

volatile unsigned char ready_tasks = FALSE;
int global_period = GLOBAL_PERIOD;

// forward declarations
void changeTempSetPoint();
void updateTemp();
void oneSecondTasks();
int16_t readTemp(void);

// global variables for the task manager
struct task_entry {
    void (*f)();
    int elapsed_time;
    int period;
    char triggered;
};

struct task_entry tasks[NUMBER_OF_TASKS] = {
    {&changeTempSetPoint, INTERRUPT_PERIOD, INTERRUPT_PERIOD, FALSE},
    {&updateTemp, TEMP_PERIOD, TEMP_PERIOD, FALSE},
    {&oneSecondTasks, UART_PERIOD, UART_PERIOD, FALSE}
};

/**
 * Function for state machine for changing the setPoint variable
 *
 * The function increments or decrements the setPoint variable
 * depending on the state of BUTTON_STATE. BUTTON_0 indicates the interrupt
 * was tripped for the increment button and BUTTON_1 is for decrement.
 * There is also a condition for both to keep the number in the range of
 * MIN_setPoint and MAX_setPoint
 * Takes no arguments and does not return anything
 *
**/
void changeTempSetPoint() {
    // Actions
    switch (BUTTON_STATE) {
        case NONE:
            break;
        case BUTTON_0:
            if (setPoint < MAX_SETPOINT) {
                setPoint++;
            }
            break;
        case BUTTON_1:
            if (setPoint > MIN_SETPOINT) {
                setPoint--;
            }
            break;
        default:
            break;
    }
    // Transitions
    switch (BUTTON_STATE) {
        case NONE:
            break;
        case BUTTON_0:
            BUTTON_STATE = NONE;
            break;
        case BUTTON_1:
            BUTTON_STATE = NONE;
            break;
        default:
            break;
    }
}

/**
 * Function for updating the temperature variable
 *
 * Function uses the readTemp() function provided
 * Does not take any arguments and does not return anything
 *
**/
void updateTemp() {
    temperature = readTemp();
}

/**
 * Function for updating the seconds variable
 *
 * Function increments second. Used to enhance readability.
 * Does not take any arguments and does not return anything
 *
**/
void incrementSeconds() {
    seconds++;
}

/**
 * Function for sending to UART
 *
 * Function sends string of 64 chars to UART. Used to enhance readability.
 * Does not take any arguments and does not return anything
 *
**/
void sendToUART() {
    DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", temperature, setPoint, HEAT_STATE, seconds))
}

/**
 * Function for setting heat on or off depending on the setPoint and current temperature
 *
 * Uses a state machine to update HEAT_STATE. If the temperature is less than
 * the setPoint the HEAT_STATE is set to HEAT_ON and the red light is turned on.
 * If the temperature is greater than or equal to setPoint the HEAT_STATE is set to
 * HEAT_OFF and the red light is turned off.
 * Does not take any arguments and does not return anything
 *
**/
void setHeat() {
    // Transitions
    switch (HEAT_STATE) {
        case HEAT_OFF:
            if (temperature < setPoint) {
                HEAT_STATE = HEAT_ON;
            }
            break;
        case HEAT_ON:
            if (temperature >= setPoint) {
                HEAT_STATE = HEAT_OFF;
            }
            break;
        default:
            break;
    }
    // Actions
    switch (HEAT_STATE) {
        case HEAT_OFF:
            // light off
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            break;
        case HEAT_ON:
            // light on
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            break;
        default:
            break;
    }
}

/**
 * Function for the tasks that need to be done at 1 second
 *
 * I put the logic into separate functions to make the code easier to read.
 * Does not take any arguments and does not return anything
 *
**/
void oneSecondTasks() {
    setHeat();
    sendToUART();
    incrementSeconds();
}

// Make sure you call initUART() before calling this function.
void initI2C(void) {
    int8_t i, found;
    I2C_Params i2cParams;

    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL) {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }
    DISPLAY(snprintf(output, 32, "Passed\n\r"))
    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses
    /* Common I2C transaction setup */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;
    found = false;
    for (i=0; i<3; ++i) {
        i2cTransaction.slaveAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;
        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
        if (I2C_transfer(i2c, &i2cTransaction)) {
            DISPLAY(snprintf(output, 64, "Found\n\r"))
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }
    if(found) {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.slaveAddress))
    } else {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}

int16_t readTemp(void) {
    int16_t temperature = 0;
    i2cTransaction.readCount = 2;
    if (I2C_transfer(i2c, &i2cTransaction)) {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;
        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80) {
            temperature |= 0xF000;
        }
    } else {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor(%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }
    return temperature;
}

void initUART(void) {
    UART_Params uartParams;
    // Init the driver
    UART_init();
    // Configure the driver
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;
    // Open the driver
    uart = UART_open(CONFIG_UART_0, &uartParams);
    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }
}

/*
 *  ======== timerCallback ========
 *  Callback function for the timer interrupt. Occurs every 100ms.
 *
 *  This is the code from the video for the task manager.
 *  Every time the timer expires it loops through the array of tasks and
 *  checks if the elapsed time is greater than or equal to the total period
 *  time for the task. If it is the flag for the task is raised and the elapsed
 *  time is reset. If not the elapsed time is incremented by the global period (100ms).
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status) {
    int x = 0;
    for (x = 0; x < NUMBER_OF_TASKS; x++) {
        if (tasks[x].elapsed_time >= tasks[x].period) {
            tasks[x].triggered = TRUE;
            ready_tasks = TRUE;
            tasks[x].elapsed_time = 0;
        } else {
            tasks[x].elapsed_time += global_period;
        }
    }
}




/*
 *  ======== timerInit ========
 *  Initialization function for the timer interrupt on timer0.
 */
void initTimer(void) {
    Timer_Params params;
    // Init the driver
    Timer_init();
    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000;  // 100ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;
    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }
    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}


/*
 *  This is the callback for interrupt button 0
 *
 *  If the button is pressed it sets BUTTON_STATE to BUTTON_0 so
 *  the next period will register a change in the state and increase
 *  the setPoint
 */
void gpioButton0Increase(uint_least8_t index)
{
    BUTTON_STATE = BUTTON_0;
}

/*
 *  This is the callback for interrupt button 1
 *
 *  If the button is pressed it sets BUTTON_STATE to BUTTON_1 so
 *  the next period will register a change in the state and decrease
 *  the setPoint
 */
void gpioButton1Decrease(uint_least8_t index)
{
    BUTTON_STATE = BUTTON_1;
}

/*
 * put the logic for initializing the GPIO into a separate function
 */
void initGPIO(void) {
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Start with LEDs off */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButton0Increase);

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
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButton1Decrease);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions for GPIO, UART, I2C, and timer */
    initGPIO();
    initUART();
    initI2C();
    initTimer();

    /* This is from the video for the task manager. It runs the timer callback until
     * a task is ready. Then it loops over all the tasks and if the task is triggered
     * it runs the associated function and resets the flag.
     */
    while (TRUE) {
        int x = 0;
        while (!ready_tasks) {}
        ready_tasks = FALSE;
        for (x = 0; x < NUMBER_OF_TASKS; x++) {
            if (tasks[x].triggered) {
                tasks[x].f();
                tasks[x].triggered = FALSE;
            }
        }
    }

    return (NULL);
}
