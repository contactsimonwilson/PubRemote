SET(SOURCES ui_SplashScreen.c
    ui_StatsScreen.c
    ui_SettingsScreen.c
    ui_CalibrationScreen.c
    ui_ConnectionScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_icon_png.c
    fonts/ui_font_Open_Sans_14.c
    fonts/ui_font_Open_Sans_Bold_14.c
    fonts/ui_font_Open_Sans_Bold_48.c)

add_library(ui ${SOURCES})
