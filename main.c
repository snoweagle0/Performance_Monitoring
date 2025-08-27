#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media.h"

lv_obj_t *parent;
lv_obj_t * bmeun;

// 启动界面相关变量
static lv_obj_t *boot_screen = NULL;
static lv_obj_t *start_btn = NULL;
static int boot_finished = 0;

// 函数声明
void B_meun(void);
void begin();
void show_boot_screen(void);
void start_button_event_cb(lv_event_t * e);
void hide_boot_screen(void);
void check_audio_status(lv_timer_t *timer);
int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t *disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_indev_t *indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    
    // 首先显示启动界面
    show_boot_screen();
    
    /*Handle LVGL tasks*/
    while (1)
    {
        lv_timer_handler();
        usleep(5000);
        
        // 当启动界面结束后，初始化主程序界面
        if (boot_finished) {
            B_meun();
            begin();
            boot_finished = 0; // 防止重复初始化
            break;
        }
    }
    
    // 主程序循环
    while (1)
    {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
LV_IMAGE_DECLARE(test_img);

void B_meun(void)
{
    /* 在活动屏幕上，创建一个基础对象
   默认大小
   */
    /* 设置该基础对象parent的大小 */
    parent=lv_obj_create(lv_screen_active());
    lv_obj_set_size(parent, 1024, 600);
    /* 在基础对象parent上，创建一个子对象child */
    //lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);

    bmeun = lv_obj_create(parent);
    lv_obj_set_size(bmeun,200,600);
    lv_obj_t * avtar = lv_img_create(bmeun);
    // 注释掉有问题的函数调用
    // void lv_image_set_src(avtar, img_test);
    lv_img_set_src(avtar, "A:./resource/img.bmp"); // 更新图片路径
    lv_obj_set_width(avtar, LV_SIZE_CONTENT);
    lv_obj_set_height(avtar, LV_SIZE_CONTENT);
    //lv_image_set_scale(avtar, 64);
    lv_obj_align(avtar, LV_ALIGN_TOP_MID, 0, 0);

    // 播放声音资源
    return;
}

// 显示启动界面
void show_boot_screen(void)
{
    // 创建全屏启动界面
    boot_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(boot_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(boot_screen, lv_color_black(), 0);
    lv_obj_clear_flag(boot_screen, LV_OBJ_FLAG_SCROLLABLE);
    // 移除边距和填充，确保真正全屏
    lv_obj_set_style_pad_all(boot_screen, 0, 0);
    lv_obj_set_style_margin_all(boot_screen, 0, 0);

    // 创建启动图片 - 使用LVGL原生缩放API
    lv_obj_t *boot_img = lv_img_create(boot_screen);
    lv_img_set_src(boot_img, "A:./resource/boot_img.bmp");
    
    // 立即设置为透明，避免图片闪现
    lv_obj_set_style_img_opa(boot_img, LV_OPA_TRANSP, 0);
    
    // 给图片加载充足时间
    for(int i = 0; i < 25; i++) {
        lv_timer_handler();
        usleep(20000); // 增加加载时间
    }
    
    // 获取屏幕尺寸
    lv_coord_t screen_width = lv_obj_get_width(lv_scr_act());
    lv_coord_t screen_height = lv_obj_get_height(lv_scr_act());
    printf("Screen size: %d x %d\n", screen_width, screen_height);
    
    // 获取图片对象的实际尺寸（加载后）
    lv_coord_t img_width = lv_obj_get_width(boot_img);
    lv_coord_t img_height = lv_obj_get_height(boot_img);
    printf("Image object size: %d x %d\n", img_width, img_height);
    
    // 确保图片已经加载
    if (img_width > 0 && img_height > 0) {
        // 计算缩放比例（LVGL使用256作为100%的基准）
        uint16_t zoom_factor_width = 256; // 默认100%
        uint16_t zoom_factor_height = 256;
        
        // 计算基于宽度的缩放比例
        if (img_width > screen_width) {
            zoom_factor_width = (screen_width * 256) / img_width;
        }
        
        // 计算基于高度的缩放比例  
        if (img_height > screen_height) {
            zoom_factor_height = (screen_height * 256) / img_height;
        }
        
        // 选择较小的缩放比例以确保图片完全显示在屏幕内
        uint16_t zoom_factor = (zoom_factor_width < zoom_factor_height) ? zoom_factor_width : zoom_factor_height;
        
        printf("Zoom factors: width=%d, height=%d, selected=%d (%.1f%%)\n", 
               zoom_factor_width, zoom_factor_height, zoom_factor, (zoom_factor * 100.0f / 256));
        
        // 应用缩放 - 使用LVGL的缩放API
        if (zoom_factor != 256) {
            lv_img_set_zoom(boot_img, zoom_factor);
            printf("Applied zoom factor: %d\n", zoom_factor);
        } else {
            printf("No zoom needed, using original size\n");
        }
        
    } else {
        printf("Warning: Could not get image dimensions, image may not be loaded\n");
        // 尝试设置一个适中的缩放
        lv_img_set_zoom(boot_img, 200); // 约78%缩放
    }
    
    // 将图片居中显示
    lv_obj_center(boot_img);
    printf("Image centered on screen\n");
    
    printf("Image display setup completed\n");
    
    // 创建 Start 按钮 - 覆盖在图片上面，但初始为禁用状态
    start_btn = lv_btn_create(boot_screen);
    lv_obj_set_size(start_btn, 200, 60);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -30); // 从底部向上30像素
    lv_obj_add_event_cb(start_btn, start_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // 初始禁用按钮，直到音频播放完成
    lv_obj_add_state(start_btn, LV_STATE_DISABLED);
    
    // 设置按钮样式 - 半透明背景以确保在图片上可见
    lv_obj_set_style_bg_color(start_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(start_btn, LV_OPA_80, 0); // 80%不透明度，略透明
    lv_obj_set_style_bg_color(start_btn, lv_palette_darken(LV_PALETTE_BLUE, 2), LV_STATE_PRESSED);
    lv_obj_set_style_radius(start_btn, 10, 0);
    lv_obj_set_style_border_color(start_btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(start_btn, 2, 0);
    lv_obj_set_style_shadow_width(start_btn, 10, 0);
    lv_obj_set_style_shadow_color(start_btn, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(start_btn, LV_OPA_50, 0);
    
    // 禁用状态样式 - 灰色显示
    lv_obj_set_style_bg_color(start_btn, lv_palette_main(LV_PALETTE_GREY), LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(start_btn, LV_OPA_50, LV_STATE_DISABLED);
    
    // 创建按钮标签
    lv_obj_t *btn_label = lv_label_create(start_btn);
    lv_label_set_text(btn_label, "Loading...");
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_label);
    
    // 启动媒体功能 - 渐变动画和音频播放
    init_boot_media(boot_img);
    
    // 创建定时器检查音频播放状态
    lv_timer_t *status_timer = lv_timer_create(check_audio_status, 500, btn_label);
    lv_timer_set_repeat_count(status_timer, -1); // 无限重复直到删除
    
    printf("Boot screen initialization completed\n");
}

// Start 按钮事件回调
void start_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        // 严格检查：必须媒体完全准备好且按钮未被禁用
        if (is_boot_media_ready() && !lv_obj_has_state(start_btn, LV_STATE_DISABLED)) {
            printf("Start button clicked, all media completed, hiding boot screen...\n");
            hide_boot_screen();
            boot_finished = 1; // 标记启动界面结束
        } else {
            printf("Start button clicked but conditions not met - Media ready: %s, Button disabled: %s\n", 
                   is_boot_media_ready() ? "YES" : "NO",
                   lv_obj_has_state(start_btn, LV_STATE_DISABLED) ? "YES" : "NO");
        }
    }
}

// 音频状态检查回调
void check_audio_status(lv_timer_t *timer)
{
    lv_obj_t *btn_label = (lv_obj_t *)timer->user_data;
    
    if (is_boot_media_ready()) {
        // 所有启动媒体都已完成，启用按钮
        lv_obj_clear_state(start_btn, LV_STATE_DISABLED);
        lv_label_set_text(btn_label, "START");
        printf("All boot media completed (fade + audio), button enabled\n");
        
        // 删除定时器
        lv_timer_delete(timer);
    } else {
        // 媒体还未完全准备好，保持禁用状态
        static int dots = 0;
        dots = (dots + 1) % 4;
        char loading_text[20];
        strcpy(loading_text, "Loading");
        for(int i = 0; i < dots; i++) {
            strcat(loading_text, ".");
        }
        lv_label_set_text(btn_label, loading_text);
    }
}

// 隐藏启动界面
void hide_boot_screen(void)
{
    if (boot_screen != NULL) {
        lv_obj_del(boot_screen);
        boot_screen = NULL;
        printf("Boot screen hidden\n");
    }
}
