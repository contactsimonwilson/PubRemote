#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/ledc_types.h"
#include "lvgl.h"
#include "powermanagement.h"
#include "settings.h"
#include "ui/ui.h"
#include <stdio.h>

// https://github.com/espressif/esp-idf/blob/master/examples/peripherals/lcd/spi_lcd_touch/main/spi_lcd_touch_example_main.c
// https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_gc9a01
// I2C touch controller
// https://github.com/krupis/T-Display-S3-esp-idf/blob/923162ab67efe6f867ab1a3cdce19fe127c5c493/components/lvgl_setup/lvgl_setup.c#L255

#if USE_GC9A01
  #include "esp_lcd_gc9a01.h"
#endif

#if USE_CST816S
  #include "esp_lcd_touch_cst816s.h"
#endif

#ifndef DISP_BL_PWM
  #define DISP_BL_PWM 0
#endif

static const char *TAG = "PUBMOTE_DISPLAY";

#define LCD_HOST SPI2_HOST
#define TP_I2C_NUM 0
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)

#if USE_CST816S
  #define TOUCH_ENABLED 1
#else
  #define TOUCH_ENABLED 0
#endif

// Bit number used to represent command and parameter
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

// LVGL
#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY 2

static SemaphoreHandle_t lvgl_mux = NULL;

#if TOUCH_ENABLED
esp_lcd_touch_handle_t tp = NULL;
#endif

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx) {
  lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
  lv_disp_flush_ready(disp_driver);
  return false;
}

static void LVGL_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;
  // copy a buffer's content to a specific area of the display
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void LVGL_port_update_callback(lv_disp_drv_t *drv) {
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

  switch (drv->rotated) {
  case LV_DISP_ROT_NONE:
    // Rotate LCD display
    esp_lcd_panel_swap_xy(panel_handle, false);
    esp_lcd_panel_mirror(panel_handle, true, false);
#if TOUCH_ENABLED
    // Rotate LCD touch
    esp_lcd_touch_set_mirror_y(tp, false);
    esp_lcd_touch_set_mirror_x(tp, false);
#endif
    break;
  case LV_DISP_ROT_90:
    // Rotate LCD display
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, true);
#if TOUCH_ENABLED
    // Rotate LCD touch
    esp_lcd_touch_set_mirror_y(tp, false);
    esp_lcd_touch_set_mirror_x(tp, false);
#endif
    break;
  case LV_DISP_ROT_180:
    // Rotate LCD display
    esp_lcd_panel_swap_xy(panel_handle, false);
    esp_lcd_panel_mirror(panel_handle, false, true);
#if TOUCH_ENABLED
    // Rotate LCD touch
    esp_lcd_touch_set_mirror_y(tp, false);
    esp_lcd_touch_set_mirror_x(tp, false);
#endif
    break;
  case LV_DISP_ROT_270:
    // Rotate LCD display
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, false, false);
#if TOUCH_ENABLED
    // Rotate LCD touch
    esp_lcd_touch_set_mirror_y(tp, false);
    esp_lcd_touch_set_mirror_x(tp, false);
#endif
    break;
  }
}

#if TOUCH_ENABLED
static void LVGL_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  /* Read touch controller data */
  esp_lcd_touch_read_data(drv->user_data);

  /* Get coordinates */
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
#endif

static void increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool LVGL_lock(int timeout_ms) {
  // Convert timeout in milliseconds to FreeRTOS ticks
  // If `timeout_ms` is set to -1, the program will block until the condition is met
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void LVGL_unlock(void) {
  xSemaphoreGiveRecursive(lvgl_mux);
}

static void LVGL_port_task(void *arg) {
  ESP_LOGI(TAG, "Starting LVGL task");
  uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
  while (1) {
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (LVGL_lock(-1)) {
      task_delay_ms = lv_timer_handler();
      // Release the mutex
      LVGL_unlock();
    }
    if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    }
    else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
      task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}

void set_bl_level(u_int8_t level) {
#if DISP_BL_PWM
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, level);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
#else
  gpio_set_level(DISP_BL, level > 0 ? 1 : 0);
#endif
}

static void init_backlight(void) {
#if DISP_BL_PWM
  ESP_LOGI(TAG, "Configure LCD backlight");
  // Configure PWM channel
  ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
      .duty_resolution = LEDC_TIMER_8_BIT, // Resolution up to 256
      .freq_hz = 1000,                     // 1 kHz frequency
  };
  ledc_timer_config(&timer_config);

  ledc_channel_config_t channel_config = {
      .gpio_num = DISP_BL,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
  };
  ledc_channel_config(&channel_config);
