#include "BME280.h"

BME280::BME280(int address, int samplingInterval, String config_path) : Sensor(config_path)
{
    this->samplingInterval = samplingInterval;
    this->address = address;
}

void BME280::enable()
{
    if(!this->bme280.begin(this->address))
    {
        debugE("BME280 sensor begin failed!");
    }
    else
    {
        this->enabled = true;
        app.onRepeat(this->samplingInterval, [this](){ this->update(); });
        debugI("BME280 sensor %d initalized!", this->bme280.sensorID());
    }
}

void BME280::update()
{
    this->temperature.set(this->bme280.readTemperature() + 273.15);
    this->pressure.set(this->bme280.readPressure());
    this->humidity.set(this->bme280.readHumidity());
}