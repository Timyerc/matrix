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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <platform.h>

#include "common/utils.h"

#include "gpio.h"
#include "system.h"

#include "fm_rtc6705.h"

#define DISABLE_RTC6705       GPIO_SetBits(RTC6705_LE_GPIO,   RTC6705_LE_PIN)
#define ENABLE_RTC6705        GPIO_ResetBits(RTC6705_LE_GPIO, RTC6705_LE_PIN)

#define RTC6705_CLK_HIGH      GPIO_SetBits(RTC6705_CLK_GPIO,   RTC6705_CLK_PIN)
#define RTC6705_CLK_LOW       GPIO_ResetBits(RTC6705_CLK_GPIO, RTC6705_CLK_PIN)

#define RTC6705_DAT_HIGH      GPIO_SetBits(RTC6705_DAT_GPIO,   RTC6705_DAT_PIN)
#define RTC6705_DAT_LOW       GPIO_ResetBits(RTC6705_DAT_GPIO, RTC6705_DAT_PIN)

uint8_t rtc6705PowerOnFlag = 0;

const uint16_t vtx_freq[] =
{
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // Boacam A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // Boscam B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // Boscam E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // FatShark
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // RaceBand
};

uint8_t current_vtx_channel;
uint8_t current_vtx_delay;

void rtc6705_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHBPeriphClockCmd(RTC6705_LE_PERIPHERAL, ENABLE);
  GPIO_InitStructure.GPIO_Pin = RTC6705_LE_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(RTC6705_LE_GPIO, &GPIO_InitStructure);

  RCC_AHBPeriphClockCmd(RTC6705_CLK_PERIPHERAL, ENABLE);
  GPIO_InitStructure.GPIO_Pin = RTC6705_CLK_PIN;
  GPIO_Init(RTC6705_CLK_GPIO, &GPIO_InitStructure);

  RCC_AHBPeriphClockCmd(RTC6705_DAT_PERIPHERAL, ENABLE);
  GPIO_InitStructure.GPIO_Pin = RTC6705_DAT_PIN;
  GPIO_Init(RTC6705_DAT_GPIO, &GPIO_InitStructure);

  RCC_AHBPeriphClockCmd(RTC6705_POWER_SW_PERIPHERAL, ENABLE);
  GPIO_InitStructure.GPIO_Pin = RTC6705_POWER_SW_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(RTC6705_POWER_SW_GPIO, &GPIO_InitStructure);

  //DISABLE_RTC6705;
  //RTC6705_CLK_HIGH;
  DISABLE_RTC6705_POWER_ON_FLAG();
}

void rtc6705_datPinOut(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = RTC6705_DAT_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(RTC6705_DAT_GPIO, &GPIO_InitStructure);
}

void rtc6705_datPinIn(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = RTC6705_DAT_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(RTC6705_DAT_GPIO, &GPIO_InitStructure);
}

void rtc6705_writeDataStream(uint8_t address, uint32_t data)
{
  uint32_t command = ((data & 0x0FFFFF) << 5) | (0x01 << 4) | (address & 0x0F);

  ENABLE_RTC6705;
  delay(1);
  for(int i = 0; i < 25; i++){
    if((command >> i) & 0x01){
      RTC6705_DAT_HIGH;
    } else{
      RTC6705_DAT_LOW;
    }

    RTC6705_CLK_HIGH;
    delay(1);
    RTC6705_CLK_LOW;
    delay(1);
  }

  DISABLE_RTC6705;
}

uint32_t rtc6705_readDataStream(uint8_t address)
{
  uint8_t command = (0x00 << 4) | (address & 0x0F);
  uint32_t data = 0;

  ENABLE_RTC6705;

  for(int i = 0; i < 5; i++){
    RTC6705_CLK_LOW;

    if((command >> i) & 0x01){
      RTC6705_DAT_HIGH;
    } else{
      RTC6705_DAT_LOW;
    }

    __asm("nop");
    __asm("nop");
    __asm("nop");
    delay(1);

    RTC6705_CLK_HIGH;

    __asm("nop");
    __asm("nop");
    __asm("nop");
  }

  rtc6705_datPinIn();

  for(int i = 0; i < 20; i++){
    RTC6705_CLK_HIGH;

    __asm("nop");
    __asm("nop");
    __asm("nop");

    RTC6705_CLK_LOW;

    __asm("nop");
    __asm("nop");
    __asm("nop");

    if(GPIO_ReadInputDataBit(RTC6705_DAT_GPIO, RTC6705_DAT_PIN)){
      data |= 0x01 << i;
    }
  }

  rtc6705_datPinOut();

  DISABLE_RTC6705;

  return data;
}

void rtc6705_setChannel(uint16_t channel_freq)
{
    uint32_t freq = (uint32_t)channel_freq * 1000;
    uint32_t N, A;

    freq /= 40;
    N = freq / 64;
    A = freq % 64;
    rtc6705_writeDataStream(0, 400);
    rtc6705_writeDataStream(1, (N << 7) | A);
}
