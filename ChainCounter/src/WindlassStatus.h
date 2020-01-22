#ifndef _windlassstatus_H_
#define _windlassstatus_H_

#include "Wire.h"
#include "sensors/sensor.h"

class WindlassStatus : public StringSensor
{
    public:
        WindlassStatus(String config_path);
        void enable() override final;
        void UpdateStatus(String status);
};

#endif