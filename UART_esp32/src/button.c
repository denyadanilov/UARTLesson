#include "button.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"

#define BUTTON_PIN GPIO_NUM_7

int button_id = 1;
task_handle_t button_task_handle;

static void button_isr_handler(void* arg) {
  configASSERT(button_task_handle != NULL);

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(button_task_handle, 0, eNoAction,
                     &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void button_init(button_handle_t* handle) {
  gpio_reset_pin(BUTTON_PIN);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

  xTaskCreate(button_control_task, "button_control_task", 2048, NULL, 5,
              &button_task_handle);

  *handle = button_id;
}

void button_control_task() {
  configASSERT(button_task_handle != NULL);

  if (xTaskNotifyWait(0, 0, NULL, portMAX_DELAY)) {
    if (gpio_get_level(BUTTON_PIN) == 0) {
      // Handle button press event here
    }
  }
}