#else
  gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << DISP_BL};
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
}

bool my_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  /* Read touch controller data */
  esp_lcd_touch_read_data(drv->user_data);

  /* Get coordinates */
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PR;

    // ESP_LOGI(TAG, "Touch event at X: %d, Y: %d\n", data->point.x, data->point.y);
    start_or_reset_deep_sleep_timer();
  }
  else {
    data->state = LV_INDEV_STATE_REL;
  }

  return false;
}

void init_display(void) {
  static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
  static lv_disp_drv_t disp_drv;      // contains callback functions

  ESP_LOGI(TAG, "Turn off LCD backlight");
  init_backlight();
  set_bl_level(0);

  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num = DISP_CLK,
      .mosi_io_num = DISP_MOSI,
      .miso_io_num = DISP_MISO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = LV_HOR_RES * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = DISP_DC,
      .cs_gpio_num = DISP_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
      .spi_mode = 0,
      .trans_queue_depth = 10,
      .on_color_trans_done = notify_lvgl_flush_ready,
      .user_ctx = &disp_drv,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = DISP_RST,
      .rgb_endian = LCD_RGB_ENDIAN_BGR,
      .bits_per_pixel = 16,
  };

#if USE_GC9A01
  ESP_LOGI(TAG, "Install GC9A01 panel driver");
  ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
#endif

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

#if USE_GC9A01
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

  // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if TOUCH_ENABLED
  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  #if USE_CST816S

  i2c_config_t i2c_conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = TP_SDA,
      .scl_io_num = TP_SCL,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 400000,
  };
  ESP_LOGI(TAG, "Initializing I2C for display touch");
  /* Initialize I2C */
  ESP_ERROR_CHECK(i2c_param_config(TP_I2C_NUM, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(TP_I2C_NUM, i2c_conf.mode, 0, 0, 0));

  i2c_cmd_handle_t cmd;
  for (int i = 0; i < 0x7f; i++) {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(TP_I2C_NUM, cmd, portMAX_DELAY) == ESP_OK) {
      ESP_LOGW("I2C_TEST", "%02X", i);
    }
    i2c_cmd_link_delete(cmd);
  }

  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
  // Attach the TOUCH to the I2C bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TP_I2C_NUM, &tp_io_config, &tp_io_handle));

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = LV_HOR_RES,
      .y_max = LV_VER_RES,
      .rst_gpio_num = TP_RST,
      .int_gpio_num = TP_INT,
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
  };

  ESP_LOGI(TAG, "Initialize touch controller CST816S");
  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &tp));
  #endif
#endif // TOUCH_ENABLED

  ESP_LOGI(TAG, "Initialize LVGL library");
  lv_init();
  // alloc draw buffers used by LVGL
  // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
  lv_color_t *buf1 = heap_caps_malloc(LV_HOR_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf1);
  lv_color_t *buf2 = heap_caps_malloc(LV_HOR_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  assert(buf2);
  // initialize LVGL draw buffers
  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LV_HOR_RES * 20);

  ESP_LOGI(TAG, "Register display driver to LVGL");
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LV_HOR_RES;
  disp_drv.ver_res = LV_VER_RES;
  disp_drv.flush_cb = LVGL_flush_cb;
  disp_drv.drv_update_cb = LVGL_port_update_callback;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.user_data = panel_handle;
  lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

#ifdef DISP_ROTATE
  lv_disp_set_rotation(disp, DISP_ROTATE);
#endif

  ESP_LOGI(TAG, "Install LVGL tick timer");
  // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
  const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &increase_lvgl_tick, .name = "lvgl_tick"};
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

#if TOUCH_ENABLED
  static lv_indev_drv_t indev_drv; // Input device driver (Touch)
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.disp = disp;
  indev_drv.read_cb = my_input_read; // Use the my_input_read function
  indev_drv.user_data = tp;

  lv_indev_drv_register(&indev_drv);
#endif

  lvgl_mux = xSemaphoreCreateRecursiveMutex();
  assert(lvgl_mux);
  ESP_LOGI(TAG, "Create LVGL task");
  xTaskCreate(LVGL_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);

  ESP_LOGI(TAG, "Display UI");
  // Lock the mutex due to the LVGL APIs are not thread-safe
  if (LVGL_lock(-1)) {
    ui_init();
    // Release the mutex
    LVGL_unlock();

    // Delay backlight turn on to avoid flickering
    vTaskDelay(pdMS_TO_TICKS(200));
    set_bl_level(settings.bl_level);
  }
}