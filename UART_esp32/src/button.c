#include "button.h"

#include "driver/gpio.h"
#include "driver/gpio_filter.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_num.h"

#define BUTTON_PIN GPIO_NUM_7
#define DEBOUNCE_TIME_MS 50

int button_id = 1;
TaskHandle_t button_task_handle;
uint32_t last_button_press_time = 0;

void button_control_task(void* arg);

static void button_isr_handler(void* arg) {
  configASSERT(button_task_handle != NULL);

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xTaskNotifyFromISR(button_task_handle, 0, eNoAction,
                     &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void button_init(button_handle_t* handle, SemaphoreHandle_t semaphore_handle) {
  gpio_reset_pin(BUTTON_PIN);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_POSEDGE);
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

  xTaskCreate(button_control_task, "button_control_task", 2048,
              semaphore_handle, 5, &button_task_handle);

  gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

  *handle = button_id;
}

void button_control_task(void* arg) {
  SemaphoreHandle_t semaphore_handle = (SemaphoreHandle_t)arg;
  configASSERT(semaphore_handle != NULL);

  while (1) {
    if (xTaskNotifyWait(0, 0, NULL, portMAX_DELAY)) {
      uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
      if (current_time - last_button_press_time >= DEBOUNCE_TIME_MS) {
        last_button_press_time = current_time;
        xSemaphoreGive(semaphore_handle);
      }
    }
  }
}