#ifndef SRC_BUTTON_H
#define SRC_BUTTON_H

typedef int button_handle_t;

void button_init(button_handle_t* handle);
void button_control_task();

#endif  // SRC_BUTTON_H