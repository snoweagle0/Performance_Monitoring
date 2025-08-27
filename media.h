#ifndef MEDIA_H
#define MEDIA_H

#include "lvgl/lvgl.h"

// 函数声明
void init_media_resources(void);
const char* get_image_path(int index);
int get_image_count(void);
const char* get_audio_path(void);

// 启动媒体相关函数
void init_boot_media(lv_obj_t *img_obj);
int is_audio_finished(void);
int is_fade_finished(void);
int is_boot_media_ready(void);
void start_audio_playback(void);
void start_fade_animation(lv_obj_t *img_obj);
void prepare_image_for_fade(lv_obj_t *img_obj);
void reset_boot_media_state(void);
void check_boot_media_completion(void);

#endif // MEDIA_H
