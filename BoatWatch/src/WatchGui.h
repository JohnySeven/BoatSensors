#include "main.h"
#include "sensesp_app.h"
#include "GuiView.h"

#ifndef _WATCHGUI_H
#define _WATCHGUI_H

class WatchGui
{
    public:
        WatchGui(TTGOClass* watch);
        void Loop();
        void Navigate(GuiView*view);
        void OnTouched(int16_t x, int16_t y);

    private:
        TTGOClass* watch;
        TFT_eSPI* tft;
        GuiView*currentView = NULL;
        bool clearView = false;
        bool tick = false;
};
#endif