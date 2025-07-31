/**
 * @file wifi_password_entry.h
 * @brief LVGL WiFi Password Entry Component
 *
 * A custom LVGL widget for secure password entry on touch screens.
 * Features include character navigation, dynamic field extension,
 * and touch-optimized controls.
 *
 * @author Your Name
 * @version 1.0
 */

#ifndef WIFI_PASSWORD_ENTRY_H
#define WIFI_PASSWORD_ENTRY_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define WIFI_PASSWORD_MIN_LENGTH 8   /**< Minimum required password length */
#define WIFI_PASSWORD_MAX_LENGTH 64  /**< Maximum allowed password length */
#define WIFI_PASSWORD_INITIAL_SIZE 8 /**< Initial number of character slots */
#define WIFI_PASSWORD_EXTEND_SIZE 2  /**< Number of slots added when extending */

  /**********************
   *      TYPEDEFS
   **********************/

  /**
   * @brief Callback function type for password completion
   * @param password The entered password string (null-terminated)
   */
  typedef void (*wifi_password_ready_cb_t)(const char *password);

  /**
   * @brief WiFi password entry widget class
   */
  extern const lv_obj_class_t wifi_password_class;

  /**********************
   * GLOBAL PROTOTYPES
   **********************/

  /**
   * @brief Create a WiFi password entry widget
   * @param parent Pointer to the parent object
   * @return Pointer to the created widget object
   *
   * @note The widget starts with 8 character positions and extends automatically
   *       when the user navigates past the last position.
   */
  lv_obj_t *wifi_password_create(lv_obj_t *parent);

  /**
   * @brief Get the current password string
   * @param obj Pointer to the WiFi password widget
   * @return Pointer to the password string (null-terminated)
   *
   * @note The returned string is valid until the widget is destroyed
   *       or the password is modified.
   */
  const char *wifi_password_get_password(lv_obj_t *obj);

  /**
   * @brief Example function to create a WiFi password screen
   * @param parent Pointer to the parent object (typically a screen)
   *
   * @note This is a convenience function that demonstrates basic usage.
   *       Remove or modify as needed for your application.
   */
  void create_wifi_password_screen(lv_obj_t *parent);

/**********************
 *      MACROS
 **********************/

/**
 * @brief Check if an object is a WiFi password widget
 * @param obj Pointer to the object to check
 * @return true if the object is a WiFi password widget, false otherwise
 */
#define WIFI_PASSWORD_IS_WIDGET(obj) (lv_obj_check_type(obj, &wifi_password_class))

/**
 * @brief Cast an object to a WiFi password widget
 * @param obj Pointer to the object to cast
 * @return Pointer to the WiFi password widget (no type checking)
 */
#define WIFI_PASSWORD_CAST(obj) ((wifi_password_t *)(obj))

/**********************
 *  USAGE EXAMPLE
 **********************/
#if 0
/*
 * Basic usage example:
 *
 * // Create the widget
 * lv_obj_t *wifi_screen = lv_scr_act();
 * lv_obj_t *pwd_widget = wifi_password_create(wifi_screen);
 * 
 * // Set up callback
 * wifi_password_set_ready_callback(pwd_widget, my_password_callback);
 * 
 * // Position the widget
 * lv_obj_center(pwd_widget);
 * 
 * // Callback implementation
 * void my_password_callback(const char *password) {
 *     printf("Connecting to WiFi with password: %s\n", password);
 *     // Add your WiFi connection code here
 *     wifi_connect("MyNetwork", password);
 * }
 */
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*WIFI_PASSWORD_ENTRY_H*/