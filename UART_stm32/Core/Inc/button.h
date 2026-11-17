#ifndef BUTTON_H
#define BUTTON_H

#include "FreeRTOS.h"
#include "semphr.h"

typedef int button_handle_t;

void button_init(button_handle_t *handle, SemaphoreHandle_t semaphore_handle);

#endif
