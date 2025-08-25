#ifndef LV_CONF_H
#define LV_CONF_H

/* Núcleo */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* Resolução */
#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

/* Widgets que vamos usar */
#define LV_USE_LABEL     1
#define LV_USE_BTN       1
#define LV_USE_TEXTAREA  1
#define LV_USE_MSGBOX    1
#define LV_USE_TABVIEW   1
#define LV_USE_WIN       1

#endif /* LV_CONF_H */
