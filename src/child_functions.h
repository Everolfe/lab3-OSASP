#ifndef CHILD_FUNCTIONS_H
#define CHILD_FUNCTIONS_H
#define _POSIX_C_SOURCE 199309L
#include "globals.h"

void child_main_loop();
void init_child();
void update_statistics();
void request_output_permission();
void print_statistics();

#endif // CHILD_FUNCTIONS_H