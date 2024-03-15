/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <fcntl.h>
#include <dirent.h>
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "esp_lvgl_simple_player.h"

static const char *TAG = "APP";
#define APP_SUPPORT_USB_MOUSE   (1)
#define APP_SUPPORT_FILE_EXT    ".mjpeg"


//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/video.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/noaudio.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_horizontal.mjpeg"
#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_vertical.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_vertical_1.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/christmas.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/christmas_low.mjpeg"

static char file_path[50] = "";

static void app_get_video_files(char * buff, uint32_t size)
{
    uint32_t len = 0;
    struct dirent *dir;
    DIR *d;

    /* Open directory */
    d = opendir(BSP_SD_MOUNT_POINT);
    if (d != NULL) {
        /* Show button in the list for file of directory (Note: Directories are not supported in SPIFFS) */
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type != DT_DIR) {
                if (strstr(dir->d_name, APP_SUPPORT_FILE_EXT) != NULL) {
                    len += snprintf(buff+len, size-len, "%s\n", dir->d_name);
                }
            }
        }

        closedir(d);
    }
}

static void file_changed(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t len = snprintf(file_path, sizeof(file_path), "%s/", BSP_SD_MOUNT_POINT);
        lv_dropdown_get_selected_str(obj, file_path+len, sizeof(file_path)-len);
        esp_lvgl_simple_player_change_file(file_path);
        esp_lvgl_simple_player_stop();
    }
}

static void app_show_ui(void)
{
    /* Create LVGL objects */
    lvgl_port_lock(0);
    
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    
    /* Rows */
    lv_obj_t *cont_col = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont_col, BSP_LCD_V_RES, BSP_LCD_H_RES);
    lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_col, 0, 0);
    lv_obj_set_flex_align(cont_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_col, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont_col, 0, 0);
   
    char files[300] = "";
    app_get_video_files(&files, sizeof(files));
    
    /* Dropdown files */
    lv_obj_t * dd = lv_dropdown_create(cont_col);
    lv_obj_set_width(dd, BSP_LCD_V_RES/2);
    lv_dropdown_set_options(dd, files);
    lv_obj_add_event_cb(dd, file_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_pad_top(dd, 5, 0);

    /* Create player */
    esp_lvgl_simple_player_cfg_t player_cfg = {
        .file = APP_VIDEO_FILE,
        .screen = cont_col,
        .screen_width = BSP_LCD_V_RES,
        .screen_height = (BSP_LCD_H_RES/2),
        .buff_size = 540*960,
        .flags = {
            .auto_height = true,
        }
    };
    lv_obj_t * player = esp_lvgl_simple_player_create(&player_cfg);
    
    /* Start playing */
    esp_lvgl_simple_player_play();
    
    lvgl_port_unlock();
}

void app_main(void)
{
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
    /* Initialize USB */
    bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true);
    /* Initialize mouse */
    const lvgl_port_hid_mouse_cfg_t mouse_cfg = {
        .disp = display,
        .sensitivity = 1,
    };
    lvgl_port_add_usb_hid_mouse_input(&mouse_cfg);
#endif

    app_show_ui();
}
