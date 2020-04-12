#include "sensors/sensor.h"
#include "sensesp.h"
#ifndef BME280_H_
#define BME280_H_
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "system/observablevalue.h"

class BME280 : public Sensor {
    public:
    BME280(int address, int samplingInterval, String config_path);
    void enable() override final;
    ObservableValue<float> temperature;
    ObservableValue<float> pressure;
    ObservableValue<float> humidity;
    private:
    void update();
    int address;
    int samplingInterval;
    bool enabled;
    Adafruit_BME280 bme280;
};

#endif