//written by shappic.
#ifndef COMPARATOR_H
#define COMPARATOR_H

#include <stdint.h>

bool compare(const char* a, const char* b);
bool text_eq_ci(const char* a, const char* b);
bool starts_with_ci(const char* text, const char* prefix);
const char* skip_spaces(const char* text);
const char* read_token(const char* text, char* token, int cap);
int kstrlen(const char* s);
void kstrcpy(char* dst, const char* src, int cap);

#endif // COMPARATOR_H