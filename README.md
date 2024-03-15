# Simple LVGL Player

This is a display + LVGL graphics library example.
Play mjpeg wideo from SD card.

## Create M-JPEG video

Create video without audio:
```
.\ffmpeg.exe -i input_video.mp4 -vcodec mjpeg -q:v 2 -vf "scale=800:450" -an output_video.mjpeg
```
