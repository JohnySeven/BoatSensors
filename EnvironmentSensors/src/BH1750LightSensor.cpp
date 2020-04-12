#include "BH1750LightSensor.h"

BH1750LightSensor::BH1750LightSensor(int address, int samplingInterval, String config_path) : NumericSensor(config_path)
{
    this->address = address;
    this->samplingInterval = samplingInterval;
}

void BH1750LightSensor::enable()
{
    if(!this->sensor.begin(BH1750::ONE_TIME_HIGH_RES_MODE, this->address))
    {
        debugE("BH1750 sensor begin failed!");
    }
    else
    {
        this->enabled = true;
        app.onRepeat(this->samplingInterval, [this](){ this->update(); });
        debugI("BH1750 sensor initalized!");
    }
}

void BH1750LightSensor::update()
{
    float lightLevel = this->sensor.readLightLevel();

    debugI("Light level %f", lightLevel);

    this->output = lightLevel;
    this->notify();
}