#include <Arduino.h>

#include "sensesp_app.h"
#include "sensesp_app_options.h"
#include "sensors/analog_input.h"
#include "transforms/linear.h"
#include "signalk/signalk_output.h"
#include "signalk/signalk_listener.h"
#include "signalk/signalk_value_listener.h"
#include "sensors/digital_output.h"
#include "BME280.h"
#include "BH1750LightSensor.h"

// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object vs defining a "main()" method.
ReactESP app([] () {

  // Some initialization boilerplate when in debug mode...
  #ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up
  delay(100);
  Debug.setSerialEnabled(true);
  #endif

  // Create the global SensESPApp() object.
  sensesp_app = new SensESPApp([] (SensESPAppOptions*o)
  {
         o->setServerOptions("192.168.89.121", 3000)
          ->setWifiOptions("DryII", "wifi4boat")
          ->setHostName("environ")
          ->setStandardSensors();
  });
  // The "Configuration path" is combined with "/config" to formulate a URL
  // used by the RESTful API for retrieving or setting configuration data.
  // It is ALSO used to specify a path to the SPIFFS file system
  // where configuration data is saved on the MCU board.  It should
  // ALWAYS start with a forward slash if specified.  If left blank,
  // that indicates this sensor or transform does not have any
  // configuration to save.
  // Note that if you want to be able to change the sk_path at runtime,
  // you will need to specify a config_path.

  // Wire up the output of the analog input to the transform,
  // and then output the results on the SignalK network...
  /*pAnalogInput -> connectTo(pTransform)
               -> connectTo(new SKOutputNumber(sk_path));*/

  Wire.begin();

  auto * bme280 = new BME280(0x76, 1000, "");
  bme280->temperature.connectTo(new Linear(1.0, 0, "/sensors/temperature/transform"), 0)
                      ->connectTo(new SKOutputNumber("environment.outside.temperature", "/sensors/temperature/sk"));

  bme280->pressure.connectTo(new Linear(1.0, 0, "/sensors/pressure/transform"), 0)
                      ->connectTo(new SKOutputNumber("environment.outside.pressure", "/sensors/pressure/sk"));

  bme280->humidity.connectTo(new Linear(1.0, 0, "/sensors/humidity/transform"), 0)
                      ->connectTo(new SKOutputNumber("environment.outside.humidity", "/sensors/humidity/sk"));


  //debugD("Scanning I2C...");

  /* for (int address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();

    if (error == 0)
    {
      debugD("I2C device found at address %d", address);
    }
    else if (error == 4)
    {
      debugD("Unknown error at address %d", address);
    }
  } */

  auto * light = new BH1750LightSensor(0x23, 1000, "");  
  light->connectTo(new SKOutputNumber("environment.outside.illuminance", "/sensors/light/sk"));

  auto * uvLight = new AnalogInput();
  auto* pTransform = new Linear(0.015641f, 0.0, "/sensors/light_uv/transform");

  uvLight->connectTo(pTransform)
         ->connectTo(new SKOutputNumber("environment.outside.uvlight", "/sensors/light_uv/sk"));
  
  // Start the SensESP application running
  sensesp_app->enable();
});