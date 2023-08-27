#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include <Time.h>
#include "boot_cfg.h"
#include <GeneralUtils.h>
#include "nvs_flash.h"
#include "system.hpp"
#include "nvs_tools.h"
#include "avdweb_Switch.h"
#include "FreeRTOS.hpp"
bool checkForReset(bool _force = false); 
extern EventLoop_p_t event_loop;
//
#endif