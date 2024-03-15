/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Player configuration structure
 */
typedef struct {
    char        *file;      /* File path to play */
    lv_obj_t    *screen;    /* LVGL screen to put the player */
    uint32_t    buff_size;      /* Size of the buffer for one video frame */
    uint32_t    screen_width;   /* Width of the video player object */    
    uint32_t    screen_height;  /* Height of the video player object */
    struct {
        unsigned int hide_controls: 1;  /* Hide control buttons and slider */ 
    } flags;
} esp_lvgl_simple_player_cfg_t;

/**
 * @brief Create Player
 *
 * This function initializes video decoder (JPEG, ...), creates LVGL objects and starts handling task.
 *
 * @return 
 *      - ESP_OK                 On success
 */
esp_err_t esp_lvgl_simple_player_create(esp_lvgl_simple_player_cfg_t * params);

/**
 * @brief Change file for playing
 */
void esp_lvgl_simple_player_change_file(const char *file);

/**
 * @brief Play player
 */
void esp_lvgl_simple_player_play(void);

/**
 * @brief Pause player
 */
void esp_lvgl_simple_player_pause(void);

/**
 * @brief Stop player
 */
void esp_lvgl_simple_player_stop(void);

/**
 * @brief Set repeat playing
 */
void esp_lvgl_simple_player_repeat(bool repeat);

/**
 * @brief Delete Player
 *
 * @return 
 *      - ESP_OK                 On success
 */
esp_err_t esp_lvgl_simple_player_del(void);

#ifdef __cplusplus
}
#endif