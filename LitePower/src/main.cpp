#include <Arduino.h>
#include <SimpleCLI.h>
#include "main.h"

#define MAX_LINE_SIZE 128
#define SWITCH_PIN 12
#define SYSTEM_UP_PIN 11
#define SENSE_PIN 5
#define RELAY_PIN 2
#define POWER_UP_DELAY 6000
#define POWER_DOWN_DELAY 30000

char line[MAX_LINE_SIZE];
int lineIndex = 0;
unsigned long stateChangeAt = 0;
bool stateChangePending = false;
PinStatus newRelayState = LOW;
PinStatus ledState = LOW;
PinStatus relayState = LOW;
SimpleCLI cli;

void HelpCommand(cmd* command)
{
    Serial.println("RPI Low Power commands:");
    Serial.println("    status - shows current state (adc, counters, gpio)");
    Serial.println("    uptime - shows CPU uptime in ms");
    Serial.println("OK");
}

void UptimeCommand(cmd* command)
{
    Serial.println(millis());
    Serial.println("OK");
}

float GetVin()
{
    int vinAnalog = analogRead(A7);
    float vin = vinAnalog / 48.0f;
    return vin;
}

void StatusCommand(cmd* command)
{
    int vinAnalog = analogRead(A7);
    Serial.print("vin=");
    Serial.print(GetVin());
    Serial.print(", digital=");
    Serial.println(vinAnalog);
    Serial.print("SystemUp=");
    Serial.println(digitalRead(SYSTEM_UP_PIN));
    Serial.print("Switch=");
    Serial.println(digitalRead(SWITCH_PIN));
    Serial.print("Relay=");
    Serial.println(relayState);
    Serial.print("Sense=");
    Serial.println(digitalRead(SENSE_PIN));
    if(stateChangePending)
    {
        Serial.print("Pending state: ");
        Serial.print(newRelayState);
        Serial.print(" in ");
        Serial.print((stateChangeAt - millis()));
        Serial.println(" ms");
    }
    Serial.println("OK");
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SYSTEM_UP_PIN, INPUT_PULLDOWN);
    pinMode(SWITCH_PIN, INPUT_PULLDOWN);
    pinMode(SENSE_PIN, INPUT_PULLDOWN);
    pinMode(RELAY_PIN, OUTPUT);
    
    cli.addCommand("help", HelpCommand);
    cli.addCommand("uptime", UptimeCommand);
    cli.addCommand("status", StatusCommand);

    delay(2500);
}

void loop()
{
    while(Serial.available())
    { 
        auto ch = Serial.read();

        if(lineIndex < MAX_LINE_SIZE)
        {
            if(ch == '\n' || ch == '\r')
            {
                line[lineIndex++] = '\0';

                String input = String(line);
                Serial.print("# ");
                Serial.println(input);
                // Parse the user input into the CLI
                cli.parse(input);
                lineIndex = 0;
            }
            else
            {
                line[lineIndex++] = ch;
            }

            Serial.write(ch);
        }
        else
        {
            lineIndex = 0;
        }        
    }

    PinStatus systemUp = digitalRead(SYSTEM_UP_PIN);
    //PinStatus sense = LOW;//digitalRead(SENSE_PIN);
    PinStatus switchState = digitalRead(SWITCH_PIN);
    PinStatus state = LOW;
    
    if((systemUp == HIGH) || (switchState == HIGH))
    {
        state = HIGH;
    }

    if(stateChangePending && stateChangeAt < millis())
    {
        if(newRelayState == LOW && systemUp == HIGH)
        {
            Serial.println("Aborted LIVE system shutdown!");
        }
        else
        {
            digitalWrite(RELAY_PIN, newRelayState);
            digitalWrite(LED_BUILTIN, newRelayState);
            relayState = newRelayState;
        }

        stateChangePending = false;
    }
    else if(!stateChangePending)
    {    
        if(relayState != state)
        {
            if(state == HIGH)
            {
                Serial.println("POWERING UP...");
                stateChangeAt = millis() + POWER_UP_DELAY; 
            }
            else
            {
                Serial.println("POWERING DOWN...");
                stateChangeAt = millis() + POWER_DOWN_DELAY;
            }            
            newRelayState = state;
            stateChangePending = true;
        }
    }
    else if(stateChangePending)
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(150);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(150);
    }


}