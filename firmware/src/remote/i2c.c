#include "i2c.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "PUBREMOTE-I2C";

#if !defined(PMIC_SDA) && !defined(TP_SDA)
  #define I2C_SDA -1
#elif defined(PMIC_SDA)
  #define I2C_SDA PMIC_SDA
#elif defined(TP_SDA)
  #define I2C_SDA TP_SDA
#endif

#if !defined(PMIC_SCL) && !defined(TP_SCL)
  #define I2C_SCL -1
#elif defined(PMIC_SCL)
  #define I2C_SCL PMIC_SCL
#elif defined(TP_SCL)
  #define I2C_SCL TP_SCL
#endif

#if defined(TP_SDA) && defined(TP_SCL) && defined(PMIC_SDA) && defined(PMIC_SCL) &&                                    \
    (TP_SDA != PMIC_SDA || TP_SCL != PMIC_SCL)
  #error                                                                                                               \
      "All I2C devices must share the same SDA and SCL pins. Please define either PMIC_SDA/PMIC_SCL or TP_SDA/TP_SCL, but not both."
#endif

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