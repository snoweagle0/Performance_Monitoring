#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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