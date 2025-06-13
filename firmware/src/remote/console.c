
#include "esp_console.h"
#include "esp_log.h"
#include "powermanagement.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>

// https://github.com/espressif/esp-idf/blob/master/examples/system/console/basic/main/console_example_main.c

static const char *TAG = "PUBREMOTE-CONSOLE";
#define PROMPT_STR "pubconsole"

#ifndef RELEASE_VARIANT
  #define RELEASE_VARIANT "dev"
#endif

static int get_version() {
  printf("Version: %d.%d.%d.%s \n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, RELEASE_VARIANT);
  printf("Variant: %s\n", RELEASE_VARIANT);
  printf("Build date: %s %s\n", __DATE__, __TIME__);
  printf("Build ID: %s\n", BUILD_ID);
  return 0;
}

static void register_version_command() {
  esp_console_cmd_t cmd = {
      .command = "version",
      .help = "Get information about the current firmware build.\n"
              "This command returns the version number, build date, and build ID.",
      .hint = NULL,
      .func = &get_version,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int reboot_command() {
  ESP_LOGI(TAG, "Rebooting...");
  esp_restart();
  return 0;
}

static void register_reboot_command() {
  esp_console_cmd_t cmd = {
      .command = "reboot",
      .help = "Reboot the device.",
      .hint = NULL,
      .func = &reboot_command,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int shutdown_command() {
  enter_sleep();
  return 0;
}

static void register_shutdown_command() {
  esp_console_cmd_t cmd = {
      .command = "shutdown",
      .help = "Shutdown the device and enter sleep mode.",
      .hint = NULL,
      .func = &shutdown_command,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int erase_command() {
  ESP_LOGI(TAG, "Erasing flash memory...");
  esp_err_t err = reset_all_settings();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to erase flash: %s", esp_err_to_name(err));
    return -1;
  }
  ESP_LOGI(TAG, "Flash memory erased successfully.");
  esp_restart();
  return 0;
}

static void register_erase_command() {
  esp_console_cmd_t cmd = {
      .command = "erase",
      .help = "Erase the flash memory.",
      .hint = NULL,
      .func = &erase_command,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void init_console() {
  ESP_LOGI(TAG, "Initializing console");
  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  /* Prompt to be printed before each line.
   * This can be customized, made dynamic, etc.
   */
  repl_config.prompt = PROMPT_STR ">";
  /* Register commands */
  esp_console_register_help_command();
  register_version_command();
  register_reboot_command();
  register_shutdown_command();
  register_erase_command();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
  esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
  #error Unsupported console type
#endif

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
  ESP_LOGI(TAG, "Console initialized");
}