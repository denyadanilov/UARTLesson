#include "led.h"

#include "main.h"

#define LED_PIN GPIO_PIN_7

static const led_handle_t led_id = 1;

void led_init(led_handle_t *handle)
{
  HAL_GPIO_WritePin(GPIOA, LED_PIN, GPIO_PIN_RESET);
  *handle = led_id;
}

void led_on(led_handle_t handle)
{
  if (handle == led_id)
  {
    HAL_GPIO_WritePin(GPIOA, LED_PIN, GPIO_PIN_SET);
  }
}

void led_off(led_handle_t handle)
{
  if (handle == led_id)
  {
    HAL_GPIO_WritePin(GPIOA, LED_PIN, GPIO_PIN_RESET);
  }
}
