
idf_component_register(
    SRCS "src/esp_lvgl_simple_player.c" "src/media_src_storage.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES esp_driver_jpeg
)
