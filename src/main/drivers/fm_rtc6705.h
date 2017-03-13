/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gpio.h"
#include "system.h"

extern uint16_t const vtx_freq[];
extern uint8_t current_vtx_channel;

#define RTC6705_POWER_ON      GPIO_ResetBits(RTC6705_POWER_SW_GPIO, RTC6705_POWER_SW_PIN)
#define RTC6705_POWER_OFF     GPIO_SetBits(RTC6705_POWER_SW_GPIO, RTC6705_POWER_SW_PIN)

extern uint8_t rtc6705PowerOnFlag;

void rtc6705_init(void);
void rtc6705_writeDataStream(uint8_t address, uint32_t data);
uint32_t rtc6705_readDataStream(uint8_t address);
void rtc6705_setChannel(uint16_t channel_freq);

#define DISABLE_RTC6705_POWER_ON_FLAG() {rtc6705PowerOnFlag = 0; RTC6705_POWER_OFF;}
#define ENABLE_RTC6705_POWER_ON_FLAG() {rtc6705PowerOnFlag = 1; \
																				RTC6705_POWER_ON; \
																				delay(200); \
																				rtc6705_setChannel(vtx_freq[current_vtx_channel]);}
																				
#define RTC6705_POWER_ON_FLAG() (rtc6705PowerOnFlag)
