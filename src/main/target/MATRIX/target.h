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

#define TARGET_BOARD_IDENTIFIER "MXF3"

#define LED0_GPIO   GPIOC
#define LED0_PIN    Pin_14
#define LED0_PERIPHERAL RCC_AHBPeriph_GPIOC

#define BEEP_GPIO   GPIOC
#define BEEP_PIN    Pin_15
#define BEEP_PERIPHERAL RCC_AHBPeriph_GPIOC
#define BEEPER_INVERTED

#define USABLE_TIMER_CHANNEL_COUNT 6

#define GYRO
#define USE_GYRO_MPU6050
#define GYRO_MPU6050_ALIGN CW90_DEG

#define ACC
#define USE_ACC_MPU6050
#define ACC_MPU6050_ALIGN CW90_DEG

#define BARO
#define USE_BARO_FBM320

#define MAG
#define USE_MAG_AK8963
#define USE_MAG_HMC5883
#define MAG_HMC5883_ALIGN CW90_DEG
#define MAG_AK8963_ALIGN CW270_DEG

#define USE_FLASHFS
#define USE_FLASH_M25P16

#define BEEPER
#define LED0

#define SONAR
#define SONAR_TRIGGER_PIN           Pin_0   // RC_CH7 (PB0) - only 3.3v ( add a 1K Ohms resistor )
#define SONAR_TRIGGER_GPIO          GPIOB
#define SONAR_ECHO_PIN              Pin_1   // RC_CH8 (PB1) - only 3.3v ( add a 1K Ohms resistor )
#define SONAR_ECHO_GPIO             GPIOB
#define SONAR_EXTI_LINE             EXTI_Line1
#define SONAR_EXTI_PIN_SOURCE       EXTI_PinSource1
#define SONAR_EXTI_IRQN             EXTI1_IRQn

#define USB_IO
 
#define USE_VCP
#define USE_UART1
#define USE_UART2
#define USE_UART3
#define USE_SOFTSERIAL1
#define USE_SOFTSERIAL2
#define SERIAL_PORT_COUNT 6

#ifndef UART1_GPIO
#define UART1_TX_PIN        GPIO_Pin_9  // PA9
#define UART1_RX_PIN        GPIO_Pin_10 // PA10
#define UART1_GPIO          GPIOA
#define UART1_GPIO_AF       GPIO_AF_7
#define UART1_TX_PINSOURCE  GPIO_PinSource9
#define UART1_RX_PINSOURCE  GPIO_PinSource10
#endif

#define UART2_TX_PIN        GPIO_Pin_3
#define UART2_RX_PIN        GPIO_Pin_4
#define UART2_GPIO          GPIOB
#define UART2_GPIO_AF       GPIO_AF_7
#define UART2_TX_PINSOURCE  GPIO_PinSource3
#define UART2_RX_PINSOURCE  GPIO_PinSource4

#ifndef UART3_GPIO
#define UART3_TX_PIN        GPIO_Pin_10 // PB10 (AF7)
#define UART3_RX_PIN        GPIO_Pin_11 // PB11 (AF7)
#define UART3_GPIO_AF       GPIO_AF_7
#define UART3_GPIO          GPIOB
#define UART3_TX_PINSOURCE  GPIO_PinSource10
#define UART3_RX_PINSOURCE  GPIO_PinSource11
#endif

#define SOFTSERIAL_1_TIMER TIM3
#define SOFTSERIAL_1_TIMER_RX_HARDWARE 4 // PWM 5
#define SOFTSERIAL_1_TIMER_TX_HARDWARE 5 // PWM 6
#define SOFTSERIAL_2_TIMER TIM3
#define SOFTSERIAL_2_TIMER_RX_HARDWARE 6 // PWM 7
#define SOFTSERIAL_2_TIMER_TX_HARDWARE 7 // PWM 8

#define USE_I2C
#define I2C_DEVICE (I2CDEV_1) // PB8/SCL, PB9/SDA

#define I2C1_SCL_GPIO        GPIOB
#define I2C1_SCL_GPIO_AF     GPIO_AF_4
#define I2C1_SCL_PIN         GPIO_Pin_8
#define I2C1_SCL_PIN_SOURCE  GPIO_PinSource8
#define I2C1_SCL_CLK_SOURCE  RCC_AHBPeriph_GPIOB
#define I2C1_SDA_GPIO        GPIOB
#define I2C1_SDA_GPIO_AF     GPIO_AF_4
#define I2C1_SDA_PIN         GPIO_Pin_9
#define I2C1_SDA_PIN_SOURCE  GPIO_PinSource9
#define I2C1_SDA_CLK_SOURCE  RCC_AHBPeriph_GPIOB

