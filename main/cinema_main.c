/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include <unistd.h>
#include <fcntl.h>

#include "esp_lvgl_simple_player.h"

//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/video.mjpeg"
//#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/noaudio.mjpeg"
#define APP_VIDEO_FILE BSP_SD_MOUNT_POINT"/p4_horizontal.mjpeg"

#if 0
static const char *TAG = "CINEMA";
static const uint16_t EOI = 0xd9ff; /* End of image */
#define ALIGN_UP(num, align)    (((num) + ((align) - 1)) & ~((align) - 1))

static bool play_loop = true;

static void app_show_video_task(void *arg)
{
    uint8_t * tmp_buff = NULL;
    uint8_t * out_buf = NULL;
    int tmp_buff_size = 800*1280;
	int one_image_size = 800*1280;
    int ret_w = 0;
    
    jpeg_decode_cfg_t decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
    };
	 
    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    
    /* Rows */
    lv_obj_t *cont_col = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont_col, BSP_LCD_V_RES, BSP_LCD_H_RES/2);
    lv_obj_align(cont_col, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_col, 0, 0);
    lv_obj_set_flex_align(cont_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_col, lv_color_black(), 0);
    
    /* Video canvas */
    lv_obj_t *video_canvas = lv_canvas_create(cont_col);
    
     /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(cont_col);
    lv_obj_set_size(slider, BSP_LCD_V_RES, 20);
    //lv_obj_center(slider);
    /*Create a label next to the slider*/
    lv_obj_t * slider_label = lv_label_create(slider);
    lv_label_set_text(slider_label, "0%");
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    
    /* Buttons */
    lv_obj_t *cont_row = lv_obj_create(cont_col);
    lv_obj_set_size(cont_row, BSP_LCD_V_RES - 20, 80);
    lv_obj_align(cont_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_top(cont_row, 2, 0);
    lv_obj_set_style_pad_bottom(cont_row, 2, 0);
    lv_obj_set_flex_align(cont_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(cont_row, lv_color_black(), 0);
    
    /* Play button */
    lv_obj_t * play_btn = lv_btn_create(cont_row);
    lv_obj_t * label = lv_label_create(play_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PLAY);
    //lv_obj_add_event_cb(play_btn, rec_play_event_cb, LV_EVENT_CLICKED, (char *)REC_FILENAME);
    
    /* Pause button */
    lv_obj_t * pause_btn = lv_btn_create(cont_row);
    label = lv_label_create(pause_btn);
    lv_label_set_text_static(label, LV_SYMBOL_PAUSE);
    //lv_obj_add_event_cb(pause_btn, rec_play_event_cb, LV_EVENT_CLICKED, (char *)REC_FILENAME);

    /* Stop button */
    lv_obj_t * stop_btn = lv_btn_create(cont_row);
    label = lv_label_create(stop_btn);
    lv_label_set_text_static(label, LV_SYMBOL_STOP);
    //lv_obj_add_event_cb(stop_btn, rec_stop_event_cb, LV_EVENT_CLICKED, NULL);

    /* Repeat button */
    lv_obj_t * repeat_btn = lv_btn_create(cont_row);
    lv_obj_add_flag(repeat_btn, LV_OBJ_FLAG_CHECKABLE);
    label = lv_label_create(repeat_btn);
    lv_label_set_text_static(label, LV_SYMBOL_REFRESH);
    //lv_obj_add_event_cb(repeat_btn, rec_stop_event_cb, LV_EVENT_CLICKED, NULL);
    
    bsp_display_unlock();
     
     
     
     
     
     
    ESP_LOGI(TAG, "Opening file %s ...", APP_VIDEO_FILE);
    
    /* Open file */
    media_src_t file;
    media_src_storage_open(&file);
    media_src_storage_connect(&file, APP_VIDEO_FILE);
    
    tmp_buff = (uint8_t *)jpeg_alloc_decoder_mem(one_image_size*2);//heap_caps_aligned_alloc(64, tmp_buff_size, MALLOC_CAP_SPIRAM);
    if (tmp_buff == NULL) {
        ESP_LOGE(TAG, "Allocation memory failed");
        goto end;
    }
     
    jpeg_decoder_handle_t jpeg;
    jpeg_decode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = 100,
    };
    jpeg_new_decoder_engine(&eng_cfg, &jpeg);
    
     
    jpeg_decode_picture_info_t header;
    //ret_w = read(f, tmp_buff, tmp_buff_size);
    media_src_storage_read(&file, tmp_buff, tmp_buff_size);
    jpeg_decoder_get_info(tmp_buff, tmp_buff_size, &header);
    
    header.width = ALIGN_UP(header.width, 16);
    one_image_size = header.width * header.height;
    out_buf = (uint8_t *)jpeg_alloc_decoder_mem(one_image_size*2);//heap_caps_aligned_alloc(64, one_image_size*2, MALLOC_CAP_SPIRAM);
    if (out_buf == NULL) {
        ESP_LOGE(TAG, "Allocation memory failed");
        goto end;
    }
	 
	/* Set buffer to LVGL canvas */ 			 
    bsp_display_lock(0);
    lv_canvas_set_buffer(video_canvas, out_buf, header.width, header.height, LV_COLOR_FORMAT_RGB565);
    lv_obj_invalidate(video_canvas);
    bsp_display_unlock();
	 
    //lseek(f, 0, SEEK_SET); 
    media_src_storage_seek(&file, 0);
    tmp_buff_size = one_image_size;
    uint32_t none_aligned = 0;
    uint32_t process_size = 0;
    uint32_t process_size_aligned = 0;
    uint8_t * match;
    uint32_t r = 0;
    //uint32_t frame = 0;
    while(1) {
        //ESP_LOGW(TAG, "1");
        ret_w = media_src_storage_read(&file, tmp_buff, tmp_buff_size);
        //ret_w = read(f, tmp_buff, tmp_buff_size);
        if (ret_w <= 0) {
            ESP_LOGE(TAG, "Encoder finished, ret %d", ret_w);
            if (play_loop) {
                //lseek(f, 0, SEEK_SET); 
                media_src_storage_seek(&file, 0);
                continue;
            } else {
                break; 
            }
        }
        //ESP_LOGW(TAG, "2");
     
        /* Search for EOI. */
        match = memmem(tmp_buff, tmp_buff_size, &EOI, 2); 
        if(match)
        {
            process_size = ((uint32_t)((match+2) - tmp_buff));  // move match by 2 for skip EOI
            process_size_aligned = ALIGN_UP(process_size, 16);
        }
        //else
            //process_size = tmp_buff_size;
       
        //ESP_LOGW(TAG, "3");
        //jpeg_decoder_get_info(tmp_buff, tmp_buff_size, &header);
        //ESP_LOGW(TAG, "Frame size: %ld x %ld; Buff: %d", header.width, header.height, tmp_buff_size);

        none_aligned = tmp_buff_size;
        jpeg_decoder_process(jpeg, &decode_cfg, tmp_buff, process_size_aligned, out_buf, &none_aligned);
        
        //ESP_LOGW(TAG, "4");
        
        bsp_display_lock(0);
        lv_obj_invalidate(video_canvas);
        bsp_display_unlock();
        
        //lseek(f, none_aligned, SEEK_CUR);     
        r += process_size;
        media_src_storage_seek(&file, r);
        
        //frame++;
        //ESP_LOGW(TAG, "Frame %ld (%lx - 0x%02x%02x)", frame, (uint32_t)(tmp_buff),*tmp_buff,*(tmp_buff+1));
    }
     
end:
     media_src_storage_disconnect(&file);
     /*if (f) {
         close(f);
     }*/
     if (tmp_buff) {
         heap_caps_free(tmp_buff);
         tmp_buff = NULL;
     }
     if (out_buf) {
         heap_caps_free(out_buf);
         out_buf = NULL;
     }
     
     if (jpeg) {
        jpeg_del_decoder_engine(jpeg);
     }

    /* Close task */
    vTaskDelete( NULL );
}
#endif

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
    
    
    //xTaskCreate(app_show_video_task, "video task", 4096, NULL, 4, NULL);
}
