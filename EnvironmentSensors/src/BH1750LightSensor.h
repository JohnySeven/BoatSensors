#include "sensors/sensor.h"
#include "sensesp.h"
#ifndef BH1750LightSensor_H_
#define BH1750LightSensor_H_
#include <Wire.h>
#include <BH1750.h>
#include "system/observablevalue.h"

class BH1750LightSensor : public NumericSensor {
    public:
    BH1750LightSensor(int address, int samplingInterval, String config_path);
    void enable() override final;
    private:
    void update();
    int address;
    int samplingInterval;
    bool enabled;
    BH1750 sensor;
};

#endif