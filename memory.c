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
extern lv_obj_t *bin_ui;