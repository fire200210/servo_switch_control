#include "data_process.h"

void search(uint8_t Rx_buffer[], char wifi_setting[], const uint8_t head, char *name, uint8_t *count, uint8_t *START, uint8_t *END)
{
  *START = *count + 1;       // 'I' 後面是 SSID 開始的位置
  (*count)++;
  while (Rx_buffer[*count])  // 繼續向後搜尋
  { 
    if (Rx_buffer[*count] == head)
    {
      *END = *count - 1;                  // 'P' 前面是 SSID 的結束位置
      size_t length = *END - *START + 1;  // 計算 SSID 長度，複製到 Wifi_SSID
      strncpy(wifi_setting, (char *)(&Rx_buffer[*START]), length);
      wifi_setting[length] = '\0';       // 確保字串以 '\0' 結束
      printf("%s: %s\n", name, wifi_setting);
      break;
    }
    (*count)++;
  }
}

void change_ssid_password(uint8_t Rx_buffer[], Setting_Param_t Setting_Param)
{
  uint8_t count = 0, START = 0, END = 0;

  while (Rx_buffer[count]) // 遍歷 wifi_data
  { 
    switch (Rx_buffer[count])
    {       
      case 0xDD: search(Rx_buffer, Setting_Param.Wifi_ssid,     0xAA,     "SSID", &count, &START, &END); break; 
      case 0xAA: search(Rx_buffer, Setting_Param.Wifi_password, 0x0D, "Password", &count, &START, &END); break;   
      default: count++; break;
    }
  }
}