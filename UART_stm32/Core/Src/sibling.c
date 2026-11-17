#include "sibling.h"

#include <string.h>

#include "main.h"
#include "task.h"

#define RX_QUEUE_LENGTH 256U
#define RX_CHUNK_SIZE 128U
#define RX_READ_TIMEOUT_MS 100U
#define SIBLING_TASK_STACK_WORDS 256U
#define RECEIVE_TASK_STACK_WORDS 512U
#define SIBLING_TASK_PRIORITY 5U

static const sibling_handle_t sibling_id = 1;
static const uint8_t command_to_send[] = "Toggle Pin";

static QueueHandle_t rx_queue;
static TaskHandle_t receive_task_handle;
static uint8_t rx_byte;
static uint8_t last_rx_chunk[RX_CHUNK_SIZE];
static size_t last_rx_chunk_length;
static volatile uint32_t rx_dropped_bytes;
static volatile uint32_t rx_rearm_errors;

static void restart_uart_receive(void) {
  uint32_t errors;

  taskENTER_CRITICAL();
  errors = rx_rearm_errors;
  rx_rearm_errors = 0U;
  taskEXIT_CRITICAL();

  if (errors != 0U && (HAL_UART_AbortReceive(&huart1) != HAL_OK ||
                       HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK)) {
    Error_Handler();
  }
}

static void send_message_task(void* argument) {
  SemaphoreHandle_t semaphore = (SemaphoreHandle_t)argument;

  for (;;) {
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
      if (HAL_UART_Transmit(&huart1, (uint8_t*)command_to_send,
                            sizeof(command_to_send) - 1U,
                            HAL_MAX_DELAY) != HAL_OK) {
        Error_Handler();
      }
    }
  }
}

static void receive_message_task(void* argument) {
  uint8_t chunk[RX_CHUNK_SIZE];
  uint8_t byte;
  size_t chunk_length;
  size_t matched_length = 0U;
  SemaphoreHandle_t semaphore = (SemaphoreHandle_t)argument;

  if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK) {
    Error_Handler();
  }

  for (;;) {
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(RX_READ_TIMEOUT_MS));
    restart_uart_receive();
    if (xQueueReceive(rx_queue, &byte, 0U) != pdTRUE) {
      continue;
    }

    chunk[0] = byte;
    chunk_length = 1U;
    while (chunk_length < RX_CHUNK_SIZE &&
           xQueueReceive(rx_queue, &chunk[chunk_length], 0U) == pdTRUE) {
      ++chunk_length;
    }

    taskENTER_CRITICAL();
    memcpy(last_rx_chunk, chunk, chunk_length);
    last_rx_chunk_length = chunk_length;
    taskEXIT_CRITICAL();

    for (size_t i = 0U; i < chunk_length; ++i) {
      if (chunk[i] == command_to_send[matched_length]) {
        ++matched_length;
        if (matched_length == sizeof(command_to_send) - 1U) {
          (void)xSemaphoreGive(semaphore);
          matched_length = 0U;
        }
      } else {
        matched_length = (chunk[i] == command_to_send[0]) ? 1U : 0U;
      }
    }
  }
}

void sibling_init(sibling_handle_t* handle, SemaphoreHandle_t send_semaphore,
                  SemaphoreHandle_t receive_semaphore) {
  configASSERT(handle != NULL);
  configASSERT(send_semaphore != NULL);
  configASSERT(receive_semaphore != NULL);

  rx_queue = xQueueCreate(RX_QUEUE_LENGTH, sizeof(uint8_t));
  if (rx_queue == NULL) {
    Error_Handler();
  }

  if (xTaskCreate(send_message_task, "send_message", SIBLING_TASK_STACK_WORDS,
                  send_semaphore, SIBLING_TASK_PRIORITY, NULL) != pdPASS) {
    Error_Handler();
  }

  if (xTaskCreate(receive_message_task, "receive_message",
                  RECEIVE_TASK_STACK_WORDS, receive_semaphore,
                  SIBLING_TASK_PRIORITY, &receive_task_handle) != pdPASS) {
    Error_Handler();
  }

  *handle = sibling_id;
}

size_t sibling_copy_last_rx(uint8_t* destination, size_t capacity) {
  size_t bytes_to_copy;

  if (destination == NULL || capacity == 0U) {
    return 0U;
  }

  taskENTER_CRITICAL();
  bytes_to_copy = last_rx_chunk_length;
  if (bytes_to_copy > capacity) {
    bytes_to_copy = capacity;
  }
  memcpy(destination, last_rx_chunk, bytes_to_copy);
  taskEXIT_CRITICAL();

  return bytes_to_copy;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (huart->Instance != USART1) {
    return;
  }

  if (xQueueSendFromISR(rx_queue, &rx_byte, &higher_priority_task_woken) !=
      pdTRUE) {
    ++rx_dropped_bytes;
  }

  if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK) {
    ++rx_rearm_errors;
  }

  vTaskNotifyGiveFromISR(receive_task_handle, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (huart->Instance == USART1) {
    ++rx_rearm_errors;
    vTaskNotifyGiveFromISR(receive_task_handle, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
}

uint32_t sibling_get_rx_dropped_bytes(void) { return rx_dropped_bytes; }
