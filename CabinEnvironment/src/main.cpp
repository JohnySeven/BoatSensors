#include <Arduino.h>
#include <EEPROM.h>
#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "sensors/analog_input.h"
#include "transforms/linear.h"
#include "signalk/signalk_output.h"
#include "signalk/signalk_listener.h"
#include "signalk/signalk_value_listener.h"
#include "sensors/digital_output.h"
#include "transforms/threshold.h"
#include "signalk/signalk_output.h"
#include "transforms/lambda_transform.h"
#include "dhtnew.h"

#ifndef OUTPUT_BUFFER_SIZE
#endif

DHTNEW am2301(D4);

// SensESP builds upon the ReactESP framework. Every ReactESP application
// defines an "app" object vs defining a "main()" method.
ReactESP app([]() {

// Some initialization boilerplate when in debug mode...
#ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up
  delay(100);
  Debug.setSerialEnabled(true);
#endif

  // Create the global SensESPApp() object.
  auto *builder = (new SensESPAppBuilder())
                      ->set_standard_sensors(ALL)
                      ->set_hostname("cabinenv")
                      ->set_sk_server("pi.boat", 3000)
                      ->set_wifi("DryII", "wifi4boat")
                      ->set_led_pin(2);

  sensesp_app = builder->get_app();

  am2301.setWaitForReading(true);
  auto result = am2301.read();

  debugI("AM2301 status, Type=%d, Status=%d", result, am2301.getType());

  auto humidity = new LambdaTransform<float, float>([](float input) {
    return am2301.getHumidity();
  });

  auto temperature = new LambdaTransform<float, float>([](float input) {
    return am2301.getTemperature();
  });

  app.onRepeat(2000, [humidity, temperature]() {
    auto result = am2301.read();
    debugI("AM2301 read result=%d", result);
    if (result == DHTLIB_OK)
    {
      humidity->set_input(0.0f, 0);
      temperature->set_input(0.0f, 0);
    }
  });

  humidity->connectTo(new Linear(1.0, 1.0, "/calibration/humidity"))
      ->connectTo(new SKOutputNumber("environment.humidity", "/sk/humidity"));

  temperature->connectTo(new Linear(1.0, 1.0, "/calibration/temperature"))
      ->connectTo(new SKOutputNumber("environment.temperature", "/sk/temperature"));
  /*app.onRepeat(1000, []() {
              debugI("Touch(1) = %d", touchRead(4));
       });*/
  // Start the SensESP application running
  sensesp_app->enable();
});