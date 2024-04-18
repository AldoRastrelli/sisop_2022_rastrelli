#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

char *split_line(char *buf, char splitter);

int block_contains(char *buf, char c);

int printf_debug(char *format, ...);
int fprintf_debug(FILE *file, char *format, ...);
int process_is_parent(pid_t pid);
int process_is_child(pid_t pid);

#endif  // UTILS_H
