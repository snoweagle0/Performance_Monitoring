#ifndef LOCKINGANDEXITING_H
#define LOCKINGANDEXITING_H

#include "lvgl/lvgl.h"

// 基本功能函数声明
void init_lock_exit_buttons(void);
int should_program_exit(void);
void lock_screen_transition(void);

// Lock和Exit按钮事件回调
void lock_button_event_cb(lv_event_t * e);
void exit_button_event_cb(lv_event_t * e);

// 媒体播放器功能
void media_play_button_event_cb(lv_event_t * e);
void create_media_player_screen(void);
void close_media_player_screen(void);
void create_video_player_screen(const char* filepath);
void close_video_player_screen(void);

#endif // LOCKINGANDEXITING_H