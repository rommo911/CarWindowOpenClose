#include "main.h"
#include "WebConfig.h"
#include "utilities.hpp"
#include "ota_server.h"
#include "main.h"

WebServer ConfigurationServer;
WebConfig confClass(true);
#define RESET_GPIO 22

static const String params = "["
                             "{'name':'DeviceName','label':'device name',"
                             "'type':" OPTION_INPUTTEXT
                             ",'default':'CarWindow'},"

                             "{'name':'MinVoltDropStop','label':'voltage drop to stop window mV',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':9000,'max':11000,'default':10500},"

                             "{'name':'DeltaVoltDropStop','label':'delta voltage drop to stop window mV',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':1,'max':4,'default':2},"

                             "{'name':'voltDropCofnirmTime','label':'voltage drop confirm try number',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':3,'max':10,'default':5},"

                             "{'name':'TimerDown','label':'blind Down duration ms',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':5000,'max':10000,'default':6500},"

                             "{'name':'TimerDown','label':'blind Down duration ms',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':5000,'max':10000,'default':6500},"

                             "{'name':'LockDoubleClickTime','label':'LockDoubleClickTime ms',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':1000,'max':4000,'default':2500},"

                             "{'name':UnlLockDoubleClickTime','label':'UnlLockDoubleClickTime ms',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':1000,'max':4000,'default':2500},"
                             "{'name':'isInverted','label':'blind Inverted output',"
                             "'type':" OPTION_INPUTCHECKBOX
                             ",'default':'1'}"

                             "{'name':'Window_1_up_pin ','label':'window 1 up pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_1_down_pin ','label':'window 1 down pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_2_up_pin ','label':'window 2 up pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_2_down_pin ','label':'window 2 down pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_3_up_pin ','label':'window 3 up pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_3_down_pin ','label':'window 3 down pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_4_up_pin ','label':'window 4 up pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "{'name':'Window_4_down_pin ','label':'window 4 down pin',"
                             "'type':" OPTION_INPUTRANGE
                             ",'min':-1,'max':40,'default':-1},"

                             "]";

bool StartupConfigPortal()
{
    tools::dumpHeapInfo();
    WiFi.enableSTA(false);
    WiFi.softAP(WiFi.macAddress().c_str(), nullptr, 10, 0, 1);
    bool configuring = true;
    confClass.setDescription(params, &ConfigurationServer);
    confClass.registerOnSave([&]()
                             { configuring = false; });
    ConfigurationServer.begin(80);
    ConfigurationServer.enableDelay(true);
    otaServer = std::make_unique<OTA_Server>(event_loop);
    otaServer->SetPreOtaCb([&](void *)
                           { Serial.println("ot started"); return ESP_OK; });
    otaServer->SetPostOtaCb([&](void *)
                            { Serial.println("ot done"); return ESP_OK; });
    otaServer->StartService();
    while (configuring)
    {
        ConfigurationServer.handleClient();
        delay(10);
    }
    ConfigurationServer.stop();
    WiFi.enableAP(false);
    return true;
}

bool checkForReset(bool _force)
{

    bool configured = true;
    do
    {
        auto nvs = OPEN_NVS("default");
        std::string device;
        configured = nvs->getS("deviceName", device) == ESP_OK ? true : false;
        nvs.reset();
        if (!configured)
            Serial.println("no  config found, starting config portal");

    } while (0);

    if (_force || (!configured))
    {
        if (StartupConfigPortal())
        {
            delay(3000);
            std::abort(); // reboot
        }
    }
    else
    {
        pinMode(RESET_GPIO, INPUT_PULLUP);
        int counter = 0;
        while (digitalRead(RESET_GPIO) == false && counter < 10)
        {
            std::this_thread::sleep_for(200ms);
            counter++;
        }
        if (counter >= 8 )
        {
            Serial.println("manual starting config portal");
            if (StartupConfigPortal())
            {
                delay(3000);
                std::abort(); // reboot
            }
        }
    }

    return false;
}
