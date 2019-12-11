#ifndef ADS1015_H_
#define ADS1015_H_

#include "sensors/sensor.h"
#include <Wire.h>
#include <Adafruit_ADS1015.h>

class ADS1015 : public Sensor {
    public:
    ADS1015(int address, String config_path);
    void enable() override final;
    uint16_t readValue(uint8_t index);
    private:
    void update();
    Adafruit_ADS1115* adc;
    int address;
};

class ADS1015Channel : public NumericSensor
{
    public:
        ADS1015Channel(int index, ADS1015* adc, String config_path);
        void enable() override final;
    private:
        void update();
        ADS1015* adc;
        int index;
};

#endif