// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.5.3
// LVGL version: 8.3.11
// Project name: PubRemote

#ifndef _PUBREMOTE_UI_H
#define _PUBREMOTE_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

#include "ui_events.h"
#include "ui_helpers.h"

  ///////////////////// SCREENS ////////////////////

#include "ui_AboutScreen.h"
#include "ui_CalibrationScreen.h"
#include "ui_ChargeScreen.h"
#include "ui_MenuScreen.h"
#include "ui_PairingScreen.h"
#include "ui_SettingsScreen.h"
#include "ui_SplashScreen.h"
#include "ui_StatsScreen.h"

  ///////////////////// VARIABLES ////////////////////

  // EVENTS

  extern lv_obj_t *ui____initial_actions0;

  // IMAGES AND IMAGE SETS
  LV_IMG_DECLARE(ui_img_icon2_png); // assets/icon2.png

  // FONTS
  LV_FONT_DECLARE(ui_font_Inter_14);
  LV_FONT_DECLARE(ui_font_Inter_28);
  LV_FONT_DECLARE(ui_font_Inter_Bold_14);
  LV_FONT_DECLARE(ui_font_Inter_Bold_28);
  LV_FONT_DECLARE(ui_font_Inter_Bold_48);
  LV_FONT_DECLARE(ui_font_Inter_Bold_96);

  // UI INIT
  void ui_init(void);
  void ui_destroy(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
