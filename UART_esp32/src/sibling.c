#include "sibling.h"

#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define RX_BUF_SIZE 1024
#define UART_NUM UART_NUM_1
#define BAUD_RATE 9600

#define TXD_PIN 17
#define RXD_PIN 18

#define RX_READ_CHUNK_SIZE 128
#define RX_READ_TIMEOUT_MS 100

static const char* TAG = "SIBLING";

const int sibling_id = 1;
const char* commandToSend = "Toggle Pin";

void send_message_task(void* arg);
static void receive_message_task(void* arg);

void sibling_init(sibling_handle_t* handle, SemaphoreHandle_t send_semaphore,
                  SemaphoreHandle_t receive_semaphore) {
  const uart_config_t uart_config = {
      .baud_rate = BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(
      uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
                               UART_PIN_NO_CHANGE));

  xTaskCreate(send_message_task, "send_message_task", 2048,
              (void*)send_semaphore, 5, NULL);
  xTaskCreate(receive_message_task, "receive_message_task", 3072,
              (void*)receive_semaphore, 5, NULL);

  *handle = sibling_id;
}

void send_message_task(void* arg) {
  SemaphoreHandle_t semaphore = (SemaphoreHandle_t)arg;
  while (1) {
    if (xSemaphoreTake(semaphore, portMAX_DELAY)) {
      size_t len = strlen(commandToSend);
      int txBytes =
          uart_write_bytes(UART_NUM, (const uint8_t*)commandToSend, len);
      ESP_LOGI(TAG, "Wrote %d bytes", txBytes);
    }
  }
}

static void receive_message_task(void* arg) {
  // Heap buffer keeps the task stack small; +1 leaves room for a terminator.
  uint8_t* data = (uint8_t*)malloc(RX_READ_CHUNK_SIZE + 1);
  if (data == NULL) {
    ESP_LOGE(TAG, "Failed to allocate RX buffer");
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    // Blocks until data arrives or the timeout expires, without busy-waiting.
    const int rxBytes = uart_read_bytes(UART_NUM, data, RX_READ_CHUNK_SIZE,
                                        pdMS_TO_TICKS(RX_READ_TIMEOUT_MS));
    if (rxBytes > 0) {
      data[rxBytes] = '\0';
      if (strcmp((const char*)data, commandToSend) == 0) {
        SemaphoreHandle_t semaphore = (SemaphoreHandle_t)arg;
        xSemaphoreGive(semaphore);
      }
    } else if (rxBytes < 0) {
      ESP_LOGE(TAG, "UART read error");
      vTaskDelay(pdMS_TO_TICKS(RX_READ_TIMEOUT_MS));
    }
  }
}
