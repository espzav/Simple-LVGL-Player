/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/jpeg_decode.h"
#include "esp_lvgl_port.h"
#include "media_src_storage.h"
#include "esp_lvgl_simple_player.h"

#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))

static const char *TAG = "PLAYER";
static const uint16_t EOI = 0xd9ff; /* End of image */

typedef struct
{
    char                    *file_path;
    media_src_t             file;
    uint64_t                filesize;
    jpeg_decoder_handle_t   jpeg;
    
    uint32_t    screen_width;   /* Width of the video player object */    
    uint32_t    screen_height;  /* Height of the video player object */
    uint32_t    video_width;      /* Maximum width of the video */
    uint32_t    video_height;     /* Maximum height of the video  */
    
    player_state_t  state;
    bool            loop;
    bool            hide_controls;
    bool            hide_slider;
    bool            hide_status;
    bool            auto_width;
    bool            auto_height;
    
    /* Buffers */
    uint8_t     *in_buff;
    uint32_t    in_buff_size;
    uint8_t     *out_buff;
    uint32_t    out_buff_size;
    
    /* LVGL objects */
    lv_obj_t    *main;
    lv_obj_t    *canvas;
    lv_obj_t    *slider;
    lv_obj_t    *btn_play;
    lv_obj_t    *btn_pause;
    lv_obj_t    *btn_stop;
    lv_obj_t    *btn_repeat;
    lv_obj_t    *img_pause;
    lv_obj_t    *img_stop;
    lv_obj_t    *controls;
} player_ctx_t;

static player_ctx_t player_ctx;

    
static const jpeg_decode_cfg_t jpeg_decode_cfg = {
    .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
    .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
};



static void play_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        esp_lvgl_simple_player_play();
    }
}

static void stop_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        esp_lvgl_simple_player_stop();
    }
}

static void pause_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        if(player_ctx.state == PLAYER_STATE_PAUSED)
            esp_lvgl_simple_player_play();
        else
            esp_lvgl_simple_player_pause();
    }
}

static void repeat_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        bool loop = lv_obj_get_state(obj) & LV_STATE_CHECKED ? true : false;
        esp_lvgl_simple_player_repeat(loop);
    }
}

static lv_obj_t * create_lvgl_objects(lv_obj_t * screen)
{
    /* Create LVGL objects */
    lvgl_port_lock(0);
    
    /* Rows */
    lv_obj_t *cont_col = lv_obj_create(screen);
    lv_obj_set_size(cont_col, player_ctx.screen_width, player_ctx.screen_height);
    lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_col, 0, 0);
    lv_obj_set_flex_align(cont_col, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_col, lv_color_black(), 0);
    lv_obj_remove_flag(cont_col, LV_OBJ_FLAG_SCROLLABLE);
    player_ctx.main = cont_col;
    
    /* Video canvas */
    player_ctx.canvas = lv_canvas_create(cont_col);
    lv_obj_add_event_cb(player_ctx.canvas, pause_event_cb, LV_EVENT_CLICKED, NULL);
    
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(cont_col);
    lv_obj_set_size(slider, player_ctx.screen_width, 5);
    lv_obj_add_state(slider, LV_STATE_DISABLED);
    lv_obj_set_style_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
    player_ctx.slider = slider;
    
    /* Buttons */
    lv_obj_t *cont_row = lv_obj_create(cont_col);
    lv_obj_set_size(cont_row, player_ctx.screen_width - 20, 80);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_row, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont_row, 0, 0);
    player_ctx.controls = cont_row;
    
    /* Play button */
    lv_obj_t * play_btn = lv_btn_create(cont_row);
    lv_obj_t * label = lv_label_create(play_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PLAY);
    lv_obj_add_event_cb(play_btn, play_event_cb, LV_EVENT_CLICKED, NULL);
    player_ctx.btn_play = play_btn;
    
    /* Pause button */
    lv_obj_t * pause_btn = lv_btn_create(cont_row);
    label = lv_label_create(pause_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PAUSE);
    lv_obj_add_event_cb(pause_btn, pause_event_cb, LV_EVENT_CLICKED, NULL);
    player_ctx.btn_pause = pause_btn;

    /* Stop button */
    lv_obj_t * stop_btn = lv_btn_create(cont_row);
    label = lv_label_create(stop_btn);
    lv_label_set_text_static(label, LV_SYMBOL_STOP);
    lv_obj_add_event_cb(stop_btn, stop_event_cb, LV_EVENT_CLICKED, NULL);
    player_ctx.btn_stop = stop_btn;

    /* Repeat button */
    lv_obj_t * repeat_btn = lv_btn_create(cont_row);
    lv_obj_add_flag(repeat_btn, LV_OBJ_FLAG_CHECKABLE);
    label = lv_label_create(repeat_btn);
    lv_label_set_text_static(label, LV_SYMBOL_REFRESH);
    lv_obj_add_event_cb(repeat_btn, repeat_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    player_ctx.btn_repeat = repeat_btn;
    
    /* Pause image */
    lv_obj_t * img_pause = lv_label_create(player_ctx.canvas);
    lv_obj_set_style_text_font(img_pause, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(img_pause, lv_color_white(), 0);
    lv_label_set_text_static(img_pause, LV_SYMBOL_PAUSE);
    lv_obj_center(img_pause);
    lv_obj_add_flag(img_pause, LV_OBJ_FLAG_HIDDEN);
    player_ctx.img_pause = img_pause;
    
    /* Stop image */
    lv_obj_t * img_stop = lv_label_create(player_ctx.canvas);
    lv_obj_set_style_text_font(img_stop, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(img_stop, lv_color_white(), 0);
    lv_label_set_text_static(img_stop, LV_SYMBOL_STOP);
    lv_obj_center(img_stop);
    lv_obj_add_flag(img_stop, LV_OBJ_FLAG_HIDDEN);
    player_ctx.img_stop = img_stop;
    
    /* Hide control buttons */
    if (player_ctx.hide_controls) {
        lv_obj_add_flag(cont_row, LV_OBJ_FLAG_HIDDEN);
    }
    /* Hide slider */
    if (player_ctx.hide_slider) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN);
    }
    /* Hide status icons */
    if (player_ctx.hide_status) {
        lv_obj_add_flag(img_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img_stop, LV_OBJ_FLAG_HIDDEN);
    }
        
    lvgl_port_unlock();
    
    return cont_col;
}

