#include "main.h"
#include "esp_pm.h"
#include "boot_cfg.h"
#include "Button2.h"
#include "Wire.h"
#include "mqttv2.hpp"
#include "nvs_flash.h"

float temp = 0, humidity = 0;
int press = 0;
EventLoop_p_t event_loop = nullptr;
homeassistant::SensorDiscovery *HassRssiSensorDescovery;
HASS_SHT *sht = nullptr;
HASS_PIR *hassPir = nullptr;
HASS_LDR *lightSensor;
HASS_SWITCH *switch1 = nullptr;
HASS_SWITCH *switch2 = nullptr;
OpenWeather *openWeather{nullptr};
Button2 *button{nullptr};
bool weatherType = true;

std::string device, room;

void CreateHassDevices()
{
    // context fill
    auto nvs = OPEN_NVS("default");
    nvs->getS("deviceName", homeassistant::hassDevCtxDescription.name);
    nvs->getS("room", homeassistant::hassDevCtxDescription.room);
    nvs.reset();
    homeassistant::hassDevCtxDescription.version = (esp_ota_get_app_description()->version);
    homeassistant::hassDevCtxDescription.manufacturer = (std::string("esp2") + std::string(WiFi.macAddress().c_str()) + esp_ota_get_app_description()->idf_ver);
    homeassistant::hassDevCtxDescription.suggested_area = homeassistant::hassDevCtxDescription.room;
    uint8_t mac[6];
    char macStr[18] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(macStr, "%02X%02X", mac[4], mac[5]);
    homeassistant::hassDevCtxDescription.MAC = macStr;
    //
    // create & hook
    homeassistant::statsPublishDiscoveryFunction = std::bind(Mqtt::PublishStaticPair, MqttClass.get(), std::placeholders::_1, 2, true);
    homeassistant::MqttPublishFunction = std::bind(Mqtt::PublishStaticPair, MqttClass.get(), std::placeholders::_1, 1, false);
    hassPir = new HASS_PIR(event_loop, PIR_PIN, GPIO_MODE_INPUT);
    hassPir->SetTimeout(60s);
    lightSensor = new HASS_LDR(homeassistant::SensorDiscovery::illuminance, event_loop, LDR_ADC, "LDR");
    lightSensor->Init();
    HassRssiSensorDescovery = new homeassistant::SensorDiscovery(homeassistant::SensorDiscovery::signal_strength, "db");
    sht = new HASS_SHT();
    sht->Begin();
    switch1 = new HASS_SWITCH("switch1", SWITCH1_PIN, HIGH);
    switch2 = new HASS_SWITCH("switch2", SWITCH2_PIN, HIGH);

    homeassistant::statsGenerateFunctions.clear();
    homeassistant::statsGenerateFunctions.push_back([]()
                                                    { return sht->GenerateHassTemp(); });
    homeassistant::statsGenerateFunctions.push_back([]()
                                                    { return sht->GenerateHassHumidity(); });
    homeassistant::statsGenerateFunctions.push_back([]()
                                                    { return hassPir->GenerateHassStatus(); });
    homeassistant::statsGenerateFunctions.push_back([]()
                                                    { return switch1->GenerateHassStatus(); });
    homeassistant::statsGenerateFunctions.push_back([]()
                                                    { return switch2->GenerateHassStatus(); });
    homeassistant::statsGenerateFunctions.push_back([&]()
                                                    {
        nlohmann::json hassJson = {{HassRssiSensorDescovery->GetClass(), WiFi.RSSI()}};
        return std::make_pair(HassRssiSensorDescovery->StatusTopic(), hassJson.dump()); });
}

