#include "led.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

#define LED_PIN GPIO_NUM_6

int led_id = 1;

void led_init(led_handle_t* handle) {
  gpio_reset_pin(LED_PIN);
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

  gpio_glitch_filter_handle_t filter_handle;
  gpio_pin_glitch_filter_config_t filter_config = {
      .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT, .gpio_num = LED_PIN};
  gpio_new_pin_glitch_filter(&filter_config, &filter_handle);
  gpio_glitch_filter_enable(filter_handle);

  *handle = led_id;
}

void led_on(led_handle_t handle) {
  if (handle == led_id) gpio_set_level(LED_PIN, 1);
}

void led_off(led_handle_t handle) {
  if (handle == led_id) gpio_set_level(LED_PIN, 0);
}