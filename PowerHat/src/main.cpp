#include <Arduino.h>
#include <SimpleCLI.h>
#include "main.h"
#include "Wire.h"
#include <PCF8523.h>
#include "EEPROM.h"

#define MAX_LINE_SIZE 128
#define BUTTON_PIN 3
#define SYSTEM_UP_PIN 7
//#define SENSE_PIN 2
#define RELAY_PIN 16
#define RTC_ALARM 2
#define SYSTEM_SHUT_DOWN_PIN 17
#define POWER_UP_DELAY 6000
#define POWER_DOWN_DELAY 30000
#define VOLTAGE A6 // I/P|______|A/I|______|Supply monitoring pin----------------------------______|ADC Pin
#define CURRENT A7 // I/P|______|A/I|______|Current monitoring pin---------------------------______|ADC Pin
#define SLEEP_AFTER_LOW 6000

char line[MAX_LINE_SIZE];
int lineIndex = 0;
unsigned long stateChangeAt = 0;
bool stateChangePending = false;
int newRelayState = LOW;
int ledState = LOW;
int relayState = LOW;
volatile int onOffState = HIGH;
volatile int onOffCounter = 0;
volatile unsigned long onOffDebounce = 0;
unsigned long lastStateChange = 0;
uint8_t wakeUpHour = 9;
uint8_t wakeUpMinute = 0;
SimpleCLI cli;
PCF8523 rtc;

void HelpCommand(cmd *command)
{
    Serial.println("RPI Low Power commands:");
    Serial.println("    status - shows current state (adc, counters, gpio)");
    Serial.println("    uptime - shows CPU uptime in ms");
    Serial.println("    now - shows RTC time");
    Serial.println("    settime -y xxxx -m xx -d xx -h xx -mi xx -s xx");
    Serial.println("    setwake -h xx -m xx");
    Serial.println("OK");
}

void SetTime(cmd *cmd)
{
    Command command = Command(cmd);
    int year = command.getArg("y").getValue().toInt();
    int month = command.getArg("m").getValue().toInt();
    int day = command.getArg("d").getValue().toInt();
    int hour = command.getArg("h").getValue().toInt();
    int minutes = command.getArg("mi").getValue().toInt();
    int seconds = command.getArg("s").getValue().toInt();

    DateTime newTime = DateTime((uint16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)hour, (uint8_t)minutes, (uint8_t)seconds);

    rtc.setTime(newTime);

    Serial.println("OK");
}

void SetWakeUpTime(cmd *cmd)
{
    Command command = Command(cmd);
    int hour = command.getArg("h").getValue().toInt();
    int minutes = command.getArg("m").getValue().toInt();

    if (hour >= 0 && hour <= 23 && minutes >= 0 && minutes <= 59)
    {
        EEPROM[0] = 'A';
        EEPROM[1] = (uint8_t)hour;
        EEPROM[2] = (uint8_t)minutes;
        wakeUpHour = hour;
        wakeUpMinute = minutes;
        Serial.println("OK");
    }
    else
    {
        Serial.println("ERROR");
    }
}

