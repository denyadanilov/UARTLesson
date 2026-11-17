#ifndef LED_H
#define LED_H

typedef int led_handle_t;

void led_init(led_handle_t *handle);
void led_on(led_handle_t handle);
void led_off(led_handle_t handle);

#endif
