#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "pico/sem.h"

#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "keypad_encoder/lv_demo_keypad_encoder.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"

#include "ws2812.pio.h"

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

lv_obj_t *img1 = NULL; // 开机图片

lv_obj_t *led1 = NULL;
lv_obj_t *led2 = NULL;
lv_obj_t *led3 = NULL;

lv_obj_t *jy_label = NULL;

uint8_t adc_en = 0;

static void keypad_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_del(img1);
        lv_obj_clean(lv_scr_act());
        vTaskDelay(100 / portTICK_PERIOD_MS);

        lv_demo_keypad_encoder();
    }
}

static void beep_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        gpio_xor_mask(0x2000);
    }
}

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_color_t color = lv_colorwheel_get_rgb(obj);
        put_pixel(urgb_u32(color.ch.red << 5, ((color.ch.green_h << 2) + color.ch.green_h) << 2, (color.ch.blue << 3)));
    }
}

static void clr_rgb_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        put_pixel(urgb_u32(0, 0, 0));
    }
}

void gpio_callback(uint gpio, uint32_t events)
{
    switch (gpio)
    {
    case 15:
        lv_led_toggle(led1);
        gpio_xor_mask(1ul << 16);
        break;
    case 14:
        lv_led_toggle(led2);
        gpio_xor_mask(1ul << 17);
        break;
    case 22:
        lv_led_toggle(led3);
        gpio_xor_mask((1ul << 16) | (1ul << 17));
        break;
    }
}

static void hw_handler(lv_event_t *e)
{
    lv_obj_t *label;

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_del(img1);
        lv_obj_clean(lv_scr_act());
        vTaskDelay(100 / portTICK_PERIOD_MS);

        // 蜂鸣器
        gpio_init(13);
        gpio_set_dir(13, GPIO_OUT);

        lv_obj_t *beep_btn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(beep_btn, beep_handler, LV_EVENT_ALL, NULL);
        lv_obj_align(beep_btn, LV_ALIGN_TOP_MID, 0, 40);
        lv_obj_add_flag(beep_btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_height(beep_btn, LV_SIZE_CONTENT);

        label = lv_label_create(beep_btn);
        lv_label_set_text(label, "Beep");
        lv_obj_center(label);

        // 清除颜色
        lv_obj_t *clr_rgb_btn = lv_btn_create(lv_scr_act());
        lv_obj_add_event_cb(clr_rgb_btn, clr_rgb_handler, LV_EVENT_ALL, NULL);
        lv_obj_align(clr_rgb_btn, LV_ALIGN_TOP_MID, 0, 80);

        label = lv_label_create(clr_rgb_btn);
        lv_label_set_text(label, "Turn off RGB");
        lv_obj_center(label);

        // RGB 灯

        /*Create a slider in the center of the display*/
        lv_obj_t *lv_colorwheel = lv_colorwheel_create(lv_scr_act(), true);
        lv_obj_set_size(lv_colorwheel, 200, 200);
        lv_obj_align(lv_colorwheel, LV_ALIGN_TOP_MID, 100, 0);

        lv_obj_center(lv_colorwheel);
        lv_obj_add_event_cb(lv_colorwheel, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

        // todo get free sm
        PIO pio = pio0;
        int sm = 0;
        uint offset = pio_add_program(pio, &ws2812_program);

        ws2812_program_init(pio, sm, offset, 12, 800000, true);
        put_pixel(urgb_u32(0, 0, 0));

        gpio_set_irq_enabled_with_callback(14, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
        gpio_set_irq_enabled_with_callback(15, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
        gpio_set_irq_enabled_with_callback(22, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

        led1 = lv_led_create(lv_scr_act());
        lv_obj_align(led1, LV_ALIGN_TOP_MID, -60, 400);
        lv_led_set_color(led1, lv_palette_main(LV_PALETTE_RED));

        lv_led_off(led1);

        led2 = lv_led_create(lv_scr_act());
        lv_obj_align(led2, LV_ALIGN_TOP_MID, 0, 400);
        lv_led_set_color(led2, lv_palette_main(LV_PALETTE_GREEN));

        lv_led_off(led2);

        led3 = lv_led_create(lv_scr_act());
        lv_obj_align(led3, LV_ALIGN_TOP_MID, 60, 400);
        lv_led_set_color(led3, lv_palette_main(LV_PALETTE_BLUE));

        lv_led_off(led3);

        gpio_init(16);
        gpio_init(17);

        gpio_set_dir(16, GPIO_OUT);
        gpio_set_dir(17, GPIO_OUT);

        gpio_put(16, 0);
        gpio_put(17, 0);

        adc_en = 1;

        jy_label = lv_label_create(lv_scr_act());
        lv_label_set_text(jy_label, "X = 0 Y = 0");
        lv_obj_set_style_text_align(jy_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(jy_label, 50);
        lv_obj_align(jy_label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *btn_label = lv_label_create(lv_scr_act());
        lv_label_set_text(btn_label, "Press Button to Toggle LED!");
        lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(btn_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    }
}

void lv_example_btn_1(void)
{
    lv_obj_t *label;

    lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, keypad_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_TOP_MID, 0, 40);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Keypad Demo");
    lv_obj_center(label);

    lv_obj_t *btn2 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, hw_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 80);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Hardware Demo");
    lv_obj_center(label);
}

void task0(void *pvParam)
{
    lv_obj_clean(lv_scr_act());
    vTaskDelay(100 / portTICK_PERIOD_MS);

    img1 = lv_img_create(lv_scr_act());
    LV_IMG_DECLARE(ai);
    lv_img_set_src(img1, &ai);
    lv_obj_align(img1, LV_ALIGN_DEFAULT, 0, 0);
    lv_example_btn_1();

    for (;;)
    {
        if (adc_en)
        {
            adc_init();
            // Make sure GPIO is high-impedance, no pullups etc
            adc_gpio_init(26);
            adc_gpio_init(27);

            for (;;)
            {
                char buf[50];

                adc_select_input(0);
                uint adc_x_raw = adc_read();
                adc_select_input(1);
                uint adc_y_raw = adc_read();

                const uint bar_width = 40;
                const uint adc_max = (1 << 12) - 1;
                int bar_x_pos = (adc_x_raw * bar_width / adc_max) - 19;
                int bar_y_pos = (adc_y_raw * bar_width / adc_max) - 19;

                vTaskDelay(200 / portTICK_PERIOD_MS);

                sprintf(buf,"X = %d Y = %d",bar_x_pos,bar_y_pos);
                lv_label_set_text(jy_label, buf);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task1(void *pvParam)
{
    for (;;)
    {
        lv_task_handler();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

int main()
{
    stdio_init_all();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    UBaseType_t task0_CoreAffinityMask = (1 << 0);
    UBaseType_t task1_CoreAffinityMask = (1 << 1);

    TaskHandle_t task0_Handle = NULL;

    xTaskCreate(task0, "task0", 2048, NULL, 1, &task0_Handle);
    vTaskCoreAffinitySet(task0_Handle, task0_CoreAffinityMask);

    TaskHandle_t task1_Handle = NULL;
    xTaskCreate(task1, "task1", 2048, NULL, 2, &task1_Handle);
    vTaskCoreAffinitySet(task1_Handle, task1_CoreAffinityMask);

    vTaskStartScheduler();

    return 0;
}