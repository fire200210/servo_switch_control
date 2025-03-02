#ifndef MAIM_H_
#define MAIN_H_

#pragma once

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lwip/sockets.h"

typedef struct{
  char Wifi_ssid[32];
  char Wifi_password[64];
}Setting_Param_t;

typedef struct WifiInfo{
  bool    WifiData_Statu;
  uint8_t Rx_buffer[256];
}Wifi_Info_t;

enum {OPEN = 0x01, CLOSE = 0x00};

typedef struct{
  bool    SwitchChange_flag;
  bool    ButtonChange_flag;
  bool    SwitchStatu_flag;
  uint8_t ServoCtrl;
}Servo_Info_t;

typedef struct {
  bool WifiConnectAP_Success;
  bool WifiConnectMoblie_Success;
  enum{
    NONE_Control,
    LED_Close,
    AP_Success,
    LED_Open,
  }Wifi;
}ConnectStatus_t;

#endif