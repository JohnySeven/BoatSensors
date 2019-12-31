#ifndef _windlasscounter_H_
#define _windlasscounter_H_

#include "Wire.h"
#include "sensors/sensor.h"

class WindlassCounter : public NumericSensor {
    public:
    WindlassCounter(int interuptPin, int upPin, int downPin, String config_path);
    void enable() override final;
    private:
    void update();
    int interuptPin;
    int upPin;
    int downPin;
    int counter;
    int change;
    int lastPinState;
    int lastMs;
    int callCounter;
};
#endif