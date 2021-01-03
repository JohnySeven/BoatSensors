#include <Arduino.h>
#include "main.h"

#include "sensesp_app.h"
#include "sensors/analog_input.h"
#include "transforms/linear.h"
#include "signalk/signalk_output.h"
#include "signalk/signalk_listener.h"
#include "signalk/signalk_value_listener.h"
#include "sensors/digital_output.h"
#include "transforms/threshold.h"
#include "signalk/signalk_value_listener.h"
#include "sensesp_app_builder.h"

#include "WatchGui.h"
#include "SplashView.h"
#include "TimeView.h"
#include "WatchHardware.h"
#include "SignalKValueView.h"

WatchHardware *hardware;
WatchGui *gui;
TTGOClass*watch;
SKNumericListener *speedValue;
SKNumericListener *depthValue;
TP_Point touchDownPoint;
bool touched = false;

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
       watch = TTGOClass::getWatch();
       watch->begin();
       gui = new WatchGui(watch);
       hardware = new WatchHardware();
       hardware->Initialize(watch);
       app.onRepeat(250, []() {
              gui->Loop();
       });

       app.onRepeat(50, []() {
              hardware->Loop();
              if (watch->touched())
              {
                     hardware->ResetSleepTimeout();
                     touchDownPoint = watch->touch->getPoint();
                     touched = true;
              }
              else if(touched)
              {
                     touched = false;
                     gui->OnTouched(touchDownPoint.x, touchDownPoint.y);
              }              
       });

       auto *splash = new SplashView();
       gui->Navigate(splash);

       // Create the global SensESPApp() object.
       auto *builder = (new SensESPAppBuilder())
                           ->set_standard_sensors(NONE)
                           ->set_led_blinker(false, 0, 0, 0)
                           ->set_hostname("watch")
                           ->set_sk_server("192.168.89.120", 3000)
                           ->set_wifi("DryII", "wifi4boat");

       sensesp_app = builder->get_app();

       speedValue = new SKNumericListener("navigation.speedOverGround", 2000);
       depthValue = new SKNumericListener("environment.depth.belowTransducer", 2000);       
       // Start the SensESP application running
       sensesp_app->enable();
       auto *watchFace = new TimeView(watch);
       watchFace->AddValue(new SignalKFloatView("Speed", "Km/h", speedValue, 3.6f, 0));
       watchFace->AddValue(new SignalKFloatView("Depth", "m", depthValue, 1));
       gui->Navigate(watchFace);
});