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
