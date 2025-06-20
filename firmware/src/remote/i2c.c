#include "i2c.h"
#include "config.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "PUBREMOTE-I2C";

// TODO -I2C mutex for thread safety
// static SemaphoreHandle_t i2c_mutex = NULL;

void init_i2c() {
  /* Initilize I2C */
  const i2c_config_t i2c_conf = {.mode = I2C_MODE_MASTER,
                                 .sda_io_num = I2C_SDA,
                                 .sda_pullup_en = GPIO_PULLUP_DISABLE,
                                 .scl_io_num = I2C_SCL,
                                 .scl_pullup_en = GPIO_PULLUP_DISABLE,
                                 .master.clk_speed = 400000};

  ESP_LOGI(TAG, "Initializing I2C for display touch");
  /* Initialize I2C */
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0));
}