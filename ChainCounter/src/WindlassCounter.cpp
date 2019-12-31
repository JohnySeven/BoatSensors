#include "WindlassCounter.h"
#include <FunctionalInterrupt.h>
#include "sensesp.h"

WindlassCounter::WindlassCounter(int interuptPin, int upPin, int downPin, String config_path) : NumericSensor(config_path)
{
    this->interuptPin = interuptPin;
    this->upPin = upPin;
    this->downPin = downPin;
    this->output = 0.0f;
    this->counter = 0;
    this->change = false;
    this->lastPinState = false;
    this->lastMs = 0;
    this->callCounter = 0;
}

void WindlassCounter::enable()
{
    pinMode(upPin, INPUT);
    pinMode(downPin, INPUT);

    app.onInterrupt(this->interuptPin, FALLING, [this]()
    {
      int value = digitalRead(this->interuptPin);
      int time = millis();

      if(lastMs == 0)
      {
          lastMs = time;
      }

      if(lastPinState != value && (time - lastMs) > 50)
      {
        lastMs = time;
        callCounter++;
        lastPinState = value;

        int up = digitalRead(this->upPin);
        int down = digitalRead(this->downPin);

        if(value == 0 && up == 1)
        { 
            this->counter++;
            this->change = true;
        }
        else if(value == 0 && down == 1)
        {           
            this->counter--;
            this->change = true;
        }
      }
    });

    app.onRepeat(500, [this]()
    {
        debugI("Interup: %d, Up: %d, Down: %d, Counter: %d, Value: %d", digitalRead(this->interuptPin),digitalRead(this->upPin), digitalRead(this->downPin), callCounter, this->counter);
        this->callCounter = 0;

        if(this->change)
        {
            debugI("Counter updated to %d", this->counter);
            this->change = false;
            this->output = (float)this->counter;
            this->notify();
        }
    });
    this->output = 0.0f;
    this->notify();
}