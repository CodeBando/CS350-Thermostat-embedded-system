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
 * CS350 Emerging Systems Architecture and Technology
 * Module 7 - Final Project
 * Author: Matthew Bandyk
 * Version: 1.0
 * Revision Date: 10/14/2023
 *
 * Problem: This code implements the Project as specified in the document:
 * "Project Thermostat Lab Guide PDF"
 *
 * Solution: developed program that utilizes I2C, UART, and Timers to implement a SM
 * that checks for button presses every 200ms, checks temperature every 500ms,
 * and reports out the current temp, the set point temp, the heater status indicated by LED
 * and the number of seconds that have passed since the thermostate was reset every 1000ms.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#define DISPLAY(x) UART_write(uart, &output, x);

// Driver Handles - Global variables
Timer_Handle timer0;
volatile bool TimerFlag = false;

// UART Global Variables
char output[64];
int bytesToSend;

// Driver Handles - Global variables
UART_Handle uart;

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

// Global variables for temp, setpoint, heat status, and seconds
int temperature = 25;  // Initial temperature
int setpoint = 22;     // Initial setpoint
bool heating = false;  // Heating status
int seconds = 0;       // Time since board reset

// Timer flag, setpoint increase and decrease
volatile bool increaseSetpoint = false;
volatile bool decreaseSetpoint = false;

// initialization of I2C.
void initI2C(void) {
    int8_t i, found;
    I2C_Params i2cParams;

    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

// Initialize the driver
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
    }
    else {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}


/*
 * ======== readTemp ========
 * callback function that will execute to read and return current temp in C
 */
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
    }
    else {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }

    return temperature;
}

/*
 *  ======== initUART ========
 *  Callback function to create UART.
 */
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
 *  Callback that updates timer flag status when executed
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status) {
    TimerFlag = true;
}

/*
 *  ======== initTimer ========
 *  Initializes the timer for the program
 */
void initTimer(void) {
    Timer_Params params;

    // Init the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000; // 100ms
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
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index) {
    /* increase the set point temp */
    increaseSetpoint = true;
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index) {
    /* decrease the set point temperature */
    decreaseSetpoint = true;
}

/*
 * ======== buttonPress function ========
 * Executes every 200ms (0.2 sec)
 * If button press was detected and flag raised
 * update set temperature based on which button was pressed, either increasing or decreasing
 */
void buttonPress() {
// Check for button presses and update set point temperature accordingly
    if (increaseSetpoint) {
        setpoint++; // Increase set point temperature
        increaseSetpoint = false; // Reset the flag
    }

    if (decreaseSetpoint) {
        setpoint--; // Decrease set point temperature
        decreaseSetpoint = false; // Reset the flag
    }
}

/*
 * ======== updateTemp function ========
 * Executes every 500ms (0.5 sec)
 * Sets the current temperature reading
 * updates heating status based on current temp and set temp
 */
void updateTemp() {
    // Read temperature sensor
    temperature = readTemp();

    // Check if heater should be on or off based on temperature and set point
    if (temperature < setpoint) {
        heating = true; // Heater on
    }

    else {
        heating = false; // Heater off
    }
}

/*
 * ======== reportUpdate function ========
 * Executes every 1000ms (1 sec)
 * Increments seconds the program has run sense a reset
 * Updates LED state based on heating status
 * Displays the current temperature(AA), set temperature(BB), LED status(S), and seconds passed(CCCC) <AA,BB,S,CCCC>
 */
void reportUpdate() {
    seconds++; // Increment seconds

    // if heating is set to true, turn LED on, else turn LED off
    if (heating == true) {
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON); // Turn on the heater LED
    }

    else {
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF); // Turn off the heater LED
    }

    // Update UART output and timer
    DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", temperature, setpoint, heating, seconds));
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0) {
    // variables to hold periods for each state, time passed for each state, and the timer period
    const unsigned long BP_period = 200; // check for button press every 200ms
    const unsigned long RT_period = 500; // read temperature every 500ms
    const unsigned long RP_period = 1000; // report to server and update LED every 1000ms
    unsigned long BP_elapsed = 200; // capture time elapsed for button press check
    unsigned long RT_elapsed = 500; // capture time elapsed for reading temperature check
    unsigned long RP_elapsed = 1000; // capture time elapsed for reporting and updating LED
    const unsigned long timerPeriod = 100; // 100ms period

    // Call driver init functions
    GPIO_init();

    // Configure the LED and button pins
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    // Turn on user LED
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    // Install Button callback
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    // Enable interrupts
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        // Configure BUTTON1 pin
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        // Install Button callback
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }

    // Initialize UART, I2C, and the main timer
    initUART();
    initI2C();
    initTimer();

    // continuous loop
    while (1) {
        TimerFlag = false;

        // loop until flag is raised
        while (!TimerFlag) {}

        // executes button press logic every 200ms
        if (BP_elapsed >= BP_period) {
            buttonPress();
            BP_elapsed = 0;
        }

        // reads temperature and updates if LED (Heater) should be on or off every 500ms
        if (RT_elapsed >= RT_period) {
            updateTemp();
            RT_elapsed = 0;
        }

        // updates LED if needed and displays current temp, set temp, if heat is on or off, and how many seconds have passed
        if (RP_elapsed >= RP_period) {
            reportUpdate();
            RP_elapsed = 0;
        }

        // increase elapsed time for each of the counters
        BP_elapsed += timerPeriod;
        RT_elapsed += timerPeriod;
        RP_elapsed += timerPeriod;
    }
}

