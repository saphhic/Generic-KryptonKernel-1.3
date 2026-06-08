
// written by saphhic.


#ifndef PRINTF_H
#define PRINTF_H

#include <stdint.h>

extern int cursor_x;
extern int cursor_y;

void printxy(const char* str, int x, int y);
void putchar(char c);
void printf(const char* str); 
void clear_screen();  
void upd_cursor();

#endif // PRINTF_H