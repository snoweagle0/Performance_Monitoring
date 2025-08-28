#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern lv_obj_t *parent;
static lv_style_t style;
static lv_style_t style_rect; /* 定义一个样式变量 */
static lv_style_t style_label;
extern lv_obj_t *bmeun;
lv_obj_t *bin_ui;

// 外部函数声明
extern void init_memory_monitor(); // 内存监控初始化函数
extern void init_lock_exit_buttons(void); // Lock和Exit按钮初始化函数

// 系统信息面板相关变量
static lv_obj_t *info_panel = NULL;
static lv_obj_t *clock_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *ip_label = NULL;
static lv_obj_t *network_status_label = NULL;
static lv_timer_t *info_timer = NULL;

// 系统信息面板函数声明
void create_info_panel(void);
void update_clock_display(void);
void update_network_status(void);
void info_timer_cb(lv_timer_t *timer);
char* get_local_ip(void);
int is_network_connected(void);

// CPU负载测试相关
static int cpu_stress_running = 0;
static pthread_t stress_thread[4]; // 创建4个线程
static int thread_count = 4;
void* cpu_stress_worker(void* arg);
void start_cpu_stress();
void stop_cpu_stress();
void stress_button_event_cb(lv_event_t * e);
int get_cpu_used();
void timer_cb(lv_timer_t *timer);
void init_arc_timer(lv_obj_t *arc, lv_obj_t *label);
void set_cpu_use_percent(lv_event_t * e);
int cpu_use_date=0;
static lv_obj_t *cpu_percent_label = NULL; // 全局变量保存CPU百分比标签引用
void begin()
{
    // 主界面
    bin_ui = lv_obj_create(parent);
    lv_obj_set_size(bin_ui, 760, 600);
    lv_obj_set_pos(bin_ui, 200, 0); /* 设置 x 和 y 坐标 */
    lv_obj_align(bin_ui, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    // 禁用滚动条和滚动功能
    lv_obj_set_scrollbar_mode(bin_ui, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(bin_ui, LV_OBJ_FLAG_SCROLLABLE);
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
    lv_obj_add_style(cpu_use_percent, &style_label, cpu_use_date);
    // 修复：传递cpu_use作为用户数据，而不是NULL
    lv_obj_add_event_cb(cpu_use_percent, set_cpu_use_percent, LV_EVENT_REFRESH,&cpu_use_date);
    lv_obj_send_event(cpu_use_percent, LV_EVENT_REFRESH, NULL);
    
    // 我们需要一个定时器来周期性地获取CPU使用率并更新弧形控件的显示
    init_arc_timer(cpu_use, cpu_use_percent);
    
    // 添加CPU压力测试按钮
    lv_obj_t *stress_btn = lv_button_create(bmeun);
    lv_obj_set_size(stress_btn, 150, 50);
    lv_obj_t *avtar = lv_obj_get_child(bmeun, 0); // 获取avtar控件
    lv_obj_align_to(stress_btn, avtar, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_event_cb(stress_btn, stress_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(stress_btn);
    lv_label_set_text(btn_label, "Start Stress Test");
    lv_obj_center(btn_label);
    
    // 创建系统信息面板
    create_info_panel();
    
    init_memory_monitor();// 初始化内存监控界面
    init_lock_exit_buttons(); // 初始化Lock和Exit按钮（放在最后，这样可以正确定位）

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
    cpu_use_date=cpu_used;
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

        // 更新CPU百分比文本标签
        if (cpu_percent_label != NULL) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d%%", cpu_usage);
            lv_label_set_text(cpu_percent_label, buf);
        }

        // 或者通过角度设置（如果版本较旧）
        // int end_angle = (cpu_usage * 360) / 100;
        // lv_arc_set_angles(arc, 0, end_angle);

        // 可选：动态改变指示弧的颜色（例如：使用率超过80%变为红色）
        if (cpu_usage > 80)
        {
            lv_obj_set_style_arc_color(arc, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
        }
        else
        {
            lv_obj_set_style_arc_color(arc, lv_color_hex(0x0000FF), LV_PART_INDICATOR);
        }
    }
}

// 在程序初始化部分创建定时器
void init_arc_timer(lv_obj_t *arc, lv_obj_t *label)
{
    cpu_percent_label = label; // 保存标签引用到全局变量
    // 创建定时器，每1000毫秒（1秒）执行一次，并将弧形控件作为用户数据传递
    lv_timer_t *timer = lv_timer_create(timer_cb, 1000, arc); // 假设arc已创建
    lv_timer_set_repeat_count(timer, -1);                     // 设置无限重复
}
void set_cpu_use_percent(lv_event_t * e)
{
    lv_obj_t * cpu_use_percent=lv_event_get_target(e);
    // 获取当前的CPU使用率
    int cpu_usage = cpu_use_date; // 使用前面定义的函数获取CPU使用率
    // 更新标签文本
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", cpu_usage);
    lv_label_set_text(cpu_use_percent, buf);
}

// CPU压力测试工作线程
void* cpu_stress_worker(void* arg)
{
    int thread_id = (int)(long)arg;
    printf("CPU stress test thread %d started...\n", thread_id);
    
    while (cpu_stress_running) {
        // 执行复杂的数学运算来增加CPU负载
        volatile double result = thread_id * 1.0;
        for (int i = 0; i < 2000000; i++) {
            result += sin(i + thread_id) * cos(i + thread_id) * tan(i + thread_id);
            result = sqrt(fabs(result) + 1);
            // 增加更多计算
            result *= log(fabs(result) + 1);
        }
        // 短暂休息，避免完全锁死系统
        usleep(500); // 0.5毫秒
    }
    
    printf("CPU stress test thread %d finished\n", thread_id);
    return NULL;
}

// 启动CPU压力测试
void start_cpu_stress()
{
    if (!cpu_stress_running) {
        cpu_stress_running = 1;
        printf("Starting %d CPU stress test threads...\n", thread_count);
        for (int i = 0; i < thread_count; i++) {
            if (pthread_create(&stress_thread[i], NULL, cpu_stress_worker, (void*)(long)i) != 0) {
                printf("Failed to create stress test thread %d\n", i);
                cpu_stress_running = 0;
                return;
            }
        }
    }
}

// 停止CPU压力测试
void stop_cpu_stress()
{
    if (cpu_stress_running) {
        cpu_stress_running = 0;
        for (int i = 0; i < thread_count; i++) {
            pthread_join(stress_thread[i], NULL);
        }
        printf("All CPU stress test threads have been stopped\n");
    }
}

// 压力测试按钮事件回调
void stress_button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t*)lv_event_get_target(e);
    
    if(code == LV_EVENT_CLICKED) {
        if (!cpu_stress_running) {
            start_cpu_stress();
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            lv_label_set_text(label, "Stop Stress Test");
            printf("Starting CPU stress test\n");
        } else {
            stop_cpu_stress();
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            lv_label_set_text(label, "Start Stress Test");
            printf("Stopping CPU stress test\n");
        }
    }
}

// 创建系统信息面板
void create_info_panel(void)
{
    if (bin_ui == NULL) {
        printf("Error: bin_ui is NULL, cannot create info panel\n");
        return;
    }
    
    // 创建信息面板容器 - 定位在bin_ui的右侧区域
    info_panel = lv_obj_create(bin_ui);
    lv_obj_set_size(info_panel, 320, 350);
    lv_obj_align(info_panel, LV_ALIGN_TOP_RIGHT, -10, 50);
    
    // 禁用滚动条和滚动功能
    lv_obj_set_scrollbar_mode(info_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(info_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    // 设置容器样式
    lv_obj_set_style_bg_color(info_panel, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(info_panel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(info_panel, 1, 0);
    lv_obj_set_style_radius(info_panel, 8, 0);
    lv_obj_set_style_pad_all(info_panel, 15, 0);
    
    // 创建标题
    lv_obj_t *title_label = lv_label_create(info_panel);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(title_label, "System Info");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
    
    // 创建时钟显示
    clock_label = lv_label_create(info_panel);
    lv_obj_set_style_text_color(clock_label, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_font(clock_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(clock_label, "00:00:00");
    lv_obj_align_to(clock_label, title_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    
    // 创建日期显示
    date_label = lv_label_create(info_panel);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xcccccc), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(date_label, "2025-08-27");
    lv_obj_align_to(date_label, clock_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    
    // 创建网络状态标题
    lv_obj_t *network_title = lv_label_create(info_panel);
    lv_obj_set_style_text_color(network_title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(network_title, &lv_font_montserrat_18, 0);
    lv_label_set_text(network_title, "Network Status");
    lv_obj_align_to(network_title, date_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    
    // 创建IP地址显示
    lv_obj_t *ip_title = lv_label_create(info_panel);
    lv_obj_set_style_text_color(ip_title, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(ip_title, &lv_font_montserrat_16, 0);
    lv_label_set_text(ip_title, "IP Address:");
    lv_obj_align_to(ip_title, network_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    
    ip_label = lv_label_create(info_panel);
    lv_obj_set_style_text_color(ip_label, lv_color_hex(0x00aaff), 0);
    lv_obj_set_style_text_font(ip_label, &lv_font_montserrat_18, 0);
    lv_label_set_text(ip_label, "Getting...");
    lv_obj_align_to(ip_label, ip_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    
    // 创建连接状态显示
    lv_obj_t *status_title = lv_label_create(info_panel);
    lv_obj_set_style_text_color(status_title, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(status_title, &lv_font_montserrat_16, 0);
    lv_label_set_text(status_title, "Connection:");
    lv_obj_align_to(status_title, ip_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    
    network_status_label = lv_label_create(info_panel);
    lv_obj_set_style_text_color(network_status_label, lv_color_hex(0xffaa00), 0);
    lv_obj_set_style_text_font(network_status_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(network_status_label, "Checking...");
    lv_obj_align_to(network_status_label, status_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
    
    // 立即更新一次信息
    update_clock_display();
    update_network_status();
    
    // 创建定时器，每秒更新一次
    info_timer = lv_timer_create(info_timer_cb, 1000, NULL);
    lv_timer_set_repeat_count(info_timer, -1);
    
    printf("System info panel created successfully\n");
}

// 更新时钟显示
void update_clock_display(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[16];
    char date_str[16];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // 格式化时间
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
    
    // 更新标签
    if (clock_label != NULL) {
        lv_label_set_text(clock_label, time_str);
    }
    if (date_label != NULL) {
        lv_label_set_text(date_label, date_str);
    }
}

// 获取本地IP地址
char* get_local_ip(void)
{
    static char ip_str[32] = "N/A";
    FILE *fp;
    char buffer[256];
    
    // 首先尝试使用hostname -I 命令获取IP地址
    fp = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            // 去除换行符
            buffer[strcspn(buffer, "\n")] = 0;
            if (strlen(buffer) > 7) { // 至少要有x.x.x.x的格式
                strncpy(ip_str, buffer, sizeof(ip_str) - 1);
                ip_str[sizeof(ip_str) - 1] = '\0';
                pclose(fp);
                return ip_str;
            }
        }
        pclose(fp);
    }
    
    // 如果第一种方法失败，尝试使用ip命令
    fp = popen("ip route get 8.8.8.8 2>/dev/null | awk '{print $7; exit}'", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            if (strlen(buffer) > 7) {
                strncpy(ip_str, buffer, sizeof(ip_str) - 1);
                ip_str[sizeof(ip_str) - 1] = '\0';
                pclose(fp);
                return ip_str;
            }
        }
        pclose(fp);
    }
    
    // 如果还是失败，尝试使用ifconfig
    fp = popen("ifconfig 2>/dev/null | grep 'inet ' | grep -v '127.0.0.1' | head -1 | awk '{print $2}'", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            if (strlen(buffer) > 7) {
                strncpy(ip_str, buffer, sizeof(ip_str) - 1);
                ip_str[sizeof(ip_str) - 1] = '\0';
            }
        }
        pclose(fp);
    }
    
    printf("Debug: Got IP address: %s\n", ip_str);
    return ip_str;
}

// 检查网络连接状态
int is_network_connected(void)
{
    FILE *fp;
    char buffer[256];
    int connected = 0;
    
    // 检查网络接口状态
    fp = popen("ip route | grep default", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            connected = 1; // 有默认路由说明网络已连接
        }
        pclose(fp);
    }
    
    return connected;
}

// 更新网络状态
void update_network_status(void)
{
    char *ip = get_local_ip();
    int connected = is_network_connected();
    
    // 更新IP地址
    if (ip_label != NULL) {
        lv_label_set_text(ip_label, ip);
    }
    
    // 更新连接状态
    if (network_status_label != NULL) {
        if (connected) {
            lv_label_set_text(network_status_label, "Connected");
            lv_obj_set_style_text_color(network_status_label, lv_color_hex(0x00ff00), 0);
        } else {
            lv_label_set_text(network_status_label, "Disconnected");
            lv_obj_set_style_text_color(network_status_label, lv_color_hex(0xff0000), 0);
        }
    }
}

// 信息面板定时器回调
void info_timer_cb(lv_timer_t *timer)
{
    update_clock_display();
    
    // 网络状态更新频率较低，每10秒更新一次
    static int network_update_counter = 0;
    network_update_counter++;
    if (network_update_counter >= 10) {
        update_network_status();
        network_update_counter = 0;
    }
}