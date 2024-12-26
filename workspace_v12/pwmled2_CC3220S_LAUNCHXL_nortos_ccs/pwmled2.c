/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
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
 *  ======== pwmled2.c ========
 */

// Problem Description:
//
// This code implements the Project as specified
// in the document:
// Milestone One PWM Lab Guide PDF
// for the SNHU Course CS-350.

// Solution:
//
// First I created variables corresponding to
// the percentages needed for the duties of each pwm light.
//
// Then I plugged the variables into the functions to change
// the PWM duty and added a sleep timer as needed.


/* For usleep() */
#include <unistd.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/PWM.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*
 *  ======== mainThread ========
 *  Task periodically increments the PWM duty for the on board LED.
 */
void *mainThread(void *arg0)
{
    /* Period and duty in microseconds */
    uint16_t   pwmPeriod = 3000;
    uint16_t   dutyPWM1Low = 0;     // variable to set duty for pmw1 to 0%
    uint16_t   dutyPWM1High = 2700; // variable to set duty for pmw1 to 90%
    uint16_t   dutyPWM2Low = 300;   // variable to set duty for pmw2 to 10%
    uint16_t   dutyPWM2High = 2700; // variable to set duty for pmw2 to 90%

    /* Sleep time in microseconds */
    uint32_t   time = 1000000;  // time for sleep function 1,000,000 microseconds or 1 second
    PWM_Handle pwm1 = NULL;
    PWM_Handle pwm2 = NULL;
    PWM_Params params;

    /* Call driver init functions. */
    PWM_init();

    PWM_Params_init(&params);
    params.dutyUnits = PWM_DUTY_US;
    params.dutyValue = 0;
    params.periodUnits = PWM_PERIOD_US;
    params.periodValue = pwmPeriod;
    pwm1 = PWM_open(CONFIG_PWM_0, &params);
    if (pwm1 == NULL) {
        /* CONFIG_PWM_0 did not open */
        while (1);
    }

    PWM_start(pwm1);

    pwm2 = PWM_open(CONFIG_PWM_1, &params);
    if (pwm2 == NULL) {
        /* CONFIG_PWM_0 did not open */
        while (1);
    }

    PWM_start(pwm2);

    /* Loop forever incrementing the PWM duty */
    while (1) {
        // set pmw1 bright and pmw2 dim
        PWM_setDuty(pwm1, dutyPWM1High);
        PWM_setDuty(pwm2, dutyPWM2Low);
        usleep(time);

        // set pmw2 bright and pmw1 dim
        PWM_setDuty(pwm1, dutyPWM1Low);
        PWM_setDuty(pwm2, dutyPWM2High);
        usleep(time);
    }
}