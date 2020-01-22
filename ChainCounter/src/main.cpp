#include <Arduino.h>

//#define SERIAL_DEBUG_DISABLED

#include "sensors/digital_input.h"
#include "sensesp_app.h"
#include "transforms/frequency.h"
#include "signalk/signalk_output.h"
#include "sensors/onewire_temperature.h"
#include "sensors/system_info.h"
#include <Wire.h>
#include "transforms/linear.h"
#include "signalk/signalk_output.h"
#include "wiring_helpers.h"
#include "WindlassCounter.h"
#include "WindlassStatus.h"

#define InterupPin D1
#define UpPin D5
#define DownPin D6

const char* config_path_counter_value = "/counter/value";
const char* config_path_counter_transform = "/counter/transform";
const char* config_path_counter_sk = "/counter/sk";
const char* config_path_status_sk = "/counter/status";

// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object vs defining a "main()" method.
ReactESP app([]() {
#ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up

  delay(250);
  Debug.setSerialEnabled(true);
#endif

 debugI("Starting system...");

  sensesp_app = new SensESPApp();

  auto*counter = new WindlassCounter(InterupPin, UpPin, DownPin, config_path_counter_value);

  counter->connectTo(new Linear(1.0, 0.0, config_path_counter_transform))
         ->connectTo(new SKOutputNumber("sensors.windlass.counter", config_path_counter_sk));

  counter->GetStatus()
         ->connectTo(new SKOutputString("sensors.windlass.status", config_path_status_sk));
  // Start the SensESP application running. Because of everything that's been set up above,
  // it constantly monitors the interrupt pin, and every read_delay ms, it sends the 
  // calculated frequency to SignalK.
  sensesp_app->enable();
});