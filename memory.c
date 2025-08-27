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
static lv_style_t style;
static lv_style_t style_rect; /* 定义一个样式变量 */
static lv_style_t style_label;
extern lv_obj_t *bmeun;
extern lv_obj_t *bin_ui;

// 内存监控相关变量
static lv_obj_t *memory_bar = NULL;
static lv_obj_t *memory_label = NULL;
static lv_obj_t *memory_bg_rect = NULL;
static lv_style_t memory_bar_style;
static lv_style_t memory_bg_style;

// 内存测试相关变量
static int memory_test_running = 0;
static pthread_t memory_test_thread;
static void **allocated_blocks = NULL;
static int allocated_count = 0;
static const int max_blocks = 200;           // 减少最大块数，但增加每块大小
static size_t block_size = 10 * 1024 * 1024; // 改为10MB per block，更快看到效果

// 内存信息结构体
typedef struct {
    unsigned long total_memory;    // 总内存 (KB)
    unsigned long used_memory;     // 已使用内存 (KB)
    unsigned long free_memory;     // 可用内存 (KB)
    float usage_percent;           // 使用率百分比
} memory_info_t;

// 函数声明
int get_memory_info(memory_info_t *mem_info);
void create_memory_monitor();
void memory_timer_cb(lv_timer_t *timer);
void init_memory_timer();
void init_memory_monitor(); // 供外部调用的接口函数
// 内存测试相关函数
void* memory_test_worker(void* arg);
void start_memory_test();
void stop_memory_test();
void memory_test_button_event_cb(lv_event_t * e);
void create_memory_test_button();

// 获取Linux系统内存信息
int get_memory_info(memory_info_t *mem_info) {
    FILE *fp;
    char buffer[256];
    unsigned long mem_total = 0, mem_free = 0, mem_available = 0, buffers = 0, cached = 0;
    
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (sscanf(buffer, "MemTotal: %lu kB", &mem_total) == 1) {
            continue;
        }
        if (sscanf(buffer, "MemFree: %lu kB", &mem_free) == 1) {
            continue;
        }
        if (sscanf(buffer, "MemAvailable: %lu kB", &mem_available) == 1) {
            continue;
        }
        if (sscanf(buffer, "Buffers: %lu kB", &buffers) == 1) {
            continue;
        }
        if (sscanf(buffer, "Cached: %lu kB", &cached) == 1) {
            continue;
        }
    }
    fclose(fp);
    
    if (mem_total == 0) {
        return -1;
    }
    
    mem_info->total_memory = mem_total;
    mem_info->free_memory = mem_available > 0 ? mem_available : mem_free;
    mem_info->used_memory = mem_total - mem_info->free_memory;
    mem_info->usage_percent = (float)mem_info->used_memory / mem_total * 100.0f;
    
    return 0;
}

