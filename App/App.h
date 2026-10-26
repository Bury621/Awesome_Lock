#include "main.h"
#include "Debug.h"
#include "Keyboard.h"
#include "stepper_motor.h"
#include "lcd.h"
#include "UserInterface.h"
#include "flash_eeprom.h"
#include <string.h>
#include "Fingerprint.h"
#include "lcd_menu.h"

#ifndef __APP_H
#define __APP_H

extern TIM_HandleTypeDef htim6;

void App_Init(void);
void App_Running(void);

#endif