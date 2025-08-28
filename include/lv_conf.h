#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), or 32 (ARGB8888)
#define LV_COLOR_DEPTH 16

// Maximal horizontal and vertical resolution
#define LV_HOR_RES_MAX          240
#define LV_VER_RES_MAX          320

// Default display refresh period
#define LV_DISP_DEF_REFR_PERIOD 30

// Enable GPU
#define LV_USE_GPU              0

// Enable file system
#define LV_USE_FS_STDIO         0

// Enable widgets
#define LV_USE_LABEL            1
#define LV_USE_BUTTON           1
#define LV_USE_SLIDER           1
#define LV_USE_TABLE            1
#define LV_USE_CHART            1
#define LV_USE_LINE             1
#define LV_USE_ARC              1

// Enable themes
#define LV_USE_THEME_DEFAULT    1

// Enable fonts - IMPORTANT: These must be enabled
#define LV_FONT_MONTSERRAT_8    1
#define LV_FONT_MONTSERRAT_10   1
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_30   1
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_34   1
#define LV_FONT_MONTSERRAT_36   1
#define LV_FONT_MONTSERRAT_38   1
#define LV_FONT_MONTSERRAT_40   1
#define LV_FONT_MONTSERRAT_42   1
#define LV_FONT_MONTSERRAT_44   1
#define LV_FONT_MONTSERRAT_46   1
#define LV_FONT_MONTSERRAT_48   1

// Enable animations
#define LV_USE_ANIMATION        1

// Enable logging
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_INFO

// Enable memory management
#define LV_MEM_CUSTOM          0
#define LV_MEM_SIZE            (32U * 1024U)

// Enable font loader
#define LV_USE_FONT_PLACEHOLDER 1

#endif
