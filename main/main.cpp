#include "main.h"
#include "windowsPins.hpp"

EventLoop_p_t event_loop = nullptr;

volatile bool windowClosingTrigger = false;
volatile bool windowOpenenigTrigger = false;

void setOpenTrigger(void *)
{
  Serial.println("unlockButton. setOpenTrigger");
  windowOpenenigTrigger = true;
}

void setCloseTrigger(void *)
{
  Serial.println("lockButton. setCloseTrigger");
  windowClosingTrigger = true;
}

Switch unlockButton(UNLOCK_INPUT_PIN, INPUT_PULLDOWN, HIGH, 50, 300, 2500, 20); // Switch object for the window trigger
Switch lockButton(LOCK_INPUT_PIN, INPUT_PULLDOWN, HIGH, 50, 300, 2500, 20);     // Switch object for the window trigger
static unsigned int LastWindowsClosed_ts = 0;
static unsigned int LastWindowsOpened_ts = 0;
static int ledStatus = 0;
void sleepNow();

void LEDTask(void *)
{
  int delayONms = 500;
  int delaymsOFF = 1000;
  while (1)
  {
    switch (ledStatus)
    {
    case 0:
    {
      delayONms = 250;
      delaymsOFF = 10;
      break;
    }
    case 1:
    {
      delayONms = 200;
      delaymsOFF = 100;
      break;
    }
    case 2:
    {
      delayONms = 100;
      delaymsOFF = 200;
      break;
    }
    default:
    {
      break;
    }
    }
    delay(delaymsOFF);
    digitalWrite(LED_PIN, HIGH);
    delay(delayONms);
    digitalWrite(LED_PIN, LOW);
  }
  vTaskDelete(NULL);
}

void setup()
{
  nvs_flash_init();
  bootConfig = std::make_unique<BootConfig_t>();
  bootConfig->Boot();
  Serial.begin(115200);
  event_loop = std::make_shared<EventLoop>();
  pinMode(LED_PIN, OUTPUT);
  checkForReset();
  auto _nvs = OPEN_NVS_DEAFULT();
  esp_err_t ret = ESP_OK;
  int UnlockDoubleClickTime = 2000 , LockDoubleClickTime = 2000 ; 
  ret = _nvs->get("LockDoubleClickTime", LockDoubleClickTime);
  ret |= _nvs->get("UnlockDoubleClickTime", UnlockDoubleClickTime);
  _nvs.reset();
  lockButton.setDoubleClickTime(LockDoubleClickTime);
  lockButton.setDoubleClickTime(UnlockDoubleClickTime);
  unlockButton.setDoubleClickCallback(setOpenTrigger);
  lockButton.setDoubleClickCallback(setCloseTrigger);
  SetupWindowsPins();
  SetupInputANALOG();
  digitalWrite(LED_PIN, HIGH);
  FreeRTOS::StartTask(LEDTask, "Handle led status", 1024, NULL, 2, NULL);

  // put your setup code here, to run once:
  float _voltage = ReadVoltage();
  Serial.printf("voltage %f \n", _voltage);
  tools::dumpHeapInfo();
}

void loop()
{
    while (1)
    {
      float _voltage = ReadVoltage();
      if (_voltage <= 14.0)
      {
        lockButton.poll();
        if (windowClosingTrigger && (millis() - LastWindowsClosed_ts > 5000))
        {
          Serial.println("windowClosingTrigger started ");
          ledStatus = 1;

          Serial.println("lockButton released ");
          windowClosingTrigger = false;
          CloseALLWindow();
          LastWindowsClosed_ts = millis();
          // sleepNow();
          ledStatus = 0;
        }
        else
        {
          unlockButton.poll();
          if (windowOpenenigTrigger && (millis() - LastWindowsOpened_ts > 5000))
          {
            Serial.println("windowOpenenigTrigger started ");
            ledStatus = 2;
            Serial.println("unlockButton released ");
            LastWindowsOpened_ts = millis();
            windowOpenenigTrigger = false;
            OpenALLWindow();
            ledStatus = 0;
            // sleepNow();
          }
        }
      }
    }
}

extern "C" void app_main(void)
{
    setup();
    loop();
}

void wakeUpInterrupt()
{
    // Interrupt service routine for wake-up
    detachInterrupt(digitalPinToInterrupt(LOCK_INPUT_PIN));
    detachInterrupt(digitalPinToInterrupt(UNLOCK_INPUT_PIN));

    // Print wake-up message
    Serial.println("Waking up from sleep mode!");
    // Add your code to handle the wake-up event
    setup();
    loop();
}

void sleepNow()
{
    attachInterrupt(digitalPinToInterrupt(LOCK_INPUT_PIN), wakeUpInterrupt, HIGH);
    attachInterrupt(digitalPinToInterrupt(UNLOCK_INPUT_PIN), wakeUpInterrupt, HIGH);
    // Prepare sleep mode
    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // sleep_enable();

    // Enter sleep mode
    // sleep_mode();
    // The microcontroller will now sleep until a wake-up event occurs
    // Any wake-up event will trigger the wakeUpInterrupt() function
    // After wake-up, the execution will continue from where it left off
}