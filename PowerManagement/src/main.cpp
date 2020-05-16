#include <Arduino.h>
#include <SimpleCLI.h>
#include "main.h"
#define ITEMS_COUNT 64
#define MAX_LINE_SIZE 128
#define PIN_COUNT 4
#define ev 12
#define FIRST_POWER_UP_TIMEOUT 6000

char line[MAX_LINE_SIZE];
int lineIndex = 0;
SimpleCLI cli;
PowerTaskItem tasks[ITEMS_COUNT];
EventItem events[ITEMS_COUNT];
PinDefinition pins[PIN_COUNT];
PinStatus buttonStatus;
unsigned long lastButtonChange = 0;

void HelpCommand(cmd* command)
{
    Serial1.println("Boat power managment commands:");
    Serial1.println("    power -pin {pin} -state {\"h\"|\"l\"} -delay {delay} - enques power action, delay is in miliseconds.");
    Serial1.println("    queue - list current queue items");
    Serial1.println("    clear - clears requests queue");
    Serial1.println("    events - lists all events");
    Serial1.println("    adc {pin}");
    Serial1.println("    pins - list all available output pins");
    Serial1.println("    uptime - shows CPU uptime in ms");
}

void InitPins()
{
    for(int i = 0; i < 4; i++)
    {
        auto pin = i+2;
        pins[i].autoUp = i == 0;
        pins[i].pin = pin;
        pins[i].lastState = LOW;
        pinMode(pin, OUTPUT);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    buttonStatus = HIGH;
}

int FindDoneItem()
{
    int index = -1;

    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        if(tasks[i].done == true)
        {
            index = i;
            break;
        }
    }

    return index;
}

void AdcCommand(cmd*command)
{
    Command cmd(command); // Create wrapper object
    auto pinArg = cmd.getArgument("pin").getValue().toInt();

    auto value = analogRead(pinArg);

    Serial1.println(value);
    Serial1.println("Done");
}

int AddPowerItem(int pin, PinStatus state, unsigned long delay)
{
    auto doneIndex = FindDoneItem();
    if(doneIndex != -1)
    {        
        tasks[doneIndex].delay = millis() + delay;
        tasks[doneIndex].pin = pin;
        tasks[doneIndex].done = false;
        tasks[doneIndex].state = state;
    }

    return doneIndex;
}

void UptimeCommand(cmd*command)
{
    Serial1.println(millis());
    Serial1.println("OK");
}

void PowerCommand(cmd*command)
{
    Command cmd(command); // Create wrapper object

    auto pinArg = cmd.getArgument("pin").getValue().toInt();
    auto stateArg = cmd.getArgument("state").getValue();
    auto delayArg = cmd.getArgument("delay").getValue().toInt();

    if(pinArg >= 0 && pinArg < PIN_COUNT)
    {
        PinStatus status;
        if(stateArg == "h" || stateArg == "high")
        {
            status = HIGH;   
        }
        else
        {
            status = LOW;
        }

        if(AddPowerItem(pinArg, status, delayArg) != -1)
        {
            Serial1.println("OK");
        }
        else
        {
            Serial1.println("ERROR: queue full!");
        }
        
    }
    else
    {
        Serial1.println("ERROR: Invalid pin!");
    }
    

}

void ClearQueue(cmd * command)
{
    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        tasks[i].done = true;
    }

    Serial1.println("Done");
}

void PrintQueue(cmd*command)
{
    auto time = millis();
    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        if(!tasks[i].done)
        {
            Serial1.print(i);
            Serial1.print(": pin=");
            Serial1.print(tasks[i].pin);
            Serial1.print(",state=");
            Serial1.print(tasks[i].state);
            Serial1.print(",in=");
            Serial1.print(tasks[i].delay - time);
            Serial1.println(" ms");
        }
    }

    Serial1.println("Done");
}

