#pragma once

#include <stdint.h>

struct RtcDateTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

extern "C" unsigned char rtc_get_seconds();
extern "C" unsigned char rtc_get_minutes();
extern "C" unsigned char rtc_get_hours();

bool rtc_get_utc_time(RtcDateTime* out_time);
bool rtc_get_local_time(RtcDateTime* out_time);
int rtc_get_utc_hour();
int rtc_get_local_hour();

void set_timezone(int offset);
int rtc_get_timezone();
uint32_t rtc_get_total_seconds(); // Nueva función
