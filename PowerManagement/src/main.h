#ifndef _main_h
#define _main_h
#include <Arduino.h>

struct PowerTaskItem
{
    int pin;
    PinStatus state;
    unsigned long delay;
    bool done;
};

struct PinDefinition
{
    int pin;
    PinStatus lastState;
    bool autoUp;
};

struct AdcDefinition
{
    int pin;
    float offset;
    float multiply;
};

struct EventItem
{
    int code;
    int argument;
    unsigned long time;
    bool isSet;
};

#define STARTED_EVENT 1
#define PIN_LOW_EVENT 2
#define PIN_HIGH_EVENT 3

const int EVENTS[] = { STARTED_EVENT, PIN_LOW_EVENT, PIN_HIGH_EVENT };

#endif