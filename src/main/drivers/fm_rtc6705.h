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

extern uint16_t const vtx_freq[];
extern uint8_t current_vtx_channel;

void rtc6705_init(void);
void rtc6705_writeDataStream(uint8_t address, uint32_t data);
uint32_t rtc6705_readDataStream(uint8_t address);
void rtc6705_setChannel(uint16_t channel_freq);