void HookEvents()
{
    button->setClickHandler([](Button2 &btn)
                            {
            weatherType = !weatherType;
            UpdateMainPageInfo();
            oled->SetPage(OLED::Disp_Page_t::MAIN_PAGE); });

    static auto reg_3df = event_loop->register_event(hassPir->EVENT_PIR_ON, [](const ESPEvent &event, void *data)
                                                     {
            time_t now = time(0);
            const tm* localtm = localtime(&now);
            if (localtm->tm_hour < 23 && localtm->tm_hour > 6)
            {
                UpdateMainPageInfo();
                Oled_Wakeup();
                oled->SetPage(OLED::Disp_Page_t::MAIN_PAGE);
            }
            MqttClass->Publish(hassPir->GenerateHassStatus(), 1); });
    static auto reg_8 = event_loop->register_event(hassPir->EVENT_PIR_OFF, [](const ESPEvent &event, void *data)
                                                   {
            MqttClass->Publish(hassPir->GenerateHassStatus(), 1);
            Oled_Sleep(); });
    static auto reg_light = event_loop->register_event(lightSensor->EVENT_CHANGED_5_PERC, [](const ESPEvent &event, void *data)
                                                       { MqttClass->Publish(lightSensor->GenerateHassLight(), 0, 0); });

    static auto reg_4 = event_loop->register_event(rot->EVENT_ROTARY_BTN_CLICKED, [](const ESPEvent &event, void *data)
                                                   { Oled_Wakeup(); });
    static auto reg_5 = event_loop->register_event(rot->EVENT_ROTARY_BTN_LONG_PRESSED, [](const ESPEvent &event, void *data)
                                                   { Oled_Wakeup(); });

    static auto reg_6 = event_loop->register_event(rot->EVENT_ROTARY_CHANGED, [](const ESPEvent &event, void *data)
                                                   { Oled_Wakeup(); });

    otaServer->SetPreOtaCb([&](void *)
                           {
        if (oled != nullptr)
        {
            oled->SetPage(OLED::Disp_Page_t::OTA_start_PAGE);
        }
        json updatejson;
        updatejson["OTA"] = "started";
        esp_err_t ret = MqttClass->Publish((hassPir->ConnectionTopic() + "/status").c_str(), updatejson.dump().c_str());
        return ret; });
    otaServer->SetPostOtaCb([&](void *)
                            {
        if (oled != nullptr)
        {
            oled->SetPage(OLED::Disp_Page_t::OTA_END_PAGE);
        }
        json updatejson;
        updatejson["OTA"] = "finished";
        esp_err_t ret = MqttClass->Publish((hassPir->ConnectionTopic() + "/status").c_str(), updatejson.dump().c_str());
        return ret; });
}

extern "C" void app_main(void)
{
    nvs_flash_init();
    bootConfig = std::make_unique<BootConfig_t>();
    bootConfig->Boot();
    esp_log_level_set("wifi-init", ESP_LOG_WARN);
    event_loop = std::make_shared<ESPEventLoop>();
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCLK_PIN, (uint32_t)4000000);
    oled = new OLED(_SCREEN_ADDR);
    oled->ClearDisplay();
    WiFi.enableSTA(true);
    MqttClass = std::make_shared<Mqtt>(event_loop);
    checkForReset(false);
    rot = new Rotary(event_loop, ROTARY_PIN2, ROTARY_PIN1, BUTTON_PIN, "Rotary", 0);
    rot->ResetRotary(0);
    tools::dumpHeapInfo();
    otaServer = std::make_unique<OTA_Server>(event_loop);
    openWeather = new OpenWeather();
    button = new Button2(BUTTON_PIN2, INPUT, false, true);
    FreeRTOS::StartTask(Handle_Wifi, "Handle wifi", 4096, NULL, 2, NULL);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
    CreateHassDevices();
    HookEvents();
    UpdateMainPageInfo();
    oled->SetPage(OLED::Disp_Page_t::MAIN_PAGE);
    tools::dumpHeapInfo();
    auto now = GET_NOW_MINUTES;
    while (1)
    {
        delay(15);
        if (GET_NOW_MINUTES - now > 3)
        {
            std::this_thread::sleep_for(3min);
            publishMqtt_stat();
            UpdateMainPageInfo();
        }
    }
}

void UpdateMainPageInfo()
{
    if (weatherType)
    {
        bool ret = sht->UpdateData();
        temp = sht->GetTemperature();
        humidity = sht->GetRelHumidity();
        if (ret == false && MqttClass->IsConnected())
        {
            openWeather->getWeather();
            temp = openWeather->getWeatherFeelsLike();
            humidity = openWeather->getWeatherHumidity();
        }
    }
    else
    {
        if (MqttClass->IsConnected())
        {
            openWeather->getWeather();
            temp = openWeather->getWeatherFeelsLike();
            humidity = openWeather->getWeatherHumidity();
        }
    }
    oled->MainPageArg.temp = temp;
    oled->MainPageArg.humidity = humidity;
    oled->MainPageArg.connection = WiFi.status() == WL_CONNECTED;
}