#include "TimeView.h"

void TimeView::Render(TFT_eSPI *display)
{
    display->setCursor(0,0);
    auto forceRender = false;
    tick++;
    if(tick > 10)
    {
        tick = 0;
    }

    display->setTextFont(1);
    display->setTextColor(TFT_DARKGREY, TFT_BLACK);
    display->drawNumber(tick, 0, 0);

    if(pendingViewIndex != viewIndex)
    {
        viewIndex = pendingViewIndex;
        display->fillScreen(TFT_BLACK);
        forceRender = true;
        lastHour = 50;
        lastMinute = 50;
    }

    if (viewIndex == -1)
    {
        RenderClockView(display, forceRender);
    }
    else if (viewIndex < signalKViews.size())
    {
        auto * view = signalKViews.at(viewIndex);
        if(forceRender || view->hasChanged())
        {
            RenderValueView(display, view);
        }
    }

    RenderIndicators(display, forceRender);
}

void TimeView::RenderIndicators(TFT_eSPI *display, bool forceRender)
{   
    if(tick == 10 || forceRender)
    {
        auto batteryPercent = pmu->getBattPercentage();
        auto x = TFT_WIDTH - 30;

        if(lastBatteryPercent != batteryPercent || forceRender)
        {
            lastBatteryPercent = batteryPercent;
            
            display->setTextFont(1);
            display->setTextSize(1);
            display->setTextColor(batteryPercent < 30 ? TFT_RED : TFT_WHITE, TFT_BLACK);
            x = x + display->drawNumber(batteryPercent, x, 4);
            display->drawString("%", x, 4);
        }

        x = TFT_WIDTH - 30;
        auto signaKConnected = sensesp_app->isSignalKConnected();
        auto wifiConnected = sensesp_app->isWifiConnected();

            display->setTextFont(1);
            display->setTextSize(1);
            display->setTextColor(signaKConnected ? TFT_WHITE : TFT_RED);
            display->drawString("SK", x - 30, 4);
            display->setTextFont(1);
            display->setTextSize(1);
            display->setTextColor(wifiConnected ? TFT_WHITE : TFT_RED);
            display->drawString("WIFI", x - 70, 4);
    }

    if(forceRender || tick == 10)
    {
        auto count = 1 + signalKViews.size();
        auto y = viewIndex == -1 ? 150 : 220;
        auto x = TFT_WIDTH_HALF - (count * 4);

        for(int i = 0; i < count; i++)
        {
            if(i == viewIndex + 1)
            {
                display->fillRect(x, y, 5, 5, TFT_WHITE);
            }
            else
            {
                display->drawRect(x, y, 5, 5, TFT_WHITE);
            }
            x+=8;
        }
        
    }
}

void TimeView::RenderValueView(TFT_eSPI *display, SignalKValue *value)
{
    auto title = value->getTitle();
    auto text = sensesp_app->isSignalKConnected() ? value->getText() : "--";
    auto unit = value->getUnit();
    short y = 30;
    display->setTextColor(TFT_WHITE, TFT_BLACK);
    display->setCursor(0,0);
    display->setTextFont(4);
    display->setTextSize(1);
    auto height = display->fontHeight();
    auto width = display->textWidth(title);
    display->drawString(title, TFT_WIDTH_HALF - (width / 2), y);
    y += height + 10;
    display->setTextFont(6);
    display->setTextSize(2);
    width = display->textWidth(text);
    height = display->fontHeight();
    display->drawString(text, TFT_WIDTH_HALF - (width / 2) , y);
    y += height;
    display->setTextSize(1);
    display->setTextFont(4);
    width = display->textWidth(unit);
    display->drawString(unit, TFT_WIDTH_HALF - (width / 2), y);
}

void TimeView::RenderClockView(TFT_eSPI *display, bool forceRender)
{
    display->setTextSize(1);
    display->setCursor(0,0);    
    
    auto *rtc = watch->rtc;
    //byte xpos = 0; // Stating position for the display
    //byte ypos = 90;
    // Get the current data
    RTC_Date tnow = rtc->getDateTime();

    auto hh = tnow.hour;
    auto mm = tnow.minute;
    auto ss = tnow.second;
    auto dday = tnow.day;
    auto mmonth = tnow.month;
    auto yyear = tnow.year;

    //debugI("LastHour=%d,Hour=%d,LastMinute=%d,Minute=%d,", lastHour, hh, lastMinute, mm);

    if(lastHour == hh && lastMinute == mm && !forceRender)
    {
        return;
    }

    lastHour = hh;
    lastMinute = mm;

    char timeString[10];

    sprintf(timeString, "%02d:%02d", hh, mm);
    

    if (ss % 2 == 0 || forceRender)
    {
        display->setTextSize(2);
        display->setTextColor(TFT_WHITE, TFT_BLACK); // Orange

        display->setTextFont(4);
        auto width = display->textWidth(timeString);
        auto height = display->fontHeight();
        display->drawString(timeString, TFT_WIDTH_HALF - (width / 2), TFT_HEIGHT_HALF - (height /2));

        display->setTextSize(1);
        display->setTextFont(4);
        display->setCursor(10, 210);

        display->print(mmonth);
        display->print("/");
        display->print(dday);
        display->print("/");
        display->print(yyear);
    }
}