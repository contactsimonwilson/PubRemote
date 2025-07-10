#ifndef __CONFIG_H
#define __CONFIG_H

// Set in env
// Powershell: $env:PLATFORMIO_BUILD_FLAGS='-D RELEASE_VARIANT=\"release\"'
#ifndef RELEASE_VARIANT
  #define RELEASE_VARIANT "dev"
#endif

// Power configuration
#ifndef FORCE_LIGHT_SLEEP
  #define FORCE_LIGHT_SLEEP 0
#endif

// i2c configuration
#if !defined(I2C_SDA) || !defined(I2C_SCL)
  #error "I2C_SDA and I2C_SCL must be defined for I2C communication. Please define them in your build flags."
#endif

// Joystick configuration
#if JOYSTICK_BUTTON_PIN < 0
  #define JOYSTICK_BUTTON_ENABLED 0
#else
  #define JOYSTICK_BUTTON_ENABLED 1
#endif

#if !defined(JOYSTICK_X_ADC) || !defined(JOYSTICK_X_ADC_UNIT)
  #define JOYSTICK_X_ENABLED 0
#else
  #define JOYSTICK_X_ENABLED 1
#endif

#if !defined(JOYSTICK_Y_ADC) || !defined(JOYSTICK_Y_ADC_UNIT)
  #define JOYSTICK_Y_ENABLED 0
#else
  #define JOYSTICK_Y_ENABLED 1
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

// Led configuration
#if defined(LED_DATA_PIN)
  #define LED_ENABLED 1
#else
  #define LED_ENABLED 0
#endif

#if LED_ENABLED && !defined(LED_COUNT)
  #define LED_COUNT 1 // Default to 1 LED if not defined
#endif

// Buzzer configuration
#if defined(BUZZER_PWM)
  #define BUZZER_ENABLED 1
#else
  #define BUZZER_ENABLED 0
#endif

#ifndef BUZZER_INVERT
  #define BUZZER_INVERT 0
#endif

// Haptic configuration
#if defined(HAPTIC_DRV2605) || defined(HAPTIC_PWM)
  #define HAPTIC_ENABLED 1
#else
  #define HAPTIC_ENABLED 0
#endif

#endif