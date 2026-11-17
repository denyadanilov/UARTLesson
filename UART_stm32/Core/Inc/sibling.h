#ifndef SIBLING_H
#define SIBLING_H

#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

typedef int sibling_handle_t;

void sibling_init(sibling_handle_t *handle, SemaphoreHandle_t send_semaphore,
                  SemaphoreHandle_t receive_semaphore);
size_t sibling_copy_last_rx(uint8_t *destination, size_t capacity);
uint32_t sibling_get_rx_dropped_bytes(void);

#endif
