#include "GuiView.h"
#include "main.h"
#include "time.h"
#include "vector"
#include "SignalKValueView.h"
#include "sensesp_app.h"
#include "signalk/signalk_value_listener.h"

#define TFT_WIDTH_HALF TFT_WIDTH / 2
#define TFT_HEIGHT_HALF TFT_HEIGHT / 2


class TimeView : public GuiView
{
public:
  TimeView(TTGOClass *watch) { viewName = "Time"; this->watch = watch; this->pmu = watch->power; };
  void Render(TFT_eSPI *display) override;
  void AddValue(SignalKValue* value) { signalKViews.push_back(value); }
  SignalKValue* First() { return signalKViews.front();}
  void OnTouched(int16_t x, int16_t y) override
  {
    auto index = pendingViewIndex+1;
    if(index >= signalKViews.size())
    {
      index = -1;
    }
    pendingViewIndex = index;
  }
private:
  TTGOClass *watch;
  AXP20X_Class *pmu;
  byte xcolon = 0;
  byte lastHour = 50;
  byte lastMinute = 50;
  int lastBatteryPercent = 0;
  byte tick = 0;
  int8_t viewIndex = -1;
  int8_t pendingViewIndex = -1;

  std::vector<SignalKValue*> signalKViews;
  void RenderClockView(TFT_eSPI *display, bool forceRender);
  void RenderValueView(TFT_eSPI *display, SignalKValue*value);
  void RenderIndicators(TFT_eSPI *display,bool forceRender);
};