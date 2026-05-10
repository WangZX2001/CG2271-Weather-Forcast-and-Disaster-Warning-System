/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    CG2271_Project_MCXC444.c
 * @brief   Application entry point.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// ================= UART CONSTANTS =================
#define BAUD_RATE 9600
#define UART_TX_PTE22 22
#define UART_RX_PTE23 23
#define UART2_INT_PRIO 128

#define MAX_MSG_LEN 256
#define QLEN 5

// ================= LED PINS =================
#define RED_PIN 31  // PTE31
#define GREEN_PIN 5 // PTD5
#define BLUE_PIN 29 // PTE29

// ================= SENSOR PINS ==================
#define SHOCK_PIN 2 // PTC2
#define TRIG_PIN 0  // PTB0
#define ECHO_PIN 1  // PTB1
#define FLAME_PIN 4 // PTD4  <-- rewire flame D0 here

typedef enum tl
{
    RED,
    GREEN,
    BLUE
} TLED;

// This is the message format for commands received from the ESP32:
typedef struct tm
{
    char message[MAX_MSG_LEN];
} TMessage;

// This is the packetization format for the temp/humidity data sent from the ESP32:
// "TEMP:xx.x,HUM:yy.y\n"
typedef struct
{
    float temperature;
    float humidity;
} TSensorData;

// ================= GLOBALS ======================
// Buffer for UART transmission (protected by uartMutex)
char send_buffer[MAX_MSG_LEN];

QueueHandle_t uartRxQueue;
QueueHandle_t tempQueue;

// Semaphores for shock and fire alerts
SemaphoreHandle_t shockSem;
SemaphoreHandle_t fireSem;
SemaphoreHandle_t uartMutex;

// ================= FUNCTION DECLARATIONS =================
void initUART2(uint32_t baud_rate);
void sendMessage(char *message);

void initGPIO(void);
void initShockInterrupt(void);
void initFlameInterrupt(void);

void ledOn(TLED led);
void ledOff(TLED led);
void setLedFromTemperature(float temp);

static void delay_us(volatile uint32_t us);
static float measureDistanceCm(void);

// ================= UART INIT ====================
void initUART2(uint32_t baud_rate)
{
    NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

    UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

    PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

    PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

    uint32_t bus_clk = CLOCK_GetBusClkFreq();
    uint32_t sbr = (bus_clk + (baud_rate * 8U)) / (baud_rate * 16U);

    UART2->BDH &= ~UART_BDH_SBR_MASK;
    UART2->BDH |= ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART2->BDL = (uint8_t)(sbr & 0xFFU);

    UART2->C1 &= ~UART_C1_LOOPS_MASK;
    UART2->C1 &= ~UART_C1_RSRC_MASK;
    UART2->C1 &= ~UART_C1_PE_MASK;
    UART2->C1 &= ~UART_C1_M_MASK;

    UART2->C2 |= UART_C2_RIE_MASK;
    UART2->C2 |= UART_C2_RE_MASK;

    NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
    NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
    NVIC_EnableIRQ(UART2_FLEXIO_IRQn);
}

