#include "button.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "led.h"
#include "sibling.h"

button_handle_t button;
led_handle_t led;
sibling_handle_t sibling;

SemaphoreHandle_t button_signal_semaphore_handle;
SemaphoreHandle_t sibling_semaphore_handle;

void button_signal_handler_task(void* arg);

void app_main() {
  button_signal_semaphore_handle = xSemaphoreCreateBinary();
  button_init(&button, button_signal_semaphore_handle);
  led_init(&led);
  sibling_semaphore_handle = xSemaphoreCreateBinary();
  sibling_init(&sibling, sibling_semaphore_handle);

  xTaskCreate(button_signal_handler_task, "button_signal_handler_task", 2048,
              NULL, 5, NULL);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void button_signal_handler_task(void* arg) {
  while (1) {
    if (xSemaphoreTake(button_signal_semaphore_handle, portMAX_DELAY)) {
      xSemaphoreGive(sibling_semaphore_handle);
    }
  }
}