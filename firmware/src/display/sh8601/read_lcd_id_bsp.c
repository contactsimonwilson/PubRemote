#include "read_lcd_id_bsp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define bit_mask (uint64_t)0x01

#ifndef DISP_CLK
  #define DISP_CLK 10
#endif
#ifndef DISP_SDIO0
  #define DISP_SDIO0 11
#endif

#define lcd_clk_1 gpio_set_level(DISP_CLK, 1)
#define lcd_clk_0 gpio_set_level(DISP_CLK, 0)
#define lcd_d0_1 gpio_set_level(DISP_SDIO0, 1)
#define lcd_d0_0 gpio_set_level(DISP_SDIO0, 0)

#define read_d0 gpio_get_level(DISP_SDIO0)

void sda_read_mode(void) {
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pin_bit_mask = (bit_mask << DISP_SDIO0);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void sda_write_mode(void) {
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = (bit_mask << DISP_SDIO0);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void delay_us(uint32_t us) {
  esp_rom_delay_us(us);
}

void SPI_1L_SendData(uint8_t dat) {
  uint8_t i;

  for (i = 0; i < 8; i++) {
    if ((dat & 0x80) != 0)
      lcd_d0_1;
    else
      lcd_d0_0;

    dat <<= 1;

    lcd_clk_0;
    lcd_clk_1;
  }
}

void SPI_ReadComm(uint8_t regval) {
  SPI_1L_SendData(0x03);
  SPI_1L_SendData(0x00);
  SPI_1L_SendData(regval);
  SPI_1L_SendData(0x00);
}

uint8_t read_lcd_id(void) {
  SPI_ReadComm(0xDA);

  uint8_t i = 0, ret = 0;

  for (i = 0; i < 8; i++) {
    lcd_clk_0;
    sda_read_mode();
    delay_us(1);
    ret = (ret << 1) | read_d0;
    sda_write_mode();
    lcd_clk_1;
    delay_us(1);
  }

  ESP_LOGI("lcd_Model", "0x%02x", ret);
  return ret;
}