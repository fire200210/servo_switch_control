#include "gpio.h"

#define CONNECT_GPIO_OUTPUT_IO   16

extern Wifi_Info_t Wifi_Info;
extern Servo_Info_t Servo_Info;
extern ConnectStatus_t ConnectStatus;

void button_gpio_init(void)
{
  gpio_config_t output_conf = {
    .pin_bit_mask = (1ULL << GPIO_OUTPUT_IO),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
  };
  gpio_config(&output_conf);

  gpio_config_t input_conf = {
    .pin_bit_mask = (1ULL << GPIO_INPUT_IO),
    .mode         = GPIO_MODE_INPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
  };
  gpio_config(&input_conf);
}

void connect_gpio_init(void)
{
  gpio_config_t output_conf = {
    .pin_bit_mask = (1ULL << CONNECT_GPIO_OUTPUT_IO),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
  };
  gpio_config(&output_conf);
}

void gpio_task(void *pvParameters)
{
  while (1)
  {
    if(!gpio_get_level(GPIO_INPUT_IO) && ((Servo_Info.SwitchChange_flag|Servo_Info.ButtonChange_flag) == false))
    {
      printf("gpio check %d\n\n", gpio_get_level(GPIO_INPUT_IO));
      Servo_Info.ButtonChange_flag = true;
      Servo_Info.ServoCtrl = (Servo_Info.ServoCtrl == OPEN && Servo_Info.SwitchStatu_flag) ?  CLOSE : OPEN;
    }
    else if(!gpio_get_level(GPIO_INPUT_IO) && Servo_Info.SwitchChange_flag)
      printf("Mobile using !!!\n");

    if(ConnectStatus.Wifi == LED_Open)
    {
      gpio_set_level(CONNECT_GPIO_OUTPUT_IO, 1); // Set output high
      ConnectStatus.Wifi = NONE_Control;
    }
    else if(ConnectStatus.Wifi == LED_Close)
    {     
      gpio_set_level(CONNECT_GPIO_OUTPUT_IO, 0); // Set output low
      ConnectStatus.Wifi = NONE_Control;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));  // Delay 1 second
  }
}