static esp_err_t get_video_size(uint32_t * width, uint32_t * height)
{
    esp_err_t err;
    jpeg_decode_picture_info_t header;
    assert(width && height);
    
    int size = media_src_storage_read(&player_ctx.file, player_ctx.in_buff, player_ctx.in_buff_size);
    if(size < 0)
        return ESP_ERR_INVALID_SIZE;
    
    err = jpeg_decoder_get_info(player_ctx.in_buff, size, &header);
    
    *width = header.width;
    *height = header.height;
    
    return err;
}

static esp_err_t video_decoder_init(void)
{
    jpeg_decode_engine_cfg_t engine_cfg = {
        .intr_priority = 0,
        .timeout_ms = 50,
    };
    return jpeg_new_decoder_engine(&engine_cfg, &player_ctx.jpeg);
}

static void video_decoder_deinit(void)
{
    if (player_ctx.jpeg) {
        jpeg_del_decoder_engine(player_ctx.jpeg);
    }
}

static uint8_t * video_decoder_malloc(uint32_t size)
{
    return (uint8_t *)jpeg_alloc_decoder_mem(size);
}

static int video_decoder_decode(void)
{
    esp_err_t err;
    uint32_t ret_size = 0;
    uint32_t jpeg_image_size = 0;
    uint32_t jpeg_image_size_aligned = 0;
    
    /* Search for EOI. */
    uint8_t * match = memmem(player_ctx.in_buff, player_ctx.in_buff_size, &EOI, 2); 
    if(match)
    {
        jpeg_image_size = ((uint32_t)((match+2) - player_ctx.in_buff));  // move match by 2 for skip EOI
        jpeg_image_size_aligned = ALIGN_UP(jpeg_image_size, 16);
        assert(jpeg_image_size < player_ctx.in_buff_size);
        assert(jpeg_image_size_aligned < player_ctx.in_buff_size);
    }
    
    /* Decode JPEG */
    ret_size = player_ctx.out_buff_size;
    err = jpeg_decoder_process(player_ctx.jpeg, &jpeg_decode_cfg, player_ctx.in_buff, jpeg_image_size_aligned, player_ctx.out_buff, &ret_size);
    if(err != ESP_OK)
        return -1;
    
    assert(ret_size < player_ctx.out_buff_size);
    
    return jpeg_image_size;
}

