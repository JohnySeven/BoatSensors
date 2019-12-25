#include <Arduino.h>

//#define SERIAL_DEBUG_DISABLED

#include "sensors/digital_input.h"
#include "sensesp_app.h"
#include "transforms/frequency.h"
#include "signalk/signalk_output.h"
#include "sensors/onewire_temperature.h"
#include "sensors/system_info.h"
#include <Wire.h>
#include "ADS1015.h"
#include "transforms/linear.h"
#include "signalk/signalk_output.h"
#include "wiring_helpers.h"

#define ONE_WIRE_BUS D4
// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object vs defining a "main()" method.
ReactESP app([]() {
#ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up

  delay(100);
  Debug.setSerialEnabled(true);
#endif

  sensesp_app = new SensESPApp();



  // The "SignalK path" identifies the output of the sensor to the SignalK network.
  // If you have multiple sensors connected to your microcontoller (ESP), each one of them
  // will (probably) have its own SignalK path variable. For example, if you have two
  // propulsion engines, and you want the RPM of each of them to go to SignalK, you might
  // have sk_path_portEngine = "propulsion.port.revolutions"
  // and sk_path_starboardEngine = "propulsion.starboard.revolutions"
  // In this example, there is only one propulsion engine, and its RPM is the only thing
  // being reported to SignalK.
  const char* sk_path = "propulsion.left.revolutions";
  const char* adc_sk_path = "sensors.adc";


  // The "Configuration path" is combined with "/config" to formulate a URL
  // used by the RESTful API for retrieving or setting configuration data.
  // It is ALSO used to specify a path to the SPIFFS file system
  // where configuration data is saved on the microcontroller.  It should
  // ALWAYS start with a forward slash if specified. If left blank,
  // that indicates this sensor or transform does not have any
  // configuration to save.
  // Note that if you want to be able to change the sk_path at runtime,
  // you will need to specify a configuration path.
  // As with the SignalK path, if you have multiple configurable sensors
  // connected to the microcontroller, you will have a configuration path
  // for each of them, such as config_path_portEngine = "/sensors/portEngine/rpm"
  // and config_path_starboardEngine = "/sensor/starboardEngine/rpm".
  const char* config_path = "/sensors/engine_rpm";

  // These two are necessary until a method is created to synthesize them. Everything
  // after "/sensors" in each of these ("/engine_rpm/calibrate" and "/engine_rpm/sk")
  // is simply a label to display what you're configuring in the Configuration UI. 
  const char* config_path_calibrate = "/sensors/engine_rpm/calibrate";
  const char* config_path_skpath = "/sensors/engine_rpm/sk";
  const char* config_path_temp_sk = "propulsion.left.temperature";
  const char* config_path_temp_sensor = "/sensors/temp/sensor";
  const char* config_path_temp_offset = "/sensors/temp/offset";
  const char* config_path_adc_linear = "/sensors/adc/offset";
  const char* config_path_adc_channel = "/sensors/adc/channel";
//////////
// connect a RPM meter. A DigitalInputCounter implements an interrupt
// to count pulses and reports the readings every read_delay ms
// (500 in the example). A Frequency
// transform takes a number of pulses and converts that into
// a frequency. The sample multiplier converts the 97 tooth
// tach output into Hz, SK native units.
const float multiplier = 1.0 / 97.0;
const uint read_delay = 500;


  // Wire it all up by connecting the producer directly to the consumer
  auto* pSensor = new DigitalInputCounter(14, INPUT_PULLUP, RISING, read_delay);

  pSensor->connectTo(new Frequency(multiplier, config_path_calibrate))  // connect the output of pSensor to the input of Frequency()
         ->connectTo(new SKOutputNumber(sk_path, config_path_skpath));   // connect the output of Frequency() to a SignalK Output as a number


  auto* adc = new ADS1015(0x49, "");

  const float adc_multiplier = 1.0;
  const float adc_offset = 0.0;

  for (int i = 0; i < 4; i++)
  {
    auto* channel = new ADS1015Channel(i, adc, "");
    auto* pTransform = new Linear(adc_multiplier, adc_offset, String(config_path_adc_linear) + String(i));

    channel->connectTo(pTransform)
           ->connectTo(new SKOutputNumber(String(adc_sk_path) + String(i), String(config_path_adc_channel) + String(i)));
  }

  auto*owSensors = new DallasTemperatureSensors(ONE_WIRE_BUS, config_path_temp_sensor);

  OWDevAddr*tempAddress;
  const float temp_multiplier = 1.0;
  const float temp_offset = 0.0;
  uint8_t count = owSensors->sensors->getDeviceCount();

  for(int i = 0; i < count; i++)
  {
    (new OneWireTemperature(owSensors))
            ->connectTo(new Linear(temp_multiplier, temp_offset, String(config_path_temp_offset) + String(i)))
            ->connectTo(new SKOutputNumber(String(config_path_temp_sk) + String(i), String(config_path_temp_sensor) + String(i)));
    //auto*tempSensor = new OneWireTemperature(owSensors, String(config_path_tempsensor) + String(i));
  }

  // Start the SensESP application running. Because of everything that's been set up above,
  // it constantly monitors the interrupt pin, and every read_delay ms, it sends the 
  // calculated frequency to SignalK.
  sensesp_app->enable();
});