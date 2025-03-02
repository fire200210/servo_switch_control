#include "main.h"
#include "servo.h"
#include "gpio.h"

static const char *TAG = "example";

static mcpwm_cmpr_handle_t comparator = NULL;
extern Wifi_Info_t Wifi_Info;
extern Servo_Info_t Servo_Info;

void servo_init(void)
{
  ESP_LOGI(TAG, "Create timer and operator");
  mcpwm_timer_handle_t timer = NULL;
  mcpwm_timer_config_t timer_config = {
      .group_id = 0,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
      .period_ticks = SERVO_TIMEBASE_PERIOD,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
  };
  ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

  mcpwm_oper_handle_t oper = NULL;
  mcpwm_operator_config_t operator_config = {
      .group_id = 0, // operator must be in the same group to the timer
  };
  ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

  ESP_LOGI(TAG, "Connect timer and operator");
  ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

  ESP_LOGI(TAG, "Create comparator and generator from the operator");
  mcpwm_comparator_config_t comparator_config = {
      .flags.update_cmp_on_tez = true,
  };
  ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

  mcpwm_gen_handle_t generator = NULL;
  mcpwm_generator_config_t generator_config = {
      .gen_gpio_num = SERVO_PULSE_GPIO,
  };
  ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

  // set the initial compare value, so that the servo will spin to the center position
  ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(0)));

  ESP_LOGI(TAG, "Set generator action on timer and compare event");
  // go high on counter empty
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
  // go low on compare threshold
  ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

  ESP_LOGI(TAG, "Enable and start timer");
  ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
  ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

void servo_task(void *pvParameters)
{
  static bool return_zero = false;

  while (1)
  {
    if (Servo_Info.SwitchChange_flag || Servo_Info.ButtonChange_flag)
    {
      return_zero = true;
      switch (Servo_Info.ServoCtrl)
      {
        case CLOSE: //close
          gpio_set_level(GPIO_OUTPUT_IO, 0); // Set output high
          mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(-60));
          vTaskDelay(pdMS_TO_TICKS(3000));
          Servo_Info.SwitchStatu_flag = false;
          break;
        case OPEN: //open
          gpio_set_level(GPIO_OUTPUT_IO, 1); // Set output high
          mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(60));
          vTaskDelay(pdMS_TO_TICKS(3000));
          Servo_Info.SwitchStatu_flag = true;
          break;
      }
      Servo_Info.SwitchChange_flag = Servo_Info.ButtonChange_flag = false;
    }
    if (return_zero)
    {
      return_zero = false;
      mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(0));
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