// 创建内存监控显示控件
void create_memory_monitor() {
    if (bin_ui == NULL) {
        printf("Error: bin_ui is NULL, cannot create memory monitor\n");
        return;
    }
    
    // 创建内存标题标签 - 调整位置与CPU标签对齐
    lv_obj_t *memory_title = lv_label_create(bin_ui);
    const lv_font_t *font = &lv_font_montserrat_20;
    lv_style_t title_style;
    lv_style_init(&title_style);
    lv_style_set_text_font(&title_style, font);
    lv_obj_add_style(memory_title, &title_style, 0);
    lv_label_set_text(memory_title, "Memory Usage");
    // 调整位置：在CPU弧形控件右侧，与CPU标签水平对齐
    // 获取cpu_use_labe控件
    
    lv_obj_t *cpu_use_lable = lv_obj_get_child(bin_ui,2); 
    lv_obj_align_to(memory_title, cpu_use_lable, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    //lv_obj_align(memory_title, LV_ALIGN_TOP_LEFT, 300, 200);
    
    // 创建内存条背景矩形
    memory_bg_rect = lv_obj_create(bin_ui);
    lv_obj_set_size(memory_bg_rect, 300, 30);
    lv_obj_align_to(memory_bg_rect, memory_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    // 设置背景样式
    lv_style_init(&memory_bg_style);
    lv_style_set_bg_color(&memory_bg_style, lv_color_hex(0x333333));
    lv_style_set_border_color(&memory_bg_style, lv_color_hex(0x666666));
    lv_style_set_border_width(&memory_bg_style, 2);
    lv_style_set_radius(&memory_bg_style, 5);
    lv_obj_add_style(memory_bg_rect, &memory_bg_style, 0);
    
    // 创建内存使用条
    memory_bar = lv_obj_create(memory_bg_rect);
    lv_obj_set_size(memory_bar, 0, 26);  // 初始宽度为0
    lv_obj_align(memory_bar, LV_ALIGN_LEFT_MID, 0, 0); // 改为0边距，紧贴左侧
    
    // 设置内存条样式
    lv_style_init(&memory_bar_style);
    lv_style_set_bg_color(&memory_bar_style, lv_color_hex(0x00FF00)); // 默认绿色
    lv_style_set_border_width(&memory_bar_style, 0);
    lv_style_set_radius(&memory_bar_style, 3);
    // 确保内存条紧贴左侧，不留边距
    lv_style_set_pad_left(&memory_bar_style, 0);
    lv_style_set_pad_right(&memory_bar_style, 0);
    lv_style_set_pad_top(&memory_bar_style, 0);
    lv_style_set_pad_bottom(&memory_bar_style, 0);
    lv_obj_add_style(memory_bar, &memory_bar_style, 0);
    
    // 创建内存信息显示标签
    memory_label = lv_label_create(bin_ui);
    lv_obj_align_to(memory_label, memory_bg_rect, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_label_set_text(memory_label, "Memory: 0 MB / 0 MB (0%)");
    
    // 创建内存测试按钮
    create_memory_test_button();
    
    // 启动定时器
    init_memory_timer();
}

// 内存监控定时器回调函数
void memory_timer_cb(lv_timer_t *timer) {
    memory_info_t mem_info;
    static int update_count = 0;
    
    if (get_memory_info(&mem_info) == 0) {
        // 更新内存条宽度 - 调整计算以适应新的定位
        int bar_width = (int)(298 * mem_info.usage_percent / 100.0f); // 298 = 300 - 2 (边框宽度)
        if (bar_width < 1) bar_width = 1; // 确保最小宽度为1像素，避免完全消失
        lv_obj_set_width(memory_bar, bar_width);
        
        // 根据内存使用率改变颜色
        lv_color_t bar_color;
        if (mem_info.usage_percent < 50.0f) {
            bar_color = lv_color_hex(0x00FF00); // 绿色
        } else if (mem_info.usage_percent < 80.0f) {
            bar_color = lv_color_hex(0xFFFF00); // 黄色
        } else if (mem_info.usage_percent < 85.0f) {
            bar_color = lv_color_hex(0xFF8000); // 橙色，接近安全限制
        } else {
            bar_color = lv_color_hex(0xFF0000); // 红色，超过安全限制
        }
        lv_style_set_bg_color(&memory_bar_style, bar_color);
        lv_obj_invalidate(memory_bar); // 刷新样式
        
        // 更新文本显示，显示更高精度
        char buf[100];
        if (mem_info.usage_percent >= 85.0f) {
            snprintf(buf, sizeof(buf), "Memory: %.0f MB / %.0f MB (%.1f%%) - SAFE LIMIT", 
                     mem_info.used_memory / 1024.0f, 
                     mem_info.total_memory / 1024.0f, 
                     mem_info.usage_percent);
        } else {
            snprintf(buf, sizeof(buf), "Memory: %.0f MB / %.0f MB (%.1f%%)", 
                     mem_info.used_memory / 1024.0f, 
                     mem_info.total_memory / 1024.0f, 
                     mem_info.usage_percent);
        }
        lv_label_set_text(memory_label, buf);
        
        // 减少控制台输出频率，只在内存测试时或每4次更新打印一次
        update_count++;
        if (memory_test_running || (update_count % 4 == 0)) {
            if (mem_info.usage_percent >= 85.0f) {
                printf("Memory usage: %.1f%% (%.0f MB / %.0f MB) - SAFE LIMIT ACTIVE\n", 
                       mem_info.usage_percent,
                       mem_info.used_memory / 1024.0f, 
                       mem_info.total_memory / 1024.0f);
            } else {
                printf("Memory usage: %.1f%% (%.0f MB / %.0f MB)\n", 
                       mem_info.usage_percent,
                       mem_info.used_memory / 1024.0f, 
                       mem_info.total_memory / 1024.0f);
            }
        }
    }
}

// 初始化内存监控定时器
void init_memory_timer() {
    // 创建定时器，每500毫秒更新一次内存信息，提高刷新频率
    lv_timer_t *timer = lv_timer_create(memory_timer_cb, 500, NULL);
    lv_timer_set_repeat_count(timer, -1); // 设置无限重复
    
    // 立即执行一次以显示初始状态
    memory_timer_cb(timer);
}

// 初始化内存监控 - 供外部调用的接口函数
void init_memory_monitor() {
    if (bin_ui != NULL) {
        create_memory_monitor();
        printf("Memory monitor initialized successfully\n");
    } else {
        printf("Warning: bin_ui is not initialized, memory monitor creation delayed\n");
    }
}

// 创建内存测试按钮
void create_memory_test_button() {
    if (bmeun == NULL) {
        printf("Error: bmeun is NULL, cannot create memory test button\n");
        return;
    }
    
    // 创建内存测试按钮
    lv_obj_t *mem_test_btn = lv_button_create(bmeun);
    lv_obj_set_size(mem_test_btn, 150, 50);
    
    // 定位按钮：在现有按钮下方
    lv_obj_t *existing_btn = NULL;
    // 查找现有的压力测试按钮
    for(int i = 0; i < lv_obj_get_child_count(bmeun); i++) {
        lv_obj_t *child = lv_obj_get_child(bmeun, i);
        if(lv_obj_check_type(child, &lv_button_class)) {
            existing_btn = child;
            break;
        }
    }
    
    if(existing_btn != NULL) {
        lv_obj_align_to(mem_test_btn, existing_btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    } else {
        // 如果没有找到现有按钮，则放在avatar下方
        lv_obj_t *avtar = lv_obj_get_child(bmeun, 0);
        lv_obj_align_to(mem_test_btn, avtar, LV_ALIGN_OUT_BOTTOM_MID, 0, 120);
    }
    
    lv_obj_add_event_cb(mem_test_btn, memory_test_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(mem_test_btn);
    lv_label_set_text(btn_label, "Start Mem Test");
    lv_obj_center(btn_label);
}

// 内存测试工作线程
void* memory_test_worker(void* arg) {
    printf("Memory test started...\n");
    
    // 初始化分配块数组
    allocated_blocks = (void**)malloc(max_blocks * sizeof(void*));
    if (allocated_blocks == NULL) {
        printf("Failed to allocate memory for block tracking\n");
        return NULL;
    }
    
    allocated_count = 0;
    int allocation_stopped = 0; // 标记是否停止分配
    
    while (memory_test_running) {
        // 如果还没有停止分配，继续检查和分配内存
        if (!allocation_stopped) {
            // 在分配前检查当前内存使用率
            memory_info_t mem_info;
            if (get_memory_info(&mem_info) == 0) {
                // 如果内存使用率已达到85%，停止分配但保持测试运行
                if (mem_info.usage_percent >= 85.0f) {
                    printf("Memory usage reached safety limit (%.1f%%), stopping new allocations but keeping test active\n", mem_info.usage_percent);
                    allocation_stopped = 1;
                }
            }
        }
        
        // 只有在未停止分配时才继续分配内存
        if (!allocation_stopped && allocated_count < max_blocks) {
            void *block = malloc(block_size);
            if (block != NULL) {
                // 写入数据以确保内存真正被使用
                memset(block, 0x55, block_size);
                allocated_blocks[allocated_count] = block;
                allocated_count++;
                float total_mb = (allocated_count * block_size) / (1024.0 * 1024.0);
                printf("Allocated block %d (%.1f MB total, %.1f GB)\n", 
                       allocated_count, total_mb, total_mb / 1024.0);
                
                // 每分配5块后检查一次内存使用率
                if (allocated_count % 5 == 0) {
                    memory_info_t mem_info;
                    if (get_memory_info(&mem_info) == 0) {
                        printf("Current memory usage: %.1f%%\n", mem_info.usage_percent);
                        if (mem_info.usage_percent >= 85.0f) {
                            printf("Safety limit reached, stopping new allocations but keeping test active\n");
                            allocation_stopped = 1;
                        }
                    }
                }
            } else {
                printf("Failed to allocate memory block %d\n", allocated_count + 1);
                allocation_stopped = 1; // 分配失败也停止继续分配
            }
        } else if (allocated_count >= max_blocks) {
            printf("Reached maximum allocation limit (%d blocks, %.1f GB)\n", 
                   max_blocks, (max_blocks * block_size) / (1024.0 * 1024.0 * 1024.0));
            allocation_stopped = 1;
        }
        
        // 如果已经停止分配，只是保持测试状态，让用户可以观察UI效果
        if (allocation_stopped) {
            printf("Memory test active - maintaining %.1f%% usage (click Stop to end test)\n", 
                   (allocated_count * block_size) / (1024.0 * 1024.0 * 1024.0) * 100.0 / 4.0); // 假设4GB总内存
            sleep(2); // 每2秒输出一次状态信息
        } else {
            // 更快的分配频率：每200毫秒分配一块
            usleep(200000); // 0.2秒
        }
    }
    
    // 清理分配的内存
    printf("Cleaning up allocated memory...\n");
    for (int i = 0; i < allocated_count; i++) {
        if (allocated_blocks[i] != NULL) {
            free(allocated_blocks[i]);
        }
    }
    free(allocated_blocks);
    allocated_blocks = NULL;
    allocated_count = 0;
    
    printf("Memory test finished\n");
    return NULL;
}

// 启动内存测试
void start_memory_test() {
    if (!memory_test_running) {
        memory_test_running = 1;
        printf("Starting memory test...\n");
        if (pthread_create(&memory_test_thread, NULL, memory_test_worker, NULL) != 0) {
            printf("Failed to create memory test thread\n");
            memory_test_running = 0;
        }
    }
}

// 停止内存测试
void stop_memory_test() {
    if (memory_test_running) {
        memory_test_running = 0;
        pthread_join(memory_test_thread, NULL);
        printf("Memory test stopped\n");
    }
}

// 内存测试按钮事件回调
void memory_test_button_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = (lv_obj_t*)lv_event_get_target(e);
    
    if(code == LV_EVENT_CLICKED) {
        if (!memory_test_running) {
            start_memory_test();
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            lv_label_set_text(label, "Stop Mem Test");
            printf("Starting memory test\n");
        } else {
            stop_memory_test();
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            lv_label_set_text(label, "Start Mem Test");
            printf("Stopping memory test\n");
        }
    }
}