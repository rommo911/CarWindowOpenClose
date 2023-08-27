#ifndef _WINDOWS_PINS_H_
#define _WINDOWS_PINS_H_
#pragma once 
#include "main.h"

std::mutex _pinsMutex;
#define WINDOW1_UP_PIN 25
#define WINDOW1_DOWN_PIN 26

#define WINDOW2_UP_PIN 16
#define WINDOW2_DOWN_PIN 17

#define WINDOW3_UP_PIN -1
#define WINDOW3_DOWN_PIN -1

#define WINDOW4_UP_PIN -1
#define WINDOW4_DOWN_PIN -1

#define VOLTAGE_MESURE_PIN 34
#define LED_PIN 27
#define LOCK_INPUT_PIN 14
#define UNLOCK_INPUT_PIN 12

// Variables
static float LastKnowVoltage = 0;
const float referenceVoltage = 12.0; // Reference voltage for comparison
float DeltaVoltageThreshold = 2.0;   // Voltage drop threshold to stop window closing
static float MinimumVoltage = 10.5;
static int window1UpPin = WINDOW1_UP_PIN;
static int window1DownPin = WINDOW1_DOWN_PIN;
static int window2UpPin = WINDOW1_UP_PIN;
static int window2DownPin = WINDOW2_DOWN_PIN;
static int window3UpPin = WINDOW3_UP_PIN;
static int window3DownPin = WINDOW3_DOWN_PIN;
static int window4UpPin = WINDOW4_UP_PIN;
static int window4DownPin = WINDOW4_DOWN_PIN;

static int MAX_WINDOW_UP_TIME_MILLIS = 6500;
static int MAX_WINDOW_DONW_TIME_MILLIS = 6500;
static int voltDropConfirmTime = 5 ;

void setOpenTrigger(void *);

void setCloseTrigger(void *);
static bool SetupWindowsPins()
{
    auto _nvs = OPEN_NVS_DEAFULT();
    esp_err_t ret = ESP_OK;
    ret = _nvs->get("Window_1_up_pin", window1UpPin);
    ret |= _nvs->get("Window_1_down_pin", window1DownPin);
    ret |=_nvs->get("Window_2_up_pin", window2UpPin);
    ret |=_nvs->get("Window_2_down_pin", window2DownPin);
    ret |=_nvs->get("Window_3_up_pin", window3UpPin);
    ret |=_nvs->get("Window_3_down_pin", window3DownPin);
    ret |=_nvs->get("Window_4_up_pin", window4UpPin);
    ret |=_nvs->get("Window_4_down_pin", window4DownPin);
    bool is_inverted = false;
    ret |=_nvs->get("isInverted", is_inverted);
    ret |=_nvs->get("TimerDown", MAX_WINDOW_UP_TIME_MILLIS);
    ret |=_nvs->get("TimerDown", MAX_WINDOW_DONW_TIME_MILLIS);
    ret |=_nvs->get("MinVoltDropStop", MinimumVoltage);
    float mvolt = (MinimumVoltage / 1000.0) ;
    if (mvolt > 9 && mvolt < 11)
    {
        MinimumVoltage = mvolt;
    }
    ret |=_nvs->get("MinVoltDropStop", DeltaVoltageThreshold);
    pinMode(window1UpPin, OUTPUT);
    pinMode(window1DownPin, OUTPUT);
    pinMode(window2UpPin, OUTPUT);
    pinMode(window2DownPin, OUTPUT);

    pinMode(window3UpPin, OUTPUT);
    pinMode(window3DownPin, OUTPUT);
    pinMode(window4UpPin, OUTPUT);
    pinMode(window4DownPin, OUTPUT);
    digitalWrite(window1UpPin, LOW);
    digitalWrite(window1DownPin, LOW);
    digitalWrite(window2UpPin, LOW);
    digitalWrite(window2DownPin, LOW);
    digitalWrite(window3UpPin, LOW);
    digitalWrite(window3DownPin, LOW);
    digitalWrite(window4UpPin, LOW);
    digitalWrite(window4DownPin, LOW);
    Serial.println("SetupWindowsPins() ok ");
    return true;
}

static bool SetupInputANALOG()
{
    pinMode(VOLTAGE_MESURE_PIN, INPUT);
    Serial.println("SetupInputANALOG() ok ");
    return true;
}

