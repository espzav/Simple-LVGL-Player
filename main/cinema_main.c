/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bsp/esp-bsp.h"
#include "esp_lvgl_simple_player.h"

#define APP_SUPPORT_USB_MOUSE   (1)

//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/video.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/noaudio.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_horizontal.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_vertical.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_vertical_1.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/christmas.mjpeg"
#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/christmas_low.mjpeg"

void app_main(void)
{
#if APP_SUPPORT_USB_MOUSE
    /* Initialize USB */
    bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true);
#endif
    /* Initialize SD card */
	bsp_sdcard_mount();
    /* Initialize display */
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
    lv_disp_t *display = bsp_display_start_with_config(&disp_cfg);
    bsp_display_backlight_on();

#if APP_SUPPORT_USB_MOUSE
    /* Initialize mouse */
    const lvgl_port_hid_mouse_cfg_t mouse_cfg = {
        .disp = display,
        .sensitivity = 1,
    };
    lvgl_port_add_usb_hid_mouse_input(&mouse_cfg);
#endif

    /* Create player */
    esp_lvgl_simple_player_cfg_t player_cfg = {
        .file = APP_VIDEO_FILE,
        .screen = lv_screen_active(),
        .screen_width = BSP_LCD_V_RES,
        .screen_height = (BSP_LCD_H_RES/2),
        .buff_size = 540*960,
    };
    lv_obj_t * player = esp_lvgl_simple_player_create(&player_cfg);
    
    
    if (player) {
        lvgl_port_lock(0);
        //lv_obj_set_size(player, BSP_LCD_V_RES, BSP_LCD_H_RES);
        lvgl_port_unlock();
    }
    
    /* Start playing */
    esp_lvgl_simple_player_play();
}
