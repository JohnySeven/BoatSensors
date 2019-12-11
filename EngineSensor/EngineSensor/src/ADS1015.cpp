#include "ADS1015.h"
#include "sensesp.h"

ADS1015::ADS1015(int address, String config_path) : Sensor(config_path)
{
    className = "ADS1015";
    this->address = address;    
}

void ADS1015::enable()
{
    adc = new Adafruit_ADS1115(this->address);
    adc->begin();
}

uint16_t ADS1015::readValue(uint8_t index)
{
     
    return adc->readADC_SingleEnded(index);
}


ADS1015Channel::ADS1015Channel(int index, ADS1015*adc, String config_path) : NumericSensor(config_path)
{
    this->adc = adc;
    this->index = index;
}

void ADS1015Channel::enable()
{
    app.onRepeat(500, [this](){ this->update(); });
}

void ADS1015Channel::update()
{
    auto value = this->adc->readValue(this->index);
    debugI("Reading ADC %d=%d.", this->index, value);
    this->output = value;
    this->notify();
}