// ================= GPIO INIT ====================
void initGPIO(void)
{
    SIM->SCGC5 |= (SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK);

    // ----- RGB LED -----
    PORTE->PCR[RED_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[RED_PIN] |= PORT_PCR_MUX(1);

    PORTE->PCR[BLUE_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[BLUE_PIN] |= PORT_PCR_MUX(1);

    PORTD->PCR[GREEN_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTD->PCR[GREEN_PIN] |= PORT_PCR_MUX(1);

    GPIOE->PDDR |= ((1U << RED_PIN) | (1U << BLUE_PIN));
    GPIOD->PDDR |= (1U << GREEN_PIN);

    // ----- Shock sensor: PTC2 -----
    PORTC->PCR[SHOCK_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTC->PCR[SHOCK_PIN] |= PORT_PCR_MUX(1);
    // Set pull-up register
    PORTC->PCR[SHOCK_PIN] &= ~PORT_PCR_PS_MASK;
    PORTC->PCR[SHOCK_PIN] |= PORT_PCR_PS_MASK;
    // Enable pull-up
    PORTC->PCR[SHOCK_PIN] &= ~PORT_PCR_PE_MASK;
    PORTC->PCR[SHOCK_PIN] |= PORT_PCR_PE(1);
    GPIOC->PDDR &= ~(1U << SHOCK_PIN);

    // ----- Ultrasonic: PTB0 trigger, PTB1 echo -----
    PORTB->PCR[TRIG_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[TRIG_PIN] |= PORT_PCR_MUX(1);

    PORTB->PCR[ECHO_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[ECHO_PIN] |= PORT_PCR_MUX(1);

    GPIOB->PDDR |= (1U << TRIG_PIN);
    GPIOB->PDDR &= ~(1U << ECHO_PIN);
    GPIOB->PCOR |= (1U << TRIG_PIN);

    // ----- Flame sensor: PTD4 -----
    PORTD->PCR[FLAME_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTD->PCR[FLAME_PIN] |= PORT_PCR_MUX(1);
    // disable pull-up first for cleaner testing
    PORTD->PCR[FLAME_PIN] &= ~PORT_PCR_PE_MASK;
    GPIOD->PDDR &= ~(1U << FLAME_PIN);

    ledOff(RED);
    ledOff(GREEN);
    ledOff(BLUE);
}

// ================= INTERRUPT INIT ===============
void initShockInterrupt(void)
{
    NVIC_DisableIRQ(PORTC_PORTD_IRQn);
    PORTC->PCR[SHOCK_PIN] &= ~PORT_PCR_IRQC_MASK;
    PORTC->PCR[SHOCK_PIN] |= PORT_PCR_IRQC(0x09); // rising edge
    NVIC_SetPriority(PORTC_PORTD_IRQn, 192);      // lower than UART
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}

void initFlameInterrupt(void)
{
    NVIC_DisableIRQ(PORTC_PORTD_IRQn);
    PORTD->PCR[FLAME_PIN] &= ~PORT_PCR_IRQC_MASK;
    PORTD->PCR[FLAME_PIN] |= PORT_PCR_IRQC(0x09); // rising edge
    NVIC_SetPriority(PORTC_PORTD_IRQn, 192);      // same as shock, lower than UART
    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}

// ================= LED FUNCTIONS =================
void ledOn(TLED led)
{
    switch (led)
    {
    case RED:
        GPIOE->PCOR |= (1U << RED_PIN);
        break;
    case GREEN:
        GPIOD->PCOR |= (1U << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PCOR |= (1U << BLUE_PIN);
        break;
    }
}

void ledOff(TLED led)
{
    switch (led)
    {
    case RED:
        GPIOE->PSOR |= (1U << RED_PIN);
        break;
    case GREEN:
        GPIOD->PSOR |= (1U << GREEN_PIN);
        break;
    case BLUE:
        GPIOE->PSOR |= (1U << BLUE_PIN);
        break;
    }
}

// Set LED color based on temperature thresholds:
void setLedFromTemperature(float temp)
{
    ledOff(RED);
    ledOff(GREEN);
    ledOff(BLUE);

    if (temp < 22.5f)
    {
        ledOn(GREEN);
    }
    else if (temp < 24.0f)
    {
        ledOn(BLUE);
    }
    else
    {
        ledOn(RED);
    }
}

// ================= UART SEND =====================
void sendMessage(char *message)
{
    if (xSemaphoreTake(uartMutex, portMAX_DELAY) == pdTRUE)
    {
        strncpy(send_buffer, message, MAX_MSG_LEN - 1);
        send_buffer[MAX_MSG_LEN - 1] = '\0';

        UART2->C2 |= UART_C2_TIE_MASK;
        UART2->C2 |= UART_C2_TE_MASK;

        xSemaphoreGive(uartMutex);
    }
}

// ================= UART ISR ======================
void UART2_FLEXIO_IRQHandler(void)
{
    static int recv_ptr = 0;
    static int send_ptr = 0;
    static char recv_buffer[MAX_MSG_LEN];
    char rx_data;

    if (UART2->S1 & UART_S1_TDRE_MASK)
    {
        if (send_buffer[send_ptr] == '\0')
        {
            send_ptr = 0;
            UART2->C2 &= ~UART_C2_TIE_MASK;
            UART2->C2 &= ~UART_C2_TE_MASK;
        }
        else
        {
            UART2->D = send_buffer[send_ptr++];
        }
    }

    if (UART2->S1 & UART_S1_RDRF_MASK)
    {
        TMessage msg;
        BaseType_t hpw = pdFALSE;

        rx_data = UART2->D;

        if (recv_ptr < MAX_MSG_LEN - 1)
        {
            recv_buffer[recv_ptr++] = rx_data;
        }

        if (rx_data == '\n')
        {
            recv_buffer[recv_ptr] = '\0';
            strncpy(msg.message, recv_buffer, MAX_MSG_LEN - 1);
            msg.message[MAX_MSG_LEN - 1] = '\0';

            xQueueSendFromISR(uartRxQueue, (void *)&msg, &hpw);
            portYIELD_FROM_ISR(hpw);
            recv_ptr = 0;
        }
    }
}

// ================= GPIO ISR ======================
// This ISR handles both shock and flame interrupts. We check the ISFR flags to determine which one triggered.
void PORTC_PORTD_IRQHandler(void)
{
    BaseType_t hpw = pdFALSE;

    // Shock sensor: PTC2
    if (PORTC->ISFR & (1U << SHOCK_PIN))
    {
        PORTC->ISFR |= (1U << SHOCK_PIN);
        // Wake shockTask via semaphore
        xSemaphoreGiveFromISR(shockSem, &hpw);
    }

    // Flame sensor: PTD0
    if (PORTD->ISFR & (1U << FLAME_PIN))
    {
        PORTD->ISFR |= (1U << FLAME_PIN);
        // Wake fireTask via semaphore
        xSemaphoreGiveFromISR(fireSem, &hpw);
    }

    portYIELD_FROM_ISR(hpw);
}

// ================= DELAY / ULTRASONIC ============
// Simple busy-wait delay. Not very accurate but sufficient for our ultrasonic timing.
static void delay_us(volatile uint32_t us)
{
    while (us--)
    {
        for (volatile int i = 0; i < 10; i++)
        {
            __asm volatile("nop");
        }
    }
}

// Measure distance using the ultrasonic sensor. Returns distance in cm, or -1.0f on timeout.
static float measureDistanceCm(void)
{
    uint32_t timeout;
    uint32_t echoCount = 0;

    GPIOB->PCOR |= (1U << TRIG_PIN);
    delay_us(2);

    GPIOB->PSOR |= (1U << TRIG_PIN);
    delay_us(10);
    GPIOB->PCOR |= (1U << TRIG_PIN);

    timeout = 30000;
    while (!(GPIOB->PDIR & (1U << ECHO_PIN)) && timeout--)
    {
        delay_us(1);
    }
    if (timeout == 0)
    {
        return -1.0f;
    }

    while (GPIOB->PDIR & (1U << ECHO_PIN))
    {
        echoCount++;
        delay_us(1);

        if (echoCount > 30000)
        {
            return -1.0f;
        }
    }
    // echoCount is the duration in microseconds. Convert to distance in cm:
    return ((float)echoCount) / 58.0f * 4;
}

// ================= TASK 1 ========================
// Receive temp/humidity from ESP32
static void uartRecvTask(void *p)
{
    TMessage msg;
    TSensorData sensor;

    while (1)
    {
        if (xQueueReceive(uartRxQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            if (sscanf(msg.message, "TEMP:%f,HUM:%f",
                       &sensor.temperature, &sensor.humidity) == 2)
            {
                // Update the latest sensor data in the tempQueue (overwrite if full)
                xQueueOverwrite(tempQueue, &sensor);

                int tempInt = (int)(sensor.temperature * 10);
                int humInt = (int)(sensor.humidity * 10);

                PRINTF("Received Temp=%d.%d Hum=%d.%d\r\n",
                       tempInt / 10, abs(tempInt % 10),
                       humInt / 10, abs(humInt % 10));
            }
        }
    }
}

// ================= TASK 2 ========================
// LED control from temperature
static void ledTask(void *p)
{
    TSensorData sensor;

    while (1)
    {
        // Wait for new temperature data from the uartRecvTask
        if (xQueueReceive(tempQueue, &sensor, portMAX_DELAY) == pdTRUE)
        {
            setLedFromTemperature(sensor.temperature);
        }
    }
}

// ================= TASK 3 ========================
// Shock alert
static void shockTask(void *p)
{
    while (1)
    {
        // Wait for shock alert from the ISR
        if (xSemaphoreTake(shockSem, portMAX_DELAY) == pdTRUE)
        {
            PRINTF("Shock detected!\r\n");
            sendMessage("SHOCK\n");
        }
    }
}

// ================= TASK 4 ========================
// Water level polling
static void waterLevelTask(void *p)
{
    float distance;
    char buffer[64];

    while (1)
    {
        distance = measureDistanceCm();

        if (distance > 0)
        {
            int dist10 = (int)(distance * 10);
            // Print and send distance with 1 decimal place
            PRINTF("Water level distance: %d.%d cm\r\n",
                   dist10 / 10, abs(dist10 % 10));

            snprintf(buffer, sizeof(buffer), "DIST:%d.%d\n",
                     dist10 / 10, abs(dist10 % 10));

            sendMessage(buffer);
        }
        else
        {
            PRINTF("Ultrasonic read timeout\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ================= TASK 5 ========================
// Fire alert
static void fireTask(void *p)
{
    while (1)
    {
        // Wait for fire alert from the ISR
        if (xSemaphoreTake(fireSem, portMAX_DELAY) == pdTRUE)
        {
            sendMessage("FIRE\n");
        }
    }
}

// ================= MAIN ==========================
int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    initGPIO();
    initShockInterrupt();
    initFlameInterrupt();
    initUART2(BAUD_RATE);

    PRINTF("MCXC444 UART + LED + Shock + Water + Fire Demo\r\n");
    // Create queues and semaphores
    uartRxQueue = xQueueCreate(QLEN, sizeof(TMessage));
    // tempQueue only needs length 1 since we only care about the latest reading
    tempQueue = xQueueCreate(1, sizeof(TSensorData));
    shockSem = xSemaphoreCreateBinary();
    fireSem = xSemaphoreCreateBinary();
    uartMutex = xSemaphoreCreateMutex();
    // Create tasks, priorities: Highest for UART receive and shock/fire alerts, lowest for LED and water level
    xTaskCreate(uartRecvTask, "uartRecvTask", configMINIMAL_STACK_SIZE + 150, NULL, 3, NULL);
    xTaskCreate(ledTask, "ledTask", configMINIMAL_STACK_SIZE + 100, NULL, 2, NULL);
    xTaskCreate(shockTask, "shockTask", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);
    xTaskCreate(waterLevelTask, "waterLevelTask", configMINIMAL_STACK_SIZE + 150, NULL, 2, NULL);
    xTaskCreate(fireTask, "fireTask", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);

    vTaskStartScheduler();

    while (1)
    {
        __asm volatile("nop");
    }

    return 0;
}
