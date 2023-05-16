/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(lv_coord_t *x, lv_coord_t *y);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t *indev_touchpad;
lv_indev_t *indev_mouse;
lv_indev_t *indev_keypad;
lv_indev_t *indev_encoder;
lv_indev_t *indev_button;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

/**********************
 *      MACROS
 **********************/
gt911_status_t gt911_status;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
int gt911_i2c_read(uint8_t slave_addr, uint16_t register_addr, uint8_t *data_buf, uint8_t len) {
    uint8_t buf[2] = {register_addr >> 8,register_addr & 0xFF};
    if (i2c_write_blocking(i2c0, slave_addr, buf, 2, true))
        return i2c_read_blocking(i2c0, slave_addr, data_buf, len, false);
    else
        return PICO_ERROR_GENERIC;
}

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    if (!gt911_status.inited)
    {
        gt911_status.i2c_dev_addr = GT911_I2C_SLAVE_ADDR;
        uint8_t data_buf;

        i2c_init(i2c0, 100 * 1000);
        gpio_set_function(8 /* SDA */, GPIO_FUNC_I2C);
        gpio_set_function(9 /* SCL */, GPIO_FUNC_I2C);
        gpio_pull_up(8);
        gpio_pull_up(9);

        if (gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PRODUCT_ID1, &data_buf, 1) == PICO_ERROR_GENERIC )
        {
            return;
        }

        // Read 4 bytes for Product ID in ASCII
        for (int i = 0; i < GT911_PRODUCT_ID_LEN; i++)
        {
            gt911_i2c_read(gt911_status.i2c_dev_addr, (GT911_PRODUCT_ID1 + i), (uint8_t *)&(gt911_status.product_id[i]), 1);
        }

        gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_VENDOR_ID, &data_buf, 1);

        gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_X_COORD_RES_L, &data_buf, 1);
        gt911_status.max_x_coord = data_buf;
        gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_X_COORD_RES_H, &data_buf, 1);
        gt911_status.max_x_coord |= ((uint16_t)data_buf << 8);

        gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_Y_COORD_RES_L, &data_buf, 1);
        gt911_status.max_y_coord = data_buf;
        gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_Y_COORD_RES_H, &data_buf, 1);
        gt911_status.max_y_coord |= ((uint16_t)data_buf << 8);
        gt911_status.inited = true;
    }
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint8_t touch_pnt_cnt;     // Number of detected touch points
    static int16_t last_x = 0; // 12bit pixel value
    static int16_t last_y = 0; // 12bit pixel value
    uint8_t data_buf;
    uint8_t status_reg;

    data->continue_reading = false;

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_STATUS_REG, &status_reg, 1);
    
    touch_pnt_cnt = status_reg & 0x0F;
    if ((status_reg & 0x80) || (touch_pnt_cnt < 6))
    {
        // Reset Status Reg Value
        // GT911_STATUS_REG => 0x814E
         uint8_t ret = 0;
         ret = i2c_write_blocking(i2c0, gt911_status.i2c_dev_addr, (uint8_t[]){ 0x81,0x4E,0x00 }, 3, true);
    }
    if (touch_pnt_cnt != 1)
    { // ignore no touch & multi touch
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    //    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_TRACK_ID1, &data_buf, 1);
    // printf("TOUCH -> \ttrack_id: %d", data_buf);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_L, &data_buf, 1);
    last_x = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_X_COORD_H, &data_buf, 1);
    last_x |= ((uint16_t)data_buf << 8);

    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_L, &data_buf, 1);
    last_y = data_buf;
    gt911_i2c_read(gt911_status.i2c_dev_addr, GT911_PT1_Y_COORD_H, &data_buf, 1);
    last_y |= ((uint16_t)data_buf << 8);

#if LV_GT911_INVERT_X
    last_x = gt911_status.max_x_coord - last_x;
#endif
#if LV_GT911_INVERT_Y
    last_y = gt911_status.max_y_coord - last_y;
#endif
#if LV_GT911_SWAPXY
    int16_t swap_buf = last_x;
    last_x = last_y;
    last_y = swap_buf;
#endif
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;
    return;
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    /*Your code comes here*/

    return false;
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(lv_coord_t *x, lv_coord_t *y)
{
    /*Your code comes here*/

    (*x) = 0;
    (*y) = 0;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
