#include "WatchHardware.h"
#include "WiFi.h"
#include "Images/skicon.h"


void WatchHardware::Initialize(TTGOClass *watch)
{
    this->watch = watch;
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    g_event_group = xEventGroupCreate();
    isr_group = xEventGroupCreate();

    InitPMU();
    InitAcc();
    watch->motor_begin();
    watch->motor->onec();

    //clear wrong date
    watch->rtc->check();
    //Synchronize time to system time
    watch->rtc->syncToSystem();
    watch->tft->setTextFont(2);
    watch->tft->setCursor(0, 0);
    watch->tft->println("Initializing...");
    watch->tft->drawXBitmap((TFT_WIDTH / 2) - (skIcon_width / 2), (TFT_HEIGHT / 2) - (skIcon_height / 2), skIcon_bits, skIcon_width, skIcon_height, TFT_WHITE);

    uptime = millis();
}

void WatchHardware::Loop()
{
    bool rlst;
    uint8_t data;
    //! Fast response wake-up interrupt
    EventBits_t bits = xEventGroupGetBits(isr_group);
    if (bits & WATCH_FLAG_SLEEP_EXIT)
    {
        if (sleepMode)
        {
            sleepMode = false;
            // rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);
            setCpuFrequencyMhz(160);
        }

        LowPower();

        if (bits & WATCH_FLAG_BMA_IRQ)
        {
            do
            {
                rlst = watch->bma->readInterrupt();
            } while (!rlst);
            xEventGroupClearBits(isr_group, WATCH_FLAG_BMA_IRQ);
        }
        if (bits & WATCH_FLAG_AXP_IRQ)
        {
            watch->power->readIRQ();
            watch->power->clearIRQ();
            //TODO: Only accept axp power pek key short press
            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
        }
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_MODE);

        log_i("Woken up from sleep");
        ResetSleepTimeout();
    }
    if ((bits & WATCH_FLAG_SLEEP_MODE))
    {
        //! No event processing after entering the information screen
        return;
    }

    //! Normal polling
    if (xQueueReceive(g_event_queue_handle, &data, 5 / portTICK_RATE_MS) == pdPASS)
    {
        switch (data)
        {
        case Q_EVENT_BMA_INT:
            do
            {
                rlst = watch->bma->readInterrupt();
            } while (!rlst);

            break;
        case Q_EVENT_AXP_INT:
            watch->power->readIRQ();
            if (watch->power->isVbusPlugInIRQ())
            {
                //updateBatteryIcon(LV_ICON_CHARGE);
                log_i("Power charging...");
            }
            if (watch->power->isVbusRemoveIRQ())
            {
                log_i("Power not charging...");
                //updateBatteryIcon(LV_ICON_CALCULATION);
            }
            if (watch->power->isChargingDoneIRQ())
            {
                log_i("Battery full!");
                //updateBatteryIcon(LV_ICON_CALCULATION);
            }
            if (watch->power->isPEKShortPressIRQ())
            {
                watch->power->clearIRQ();
                log_i("Button press!");
                LowPower();
                return;
            }
            watch->power->clearIRQ();
            break;
        /*case Q_EVENT_WIFI_SCAN_DONE: {
            int16_t len =  WiFi.scanComplete();
            for (int i = 0; i < len; ++i) {
                wifi_list_add(WiFi.SSID(i).c_str());
            }
            break;
        }*/
        default:
            break;
        }
    }
    
    auto timeout = (millis() - uptime);
    if (timeout > DEFAULT_SCREEN_TIMEOUT) {
        log_i("Activity timeout, entering sleep mode!");
        LowPower();
    }
}

void AXP202Interupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
    if (bits & WATCH_FLAG_SLEEP_MODE)
    {
        //! For quick wake up, use the group flag
        xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_AXP_IRQ, &xHigherPriorityTaskWoken);
    }
    else
    {
        uint8_t data = Q_EVENT_AXP_INT;
        xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void WatchHardware::InitPMU()
{
    // Turn on the IRQ used
    watch->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    watch->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    watch->power->clearIRQ();

    // Turn off unused power
    watch->power->setPowerOutPut(AXP202_EXTEN, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_DCDC2, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
    watch->power->setPowerOutPut(AXP202_LDO4, AXP202_OFF);

    pinMode(AXP202_INT, INPUT);
    attachInterrupt(AXP202_INT, AXP202Interupt, FALLING);
}

void BMA423Interupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventBits_t bits = xEventGroupGetBitsFromISR(isr_group);
    if (bits & WATCH_FLAG_SLEEP_MODE)
    {
        //! For quick wake up, use the group flag
        xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
    }
    else
    {
        uint8_t data = Q_EVENT_BMA_INT;
        xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void WatchHardware::InitAcc()
{
    auto *sensor = watch->bma;

    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor->accelConfig(cfg);
    sensor->enableAccel();

    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, BMA423Interupt, RISING);
    sensor->enableFeature(BMA423_TILT, true);
    sensor->enableTiltInterrupt();
    sensor->enableFeature(BMA423_WAKEUP, true);
    sensor->enableWakeupInterrupt();
}

void WatchHardware::LowPower()
{
    if (watch->bl->isOn())
    {
        xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        watch->closeBL();
        watch->displaySleep();
        sleepMode = true;
        WiFi.mode(WIFI_OFF);
        setCpuFrequencyMhz(20);
        Serial.println("ENTER IN LIGHT SLEEEP MODE");
        gpio_wakeup_enable((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
        gpio_wakeup_enable((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
    }
    else
    {
        watch->displayWakeup();
        watch->rtc->syncToSystem();
        watch->openBL();
    }
}