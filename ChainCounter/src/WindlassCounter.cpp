#include "WindlassCounter.h"
#include <FunctionalInterrupt.h>
#include "sensesp.h"

WindlassCounter::WindlassCounter(int interuptPin, int upPin, int downPin, String config_path) : NumericSensor(config_path)
{
    className = "WindlassCounter";
    this->interuptPin = interuptPin;
    this->upPin = upPin;
    this->downPin = downPin;
    this->output = 0.0f;
    this->counter = 0;
    this->change = false;
    this->lastPinState = false;
    this->lastMs = 0;
    this->callCounter = 0;
    this->status = new WindlassStatus("");
    load_configuration();
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

        if(value == 0 && up == 1 && down == 0)
        { 
            this->counter++;
            this->change = true;
        }
        else if(value == 0 && down == 1 && up == 0)
        {           
            this->counter--;
            this->change = true;
        }
      }
    });

    app.onRepeat(5000, [this]()
    {
        int counterState = this->counter;

        if(this->lastStored != counterState)
        {
            this->save_configuration();
            this->lastStored = counterState;
            debugI("Storing counter value %d to flash.", counterState);
            //store to settings
        }
    });

    app.onRepeat(500, [this]()
    {
        int interupt = digitalRead(this->interuptPin);
        int up = digitalRead(this->upPin);
        int down = digitalRead(this->downPin);
        char statusText[128] = "OK\0";
        //String status = "OK";

        if(up == 1 && down == 1)
        {
            sprintf(statusText, "ERR: UP and DOWN pressed!");
        }
        else
        {
            sprintf(statusText, "U%dD%dCC%d", up, down, callCounter);
        }

        this->status->UpdateStatus(String(statusText));

        debugI("Interup: %d, Up: %d, Down: %d, Counter: %d, Value: %d", interupt, up, down, callCounter, this->counter);
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

WindlassStatus* WindlassCounter::GetStatus()
{
    return this->status;
}

JsonObject& WindlassCounter::get_configuration(JsonBuffer& buf) {
  JsonObject& root = buf.createObject();
  root.set("value", (float)this->counter);
  debugI("Counter value %d requested for config.", this->counter);

  return root;
}

static const char SCHEMA[] PROGMEM = R"({
    "type": "object",
    "properties": {
        "value": { "title": "Last value", "type" : "number" }
    }
  })";

String WindlassCounter::get_config_schema() {
  return FPSTR(SCHEMA);
}

bool WindlassCounter::set_configuration(const JsonObject& config) {
  if (config.containsKey("value")) {
      this->counter = (int)atof(config["value"]);
      this->lastStored = this->counter;
      this->notify();
      debugI("Counter value %d loaded from config.", this->counter);
  }
  return true;
}