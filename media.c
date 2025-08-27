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
    printf("Cleaning up system state...\n");
    
    // 停止并清理渐变定时器（如果存在且有效）
    if (fade_timer != NULL) {
        lv_timer_delete(fade_timer);
        fade_timer = NULL;
        printf("Fade timer cleaned up\n");
    }
    
    // 清理渐变图片引用，但不删除图片对象本身
    fade_img = NULL;
    
    // 等待音频线程完成（如果正在运行）
    if (audio_playing && audio_thread != 0) {
        printf("Waiting for audio thread to finish...\n");
        pthread_join(audio_thread, NULL);
        audio_thread = 0;
    }
    
    // 重置状态变量
    audio_playing = 0;
    audio_finished = 0;
    fade_finished = 0;
    boot_media_initialized = 0;
    boot_media_ready = 0;
    fade_step = 0;
    
    printf("Boot media state reset completely\n");
}

// 渐变动画回调函数
void fade_animation_cb(lv_timer_t *timer)
{
    if (fade_img == NULL) {
        printf("Error: fade_img is NULL, stopping animation\n");
        lv_timer_delete(timer);
        fade_timer = NULL;
        return;
    }
    
    // 检查图片对象是否仍然有效
    if (!lv_obj_is_valid(fade_img)) {
        printf("Error: fade_img object is no longer valid, stopping animation\n");
        lv_timer_delete(timer);
        fade_timer = NULL;
        fade_img = NULL;
        return;
    }
    
    // 计算当前不透明度 (0-255)
    lv_opa_t opacity = (lv_opa_t)((fade_step * 255) / fade_steps);
    
    // 检查对象类型并应用相应的不透明度
    if (lv_obj_check_type(fade_img, &lv_image_class)) {
        // 图片对象
        lv_obj_set_style_img_opa(fade_img, opacity, 0);
    } else {
        // 其他对象（如占位符矩形）
        lv_obj_set_style_bg_opa(fade_img, opacity, 0);
    }
    lv_obj_invalidate(fade_img); // 强制刷新图片对象
    
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
    
    // 根据对象类型设置透明状态
    if (lv_obj_check_type(img_obj, &lv_image_class)) {
        // 图片对象
        lv_obj_set_style_img_opa(img_obj, LV_OPA_TRANSP, 0);
    } else {
        // 其他对象（如占位符矩形）
        lv_obj_set_style_bg_opa(img_obj, LV_OPA_TRANSP, 0);
    }
    lv_obj_invalidate(img_obj); // 强制刷新
    
    printf("Image set to transparent state\n");
}

// 开始渐变动画
void start_fade_animation(lv_obj_t *img_obj)
{
    if (img_obj == NULL) {
        printf("Error: Image object is NULL, cannot start fade animation\n");
        return;
    }
    
    if (!lv_obj_is_valid(img_obj)) {
        printf("Error: Image object is not valid, cannot start fade animation\n");
        return;
    }
    
    printf("Starting fade animation with image object: %p\n", (void*)img_obj);
    
    fade_img = img_obj;
    fade_step = 0;
    
    // 计算间隔时间
    fade_step_interval = fade_duration_ms / fade_steps;
    
    // 确保图片已经是透明状态（可能在之前已经设置过）
    if (lv_obj_check_type(fade_img, &lv_image_class)) {
        lv_obj_set_style_img_opa(fade_img, LV_OPA_TRANSP, 0);
    } else {
        lv_obj_set_style_bg_opa(fade_img, LV_OPA_TRANSP, 0);
    }
    lv_obj_invalidate(fade_img); // 强制刷新
    
    // 给系统时间处理透明度设置
    for(int i = 0; i < 5; i++) {
        lv_timer_handler();
        usleep(5000);
    }
    
    // 创建渐变定时器
    fade_timer = lv_timer_create(fade_animation_cb, fade_step_interval, NULL);
    lv_timer_set_repeat_count(fade_timer, fade_steps + 1);
    
    printf("Fade animation started: %d steps, %dms interval, timer: %p\n", 
           fade_steps, fade_step_interval, (void*)fade_timer);
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
    
    // 检查图片对象的有效性和尺寸
    if (img_obj == NULL) {
        printf("Error: Image object is NULL, cannot initialize boot media\n");
        // 仍然开始音频播放
        start_audio_playback();
        return;
    }
    
    if (!lv_obj_is_valid(img_obj)) {
        printf("Error: Image object is not valid, cannot initialize boot media\n");
        start_audio_playback();
        return;
    }
    
    // 检查图片是否有有效尺寸
    lv_coord_t img_width = lv_obj_get_width(img_obj);
    lv_coord_t img_height = lv_obj_get_height(img_obj);
    
    if (img_width <= 0 || img_height <= 0) {
        printf("Warning: Image has invalid dimensions (%dx%d), skipping fade animation\n", 
               img_width, img_height);
        // 直接标记渐变完成，只进行音频播放
        fade_finished = 1;
        start_audio_playback();
        return;
    }
    
    printf("Image object validated: %dx%d\n", img_width, img_height);
    
    // 首先将图片设置为透明状态
    prepare_image_for_fade(img_obj);
    
    // 开始渐变动画
    start_fade_animation(img_obj);
    
    // 开始音频播放
    start_audio_playback();
    
    printf("Boot media initialization completed\n");
}