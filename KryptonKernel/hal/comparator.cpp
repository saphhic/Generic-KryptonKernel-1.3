//written by shappic.
#include "comparator.h"

bool compare(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

bool text_eq_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (to_lower(*a) != to_lower(*b)) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

bool starts_with_ci(const char* text, const char* prefix) {
    if (!text || !prefix) return false;
    while (*prefix) {
        if (!*text || to_lower(*text) != to_lower(*prefix)) return false;
        text++;
        prefix++;
    }
    return true;
}

const char* skip_spaces(const char* text) {
    if (!text) return "";
    while (*text == ' ') text++;
    return text;
}

const char* read_token(const char* text, char* token, int cap) {
    text = skip_spaces(text);
    int i = 0;
    while (*text && *text != ' ' && i < cap - 1) {
        token[i++] = *text++;
    }
    token[i] = '\0';
    return skip_spaces(text);
}

int kstrlen(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

void kstrcpy(char* dst, const char* src, int cap) {
    if (!dst || cap <= 0) return;
    int i = 0;
    if (src) {
        while (src[i] && i < cap - 1) { dst[i] = src[i]; i++; }
    }
    dst[i] = '\0';
}