#include "button.h"

#include "main.h"
#include "task.h"

#define BUTTON_PIN GPIO_PIN_6
#define BUTTON_DEBOUNCE_MS 50U
#define BUTTON_TASK_STACK_WORDS 256U
#define BUTTON_TASK_PRIORITY 5U

static const button_handle_t button_id = 1;
static SemaphoreHandle_t button_signal_semaphore;
static TaskHandle_t button_task_handle;

static void button_control_task(void *argument)
{
  TickType_t last_accepted_tick =
      xTaskGetTickCount() - pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);

  __HAL_GPIO_EXTI_CLEAR_IT(BUTTON_PIN);
  HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  for (;;)
  {
    if (xTaskNotifyWait(0U, 0U, NULL, portMAX_DELAY) == pdTRUE)
    {
      TickType_t current_tick = xTaskGetTickCount();
      if ((TickType_t)(current_tick - last_accepted_tick) >=
          pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS))
      {
        last_accepted_tick = current_tick;
        (void)xSemaphoreGive(button_signal_semaphore);
      }
    }
  }
}

void button_init(button_handle_t *handle, SemaphoreHandle_t semaphore_handle)
{
  configASSERT(handle != NULL);
  configASSERT(semaphore_handle != NULL);

  button_signal_semaphore = semaphore_handle;
  if (xTaskCreate(button_control_task, "button_control", BUTTON_TASK_STACK_WORDS,
                  NULL, BUTTON_TASK_PRIORITY, &button_task_handle) != pdPASS)
  {
    Error_Handler();
  }

  *handle = button_id;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == BUTTON_PIN && button_task_handle != NULL)
  {
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xTaskNotifyFromISR(button_task_handle, 0U, eNoAction,
                             &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}
