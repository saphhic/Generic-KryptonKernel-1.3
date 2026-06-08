//written by saphhic

#pragma once

const char* int_to_string(int value, char* buffer, int base) {

    if (base < 2 || base > 36) {

        buffer[0] = '\0';

        return buffer;
    }

    char* ptr = buffer;
    char* ptr1 = buffer;
    char tmp_char;

    int tmp_value;

    bool negative = false;

    if (value < 0 && base == 10) {

        negative = true;

        value = -value;
    }

    do {

        tmp_value = value;

        value /= base;

        *ptr++ =
            "0123456789abcdefghijklmnopqrstuvwxyz"
            [tmp_value - value * base];

    } while (value);

    if (negative) {

        *ptr++ = '-';
    }

    *ptr-- = '\0';

    while (ptr1 < ptr) {

        tmp_char = *ptr;

        *ptr-- = *ptr1;

        *ptr1++ = tmp_char;
    }

    return buffer;
}