void PrintRTCInfo(cmd *cmd)
{
    //This message is just for debug
    uint8_t reg_value;
    reg_value = rtc.rtcReadReg(PCF8523_CONTROL_1);
    Serial.print("Control 1: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_CONTROL_2);
    Serial.print("Control 2: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_CONTROL_3);
    Serial.print("Control 3: 0x");
    Serial.println(reg_value, HEX);

    reg_value = rtc.rtcReadReg(PCF8523_SECONDS);
    Serial.print("Seconds: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_MINUTES);
    Serial.print("Minutes: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_HOURS);
    Serial.print("Hours: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_DAYS);
    Serial.print("Days: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_WEEKDAYS);
    Serial.print("Week Days: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_MONTHS);
    Serial.print("Months: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_YEARS);
    Serial.print("Years: ");
    Serial.println(reg_value, HEX);

    reg_value = rtc.rtcReadReg(PCF8523_MINUTE_ALARM);
    Serial.print("Minute Alarm: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_HOUR_ALARM);
    Serial.print("Hour Alarm: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_DAY_ALARM);
    Serial.print("Day Alarm: ");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_WEEKDAY_ALARM);
    Serial.print("Weekday Alarm: ");
    Serial.println(reg_value, HEX);

    reg_value = rtc.rtcReadReg(PCF8523_OFFSET);
    Serial.print("Offset: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_TMR_CLKOUT_CTRL);
    Serial.print("TMR_CLKOUT_CTRL: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_TMR_A_FREQ_CTRL);
    Serial.print("TMR_A_FREQ_CTRL: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_TMR_A_REG);
    Serial.print("TMR_A_REG: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_TMR_B_FREQ_CTRL);
    Serial.print("TMR_B_FREQ_CTRL: 0x");
    Serial.println(reg_value, HEX);
    reg_value = rtc.rtcReadReg(PCF8523_TMR_B_REG);
    Serial.print("TMR_B_REG: 0x");
    Serial.println(reg_value, HEX);
}

void UptimeCommand(cmd *command)
{
    Serial.println(millis());
    Serial.println("OK");
}

float GetVin()
{
    int tempVoltage;
    float powerVoltage;

    tempVoltage = analogRead(VOLTAGE);

    // 3.3V/1024 = 3.22mV
    powerVoltage = 3.22 * (float)tempVoltage;
    // covert it to real voltage
    powerVoltage /= 52;

    return powerVoltage;
}

float getCurrent(void)
{
    int tempCurrent;
    float powerCurrent;

    tempCurrent = analogRead(CURRENT);
    //filter the noise
    if (tempCurrent <= 3)
    {
        tempCurrent = 0;
    }

    //3.3V/1024 = 3.22mV
    powerCurrent = 3.22 * (float)tempCurrent;

    return powerCurrent;
}

void StatusCommand(cmd *command)
{
    Serial.print("vin=");
    Serial.print(GetVin());
    Serial.print("V,current=");
    Serial.print(getCurrent());
    Serial.print("mA,systemup=");
    Serial.print(digitalRead(SYSTEM_UP_PIN));
    Serial.print(",onoff=");
    Serial.print(onOffState);
    Serial.print(",onOffTime=");
    Serial.print(onOffDebounce);
    Serial.print(",power=");
    Serial.print(relayState);
    Serial.print(",rtc=");
    Serial.print(digitalRead(RTC_ALARM));
    Serial.print("wakeupat=");
    Serial.print(wakeUpHour);
    Serial.print(":");
    Serial.println(wakeUpMinute);

    if (stateChangePending)
    {
        Serial.print("Pending state: ");
        Serial.print(newRelayState);
        Serial.print(" in ");
        Serial.print((stateChangeAt - millis()));
        Serial.println(" ms");
    }
    Serial.println("OK");
}

void PrintRtcTime(cmd *cmd)
{
    DateTime now = rtc.readTime();

    Serial.print(now.day());
    Serial.print(".");
    Serial.print(now.month());
    Serial.print(".");
    Serial.print(now.year());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.print(now.second());
    Serial.println();
}

void ButtonInterrupt()
{
    if (onOffDebounce < millis())
    {
        onOffCounter = onOffCounter + 1;
        onOffDebounce = millis() + 500;
    }
}

void PrintVersion(cmd *cmd)
{
    Serial.print("Build date=");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    Serial.println("OK");
}

void RTCAlarm()
{
    onOffState = HIGH;
}

uint8_t _resetFlags __attribute__ ((section(".noinit")));
void resetFlagsInit(void) __attribute__ ((naked)) __attribute__ ((section (".init0")));
void resetFlagsInit(void)
{
  // save the reset flags passed from the bootloader
  __asm__ __volatile__ ("mov %0, r2\n" : "=r" (_resetFlags) :);
}

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.println("Starting Power HAT...");

    uint8_t _mcusr = MCUSR;
    Serial.print("Reset flags / MCUSR=");
    Serial.print(_resetFlags, HEX);
    Serial.print(" / ");
    Serial.println(_mcusr);
    Serial.flush();
    //Clear register
    MCUSR = 0x00;

    if(_resetFlags & BORF)
    {
        Serial.println("Power issue reset!");
    }

    if(_resetFlags & EXTRF)
    {
        Serial.println("External reset");
    }

    if(_resetFlags & WDRF)
    {
        Serial.println("WDT reset!");
    }

    if(_resetFlags & PORF)
    {
        Serial.println("Normal reset");
    }

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SYSTEM_UP_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(RTC_ALARM, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SYSTEM_SHUT_DOWN_PIN, OUTPUT);
    digitalWrite(SYSTEM_SHUT_DOWN_PIN, LOW);

    if (EEPROM[0] == 'A')
    {
        wakeUpHour = EEPROM[1];
        wakeUpMinute = EEPROM[2];
    }

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonInterrupt, FALLING);

    cli.addCommand("ver", PrintVersion);
    cli.addCommand("help", HelpCommand);
    cli.addCommand("uptime", UptimeCommand);
    cli.addCommand("status", StatusCommand);
    cli.addCommand("now", PrintRtcTime);
    cli.addCommand("rtc", PrintRTCInfo);
    Command setTimeCommand = cli.addCommand("settime", SetTime);
    setTimeCommand.addArg("y");
    setTimeCommand.addArg("m");
    setTimeCommand.addArg("d");
    setTimeCommand.addArg("h");
    setTimeCommand.addArg("mi");
    setTimeCommand.addArg("s");
    Command wakeup = cli.addCommand("setwake", SetWakeUpTime);
    wakeup.addArgument("h");
    wakeup.addArgument("m");

    rtc.begin();
    rtc.ackAlarm();
    delay(2500);
    Serial.println("HAT ready!");
}

void SleepUntilWakeUp()
{
    Serial.println("Entering sleep mode...");
    attachInterrupt(digitalPinToInterrupt(RTC_ALARM), RTCAlarm, FALLING);
    rtc.setAlarm(wakeUpHour, wakeUpMinute);
    rtc.enableAlarm(true);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    Serial.println("Woken up from sleep mode.");
    detachInterrupt(digitalPinToInterrupt(RTC_ALARM));
    rtc.ackAlarm();
}

void loop()
{
    while (Serial.available())
    {
        auto ch = Serial.read();

        if (ch == 225)
        {
            continue;
        }

        if (lineIndex < MAX_LINE_SIZE)
        {
            if (ch == '\n' || ch == '\r')
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
        }
        else
        {
            lineIndex = 0;
        }
    }

    int systemUp = digitalRead(SYSTEM_UP_PIN);
    //PinStatus sense = LOW;//digitalRead(SENSE_PIN);
    //int switchState = digitalRead(SENSE_PIN);
    int state = LOW;

    if (onOffCounter > 0)
    {
        Serial.println("Button action");
        onOffState = onOffState == HIGH ? LOW : HIGH;
        onOffCounter = 0;
    }

    if ((systemUp == HIGH) || (onOffState == HIGH))
    {
        state = HIGH;
    }

    if (stateChangePending && stateChangeAt < millis())
    {
        if (newRelayState == LOW && systemUp == HIGH)
        {
            Serial.println("Aborted LIVE system shutdown!");
        }
        else
        {
            Serial.print("Relay=");
            Serial.println(newRelayState);
            digitalWrite(RELAY_PIN, newRelayState);
            relayState = newRelayState;
            lastStateChange = millis();
        }

        stateChangePending = false;
    }
    else if (!stateChangePending)
    {
        if (relayState != state)
        {
            if (state == HIGH)
            {
                Serial.println("POWERING UP...");
                stateChangeAt = millis() + POWER_UP_DELAY;
                digitalWrite(SYSTEM_SHUT_DOWN_PIN, LOW);
            }
            else
            {
                digitalWrite(SYSTEM_SHUT_DOWN_PIN, HIGH);
                Serial.println("POWERING DOWN...");
                stateChangeAt = millis() + POWER_DOWN_DELAY;
            }
            newRelayState = state;
            stateChangePending = true;
        }
        else
        {
            digitalWrite(LED_BUILTIN, state == 0 ? 1 : 0);
            digitalWrite(SYSTEM_SHUT_DOWN_PIN, LOW);

            if (state == 0 && lastStateChange + 6000 < millis())
            {
                SleepUntilWakeUp();
                lastStateChange = millis();
            }
        }
    }
    else if (stateChangePending)
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(150);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(150);
    }
}