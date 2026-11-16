#ifndef SRC_LED_H
#define SRC_LED_H

typedef int led_handle_t;

void led_init(led_handle_t* handle);
void led_on(led_handle_t handle);
void led_off(led_handle_t handle);

#endif  // SRC_LED_H