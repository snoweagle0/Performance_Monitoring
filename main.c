#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lv_obj_t *parent;
lv_obj_t * bmeun;

// 启动界面相关变量
static lv_obj_t *boot_screen = NULL;
static int boot_finished = 0;

// 函数声明
void B_meun(void);
void begin();
void show_boot_screen(void);
void start_button_event_cb(lv_event_t * e);
void hide_boot_screen(void);
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

    // 创建启动图片 - 自适应全屏显示
    lv_obj_t *boot_img = lv_img_create(boot_screen);
    lv_img_set_src(boot_img, "A:./resource/boot_img.bmp");
    
    // 等待图片加载完成
    lv_timer_handler();
    usleep(10000);
    
    // 获取图片的原始尺寸
    lv_coord_t img_w = lv_obj_get_width(boot_img);
    lv_coord_t img_h = lv_obj_get_height(boot_img);
    
    printf("Original image size: %d x %d\n", img_w, img_h);
    printf("Screen size: 1024 x 600\n");
    
    // 计算缩放比例以适应屏幕，保持宽高比
    float scale_x = 1024.0f / img_w;
    float scale_y = 600.0f / img_h;
    float scale = (scale_x < scale_y) ? scale_x : scale_y; // 选择较小的缩放比例
    
    printf("Scale factors: x=%.3f, y=%.3f, final=%.3f\n", scale_x, scale_y, scale);
    
    // 计算缩放后的实际尺寸
    lv_coord_t scaled_w = (lv_coord_t)(img_w * scale);
    lv_coord_t scaled_h = (lv_coord_t)(img_h * scale);
    
    printf("Scaled image size: %d x %d\n", scaled_w, scaled_h);
    
    // 使用变换矩阵进行缩放（LVGL v8/v9的方法）
    // 或者直接设置对象大小让LVGL自动缩放
    if (scale < 1.0f) {
        // 如果需要缩小，设置目标大小让LVGL自动处理
        lv_obj_set_size(boot_img, scaled_w, scaled_h);
        printf("Applied scaling by setting size to %d x %d\n", scaled_w, scaled_h);
    } else {
        // 如果不需要缩放或需要放大，保持原始大小
        lv_obj_set_size(boot_img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        printf("Using original image size\n");
    }
    
    // 居中对齐
    lv_obj_align(boot_img, LV_ALIGN_CENTER, 0, 0);
    
    // 设置图片样式
    lv_obj_set_style_pad_all(boot_img, 0, 0);
    lv_obj_set_style_margin_all(boot_img, 0, 0);
    
    // 创建 Start 按钮 - 覆盖在图片上面
    lv_obj_t *start_btn = lv_btn_create(boot_screen);
    lv_obj_set_size(start_btn, 200, 60);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_MID, 0, -30); // 从底部向上30像素
    lv_obj_add_event_cb(start_btn, start_button_event_cb, LV_EVENT_CLICKED, NULL);
    
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
    
    // 创建按钮标签
    lv_obj_t *btn_label = lv_label_create(start_btn);
    lv_label_set_text(btn_label, "START");
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_label);
}

// Start 按钮事件回调
void start_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Start button clicked, hiding boot screen...\n");
        hide_boot_screen();
        boot_finished = 1; // 标记启动界面结束
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
