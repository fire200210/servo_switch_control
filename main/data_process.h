#ifndef DATA_PROCESS_H_
#define DATA_PROCESS_H_

#include "main.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

void search(uint8_t Rx_buffer[], char wifi_setting[], const uint8_t head, char *name, uint8_t *count, uint8_t *START, uint8_t *END);
void change_ssid_password(uint8_t Rx_buffer[], Setting_Param_t Setting_Param);

#endif