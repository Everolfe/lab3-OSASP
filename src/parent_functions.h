#ifndef PARENT_FUNCTIONS_H
#define PARENT_FUNCTIONS_H
#define _POSIX_C_SOURCE 199309L
#include "globals.h"

void parent_main_loop();
void init_parent();
void cleanup_parent();
void create_child();
void remove_last_child();
void remove_all_children();
void list_processes();
void toggle_child_output(pid_t pid, bool allow);
void handle_user_input(const char* input);
void print_status(const char* message);
// Новые функции
void disable_all_output();
void enable_all_output();
void disable_child_output(int child_num);
void enable_child_output(int child_num);
void request_child_output(int child_num);
void alarm_handler(int sig);
#endif // PARENT_FUNCTIONS_H