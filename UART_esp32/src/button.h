#ifndef SRC_BUTTON_H
#define SRC_BUTTON_H
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
typedef int button_handle_t;

void button_init(button_handle_t* handle, SemaphoreHandle_t semaphore_handle);

#endif  // SRC_BUTTON_H