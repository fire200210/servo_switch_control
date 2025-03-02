#ifndef GPIO_H_
#define GPIO_H_

#include "main.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_OUTPUT_IO 2  // Example GPIO for output
#define GPIO_INPUT_IO  4  // Example GPIO for input

void button_gpio_init(void);
void connect_gpio_init(void);

void gpio_task(void *pvParameters);

#endif