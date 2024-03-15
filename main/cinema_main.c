/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bsp/esp-bsp.h"
#include "esp_lvgl_simple_player.h"

//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/video.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/noaudio.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_horizontal.mjpeg"
#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_vertical.mjpeg"

void app_main(void)
{
	bsp_sdcard_mount();
    bsp_display_cfg_t disp_cfg = {
        .lvgl_port_cfg = {
            .task_priority = 2,
            .task_stack = 4096,
            .task_affinity = 1,
            .timer_period_ms = 5,
            .task_max_sleep_ms = 1,
        },
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&disp_cfg);
    bsp_display_backlight_on();
	
    /* Create player */
    esp_lvgl_simple_player_cfg_t player_cfg = {
        .file = APP_VIDEO_FILE,
        .screen = lv_screen_active(),
        .screen_width = BSP_LCD_V_RES,
        .screen_height = (BSP_LCD_H_RES/2),
        .buff_size = 800*450,
    };
    esp_lvgl_simple_player_create(&player_cfg);
    
    /* Start playing */
    esp_lvgl_simple_player_play();
}
