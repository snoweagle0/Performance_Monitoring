#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

extern lv_obj_t *parent;
static lv_style_t style;
static lv_style_t style_rect; /* 定义一个样式变量 */
static lv_style_t style_label;
extern lv_obj_t *bmeun;
int get_cpu_used();
void timer_cb(lv_timer_t *timer);
void init_arc_timer(lv_obj_t *arc);
void begin()
{
    // 主界面
    lv_obj_t *bin_ui = lv_obj_create(parent);
    lv_obj_set_size(bin_ui, 760, 600);
    lv_obj_set_pos(bin_ui, 200, 0); /* 设置 x 和 y 坐标 */
    lv_obj_align(bin_ui, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_style_init(&style_rect); /* 初始化样式 */
    // lv_style_set_bg_color(&style_rect, lv_palette_main(LV_PALETTE_BLUE));            /* 设置背景色 */
    lv_style_set_border_color(&style_rect, lv_palette_main(LV_PALETTE_DEEP_ORANGE)); /* 设置边框颜色 */
    lv_style_set_border_width(&style_rect, 0);                                       /* 设置边框宽度 */
    lv_style_set_radius(&style_rect, 0);                                             /* 设置圆角半径 */
    lv_obj_add_style(bin_ui, &style_rect, 0);                                        /* 将样式应用到矩形控件 */
    lv_style_init(&style);

    lv_obj_t *machine_name = lv_label_create(bin_ui);
    const lv_font_t *font = &lv_font_montserrat_30;
    lv_style_set_text_font(&style, font);
    lv_obj_add_style(machine_name, &style, 0);
    lv_label_set_text(machine_name, "RK356X");
    lv_obj_align(machine_name, LV_ALIGN_TOP_LEFT, 0, 0);

    // cpu使用率显示
    lv_obj_t *cpu_use = lv_arc_create(bin_ui);
    lv_obj_align_to(cpu_use, machine_name, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_arc_color(cpu_use, lv_color_hex3(0xddc), LV_PART_MAIN);
    lv_obj_set_style_arc_color(cpu_use, lv_color_hex3(0xff0), LV_PART_INDICATOR);
    lv_obj_remove_flag(cpu_use, LV_OBJ_FLAG_CLICKABLE); // 防止用户拖动
    lv_obj_remove_flag(cpu_use, LV_OBJ_FLAG_CLICKABLE); // 移除点击
    lv_arc_set_range(cpu_use, 0, 100);                  // 范围0到100，对应0%到100%
    lv_arc_set_value(cpu_use, 0);                       // 初始值设为0
    // lv_obj_set_style_bg_color(cpu_use, lv_color_hex3(0xf60), LV_PART_KNOB);
    // 我们需要一个定时器来周期性地获取CPU使用率并更新弧形控件的显示
    init_arc_timer(cpu_use);
    

    lv_obj_remove_style(cpu_use, NULL, LV_PART_KNOB);
    lv_obj_t *cpu_use_labe = lv_label_create(bin_ui);
    lv_obj_align_to(cpu_use_labe, cpu_use, LV_ALIGN_BOTTOM_MID, 0, 30);
    // 设置cpu_use_labe为无边框，边框宽度为0像素
    lv_style_init(&style_label);
    lv_style_set_border_width(&style_label, 0);
    lv_obj_add_style(cpu_use_labe, &style_label, 0);
    lv_label_set_text(cpu_use_labe, "CPU");
    //显示cpu的使用百分比，用lable设置实时显示
    lv_obj_t *cpu_use_percent = lv_label_create(bin_ui);
    lv_obj_align_to(cpu_use_percent, cpu_use, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(cpu_use_percent, &style_label, 0);

    return;
}

// 获取Linux环境下的cpu使用率，并返回cpu以使用的百分比
int get_cpu_used()
{
    static long last_total = 0;
    static long last_idle = 0;
    FILE *fp;
    char buffer[1024];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    long total, total_diff, idle_diff;
    int cpu_used;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Failed to open /proc/stat");
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Failed to read /proc/stat");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    sscanf(buffer, "cpu  %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

    total = user + nice + system + idle + iowait + irq + softirq + steal;

    total_diff = total - last_total;
    idle_diff = idle - last_idle;

    if (total_diff == 0)
    {
        return 0; // 避免除以零
    }

    cpu_used = (int)((total_diff - idle_diff) * 100 / total_diff);

    last_total = total;
    last_idle = idle;

    return cpu_used;
}

void timer_cb(lv_timer_t *timer)
{
    // 假设 arc_obj 是之前创建的弧形控件，可以通过 timer->user_data 传递
     lv_obj_t *arc = (lv_obj_t *)timer->user_data;
    int cpu_usage = get_cpu_used(); // 使用前面定义的函数获取CPU使用率

    if (cpu_usage >= 0)
    {
        // 更新弧形显示
        lv_arc_set_value(arc, cpu_usage); // 推荐方式，如果LVGL版本支持

        // 或者通过角度设置（如果版本较旧）
        // int end_angle = (cpu_usage * 360) / 100;
        // lv_arc_set_angles(arc, 0, end_angle);

        // 可选：动态改变指示弧的颜色（例如：使用率超过80%变为红色）
        if (cpu_usage > 80)
        {
            lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
        }
    }
}

// 在程序初始化部分创建定时器
void init_arc_timer(lv_obj_t *arc)
{
    // 创建定时器，每1000毫秒（1秒）执行一次，并将弧形控件作为用户数据传递
    lv_timer_t *timer = lv_timer_create(timer_cb, 1000, arc); // 假设arc已创建
    lv_timer_set_repeat_count(timer, -1);                     // 设置无限重复
}
