#include "password_input.h"
#include <stdio.h>
#include <string.h>

#define FIELD_EXTEND_SIZE 2

typedef struct {
  lv_obj_t obj;                                    /**< Base LVGL object */
  char password[WIFI_PASSWORD_MAX_LENGTH + 1];     /**< Password string buffer */
  int current_pos;                                 /**< Current cursor position */
  int field_size;                                  /**< Current number of character slots */
  int char_indices[WIFI_PASSWORD_MAX_LENGTH];      /**< Character indices for each position */
  lv_obj_t *char_labels[WIFI_PASSWORD_MAX_LENGTH]; /**< Character display labels */
  lv_obj_t *container;                             /**< Main container */
  lv_obj_t *nav_left_btn;                          /**< Left navigation button */
  lv_obj_t *nav_right_btn;                         /**< Right navigation button */
  lv_obj_t *char_up_btn;                           /**< Character up button */
  lv_obj_t *char_down_btn;                         /**< Character down button */
} wifi_password_t;

static const char char_set[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
static const int char_set_size = sizeof(char_set) - 1;

static void wifi_password_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void wifi_password_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void wifi_password_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void update_display(wifi_password_t *wifi_pwd);
static void extend_field(wifi_password_t *wifi_pwd);
static void nav_left_event_handler(lv_event_t *e);
static void nav_right_event_handler(lv_event_t *e);
static void char_up_event_handler(lv_event_t *e);
static void char_down_event_handler(lv_event_t *e);

const lv_obj_class_t wifi_password_class = {.constructor_cb = wifi_password_constructor,
                                            .destructor_cb = wifi_password_destructor,
                                            .event_cb = wifi_password_event,
                                            .width_def = LV_PCT(100),      // Default to full width
                                            .height_def = LV_SIZE_CONTENT, // Content-based height
                                            .instance_size = sizeof(wifi_password_t),
                                            .base_class = &lv_obj_class};

lv_obj_t *wifi_password_create(lv_obj_t *parent) {
  lv_obj_t *obj = lv_obj_class_create_obj(&wifi_password_class, parent);
  lv_obj_class_init_obj(obj);
  return obj;
}

void wifi_password_set_size(lv_obj_t *obj, lv_coord_t width, lv_coord_t height) {
  LV_ASSERT_OBJ(obj, &wifi_password_class);
  lv_obj_set_size(obj, width, height);
}

void wifi_password_set_width(lv_obj_t *obj, lv_coord_t width) {
  LV_ASSERT_OBJ(obj, &wifi_password_class);
  lv_obj_set_width(obj, width);
}

const char *wifi_password_get_password(lv_obj_t *obj) {
  LV_ASSERT_OBJ(obj, &wifi_password_class);
  wifi_password_t *wifi_pwd = (wifi_password_t *)obj;
  return wifi_pwd->password;
}

void create_wifi_password_screen(lv_obj_t *parent) {
  lv_obj_t *wifi_pwd = wifi_password_create(parent);
  lv_obj_center(wifi_pwd);
}

static void wifi_password_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
  LV_UNUSED(class_p);
  wifi_password_t *wifi_pwd = (wifi_password_t *)obj;

  // Make the widget responsive by default - fill parent width
  lv_obj_set_size(obj, LV_PCT(100), LV_SIZE_CONTENT);

  // Initialize password data
  memset(wifi_pwd->password, 0, sizeof(wifi_pwd->password));
  wifi_pwd->current_pos = 0;
  wifi_pwd->field_size = WIFI_PASSWORD_INITIAL_SIZE;

  // Initialize character indices to blank (-1)
  for (int i = 0; i < WIFI_PASSWORD_MAX_LENGTH; i++) {
    wifi_pwd->char_indices[i] = -1; // -1 represents blank/empty
    wifi_pwd->char_labels[i] = NULL;
  }

  // Create main container
  wifi_pwd->container = lv_obj_create(obj);
  lv_obj_set_size(wifi_pwd->container, LV_PCT(100), LV_SIZE_CONTENT); // Fill parent width, content height
  lv_obj_set_flex_flow(wifi_pwd->container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(wifi_pwd->container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(wifi_pwd->container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(wifi_pwd->container, LV_OPA_0, 0);     // Transparent background
  lv_obj_set_style_border_opa(wifi_pwd->container, LV_OPA_0, 0); // No border
  // lv_obj_set_style_pad_all(wifi_pwd->container, 0, 0);           // Remove all outer padding
  // lv_obj_set_style_pad_gap(wifi_pwd->container, 2, 0);           // Small gap between password field and nav buttons

  // Create password display container
  lv_obj_t *pwd_container = lv_obj_create(wifi_pwd->container);
  lv_obj_set_size(pwd_container, LV_PCT(100), 35); // Use 95% of parent width, fixed height
  lv_obj_set_flex_flow(pwd_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(pwd_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER); // Distribute evenly
  lv_obj_clear_flag(pwd_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(pwd_container, 1, 0);
  lv_obj_set_style_border_color(pwd_container, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(pwd_container, 3, 0);
  lv_obj_set_style_bg_opa(pwd_container, LV_OPA_0, 0); // Transparent background
  lv_obj_set_style_pad_all(pwd_container, 4, 0);       // Smaller padding

  // Create initial character slots
  for (int i = 0; i < wifi_pwd->field_size; i++) {
    wifi_pwd->char_labels[i] = lv_label_create(pwd_container);
    lv_label_set_text(wifi_pwd->char_labels[i], "_"); // Start with underscore placeholder
    // lv_obj_set_style_text_font(wifi_pwd->char_labels[i], &ui_font_Inter_28, 0);
    lv_obj_set_style_text_align(wifi_pwd->char_labels[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_all(wifi_pwd->char_labels[i], 2, 0); // Small padding for touch area
    if (i == 0) {
      lv_obj_set_style_bg_color(wifi_pwd->char_labels[i], lv_color_hex(0x2196F3), 0);
      lv_obj_set_style_bg_opa(wifi_pwd->char_labels[i], LV_OPA_30, 0);
    }
  }

  // Create navigation button container
  lv_obj_t *nav_container = lv_obj_create(wifi_pwd->container);
  lv_obj_set_size(nav_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(nav_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(nav_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(nav_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(nav_container, LV_OPA_0, 0);     // Transparent background
  lv_obj_set_style_border_opa(nav_container, LV_OPA_0, 0); // No border
  lv_obj_set_style_pad_all(nav_container, 8, 0);           // Fixed 8px padding
  lv_obj_set_style_pad_gap(nav_container, 8, 0);           // Fixed 8px gap between buttons

  // Create character up button - fixed 30x30px
  wifi_pwd->char_up_btn = lv_btn_create(nav_container);
  lv_obj_set_size(wifi_pwd->char_up_btn, 30, 30);                            // Fixed 30x30px
  lv_obj_set_style_bg_opa(wifi_pwd->char_up_btn, LV_OPA_0, 0);               // Transparent background
  lv_obj_set_style_border_width(wifi_pwd->char_up_btn, 2, 0);                // 2px border
  lv_obj_set_style_border_color(wifi_pwd->char_up_btn, lv_color_white(), 0); // White border
  lv_obj_set_style_radius(wifi_pwd->char_up_btn, 4, 0);                      // Slightly rounded corners
  lv_obj_t *up_label = lv_label_create(wifi_pwd->char_up_btn);
  lv_label_set_text(up_label, LV_SYMBOL_UP);
  lv_obj_set_style_text_color(up_label, lv_color_white(), 0);      // White text
  lv_obj_set_style_text_font(up_label, &lv_font_montserrat_14, 0); // Montserrat 14 font
  lv_obj_center(up_label);
  lv_obj_add_event_cb(wifi_pwd->char_up_btn, char_up_event_handler, LV_EVENT_CLICKED, wifi_pwd);

  // Create character down button - fixed 30x30px
  wifi_pwd->char_down_btn = lv_btn_create(nav_container);
  lv_obj_set_size(wifi_pwd->char_down_btn, 30, 30);                            // Fixed 30x30px
  lv_obj_set_style_bg_opa(wifi_pwd->char_down_btn, LV_OPA_0, 0);               // Transparent background
  lv_obj_set_style_border_width(wifi_pwd->char_down_btn, 2, 0);                // 2px border
  lv_obj_set_style_border_color(wifi_pwd->char_down_btn, lv_color_white(), 0); // White border
  lv_obj_set_style_radius(wifi_pwd->char_down_btn, 4, 0);                      // Slightly rounded corners
  lv_obj_t *down_label = lv_label_create(wifi_pwd->char_down_btn);
  lv_label_set_text(down_label, LV_SYMBOL_DOWN);
  lv_obj_set_style_text_color(down_label, lv_color_white(), 0);      // White text
  lv_obj_set_style_text_font(down_label, &lv_font_montserrat_14, 0); // Montserrat 14 font
  lv_obj_center(down_label);
  lv_obj_add_event_cb(wifi_pwd->char_down_btn, char_down_event_handler, LV_EVENT_CLICKED, wifi_pwd);

  // Create left navigation button - fixed 30x30px
  wifi_pwd->nav_left_btn = lv_btn_create(nav_container);
  lv_obj_set_size(wifi_pwd->nav_left_btn, 30, 30);                            // Fixed 30x30px
  lv_obj_set_style_bg_opa(wifi_pwd->nav_left_btn, LV_OPA_0, 0);               // Transparent background
  lv_obj_set_style_border_width(wifi_pwd->nav_left_btn, 2, 0);                // 2px border
  lv_obj_set_style_border_color(wifi_pwd->nav_left_btn, lv_color_white(), 0); // White border
  lv_obj_set_style_radius(wifi_pwd->nav_left_btn, 4, 0);                      // Slightly rounded corners
  lv_obj_t *left_label = lv_label_create(wifi_pwd->nav_left_btn);
  lv_label_set_text(left_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(left_label, lv_color_white(), 0);      // White text
  lv_obj_set_style_text_font(left_label, &lv_font_montserrat_14, 0); // Montserrat 14 font
  lv_obj_center(left_label);
  lv_obj_add_event_cb(wifi_pwd->nav_left_btn, nav_left_event_handler, LV_EVENT_CLICKED, wifi_pwd);

  // Create right navigation button - fixed 30x30px
  wifi_pwd->nav_right_btn = lv_btn_create(nav_container);
  lv_obj_set_size(wifi_pwd->nav_right_btn, 30, 30);                            // Fixed 30x30px
  lv_obj_set_style_bg_opa(wifi_pwd->nav_right_btn, LV_OPA_0, 0);               // Transparent background
  lv_obj_set_style_border_width(wifi_pwd->nav_right_btn, 2, 0);                // 2px border
  lv_obj_set_style_border_color(wifi_pwd->nav_right_btn, lv_color_white(), 0); // White border
  lv_obj_set_style_radius(wifi_pwd->nav_right_btn, 4, 0);                      // Slightly rounded corners
  lv_obj_t *right_label = lv_label_create(wifi_pwd->nav_right_btn);
  lv_label_set_text(right_label, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(right_label, lv_color_white(), 0);      // White text
  lv_obj_set_style_text_font(right_label, &lv_font_montserrat_14, 0); // Montserrat 14 font
  lv_obj_center(right_label);
  lv_obj_add_event_cb(wifi_pwd->nav_right_btn, nav_right_event_handler, LV_EVENT_CLICKED, wifi_pwd);

  update_display(wifi_pwd);
}

static void wifi_password_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
  LV_UNUSED(class_p);
  LV_UNUSED(obj);
  // Cleanup handled by LVGL automatically
}

static void wifi_password_event(const lv_obj_class_t *class_p, lv_event_t *e) {
  LV_UNUSED(class_p);
  LV_UNUSED(e);
  // Handle any widget-level events here if needed
}

static void update_display(wifi_password_t *wifi_pwd) {
  // Update password display - show actual characters in all positions
  for (int i = 0; i < wifi_pwd->field_size; i++) {
    if (wifi_pwd->char_labels[i]) {
      // Clear previous highlighting
      lv_obj_set_style_bg_opa(wifi_pwd->char_labels[i], LV_OPA_0, 0);

      // Update the character display
      if (wifi_pwd->char_indices[i] == -1) {
        // Show underscore for blank positions
        lv_label_set_text(wifi_pwd->char_labels[i], "_");
      }
      else {
        // Show actual character
        char current_char[2] = {char_set[wifi_pwd->char_indices[i]], '\0'};
        lv_label_set_text(wifi_pwd->char_labels[i], current_char);
      }

      // Highlight current position
      if (i == wifi_pwd->current_pos) {
        lv_obj_set_style_bg_color(wifi_pwd->char_labels[i], lv_color_hex(0x2196F3), 0);
        lv_obj_set_style_bg_opa(wifi_pwd->char_labels[i], LV_OPA_30, 0);
      }
    }
  }

  // Update password string (only include non-blank characters)
  int pwd_index = 0;
  for (int i = 0; i < wifi_pwd->field_size; i++) {
    if (wifi_pwd->char_indices[i] != -1 && pwd_index < WIFI_PASSWORD_MAX_LENGTH) {
      wifi_pwd->password[pwd_index] = char_set[wifi_pwd->char_indices[i]];
      pwd_index++;
    }
  }
  wifi_pwd->password[pwd_index] = '\0';

  // Update navigation button states
  lv_obj_clear_state(wifi_pwd->nav_left_btn, LV_STATE_DISABLED);
  lv_obj_clear_state(wifi_pwd->nav_right_btn, LV_STATE_DISABLED);

  if (wifi_pwd->current_pos == 0) {
    lv_obj_add_state(wifi_pwd->nav_left_btn, LV_STATE_DISABLED);
  }
}

static void extend_field(wifi_password_t *wifi_pwd) {
  if (wifi_pwd->field_size >= WIFI_PASSWORD_MAX_LENGTH)
    return;

  lv_obj_t *pwd_container = lv_obj_get_child(wifi_pwd->container, 0); // Password container is now first child

  // Add new character slots
  int new_size = wifi_pwd->field_size + WIFI_PASSWORD_EXTEND_SIZE;
  if (new_size > WIFI_PASSWORD_MAX_LENGTH) {
    new_size = WIFI_PASSWORD_MAX_LENGTH;
  }

  for (int i = wifi_pwd->field_size; i < new_size; i++) {
    wifi_pwd->char_labels[i] = lv_label_create(pwd_container);
    lv_label_set_text(wifi_pwd->char_labels[i], "_"); // Start with underscore for blank
    // lv_obj_set_style_text_font(wifi_pwd->char_labels[i], &ui_font_Inter_28, 0);
    lv_obj_set_style_pad_hor(wifi_pwd->char_labels[i], 1, 0); // Reduced horizontal padding
    lv_obj_set_style_text_align(wifi_pwd->char_labels[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(wifi_pwd->char_labels[i], 18); // Smaller width for denser spacing
    wifi_pwd->char_indices[i] = -1;                 // Initialize to blank
  }

  wifi_pwd->field_size = new_size;
}

static void nav_left_event_handler(lv_event_t *e) {
  wifi_password_t *wifi_pwd = (wifi_password_t *)lv_event_get_user_data(e);

  if (wifi_pwd->current_pos > 0) {
    wifi_pwd->current_pos--;
    update_display(wifi_pwd);
  }
}

static void nav_right_event_handler(lv_event_t *e) {
  wifi_password_t *wifi_pwd = (wifi_password_t *)lv_event_get_user_data(e);

  // If at the last position, extend the field
  if (wifi_pwd->current_pos == wifi_pwd->field_size - 1) {
    extend_field(wifi_pwd);
  }

  if (wifi_pwd->current_pos < wifi_pwd->field_size - 1) {
    wifi_pwd->current_pos++;
    update_display(wifi_pwd);
  }
}

static void char_up_event_handler(lv_event_t *e) {
  wifi_password_t *wifi_pwd = (wifi_password_t *)lv_event_get_user_data(e);

  wifi_pwd->char_indices[wifi_pwd->current_pos]++;
  if (wifi_pwd->char_indices[wifi_pwd->current_pos] >= char_set_size) {
    wifi_pwd->char_indices[wifi_pwd->current_pos] = -1; // Wrap to blank
  }

  update_display(wifi_pwd);
}

static void char_down_event_handler(lv_event_t *e) {
  wifi_password_t *wifi_pwd = (wifi_password_t *)lv_event_get_user_data(e);

  wifi_pwd->char_indices[wifi_pwd->current_pos]--;
  if (wifi_pwd->char_indices[wifi_pwd->current_pos] < -1) {
    wifi_pwd->char_indices[wifi_pwd->current_pos] = char_set_size - 1; // Wrap to last character
  }

  update_display(wifi_pwd);
}