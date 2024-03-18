# Simple LVGL Player

This component can play M-JPEG video on ESP32P4 board with LVGL9 objects.

## Usage

Create LVGL simple player object:
```
/* Create player */
esp_lvgl_simple_player_cfg_t player_cfg = {
    .file = "/sdcard/video.mjpeg",
    .screen = lv_screen_active(),
    .screen_width = BSP_LCD_V_RES,
    .screen_height = (BSP_LCD_H_RES),
    .buff_size = 540*960,
    .flags = {
        .hide_controls = false, /* Show/hide control buttons */ 
        .hide_slider = false,   /* Show/hide indication slider */ 
        .hide_status = false,   /* Hide status icons in video (paused, stopped) */ 

        .auto_width = false,    /* Set automatic width by video size */ 
        .auto_height = true,    /* Set automatic height by video size */ 
    }
};
lv_obj_t * player = esp_lvgl_simple_player_create(&player_cfg);
/* Align to center */
lv_obj_center(player);
```

Change playing file (stop played file):
```
esp_lvgl_simple_player_change_file("/sdcard/video1.mjpeg");
```

Control video:
```
esp_lvgl_simple_player_play();
esp_lvgl_simple_player_pause();
esp_lvgl_simple_player_stop();
```

## How to create M-JPEG video

Create video without audio:
```
.\ffmpeg.exe -i input_video.mp4 -vcodec mjpeg -q:v 2 -vf "scale=800:450" -an output_video.mjpeg
```
