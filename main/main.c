/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include <math.h>
#include <stdlib.h>

#define deadZone 150

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY)) {
            int val = data.val/100;
            int msb = val >> 8;
            int lsb = val & 0xFF;

            uart_putc_raw(uart0, data.axis);
            uart_putc_raw(uart0, msb);
            uart_putc_raw(uart0, lsb);
            uart_putc_raw(uart0, -1);
        }
    }
}

void x_adc_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(26);

    float result;

    while (1) {
        adc_select_input(0); // Select ADC input 0 (GPIO26)
        result = adc_read();

        result = result - 2048;
        result = result / 8;

        if (abs(result) < deadZone) {
            result = 0;
        }

        data.val = result;
        data.axis = 0;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_adc_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(27);

    float result;

    while (1) {
        adc_select_input(1); // Select ADC input 1 (GPIO27)
        result = adc_read();

        result = result - 2048;
        result = result / 8;

        if (abs(result) < deadZone) {
            result = 0;
        }
        data.val = result;
        data.axis = 1;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_adc_task, "adc_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_adc_task, "adc_task2", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
