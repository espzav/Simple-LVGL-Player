
set(IMAGE_SOURCES "images/lvgl9/breaking_news.c")

idf_component_register(
    SRCS "cinema_main.c" ${IMAGE_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES esp_lvgl_simple_player
)
