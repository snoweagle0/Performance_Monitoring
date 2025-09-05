<<<<<<< HEAD
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media.h"

// 外部变量声明
extern lv_obj_t *parent;
extern lv_obj_t *bmeun;
extern lv_obj_t *bin_ui;

// 外部函数声明
extern void show_boot_screen(void);
extern void hide_boot_screen(void);

// 静态变量
static int program_should_exit = 0;
static lv_obj_t *lock_screen = NULL;
static lv_obj_t *media_player_screen = NULL;
static lv_obj_t *video_player_screen = NULL;
static int video_paused = 0;
static int mplayer_pid = 0;

// 函数声明
void lock_button_event_cb(lv_event_t * e);
void exit_button_event_cb(lv_event_t * e);
void media_play_button_event_cb(lv_event_t * e);
void init_lock_exit_buttons(void);
int should_program_exit(void);
void lock_screen_transition(void);
void create_lock_screen(void);
void lock_screen_button_event_cb(lv_event_t * e);

// 媒体播放器相关
void create_media_player_screen(void);
void close_media_player_screen(void);
void create_video_player_screen(const char* filepath);
void close_video_player_screen(void);
void video_screen_event_cb(lv_event_t * e);
void video_close_button_event_cb(lv_event_t * e);

// Lock按钮事件回调
void lock_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Lock button clicked - returning to boot screen\n");
        lock_screen_transition();
    }
}

// Exit按钮事件回调
void exit_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Exit button clicked - program will terminate\n");
        program_should_exit = 1;
    }
}

// Media Play按钮事件回调
void media_play_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Media Play button clicked - opening media player\n");
        create_media_player_screen();
    }
}

// 锁屏转换功能
void lock_screen_transition(void)
{
    printf("Starting lock screen transition...\n");
    
    // 隐藏当前界面
    if (parent != NULL) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
        printf("Main interface hidden\n");
    }
    
    // 给系统时间处理界面隐藏
    for(int i = 0; i < 10; i++) {
        lv_timer_handler();
        usleep(10000);
    }
    
    // 重置媒体状态以便重新播放
    reset_boot_media_state();
    
    // 创建锁屏界面（黑底白字"Locking"）
    create_lock_screen();
    printf("Lock screen created and displayed\n");
}

// 检查程序是否应该退出
int should_program_exit(void)
{
    return program_should_exit;
}