static void show_video_task(void *arg)
{
    esp_err_t ret = ESP_OK;
    int read_size = 0;
    int processed = 0;
    uint32_t all_size = 0;
    
    /* Open file */
    ESP_LOGI(TAG, "Opening file %s ...", player_ctx.file_path);
    ESP_GOTO_ON_FALSE(media_src_storage_open(&player_ctx.file) == 0, ESP_ERR_NO_MEM, err, TAG, "Storage open failed");
    ESP_GOTO_ON_FALSE(media_src_storage_connect(&player_ctx.file, player_ctx.file_path) == 0, ESP_ERR_NO_MEM, err, TAG, "Storage connect failed");

    /* Get file size */
    ESP_GOTO_ON_FALSE(media_src_storage_get_size(&player_ctx.file, &player_ctx.filesize) == 0, ESP_ERR_NO_MEM, err, TAG, "Get file size failed");

    /* Create input buffer */
    player_ctx.in_buff = video_decoder_malloc(player_ctx.in_buff_size);
    ESP_GOTO_ON_FALSE(player_ctx.in_buff, ESP_ERR_NO_MEM, err, TAG, "Allocation in_buff failed");

    /* Init video decoder */
    ESP_GOTO_ON_ERROR(video_decoder_init(), err, TAG, "Initialize video decoder failed");
    /* Get video output size */
    uint32_t height = 0;
    uint32_t width = 0;
    ESP_GOTO_ON_ERROR(get_video_size(&width, &height), err, TAG, "Get video file size failed");
    width = ALIGN_UP(width, 16);
    
    ESP_LOGI(TAG, "Video size: %ld x %ld", width, height);
    
    /* Create output buffer */
    player_ctx.out_buff_size = width * height * 3;
    player_ctx.out_buff = video_decoder_malloc(player_ctx.out_buff_size);
    ESP_GOTO_ON_FALSE(player_ctx.out_buff, ESP_ERR_NO_MEM, err, TAG, "Allocation out_buff failed");
    			 
    lvgl_port_lock(0);
	/* Set buffer to LVGL canvas */ 
    lv_canvas_set_buffer(player_ctx.canvas, player_ctx.out_buff, width, height, LV_COLOR_FORMAT_RGB565);
    lv_obj_invalidate(player_ctx.canvas);
    
    if (player_ctx.auto_width || player_ctx.auto_height) {
        uint32_t h = (player_ctx.auto_height ? (height+120) : lv_obj_get_height(player_ctx.main));
        uint32_t w = (player_ctx.auto_width ? width : lv_obj_get_width(player_ctx.main));
        lv_obj_set_size(player_ctx.main, w, h);
    }
    
    
    lv_obj_remove_state(player_ctx.slider, LV_STATE_DISABLED);
    /* Enable/disable buttons */
    lv_obj_add_state(player_ctx.btn_play, LV_STATE_DISABLED);
    lv_obj_remove_state(player_ctx.btn_stop, LV_STATE_DISABLED);
    lv_obj_remove_state(player_ctx.btn_pause, LV_STATE_DISABLED);
    lv_obj_remove_state(player_ctx.btn_repeat, LV_STATE_DISABLED);
    /* Hide Stop button */
    lv_obj_add_flag(player_ctx.img_stop, LV_OBJ_FLAG_HIDDEN);
    /* Set slider range */
    lv_slider_set_range(player_ctx.slider, 0, 1000);
    lvgl_port_unlock();

    player_ctx.state = PLAYER_STATE_PLAYING;
    
    ESP_LOGI(TAG, "Video player initialized");
   
    media_src_storage_seek(&player_ctx.file, 0);
    while(player_ctx.state != PLAYER_STATE_STOPPED)
    {
        if (player_ctx.state == PLAYER_STATE_PAUSED) {		 
            lvgl_port_lock(0);
            lv_obj_remove_flag(player_ctx.img_pause, LV_OBJ_FLAG_HIDDEN);
            lvgl_port_unlock();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        read_size = media_src_storage_read(&player_ctx.file, player_ctx.in_buff, player_ctx.in_buff_size);
        if (read_size <= 0) {
            ESP_LOGI(TAG, "Playing finished.");
            if (player_ctx.loop) {
                ESP_LOGI(TAG, "Playing loop enabled. Play again...");
                media_src_storage_seek(&player_ctx.file, 0);
                all_size = 0;
                continue;
            } else {
                esp_lvgl_simple_player_stop();
                continue;
            }
        }
        
        /* Decode one frame */
        processed = video_decoder_decode();
        
        /* Move in video file */
        if(processed > 0)
        {  
            all_size += processed;
            media_src_storage_seek(&player_ctx.file, all_size);
        }
        
        lvgl_port_lock(0);
        /* Refresh video canvas object */
        lv_obj_invalidate(player_ctx.canvas);
        /* Set slider */
        lv_slider_set_value(player_ctx.slider, ((float)all_size/(float)player_ctx.filesize)*1000, LV_ANIM_ON);
        lvgl_port_unlock();
    }
    

err:    
    lvgl_port_lock(0);
    /* Show black on screen */
    memset(player_ctx.out_buff, 0, player_ctx.out_buff_size);
    if (player_ctx.auto_height) {
        lv_obj_set_height(player_ctx.main, 320);
    }
    lv_obj_invalidate(player_ctx.canvas);
    /* Set slider */
    lv_slider_set_value(player_ctx.slider, 0, LV_ANIM_ON);
    lvgl_port_unlock();
    
    /* Close storage */
    media_src_storage_disconnect(&player_ctx.file);
    media_src_storage_close(&player_ctx.file);
    
    /* Deinit video decoder */
    video_decoder_deinit();
    
    if (player_ctx.in_buff) {
        heap_caps_free(player_ctx.in_buff);
        player_ctx.in_buff = NULL;
    }
    if (player_ctx.out_buff) {
        heap_caps_free(player_ctx.out_buff);
        player_ctx.out_buff = NULL;
        player_ctx.out_buff_size = 0;
    }

    /* Close task */
    vTaskDelete( NULL );
}

lv_obj_t * esp_lvgl_simple_player_create(esp_lvgl_simple_player_cfg_t * params)
{    
    ESP_RETURN_ON_FALSE(params->file, NULL, TAG, "File path must be filled");
    ESP_RETURN_ON_FALSE(params->screen, NULL, TAG, "LVGL screen must be filled");
    ESP_RETURN_ON_FALSE(params->buff_size, NULL, TAG, "Size of the video frame buffer must be filled");
    ESP_RETURN_ON_FALSE(params->screen_width > 0 && params->screen_height > 0, NULL, TAG, "Object size must be filled");
    
    player_ctx.file_path = params->file;
    player_ctx.in_buff_size = params->buff_size;
    player_ctx.screen_width = params->screen_width;
    player_ctx.screen_height = params->screen_height;
    player_ctx.hide_controls = params->flags.hide_controls;
    player_ctx.hide_slider = params->flags.hide_slider;
    player_ctx.hide_status = params->flags.hide_status;
    player_ctx.auto_width = params->flags.auto_width;
    player_ctx.auto_height = params->flags.auto_height;
    
    /* Create LVGL objects */
    lv_obj_t * player_screen = create_lvgl_objects(params->screen);
    
    /* Default player state */
    esp_lvgl_simple_player_stop();
    
    return player_screen;
}

player_state_t esp_lvgl_simple_player_get_state(void)
{
    return player_ctx.state;
}

void esp_lvgl_simple_player_hide_controls(bool hide)
{
    if (hide) {
        lv_obj_add_flag(player_ctx.controls, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(player_ctx.controls, LV_OBJ_FLAG_HIDDEN);
    }
}

void esp_lvgl_simple_player_change_file(char *file)
{
    if (player_ctx.state != PLAYER_STATE_STOPPED) {
        ESP_LOGW(TAG, "Playing file can be changed only when video is stopped.");
    }
    player_ctx.file_path = file;
}

void esp_lvgl_simple_player_play(void)
{
    if (player_ctx.state == PLAYER_STATE_STOPPED) {
        ESP_LOGI(TAG, "Player starting playing.");
        /* Create video task */
        xTaskCreate(show_video_task, "video task", 4096, NULL, 4, NULL);
    } else if(player_ctx.state == PLAYER_STATE_PAUSED) {
        esp_lvgl_simple_player_pause();
    }
}

void esp_lvgl_simple_player_pause(void)
{
    lvgl_port_lock(0);
    if (player_ctx.state != PLAYER_STATE_PAUSED) {
        ESP_LOGI(TAG, "Player paused.");
        player_ctx.state = PLAYER_STATE_PAUSED;
            
        lv_obj_remove_state(player_ctx.btn_play, LV_STATE_DISABLED);
        lv_obj_remove_state(player_ctx.btn_stop, LV_STATE_DISABLED);
        lv_obj_remove_state(player_ctx.btn_pause, LV_STATE_DISABLED);
        lv_obj_remove_state(player_ctx.btn_repeat, LV_STATE_DISABLED);
    } else {
        ESP_LOGI(TAG, "Player resume playing.");
        player_ctx.state = PLAYER_STATE_PLAYING;
        lv_obj_add_flag(player_ctx.img_pause, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(player_ctx.btn_play, LV_STATE_DISABLED);
    }
    lvgl_port_unlock();
}

void esp_lvgl_simple_player_stop(void)
{
    ESP_LOGI(TAG, "Player stopped.");
    player_ctx.state = PLAYER_STATE_STOPPED;
        
    lvgl_port_lock(0);
    lv_obj_remove_state(player_ctx.btn_play, LV_STATE_DISABLED);
    lv_obj_add_state(player_ctx.btn_stop, LV_STATE_DISABLED);
    lv_obj_add_state(player_ctx.btn_pause, LV_STATE_DISABLED);
    lv_obj_add_state(player_ctx.btn_repeat, LV_STATE_DISABLED);
    
    lv_obj_remove_flag(player_ctx.img_stop, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
}

void esp_lvgl_simple_player_repeat(bool repeat)
{
    ESP_LOGI(TAG, "Player repeat %s.", (repeat ? "enabled" : "disabled"));
    player_ctx.loop = repeat;
}