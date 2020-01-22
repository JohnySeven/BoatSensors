#include "WindlassStatus.h"
#include "sensesp.h"

WindlassStatus::WindlassStatus(String configPath) : StringSensor(configPath)
{
    className = "WindlassStatus";
    this->UpdateStatus("Initialized");
}

void WindlassStatus::enable()
{

}

void WindlassStatus::UpdateStatus(String status)
{
    if(!this->output.equalsIgnoreCase(status))
    {
        debugI("Windlass status updated.");
        this->output = status;
        this->notify();
    }
}