/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"
#include "gpio.h"
#include "servo.h"
#include "wifi_station.h"

ConnectStatus_t ConnectStatus = { 0 };
Wifi_Info_t     Wifi_Info     = { 0 };
Setting_Param_t Setting_Param = { .Wifi_ssid = CONFIG_ESP_WIFI_SSID, .Wifi_password = CONFIG_ESP_WIFI_PASSWORD, };
Servo_Info_t    Servo_Info    = { 0 };

static void flash_nvs_init(void);

void app_main(void)
{
  flash_nvs_init();
  button_gpio_init();
  connect_gpio_init();
  wifi_init_sta();
  servo_init();

  xTaskCreate(tcp_server_task, "tcp_server_task", 4096,  (void *)AF_INET,  5, NULL);
  xTaskCreate(servo_task,      "servo_task",      4096,   NULL,           10, NULL);
  xTaskCreate(gpio_task,       "gpio_task",       2048,   NULL,            6, NULL);
}

static void flash_nvs_init(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}