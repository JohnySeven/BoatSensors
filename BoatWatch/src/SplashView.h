#include "GuiView.h"
#ifndef _SPLASHVIEW_H
#define _SPLASHVIEW_H
class SplashView : public GuiView
{
private:
    String text = "Initializing...";
    bool hasUpdated = true;
public:
    SplashView() { viewName = "Splash"; };
    void UpdateText(String text) { 
        this->text = text;
        hasUpdated = true;
    }
    void Render(TFT_eSPI*display) override;
};


void SplashView::Render(TFT_eSPI*display)
{
    if(hasUpdated)
    {
        debugI("Rending splash!");
        display->textcolor = TFT_WHITE;
        display->println(text);
        hasUpdated = false;
    }
}


#endif