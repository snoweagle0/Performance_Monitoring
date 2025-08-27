#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef __linux__
#include <sys/wait.h>
#endif

extern lv_obj_t *parent;
extern lv_obj_t *bmeun;
extern lv_obj_t *bin_ui;

// 媒体播放相关功能
// 这个文件用于处理图片、音频等媒体资源的加载和播放

// 图片资源路径数组
static const char* image_paths[] = {
    "A:./resource/1.bmp",
    "A:./resource/2.bmp", 
    "A:./resource/3.bmp",
    "A:./resource/4.bmp",
    "A:./resource/5.bmp",
    "A:./resource/6.bmp",
    "A:./resource/8.bmp",
    "A:./resource/9.bmp",
    "A:./resource/boot_img.bmp",
    "A:./resource/img.bmp"
};

static const int image_count = sizeof(image_paths) / sizeof(image_paths[0]);

// 音频资源路径
static const char* audio_path = "A:./resource/intel.wav";
static const char* mplayer_path = "A:./mplayer64";

// 音频播放状态
static int audio_playing = 0;
static int audio_finished = 0;
static pthread_t audio_thread;

// 渐变动画相关
static lv_obj_t *fade_img = NULL;
static lv_timer_t *fade_timer = NULL;
static int fade_step = 0;
static const int fade_duration_ms = 3000; // 3秒
static const int fade_steps = 30; // 30步渐变
static int fade_finished = 0; // 渐变是否完成
// 移除静态初始化，改为在函数中计算
static int fade_step_interval; // 每步间隔时间

// 启动媒体状态管理
static int boot_media_initialized = 0; // 媒体是否已初始化
static int boot_media_ready = 0;       // 媒体是否完全准备好（图像+音频都完成）

// 函数声明
void* play_audio_thread(void* arg);
void fade_animation_cb(lv_timer_t *timer);
void check_boot_media_completion(void);
int is_audio_finished(void);

// 媒体资源初始化函数
void init_media_resources(void)
{
    printf("Initializing media resources...\n");
    printf("Available images: %d\n", image_count);
    printf("Audio file: %s\n", audio_path);
}

// 获取图片路径
const char* get_image_path(int index)
{
    if (index >= 0 && index < image_count) {
        return image_paths[index];
    }
    return NULL;
}

// 获取图片数量
int get_image_count(void)
{
    return image_count;
}

// 获取音频路径
const char* get_audio_path(void)
{
    return audio_path;
}

// 音频播放线程函数
void* play_audio_thread(void* arg)
{
    printf("Starting audio playback thread...\n");
    audio_playing = 1;
    audio_finished = 0;
    
    // 构建MPlayer命令
    char command[512];
    snprintf(command, sizeof(command), "%s \"%s\"", mplayer_path, audio_path);
    printf("Executing command: %s\n", command);
    
    // 执行MPlayer命令
    int result = system(command);
    
    if (result == 0) {
        printf("Audio playback completed successfully\n");
    } else {
        printf("Audio playback failed with code: %d\n", result);
    }
    
    audio_playing = 0;
    audio_finished = 1;
    
    // 检查是否所有媒体都完成了
    check_boot_media_completion();
    
    printf("Audio thread finished\n");
    return NULL;
}

// 开始播放音频
void start_audio_playback(void)
{
    if (!audio_playing) {
        printf("Starting audio playback...\n");
        if (pthread_create(&audio_thread, NULL, play_audio_thread, NULL) != 0) {
            printf("Failed to create audio thread\n");
            audio_finished = 1; // 如果线程创建失败，标记为完成
        }
    }
}

// 检查音频是否播放完成
int is_audio_finished(void)
{
    return audio_finished;
}

// 检查渐变动画是否完成
int is_fade_finished(void)
{
    return fade_finished;
}

// 检查启动媒体是否完全准备好
int is_boot_media_ready(void)
{
    return boot_media_ready;
}

// 检查启动媒体完成状态
void check_boot_media_completion(void)
{
    if (fade_finished && audio_finished) {
        boot_media_ready = 1;
        printf("Boot media completely finished - both fade and audio completed\n");
    }
}

// 重置启动媒体状态
void reset_boot_media_state(void)
{
    audio_playing = 0;
    audio_finished = 0;
    fade_finished = 0;
    boot_media_initialized = 0;
    boot_media_ready = 0;
    fade_step = 0;
    printf("Boot media state reset\n");
}

// 渐变动画回调函数
void fade_animation_cb(lv_timer_t *timer)
{
    if (fade_img == NULL) {
        lv_timer_delete(timer);
        return;
    }
    
    // 计算当前不透明度 (0-255)
    lv_opa_t opacity = (lv_opa_t)((fade_step * 255) / fade_steps);
    
    // 应用不透明度
    lv_obj_set_style_img_opa(fade_img, opacity, 0);
    
    printf("Fade step %d/%d, opacity: %d\n", fade_step, fade_steps, opacity);
    
    fade_step++;
    
    // 如果渐变完成
    if (fade_step > fade_steps) {
        lv_timer_delete(timer);
        fade_timer = NULL;
        fade_finished = 1; // 标记渐变完成
        printf("Fade animation completed\n");
        
        // 检查是否所有媒体都完成了
        check_boot_media_completion();
    }
}

// 预设图片为透明状态
void prepare_image_for_fade(lv_obj_t *img_obj)
{
    if (img_obj == NULL) {
        printf("Error: Image object is NULL\n");
        return;
    }
    
    // 立即设置为完全透明，避免闪现
    lv_obj_set_style_img_opa(img_obj, LV_OPA_TRANSP, 0);
    printf("Image set to transparent state\n");
}

// 开始渐变动画
void start_fade_animation(lv_obj_t *img_obj)
{
    if (img_obj == NULL) {
        printf("Error: Image object is NULL\n");
        return;
    }
    
    fade_img = img_obj;
    fade_step = 0;
    
    // 计算间隔时间
    fade_step_interval = fade_duration_ms / fade_steps;
    
    // 确保图片已经是透明状态（可能在之前已经设置过）
    lv_obj_set_style_img_opa(fade_img, LV_OPA_TRANSP, 0);
    
    // 创建渐变定时器
    fade_timer = lv_timer_create(fade_animation_cb, fade_step_interval, NULL);
    lv_timer_set_repeat_count(fade_timer, fade_steps + 1);
    
    printf("Fade animation started: %d steps, %dms interval\n", fade_steps, fade_step_interval);
}

// 启动界面媒体初始化 - 包含图片渐变和音频播放
void init_boot_media(lv_obj_t *img_obj)
{
    // 防止重复初始化
    if (boot_media_initialized) {
        printf("Boot media already initialized, skipping...\n");
        return;
    }
    
    printf("Initializing boot media...\n");
    
    // 重置状态
    reset_boot_media_state();
    
    // 标记为已初始化
    boot_media_initialized = 1;
    
    // 首先将图片设置为透明状态
    prepare_image_for_fade(img_obj);
    
    // 开始渐变动画
    start_fade_animation(img_obj);
    
    // 开始音频播放
    start_audio_playback();
    
    printf("Boot media initialization completed\n");
}