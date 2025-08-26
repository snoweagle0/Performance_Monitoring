#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>

lv_obj_t *parent;
lv_obj_t * bmeun;
void B_meun(void);
void begin();
int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t *disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_indev_t *indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    /*Create a Demo*/
    // lv_demo_widgets();
    // lv_demo_widgets_start_slideshow();
    B_meun();
    begin();
    /*Handle LVGL tasks*/
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
    void lv_image_set_src(avtar, img_test);
    lv_img_set_src(avtar, "A:/lying/lvgl/img.bmp");
    lv_obj_set_width(avtar, LV_SIZE_CONTENT);
    lv_obj_set_height(avtar, LV_SIZE_CONTENT);
    //lv_image_set_scale(avtar, 64);
    lv_obj_align(avtar, LV_ALIGN_TOP_MID, 0, 0);
    return;
}