void PrintEvents(cmd*command)
{
    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        if(events[i].isSet)
        {
            Serial1.print("event=");
            Serial1.print(events[i].code);
            Serial1.print(",argument=");
            Serial1.print(events[i].argument);
            Serial1.print(",time=");
            Serial1.println(events[i].time);
            events[i].isSet = false;
            events[i].code = 0;
            events[i].argument = 0;
        }
    }

    Serial1.println("Done");    
}

void AddEvent(int code, int argument = 0)
{
    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        if(!events[i].isSet)
        {
            events[i].code = code;
            events[i].argument = argument;
            events[i].time = millis();
            events[i].isSet = true;
            break;
        }
    }
}

void PinsCommand(cmd*cmd)
{
    for (int i = 0; i < PIN_COUNT; i++)
    {
        Serial1.print("pin=");
        Serial1.print(pins[i].pin);
        Serial1.print(",state=");
        Serial1.print(pins[i].lastState);
        Serial1.println();
    }

    Serial1.println("OK");    
}

void errorCallback(cmd_error* e) {
    CommandError cmdError(e); // Create wrapper object

    Serial1.print("ERROR: ");
    Serial1.println(cmdError.toString());
}

void setup()
{
    Serial1.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    lastButtonChange = millis();
    InitPins();

    digitalWrite(LED_BUILTIN, HIGH);

    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        tasks[i].done = true;
        tasks[i].delay = 0;
        tasks[i].pin = 0;
        tasks[i].state = LOW;

        events[i].isSet = false;
        events[i].time = 0;
        events[i].code = 0;
        events[i].argument = 0;
    }    

    cli.addBoundlessCmd("help", HelpCommand);

    auto powerCommand = cli.addCmd("power", PowerCommand);
    cli.addCmd("queue", PrintQueue);

    powerCommand.addArgument("pin");
    powerCommand.addArgument("state");
    powerCommand.addArgument("delay");

    auto clearCommand = cli.addCmd("clear", ClearQueue);

    auto eventsCommands = cli.addCmd("events", PrintEvents);

    auto adcCommand = cli.addCmd("adc", AdcCommand);
    adcCommand.addPosArg("pin");

    auto pinsCommand = cli.addCmd("pins", PinsCommand);

    cli.setCaseSensetive(false);
    cli.setErrorCallback(errorCallback);

    cli.addCmd("uptime", UptimeCommand);

    Serial1.println("OK: Boat power managment ready. V1");
    AddEvent(STARTED_EVENT, 1);

    digitalWrite(LED_BUILTIN, LOW);

    for (int i = 0; i < PIN_COUNT; i++)
    {
        if(pins[i].autoUp)
        {
            AddPowerItem(i, HIGH, FIRST_POWER_UP_TIMEOUT);
        }
    }
    
}

void loop()
{
    while(Serial1.available())
    { 
        auto ch = Serial1.read();

        if(lineIndex < MAX_LINE_SIZE)
        {
            if(ch == '\n' || ch == '\r')
            {
                line[lineIndex++] = '\0';

                String input = String(line);
                Serial1.print("# ");
                Serial1.println(input);
                // Parse the user input into the CLI
                cli.parse(input);
                lineIndex = 0;
            }
            else
            {
                line[lineIndex++] = ch;
            }
        }
    }

    auto button = digitalRead(BUTTON_PIN);

    if(button != buttonStatus && (millis() - lastButtonChange) > 1000)
    {
        buttonStatus = button;
        lastButtonChange = millis();

        if(button == LOW)
        {
            if(pins[0].lastState == LOW)
            {
                AddPowerItem(0, HIGH, FIRST_POWER_UP_TIMEOUT);
            }

            AddEvent(PIN_HIGH_EVENT, BUTTON_PIN);
        }
        else
        {
            AddEvent(PIN_LOW_EVENT, BUTTON_PIN);
        }
    }

    for (int i = 0; i < ITEMS_COUNT; i++)
    {
        if(!tasks[i].done && tasks[i].delay < millis())
        {
            auto pin = pins[tasks[i].pin].pin;
            digitalWrite(pin, tasks[i].state);
            pins[tasks[i].pin].lastState = tasks[i].state;
            tasks[i].done = true;
        }
    }    
}