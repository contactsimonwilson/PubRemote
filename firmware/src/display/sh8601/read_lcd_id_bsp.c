#if DISP_CO5300 || DISP_SH8601

  #include "read_lcd_id_bsp.h"
  #include "driver/gpio.h"
  #include "esp_log.h"
  #include "esp_rom_sys.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include <stdio.h>

static const char *TAG = "PUBREMOTE-SH8601";

  #define bit_mask (uint64_t)0x01

  #define lcd_cs_1 gpio_set_level(DISP_CS, 1)
  #define lcd_cs_0 gpio_set_level(DISP_CS, 0)
  #define lcd_clk_1 gpio_set_level(DISP_CLK, 1)
  #define lcd_clk_0 gpio_set_level(DISP_CLK, 0)
  #define lcd_d0_1 gpio_set_level(DISP_SDIO0, 1)
  #define lcd_d0_0 gpio_set_level(DISP_SDIO0, 0)
  #define lcd_rst_1 gpio_set_level(DISP_RST, 1)
  #define lcd_rst_0 gpio_set_level(DISP_RST, 0)

  #define read_d0 gpio_get_level(DISP_SDIO0)

void lcd_gpio_init(void) {
  gpio_config_t gpio_conf = {};

  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = (bit_mask << DISP_CS) | (bit_mask << DISP_CLK) | (bit_mask << DISP_SDIO0) |
                           (bit_mask << DISP_SDIO1) | (bit_mask << DISP_SDIO2) | (bit_mask << DISP_SDIO3) |
                           (bit_mask << DISP_RST);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

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

void SPI_1L_SendData(uint8_t dat) {
  uint8_t i;

  for (i = 0; i < 8; i++) {
    if ((dat & 0x80) != 0) {
      lcd_d0_1;
    }
    else {
      lcd_d0_0;
    }

    dat <<= 1;
    lcd_clk_0;
    lcd_clk_1;
  }
}

void WriteComm(uint8_t regval) {
  lcd_cs_0;
  lcd_d0_0;
  lcd_clk_0;

  lcd_d0_0;
  lcd_clk_0;
  lcd_clk_1;
  SPI_1L_SendData(regval);
  lcd_cs_1;
}

void WriteData(uint8_t val) {
  lcd_cs_0;
  lcd_d0_0;
  lcd_clk_0;

  lcd_d0_1;
  lcd_clk_0;
  lcd_clk_1;
  SPI_1L_SendData(val);
  lcd_cs_1;
}

void SPI_WriteComm(uint8_t regval) {
  SPI_1L_SendData(0x02);
  SPI_1L_SendData(0x00);
  SPI_1L_SendData(regval);
  SPI_1L_SendData(0x00);
}

void SPI_ReadComm(uint8_t regval) {
  SPI_1L_SendData(0x03);
  SPI_1L_SendData(0x00);
  SPI_1L_SendData(regval);
  SPI_1L_SendData(0x00);
}

uint8_t SPI_ReadData(void) {
  uint8_t i = 0, dat = 0;

  for (i = 0; i < 8; i++) {
    lcd_clk_0;
    sda_read_mode();
    dat = (dat << 1) | read_d0;
    sda_write_mode();
    lcd_clk_1;
    esp_rom_delay_us(1);
  }

  return dat;
}

uint8_t SPI_ReadData_Continue(void) {
  uint8_t i = 0, dat = 0;

  for (i = 0; i < 8; i++) {
    lcd_clk_0;
    sda_read_mode();
    esp_rom_delay_us(1);
    dat = (dat << 1) | read_d0;
    sda_write_mode();
    lcd_clk_1;
    esp_rom_delay_us(1);
  }

  return dat;
}

uint8_t read_lcd_id(void) {
  lcd_gpio_init();

  lcd_rst_1;
  vTaskDelay(pdMS_TO_TICKS(120));
  lcd_rst_0;
  vTaskDelay(pdMS_TO_TICKS(120));
  lcd_rst_1;
  vTaskDelay(pdMS_TO_TICKS(120));
  SPI_ReadComm(0xDA);

  uint8_t ret = SPI_ReadData_Continue();
  ESP_LOGI(TAG, "SH8601/CO5300 Check: 0x%02x", ret);

  return ret;
}

#endif