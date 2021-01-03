#ifndef _SIGNALKVALUEVIEW_H
#define _SIGNALKVALUEVIEW_H
#include "signalk/signalk_value_listener.h"

class SignalKValue
{
    public: 
        String getText() { return text; }
        String getTitle() { return title; }
        String getUnit() { return unit; }
        bool hasChanged() { return changed; }
    protected:
        String title = String();
        String text = String("--");
        String unit = String();
        bool changed = true;
};

template <class T>
class SignalKValueViewBase : public SignalKValue
{
    protected:
        SKValueListener<T>* valueListener = NULL;
        T value;
    public:
        SignalKValueViewBase(String title, String unit, SKValueListener<T>* listener)
        {
            this->title = title;
            this->unit = unit;
            valueListener = listener;
        }
};

class SignalKIntView : public SignalKValueViewBase<int>
{
private:
    int multiplyFactor;
public:
    SignalKIntView(String title, String unit, SKIntListener* intListener, int multiply = 1) : SignalKValueViewBase(title,unit,intListener)
    {
        value = 0;
        text = String("--");
        multiplyFactor = multiply;

        intListener->attach([this,intListener]()
        {
            value = intListener->get() * multiplyFactor;
            auto newText = String(value);
            if(newText != text)
            {
                text = newText;
                this->changed = true;
            }
        });
    }
};

class SignalKFloatView : public SignalKValueViewBase<float>
{
private:
    float multiplyFactor;
    uint8_t decimalPlaces;
public:
    SignalKFloatView(String title, String unit, SKNumericListener* floatListener, float multiply = 1.0f, uint8_t decimalPlaces = 1) : SignalKValueViewBase(title,unit,floatListener)
    {
        value = 0.0f;
        text = String("--");
        multiplyFactor = multiply;
        this->decimalPlaces = decimalPlaces;

        floatListener->attach([this,floatListener]()
        {
            value = floatListener->get() * multiplyFactor;
            auto newText = String(value, this->decimalPlaces);
            if(newText != text)
            {
                text = newText;
                this->changed = true;
            }
        });
    }
};

class SignalKStringView : public SignalKValueViewBase<String>
{
public:
    SignalKStringView(String title, String unit, SKStringListener* stringListener) : SignalKValueViewBase(title,unit,stringListener)
    {
        value = "--";
        text = value;

        stringListener->attach([this,stringListener]()
        {
            value = stringListener->get();
            if(text != value)
            {
                text = value;
                this->changed = true;
            }
        });
    }
};



#endif