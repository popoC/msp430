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
 *  ======== uartecho.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>
/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Example/Board Header files */
#include "Board.h"

 double timestamp_GPS;
 double timestamp_Seascan;

 UART_Handle uart;
 char RS232_Out[100];
 Types_FreqHz freq;

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON0.
 *  回呼函式 主要處理中斷發生後的事情
 */
void gpioButtonFxn0(uint_least8_t index)
{
    timestamp_Seascan = Timestamp_get32();
    sprintf(RS232_Out, "Timestamp1 is %f ms\n\t", ((double)(timestamp_Seascan - timestamp_GPS)/(double)freq.lo)*1000);
    UART_write(uart, &RS232_Out, strlen(RS232_Out));
    /* Clear the GPIO interrupt and toggle an LED */
    GPIO_toggle(Board_GPIO_LED0);
}
/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON1.
 *  This may not be used for all boards.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    timestamp_GPS =  Timestamp_get32();
    /* Clear the GPIO interrupt and toggle an LED */
    GPIO_toggle(Board_GPIO_LED1);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    const char  echoPrompt[] = "Echoing characters:\r\n";

    UART_Params uartParams;

    /* Call driver init functions */
    GPIO_init();
    UART_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_GPIO_LED1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_P3_6, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING);  //-- 設定P3.6 下源觸發 給Seascan
    GPIO_setConfig(Board_P3_7, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING); //-- 設定P3.7 上源觸發給GPS
    /* install Button callback */
    GPIO_setCallback(Board_P3_6, gpioButtonFxn0);
    GPIO_setCallback(Board_P3_7, gpioButtonFxn1);

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;

    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    UART_write(uart, echoPrompt, sizeof(echoPrompt));



    Timestamp_getFreq(&freq);
    /* Enable interrupts */
    GPIO_enableInt(Board_P3_6);
    GPIO_enableInt(Board_P3_7);




    /* Loop forever echoing */
    while (1) {
      //  UART_read(uart, &input, 1);
      //  UART_write(uart, &input, 1);
      //  double  Dt1 =  Timestamp_get32();
    }
}
