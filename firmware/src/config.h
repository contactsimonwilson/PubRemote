#ifndef __CONFIG_H
#define __CONFIG_H

// Set in env
// Powershell: $env:PLATFORMIO_BUILD_FLAGS='-D RELEASE_VARIANT=\"release\"'
#ifndef RELEASE_VARIANT
  #define RELEASE_VARIANT "dev"
#endif

// Power configuration
#ifndef PMU_SDA
  #define PMU_SDA -1
#endif
#ifndef PMU_SCL
  #define PMU_SCL -1
#endif
#ifndef PMU_INT
  #define PMU_INT -1
#endif

#ifndef FORCE_LIGHT_SLEEP
  #define FORCE_LIGHT_SLEEP 0
#endif

// Joystick configuration
#ifndef JOYSTICK_BUTTON_PIN
  #define JOYSTICK_BUTTON_PIN -1
#endif

#ifndef JOYSTICK_X_ADC
  #define JOYSTICK_X_ADC -1
#endif

#ifndef JOYSTICK_Y_ADC
  #define JOYSTICK_Y_ADC -1
#endif

#if JOYSTICK_BUTTON_PIN < 0
  #define JOYSTICK_BUTTON_ENABLED 0
#else
  #define JOYSTICK_BUTTON_ENABLED 1
#endif

#if JOYSTICK_X_ADC < 0
  #define JOYSTICK_X_ENABLED 0
#else
  #define JOYSTICK_X_ENABLED 1
#endif

#if JOYSTICK_Y_ADC < 0
  #define JOYSTICK_Y_ENABLED 0
#else
  #define JOYSTICK_Y_ENABLED 1
#endif

#if (JOYSTICK_Y_ENABLED && !defined(JOYSTICK_Y_ADC_UNIT))
  #error "JOYSTICK_Y_ADC_UNIT must be defined"
#endif

#if (JOYSTICK_X_ENABLED && !defined(JOYSTICK_X_ADC_UNIT))
  #error "JOYSTICK_X_ADC_UNIT must be defined"
#endif

#define JOYSTICK_ENABLED (JOYSTICK_X_ENABLED || JOYSTICK_Y_ENABLED)

// Display configuration
#if TP_CST816S
  #include "esp_lcd_touch_cst816s.h"
  #define TOUCH_ENABLED 1
#elif TP_FT3168
  #include "esp_lcd_touch_ft3168.h"
  #define TOUCH_ENABLED 1
#else
  #define TOUCH_ENABLED 0
#endif

// i2c configuration
#if !defined(PMU_SDA) && !defined(TP_SDA)
  #define I2C_SDA -1
#elif defined(PMU_SDA)
  #define I2C_SDA PMU_SDA
#elif defined(TP_SDA)
  #define I2C_SDA TP_SDA
#endif

#if !defined(PMU_SCL) && !defined(TP_SCL)
  #define I2C_SCL -1
#elif defined(PMU_SCL)
  #define I2C_SCL PMU_SCL
#elif defined(TP_SCL)
  #define I2C_SCL TP_SCL
#endif

#if defined(TP_SDA) && defined(TP_SCL) && defined(PMU_SDA) && defined(PMU_SCL) &&                                      \
    (TP_SDA != PMU_SDA || TP_SCL != PMU_SCL)
  #error                                                                                                               \
      "All I2C devices must share the same SDA and SCL pins. Please define either PMU_SDA/PMU_SCL or TP_SDA/TP_SCL, but not both."
#endif

// Led configuration
#ifndef LED_POWER_PIN
  #define LED_POWER_PIN -1
#endif

#ifndef LED_DATA_PIN
  #define LED_DATA_PIN -1
#endif

#if (LED_POWER_PIN < 0 || LED_DATA_PIN < 0)
  #define LED_ENABLED 0
#else
  #define LED_ENABLED 1
#endif

// Buzzer configuration
#ifndef BUZZER_PIN
  #define BUZZER_PIN -1
#endif

#if (BUZZER_PIN < 0)
  #define BUZZER_ENABLED 0
#else
  #define BUZZER_ENABLED 1
#endif

#ifndef BUZZER_INVERT
  #define BUZZER_INVERT 0
#endif

// Haptic configuration

#endif