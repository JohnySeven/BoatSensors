#ifndef _windlasscounter_H_
#define _windlasscounter_H_

#include "Wire.h"
#include "sensors/sensor.h"
#include "WindlassStatus.h"

class WindlassCounter : public NumericSensor {
    public:
    WindlassCounter(int interuptPin, int upPin, int downPin, String config_path);
    void enable() override final;
    virtual JsonObject& get_configuration(JsonBuffer& buf) override final;
    virtual bool set_configuration(const JsonObject& config) override final;
    virtual String get_config_schema() override;
    WindlassStatus* GetStatus();
    private:
    int interuptPin;
    int upPin;
    int downPin;
    int counter;
    int change;
    int lastPinState;
    int lastMs;
    int callCounter;
    int lastStored;
    WindlassStatus*status;
};
#endif