static float ReadVoltage()
{
    return analogRead(VOLTAGE_MESURE_PIN) * (16.0 / 4096.0);
}

static bool IsVoltageDropped()
{
    float _voltage = 0;
    float checkVoltage = LastKnowVoltage >= 13.0 ? LastKnowVoltage : referenceVoltage;
    for (int i = 0; i < 5; i++)
    {
        _voltage += ReadVoltage();
        delay(10);
    }
    _voltage = _voltage / 5;
    if (_voltage <= std::abs(checkVoltage - DeltaVoltageThreshold))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void CloseWindow(int pinDown, int pinUP, int timeout )
{
    Serial.println("CloseWindow call ... ");
    std::lock_guard<std::mutex> _mutex(_pinsMutex);
    auto _now = millis();
    LastKnowVoltage = ReadVoltage();
    delay(50);
    digitalWrite(pinDown, 0);
    delay(50);
    _now = millis();
    Serial.print("now = ");
    Serial.println(millis());
    float _voltage = ReadVoltage();
    delay(150);
    Serial.print("volt = ");
    Serial.println((int)_voltage);
    int counter = 0;
    while (_voltage <= MinimumVoltage && counter++ < 5)
    {
        _voltage = ReadVoltage();
        Serial.println("_voltage < 10.5 ... ");
        delay(50);
    }
    if (_voltage <= MinimumVoltage)
    {
        Serial.println("_voltage too low exit  ");
        return;
    }
    digitalWrite(pinUP, 1);
    delay(500);
    _voltage = ReadVoltage();
    if (_voltage <= MinimumVoltage)
    {
        Serial.println("_voltage < 10.5  exit 2 ... ");
        return;
    }
    Serial.println("closing. ");
    int voltageCounter = 0;
    while ((voltageCounter < voltDropConfirmTime) && ((millis() - _now) < timeout))
    {
        if (IsVoltageDropped())
        {
            voltageCounter++;
        }
        delay(50);
        Serial.print('.');
    }
    Serial.println(".");
    digitalWrite(pinUP, 0);

    if (voltageCounter < 5)
        Serial.println("CloseWindow ended as tiemout ");
    else
        Serial.println("CloseWindow ended as voltage drop  ");
}

static void CloseALLWindow()
{
    Serial.println("CloseALLWindow CloseWindow 1  ");
    CloseWindow(WINDOW1_DOWN_PIN, WINDOW1_UP_PIN ,MAX_WINDOW_DONW_TIME_MILLIS );
    delay(1000);
    Serial.println("CloseALLWindow CloseWindow 2  ");
    CloseWindow(WINDOW2_DOWN_PIN, WINDOW2_UP_PIN, MAX_WINDOW_DONW_TIME_MILLIS);
    /* if (WINDOW3_DOWN_PIN != -1 ) ;
     {
         Serial.println("CloseALLWindow CloseWindow 3  ");
         CloseWindow(WINDOW3_DOWN_PIN ,WINDOW3_UP_PIN);
     }
     if (WINDOW4_DOWN_PIN != -1 ) ;
     {
         Serial.println("CloseALLWindow CloseWindow 4  ");
         CloseWindow(WINDOW4_DOWN_PIN ,WINDOW4_UP_PIN);
     }*/
}

static void OpenALLWindow()
{
    Serial.println("OpenALLWindow OpenWindow 1  ");
    CloseWindow(WINDOW1_UP_PIN, WINDOW1_DOWN_PIN, MAX_WINDOW_UP_TIME_MILLIS);
    delay(1000);
    Serial.println("OpenALLWindow OpenWindow 2  ");
    CloseWindow(WINDOW2_UP_PIN, WINDOW2_DOWN_PIN, MAX_WINDOW_UP_TIME_MILLIS);
    /*if (WINDOW3_DOWN_PIN != -1 ) ;
    {
        Serial.println("OpenALLWindow OpenWindow 3 ");
        CloseWindow(WINDOW3_UP_PIN, WINDOW3_DOWN_PIN );
    }
    if (WINDOW4_DOWN_PIN != -1 ) ;
    {
        Serial.println("OpenALLWindow OpenWindow 4 ");
        CloseWindow(WINDOW4_UP_PIN,WINDOW4_DOWN_PIN);
    }*/
}

#endif