#define USE_SPI
#define USE_SPI_DEVICE_2 // PB12,13,14,15 on AF5

#define SPI2_GPIO               GPIOB
#define SPI2_GPIO_PERIPHERAL    RCC_AHBPeriph_GPIOB
#define SPI2_NSS_PIN            Pin_12
#define SPI2_NSS_PIN_SOURCE     GPIO_PinSource12
#define SPI2_SCK_PIN            Pin_13
#define SPI2_SCK_PIN_SOURCE     GPIO_PinSource13
#define SPI2_MISO_PIN           Pin_14
#define SPI2_MISO_PIN_SOURCE    GPIO_PinSource14
#define SPI2_MOSI_PIN           Pin_15
#define SPI2_MOSI_PIN_SOURCE    GPIO_PinSource15

#define M25P16_CS_GPIO          GPIOB
#define M25P16_CS_PIN           GPIO_Pin_12
#define M25P16_SPI_INSTANCE     SPI2

#define USE_RTC6705
#define RTC6705_LE_GPIO   GPIOA
#define RTC6705_LE_PIN    Pin_0
#define RTC6705_LE_PERIPHERAL RCC_AHBPeriph_GPIOA
#define RTC6705_CLK_GPIO   GPIOA
#define RTC6705_CLK_PIN    Pin_1
#define RTC6705_CLK_PERIPHERAL RCC_AHBPeriph_GPIOA
#define RTC6705_DAT_GPIO   GPIOA
#define RTC6705_DAT_PIN    Pin_7
#define RTC6705_DAT_PERIPHERAL RCC_AHBPeriph_GPIOA
#define RTC6705_POWER_SW_GPIO   GPIOB
#define RTC6705_POWER_SW_PIN    Pin_5
#define RTC6705_POWER_SW_PERIPHERAL RCC_AHBPeriph_GPIOB

#define OSD_DTR_GPIO   GPIOA
#define OSD_DTR_PIN    Pin_15
#define OSD_DTR_PERIPHERAL RCC_AHBPeriph_GPIOA

#define USE_ADC
//#define BOARD_HAS_VOLTAGE_DIVIDER


#define ADC_INSTANCE                ADC2
#define ADC_DMA_CHANNEL             DMA2_Channel1
#define ADC_AHB_PERIPHERAL          RCC_AHBPeriph_DMA2

#define VBAT_ADC_GPIO               GPIOA
#define VBAT_ADC_GPIO_PIN           GPIO_Pin_5
#define VBAT_ADC_CHANNEL            ADC_Channel_2

#define CURRENT_METER_ADC_GPIO      GPIOA
#define CURRENT_METER_ADC_GPIO_PIN  GPIO_Pin_4
#define CURRENT_METER_ADC_CHANNEL   ADC_Channel_1

#define RSSI_ADC_GPIO               GPIOB
#define RSSI_ADC_GPIO_PIN           GPIO_Pin_2
#define RSSI_ADC_CHANNEL            ADC_Channel_12

#define LED_STRIP
#define LED_STRIP_TIMER TIM1

#define WS2811_GPIO                     GPIOA
#define WS2811_GPIO_AHB_PERIPHERAL      RCC_AHBPeriph_GPIOA
#define WS2811_GPIO_AF                  GPIO_AF_6
#define WS2811_PIN                      GPIO_Pin_8
#define WS2811_PIN_SOURCE               GPIO_PinSource8
#define WS2811_TIMER                    TIM1
#define WS2811_TIMER_APB2_PERIPHERAL    RCC_APB2Periph_TIM1
#define WS2811_DMA_CHANNEL              DMA1_Channel2
#define WS2811_IRQ                      DMA1_Channel2_IRQn
#define WS2811_DMA_TC_FLAG              DMA1_FLAG_TC2
#define WS2811_DMA_HANDLER_IDENTIFER    DMA1_CH2_HANDLER


#define DEFAULT_RX_FEATURE FEATURE_RX_PPM

#define BLACKBOX
#define ENABLE_BLACKBOX_LOGGING_ON_SPIFLASH_BY_DEFAULT

#define DISPLAY
#define GPS
#define GTUNE
#define SERIAL_RX
#define TELEMETRY
#define USE_SERVOS
#define USE_CLI

#define SPEKTRUM_BIND
// UART3,
#define BIND_PORT  GPIOB
#define BIND_PIN   Pin_11

#define USE_SERIAL_4WAY_BLHELI_INTERFACE