// 初始化Lock和Exit按钮
void init_lock_exit_buttons(void)
{
    if (bmeun == NULL) {
        printf("Error: bmeun is NULL, cannot create lock/exit buttons\n");
        return;
    }
    
    // 获取已有的头像控件作为参考位置
    lv_obj_t *avtar = lv_obj_get_child(bmeun, 0);
    if (avtar == NULL) {
        printf("Error: Cannot find avatar widget for button positioning\n");
        return;
    }
    
    // 查找最后一个按钮作为参考
    lv_obj_t *last_btn = NULL;
    for (int i = 0; i < lv_obj_get_child_cnt(bmeun); i++) {
        lv_obj_t *child = lv_obj_get_child(bmeun, i);
        if (lv_obj_check_type(child, &lv_button_class)) {
            last_btn = child;
        }
    }
    
    // 创建Lock按钮
    lv_obj_t *lock_btn = lv_button_create(bmeun);
    lv_obj_set_size(lock_btn, 150, 50);
    
    if (last_btn != NULL) {
        // 如果找到最后一个按钮，将Lock按钮放在其下方
        lv_obj_align_to(lock_btn, last_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    } else {
        // 否则放在头像下方
        lv_obj_align_to(lock_btn, avtar, LV_ALIGN_OUT_BOTTOM_MID, 0, 80);
    }
    
    lv_obj_add_event_cb(lock_btn, lock_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Lock按钮样式 - 使用蓝色
    lv_obj_set_style_bg_color(lock_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_color(lock_btn, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
    
    // Lock按钮标签
    lv_obj_t *lock_label = lv_label_create(lock_btn);
    lv_label_set_text(lock_label, "LOCK");
    lv_obj_set_style_text_color(lock_label, lv_color_white(), 0);
    lv_obj_center(lock_label);
    
    // 创建Media Play按钮
    lv_obj_t *media_btn = lv_button_create(bmeun);
    lv_obj_set_size(media_btn, 150, 50);
    lv_obj_align_to(media_btn, lock_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(media_btn, media_play_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Media Play按钮样式 - 使用绿色
    lv_obj_set_style_bg_color(media_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_color(media_btn, lv_palette_darken(LV_PALETTE_GREEN, 2), LV_STATE_PRESSED);
    
    // Media Play按钮标签
    lv_obj_t *media_label = lv_label_create(media_btn);
    lv_label_set_text(media_label, "Media Play");
    lv_obj_set_style_text_color(media_label, lv_color_white(), 0);
    lv_obj_center(media_label);
    
    // 创建Exit按钮
    lv_obj_t *exit_btn = lv_button_create(bmeun);
    lv_obj_set_size(exit_btn, 150, 50);
    lv_obj_align_to(exit_btn, media_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(exit_btn, exit_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Exit按钮样式 - 使用红色
    lv_obj_set_style_bg_color(exit_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_palette_darken(LV_PALETTE_RED, 2), LV_STATE_PRESSED);
    
    // Exit按钮标签
    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "EXIT");
    lv_obj_set_style_text_color(exit_label, lv_color_white(), 0);
    lv_obj_center(exit_label);
    
    printf("Lock and Exit buttons initialized successfully\n");
}

// 创建锁屏界面
void create_lock_screen(void)
{
    // 创建全屏黑色背景
    lock_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(lock_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(lock_screen, lv_color_black(), 0);
    lv_obj_clear_flag(lock_screen, LV_OBJ_FLAG_SCROLLABLE);
    // 移除边距和填充
    lv_obj_set_style_pad_all(lock_screen, 0, 0);
    lv_obj_set_style_margin_all(lock_screen, 0, 0);
    
    // 创建"Locking"文本标签
    lv_obj_t *locking_label = lv_label_create(lock_screen);
    lv_label_set_text(locking_label, "Locking");
    lv_obj_set_style_text_color(locking_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(locking_label, &lv_font_montserrat_48, 0);
    lv_obj_center(locking_label);
    
    // 创建解锁按钮
    lv_obj_t *unlock_btn = lv_btn_create(lock_screen);
    lv_obj_set_size(unlock_btn, 200, 60);
    lv_obj_align(unlock_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_add_event_cb(unlock_btn, lock_screen_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 设置解锁按钮样式
    lv_obj_set_style_bg_color(unlock_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_color(unlock_btn, lv_palette_darken(LV_PALETTE_GREEN, 2), LV_STATE_PRESSED);
    lv_obj_set_style_radius(unlock_btn, 10, 0);
    lv_obj_set_style_border_color(unlock_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(unlock_btn, 2, 0);
    
    // 创建解锁按钮标签
    lv_obj_t *unlock_label = lv_label_create(unlock_btn);
    lv_label_set_text(unlock_label, "UNLOCK");
    lv_obj_set_style_text_color(unlock_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(unlock_label, &lv_font_montserrat_20, 0);
    lv_obj_center(unlock_label);
    
    printf("Lock screen created with black background and white text\n");
}

// 锁屏按钮事件回调（解锁）
void lock_screen_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Unlock button clicked - returning to main interface\n");
        
        // 删除锁屏界面
        if (lock_screen != NULL) {
            lv_obj_del(lock_screen);
            lock_screen = NULL;
            printf("Lock screen removed\n");
        }
        
        // 恢复主界面显示
        if (parent != NULL && lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
            printf("Main interface restored\n");
        }
    }
}

// 媒体播放器界面相关变量和函数
static lv_obj_t *current_media_obj = NULL;
static int current_media_index = 0;
static int is_playing_video = 0;
static char media_files[20][256]; // 存储媒体文件路径
static int media_count = 0;

// 媒体文件类型检测
int is_video_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (ext == NULL) return 0;
    return (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".avi") == 0 || 
            strcmp(ext, ".mkv") == 0 || strcmp(ext, ".mov") == 0);
}

int is_audio_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (ext == NULL) return 0;
    return (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0 || 
            strcmp(ext, ".ogg") == 0 || strcmp(ext, ".m4a") == 0);
}

// 扫描resource文件夹中的媒体文件
void scan_media_files(void) {
    media_count = 0;
    
    // 音频文件列表
    const char* audio_files[] = {
        "./resource/answer.mp3",
        "./resource/Avid.mp3", 
        "./resource/handsup.mp3",
        "./resource/intel.wav"
    };
    
    // 视频文件列表
    const char* video_files[] = {
        "./resource/chunwu.mp4",
        "./resource/3.mp4"
    };
    
    // 添加音频文件
    for (int i = 0; i < sizeof(audio_files)/sizeof(audio_files[0]); i++) {
        if (media_count < 20) {
            strncpy(media_files[media_count], audio_files[i], 255);
            media_files[media_count][255] = '\0';
            media_count++;
        }
    }
    
    // 添加视频文件
    for (int i = 0; i < sizeof(video_files)/sizeof(video_files[0]); i++) {
        if (media_count < 20) {
            strncpy(media_files[media_count], video_files[i], 255);
            media_files[media_count][255] = '\0';
            media_count++;
        }
    }
    
    printf("Found %d media files\n", media_count);
}

// 播放媒体文件
void play_media_file(const char* filepath) {
    char command[512];
    
    if (is_video_file(filepath)) {
        // 为视频文件创建全屏播放界面
        create_video_player_screen(filepath);
        is_playing_video = 1;
    } else if (is_audio_file(filepath)) {
        // 播放音频文件
        snprintf(command, sizeof(command), "mplayer \"%s\" &", filepath);
        printf("Playing: %s\n", filepath);
        printf("Command: %s\n", command);
        system(command);
        is_playing_video = 0;
    } else {
        printf("Unsupported media file: %s\n", filepath);
        return;
    }
}

// 停止当前播放
void stop_current_playback(void) {
    system("pkill -f mplayer");
    printf("Stopped current playback\n");
}

// 媒体播放器按钮事件回调
void media_player_button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    
    if(code == LV_EVENT_CLICKED) {
        const char* btn_text = lv_label_get_text(lv_obj_get_child(btn, 0));
        
        if (strcmp(btn_text, "Play") == 0) {
            if (media_count > 0) {
                play_media_file(media_files[current_media_index]);
            }
        } else if (strcmp(btn_text, "Stop") == 0) {
            stop_current_playback();
        } else if (strcmp(btn_text, "Previous") == 0) {
            stop_current_playback();
            current_media_index = (current_media_index - 1 + media_count) % media_count;
            if (media_count > 0) {
                // 更新当前文件显示
                lv_obj_t *file_label = (lv_obj_t*)e->user_data;
                if (file_label) {
                    const char* filename = strrchr(media_files[current_media_index], '/');
                    filename = filename ? filename + 1 : media_files[current_media_index];
                    lv_label_set_text(file_label, filename);
                }
            }
        } else if (strcmp(btn_text, "Next") == 0) {
            stop_current_playback();
            current_media_index = (current_media_index + 1) % media_count;
            if (media_count > 0) {
                // 更新当前文件显示
                lv_obj_t *file_label = (lv_obj_t*)e->user_data;
                if (file_label) {
                    const char* filename = strrchr(media_files[current_media_index], '/');
                    filename = filename ? filename + 1 : media_files[current_media_index];
                    lv_label_set_text(file_label, filename);
                }
            }
        } else if (strcmp(btn_text, "Close") == 0) {
            close_media_player_screen();
        }
    }
}

// 创建媒体播放器界面
void create_media_player_screen(void) {
    if (media_player_screen != NULL) {
        printf("Media player already open\n");
        return;
    }
    
    // 扫描媒体文件
    scan_media_files();
    
    // 隐藏主界面
    if (parent != NULL) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 创建媒体播放器全屏界面
    media_player_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(media_player_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(media_player_screen, lv_color_hex(0x000020), 0);
    lv_obj_clear_flag(media_player_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(media_player_screen, 0, 0);
    lv_obj_set_style_margin_all(media_player_screen, 0, 0);
    
    // 创建标题
    lv_obj_t *title_label = lv_label_create(media_player_screen);
    lv_label_set_text(title_label, "Media Player");
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_32, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    
    // 显示当前文件名
    lv_obj_t *file_label = lv_label_create(media_player_screen);
    if (media_count > 0) {
        const char* filename = strrchr(media_files[current_media_index], '/');
        filename = filename ? filename + 1 : media_files[current_media_index];
        lv_label_set_text(file_label, filename);
    } else {
        lv_label_set_text(file_label, "No media files found");
    }
    lv_obj_set_style_text_color(file_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(file_label, &lv_font_montserrat_20, 0);
    lv_obj_align(file_label, LV_ALIGN_CENTER, 0, -50);
    
    // 创建控制按钮容器
    lv_obj_t *btn_container = lv_obj_create(media_player_screen);
    lv_obj_set_size(btn_container, 600, 100);
    lv_obj_align(btn_container, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // 创建按钮
    const char* btn_texts[] = {"Previous", "Play", "Stop", "Next", "Close"};
    lv_color_t btn_colors[] = {
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_GREEN), 
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_GREY)
    };
    
    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(btn_container);
        lv_obj_set_size(btn, 100, 60);
        lv_obj_set_pos(btn, i * 110, 20);
        lv_obj_add_event_cb(btn, media_player_button_event_cb, LV_EVENT_CLICKED, file_label);
        
        lv_obj_set_style_bg_color(btn, btn_colors[i], 0);
        lv_obj_set_style_bg_color(btn, lv_color_darken(btn_colors[i], 20), LV_STATE_PRESSED);
        
        lv_obj_t *btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, btn_texts[i]);
        lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
        lv_obj_center(btn_label);
    }
    
    // 显示媒体文件列表
    lv_obj_t *list_label = lv_label_create(media_player_screen);
    char list_text[1024] = "Available files:\n";
    for (int i = 0; i < media_count && i < 10; i++) {
        const char* filename = strrchr(media_files[i], '/');
        filename = filename ? filename + 1 : media_files[i];
        strcat(list_text, filename);
        strcat(list_text, "\n");
    }
    lv_label_set_text(list_label, list_text);
    lv_obj_set_style_text_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(list_label, &lv_font_montserrat_14, 0);
    lv_obj_align(list_label, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    
    printf("Media player screen created\n");
}

// 关闭媒体播放器界面
void close_media_player_screen(void) {
    if (media_player_screen != NULL) {
        // 停止当前播放
        stop_current_playback();
        
        // 删除媒体播放器界面
        lv_obj_del(media_player_screen);
        media_player_screen = NULL;
        
        // 恢复主界面
        if (parent != NULL && lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
        }
        
        printf("Media player screen closed\n");
    }
}

// 视频屏幕事件回调 - 处理单击暂停/继续
void video_screen_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        // 简单的暂停/继续逻辑 - 使用STOP/CONT信号
        if (video_paused) {
            // 继续播放
            system("pkill -CONT mplayer");
            video_paused = 0;
            printf("Video resumed\n");
        } else {
            // 暂停播放
            system("pkill -STOP mplayer");
            video_paused = 1;
            printf("Video paused\n");
        }
    }
}

// 视频关闭按钮事件回调
void video_close_button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        close_video_player_screen();
    }
}

// 创建全屏视频播放界面
void create_video_player_screen(const char* filepath) {
    if (video_player_screen != NULL) {
        printf("Video player already open\n");
        return;
    }
    
    // 隐藏媒体播放器界面
    if (media_player_screen != NULL) {
        lv_obj_add_flag(media_player_screen, LV_OBJ_FLAG_HIDDEN);
    }
    
    // 创建全屏视频播放界面
    video_player_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(video_player_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(video_player_screen, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(video_player_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(video_player_screen, 0, 0);
    lv_obj_set_style_margin_all(video_player_screen, 0, 0);
    
    // 添加单击事件用于暂停/继续播放
    lv_obj_add_event_cb(video_player_screen, video_screen_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 创建右上角关闭按钮
    lv_obj_t *close_btn = lv_button_create(video_player_screen);
    lv_obj_set_size(close_btn, 60, 60);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(close_btn, video_close_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 设置关闭按钮样式
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_darken(lv_palette_main(LV_PALETTE_RED), 20), LV_STATE_PRESSED);
    lv_obj_set_style_radius(close_btn, 30, 0);
    
    // 创建关闭按钮标签
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "X");
    lv_obj_set_style_text_color(close_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_24, 0);
    lv_obj_center(close_label);
    
    // 创建视频播放提示标签
    lv_obj_t *hint_label = lv_label_create(video_player_screen);
    lv_label_set_text(hint_label, "Click screen to pause/resume\nClick X to exit");
    lv_obj_set_style_text_color(hint_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint_label, LV_ALIGN_TOP_LEFT, 20, 20);
    
    // 开始播放视频
    char command[512];
    snprintf(command, sizeof(command), "mplayer \"%s\" -vo fbdev2 -fs -quiet &", filepath);
    printf("Playing video: %s\n", filepath);
    printf("Command: %s\n", command);
    system(command);
    
    video_paused = 0;
    printf("Video player screen created\n");
}

// 关闭视频播放器界面
void close_video_player_screen(void) {
    if (video_player_screen != NULL) {
        // 停止视频播放
        system("pkill -f mplayer");
        
        // 删除视频播放器界面
        lv_obj_del(video_player_screen);
        video_player_screen = NULL;
        video_paused = 0;
        
        // 恢复媒体播放器界面
        if (media_player_screen != NULL && lv_obj_has_flag(media_player_screen, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(media_player_screen, LV_OBJ_FLAG_HIDDEN);
        }
        
        printf("Video player screen closed\n");
    }
}
=======
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media.h"

// 外部变量声明
extern lv_obj_t *parent;
extern lv_obj_t *bmeun;
extern lv_obj_t *bin_ui;

// 外部函数声明
extern void show_boot_screen(void);
extern void hide_boot_screen(void);

// 静态变量
static int program_should_exit = 0;
static lv_obj_t *lock_screen = NULL;

// 函数声明
void lock_button_event_cb(lv_event_t * e);
void exit_button_event_cb(lv_event_t * e);
void init_lock_exit_buttons(void);
int should_program_exit(void);
void lock_screen_transition(void);
void create_lock_screen(void);
void lock_screen_button_event_cb(lv_event_t * e);

// Lock按钮事件回调
void lock_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Lock button clicked - returning to boot screen\n");
        lock_screen_transition();
    }
}

// Exit按钮事件回调
void exit_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Exit button clicked - program will terminate\n");
        program_should_exit = 1;
    }
}

// 锁屏转换功能
void lock_screen_transition(void)
{
    printf("Starting lock screen transition...\n");
    
    // 隐藏当前界面
    if (parent != NULL) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
        printf("Main interface hidden\n");
    }
    
    // 给系统时间处理界面隐藏
    for(int i = 0; i < 10; i++) {
        lv_timer_handler();
        usleep(10000);
    }
    
    // 重置媒体状态以便重新播放
    reset_boot_media_state();
    
    // 创建锁屏界面（黑底白字"Locking"）
    create_lock_screen();
    printf("Lock screen created and displayed\n");
}

// 检查程序是否应该退出
int should_program_exit(void)
{
    return program_should_exit;
}

// 初始化Lock和Exit按钮
void init_lock_exit_buttons(void)
{
    if (bmeun == NULL) {
        printf("Error: bmeun is NULL, cannot create lock/exit buttons\n");
        return;
    }
    
    // 获取已有的头像控件作为参考位置
    lv_obj_t *avtar = lv_obj_get_child(bmeun, 0);
    if (avtar == NULL) {
        printf("Error: Cannot find avatar widget for button positioning\n");
        return;
    }
    
    // 查找最后一个按钮作为参考
    lv_obj_t *last_btn = NULL;
    for (int i = 0; i < lv_obj_get_child_cnt(bmeun); i++) {
        lv_obj_t *child = lv_obj_get_child(bmeun, i);
        if (lv_obj_check_type(child, &lv_button_class)) {
            last_btn = child;
        }
    }
    
    // 创建Lock按钮
    lv_obj_t *lock_btn = lv_button_create(bmeun);
    lv_obj_set_size(lock_btn, 150, 50);
    
    if (last_btn != NULL) {
        // 如果找到最后一个按钮，将Lock按钮放在其下方
        lv_obj_align_to(lock_btn, last_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    } else {
        // 否则放在头像下方
        lv_obj_align_to(lock_btn, avtar, LV_ALIGN_OUT_BOTTOM_MID, 0, 80);
    }
    
    lv_obj_add_event_cb(lock_btn, lock_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Lock按钮样式 - 使用蓝色
    lv_obj_set_style_bg_color(lock_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_color(lock_btn, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
    
    // Lock按钮标签
    lv_obj_t *lock_label = lv_label_create(lock_btn);
    lv_label_set_text(lock_label, "LOCK");
    lv_obj_set_style_text_color(lock_label, lv_color_white(), 0);
    lv_obj_center(lock_label);
    
    // 创建Exit按钮
    lv_obj_t *exit_btn = lv_button_create(bmeun);
    lv_obj_set_size(exit_btn, 150, 50);
    lv_obj_align_to(exit_btn, lock_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(exit_btn, exit_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Exit按钮样式 - 使用红色
    lv_obj_set_style_bg_color(exit_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_color(exit_btn, lv_palette_darken(LV_PALETTE_RED, 2), LV_STATE_PRESSED);
    
    // Exit按钮标签
    lv_obj_t *exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "EXIT");
    lv_obj_set_style_text_color(exit_label, lv_color_white(), 0);
    lv_obj_center(exit_label);
    
    printf("Lock and Exit buttons initialized successfully\n");
}

// 创建锁屏界面
void create_lock_screen(void)
{
    // 创建全屏黑色背景
    lock_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(lock_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(lock_screen, lv_color_black(), 0);
    lv_obj_clear_flag(lock_screen, LV_OBJ_FLAG_SCROLLABLE);
    // 移除边距和填充
    lv_obj_set_style_pad_all(lock_screen, 0, 0);
    lv_obj_set_style_margin_all(lock_screen, 0, 0);
    
    // 创建"Locking"文本标签
    lv_obj_t *locking_label = lv_label_create(lock_screen);
    lv_label_set_text(locking_label, "Locking");
    lv_obj_set_style_text_color(locking_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(locking_label, &lv_font_montserrat_48, 0);
    lv_obj_center(locking_label);
    
    // 创建解锁按钮
    lv_obj_t *unlock_btn = lv_btn_create(lock_screen);
    lv_obj_set_size(unlock_btn, 200, 60);
    lv_obj_align(unlock_btn, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_add_event_cb(unlock_btn, lock_screen_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 设置解锁按钮样式
    lv_obj_set_style_bg_color(unlock_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_color(unlock_btn, lv_palette_darken(LV_PALETTE_GREEN, 2), LV_STATE_PRESSED);
    lv_obj_set_style_radius(unlock_btn, 10, 0);
    lv_obj_set_style_border_color(unlock_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(unlock_btn, 2, 0);
    
    // 创建解锁按钮标签
    lv_obj_t *unlock_label = lv_label_create(unlock_btn);
    lv_label_set_text(unlock_label, "UNLOCK");
    lv_obj_set_style_text_color(unlock_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(unlock_label, &lv_font_montserrat_20, 0);
    lv_obj_center(unlock_label);
    
    printf("Lock screen created with black background and white text\n");
}

// 锁屏按钮事件回调（解锁）
void lock_screen_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Unlock button clicked - returning to main interface\n");
        
        // 删除锁屏界面
        if (lock_screen != NULL) {
            lv_obj_del(lock_screen);
            lock_screen = NULL;
            printf("Lock screen removed\n");
        }
        
        // 恢复主界面显示
        if (parent != NULL && lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
            printf("Main interface restored\n");
        }
    }
}
>>>>>>> 10753538bc69c7885235a8dce5e419152119448f
