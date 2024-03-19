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
#define APP_SUPPORT_USB_MOUSE    (1)
#define APP_SUPPORT_USB_KEYBOARD (1)
#define APP_SUPPORT_FILE_EXT     ".mjpeg"

// LVGL image declare
LV_IMG_DECLARE(breaking_news)

#define APP_VIDEO_FILE "01_P4_vertical_540x960.mjpeg"
#define APP_VIDEO_FILE_PATH BSP_SD_MOUNT_POINT"/"APP_VIDEO_FILE
#define APP_BREAKING_NEWS_TEXT  "New ESP32P4 chip is here! This demo was made with ESP-BSP and LVGL port (with LVGL9). *** Demo can be downloaded here: https://github.com/espzav/Simple-LVGL-Player ***"

static char file_path[50] = "";
static char breaking_news_text[500] = {APP_BREAKING_NEWS_TEXT};
static lv_obj_t * img_breaking_news;
static int sel_file = 0;

static void app_get_video_files(char * buff, uint32_t size)
{
    int i = 0;
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
                    if (strcmp(dir->d_name, APP_VIDEO_FILE) == 0) {
                        sel_file = i;
                    }
                }
            }
            i++;
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

static void breaking_news_changed(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        if (img_breaking_news) {
            const bool state = (lv_obj_get_state(obj) & LV_STATE_CHECKED);
            if (state) {
                lv_obj_remove_flag(img_breaking_news, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(img_breaking_news, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void hide_controls_changed(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        const bool state = (lv_obj_get_state(obj) & LV_STATE_CHECKED);
        esp_lvgl_simple_player_hide_controls(state);
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
    app_get_video_files(files, sizeof(files));
    
    lv_obj_t *cont_row = lv_obj_create(cont_col);
    lv_obj_set_size(cont_row, BSP_LCD_V_RES - 20, 80);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_row, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont_row, 0, 0);
    
    /* Dropdown files */
    lv_obj_t * dd = lv_dropdown_create(cont_row);
    lv_obj_set_width(dd, BSP_LCD_V_RES/3);
    lv_dropdown_set_options(dd, files);
    lv_obj_add_event_cb(dd, file_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_dropdown_set_selected(dd, sel_file);
    lv_obj_set_style_pad_top(dd, 5, 0);
    
    /* Checkbox - breaking news */
    lv_obj_t * cb = lv_checkbox_create(cont_row);
    lv_checkbox_set_text(cb, "BREAKING NEWS");
    lv_obj_add_state(cb, LV_STATE_CHECKED);
    lv_obj_set_style_text_color(cb, lv_color_white(), 0);
    lv_obj_add_event_cb(cb, breaking_news_changed, LV_EVENT_VALUE_CHANGED, NULL);
    
    /* Checkbox - hide controls */
    cb = lv_checkbox_create(cont_row);
    lv_checkbox_set_text(cb, "HIDE CONTROLS");
    lv_obj_set_style_text_color(cb, lv_color_white(), 0);
    lv_obj_add_event_cb(cb, hide_controls_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* Create player */
    esp_lvgl_simple_player_cfg_t player_cfg = {
        .file = APP_VIDEO_FILE_PATH,
        .screen = cont_col,
        .screen_width = BSP_LCD_V_RES,
        .screen_height = (BSP_LCD_H_RES/2),
        .buff_size = 540*960,
        .flags = {
            .auto_height = true,
        }
    };
    esp_lvgl_simple_player_create(&player_cfg);
    
    /* Breaking news image */
    img_breaking_news = lv_img_create(lv_screen_active());
    lv_img_set_src(img_breaking_news, &breaking_news);
    lv_obj_align(img_breaking_news, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    lv_obj_t * label = lv_label_create(img_breaking_news);
    lv_obj_set_width(label, BSP_LCD_V_RES - 155);
    lv_label_set_text(label, breaking_news_text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label, LV_ALIGN_BOTTOM_RIGHT, -40, -25);
    
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

#if (APP_SUPPORT_USB_MOUSE || APP_SUPPORT_USB_KEYBOARD)
    /* Initialize USB */
    bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true);
#endif

#if APP_SUPPORT_USB_MOUSE
    /* Initialize mouse */
    const lvgl_port_hid_mouse_cfg_t mouse_cfg = {
        .disp = display,
        .sensitivity = 1,
    };
    lvgl_port_add_usb_hid_mouse_input(&mouse_cfg);
#endif


#if APP_SUPPORT_USB_KEYBOARD
    /* Initialize keyboard */
    const lvgl_port_hid_keyboard_cfg_t kb_cfg = {
        .disp = display,
    };
    lvgl_port_add_usb_hid_keyboard_input(&kb_cfg);
#endif

    app_show_ui();
}
