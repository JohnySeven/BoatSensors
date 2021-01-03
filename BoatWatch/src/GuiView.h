#ifndef _GUIVIEW_H
#define _GUIVIEW_H
#include "main.h"

class GuiView
{
public:
    GuiView() { };
    virtual void Render(TFT_eSPI*display);
    virtual void OnTouched(int16_t x, int16_t y) { };
private:
    TFT_eSPI* display;
protected:
    void navigate(GuiView*view) { navigate(view);};
    const char * viewName = "GenericView";
    /* data */
};

static std::function<void(GuiView*)> navigate;
#endif