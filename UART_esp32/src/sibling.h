#ifndef SRC_SIBLING_H
#define SRC_SIBLING_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef int sibling_handle_t;

void sibling_init(sibling_handle_t* handle, SemaphoreHandle_t semaphore);

#endif  // SRC_SIBLING_H