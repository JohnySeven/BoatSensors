#include "WatchGui.h"
#include "main.h"

WatchGui::WatchGui(TTGOClass* watch)
{
    auto navigate_cb = [this](GuiView* view)
    {
        Navigate(view);
    };

    debugI("Display size=%dx%d", TFT_WIDTH, TFT_HEIGHT);

    navigate = navigate_cb;
    watch->openBL();
    watch = watch;
    tft = watch->tft;
}

void WatchGui::Loop()
{
    if(clearView)
    {
        debugI("Blanking screen.");
        tft->fillScreen(TFT_BLACK);
        clearView = false;
    }

    if(currentView != NULL)
    {
        currentView->Render(this->tft);
    }
}

void WatchGui::OnTouched(int16_t x, int16_t y)
{
    if(currentView != NULL)
    {
        currentView->OnTouched(x,y);
    }
}

void WatchGui::Navigate(GuiView*view)
{
    if(currentView != NULL)
    {
        clearView = true;
    }
    
    currentView = view;
}