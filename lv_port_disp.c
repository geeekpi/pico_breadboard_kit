/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES    240
#define MY_DISP_VER_RES    240

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    // /* Example for 2) */
    // static lv_disp_draw_buf_t draw_buf_dsc_2;
    // static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
    // static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    // /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    // static lv_disp_draw_buf_t draw_buf_dsc_3;
    // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
    //                       MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static const int gpio_din = 3;
static const int gpio_clk = 2;
static const int gpio_cs = 6;
static const int gpio_dc = 7;
static const int gpio_rst = 8;
static const int gpio_bl = 9;

static void st7789_send_cmd(uint8_t cmd)
{
    gpio_put(gpio_dc, 0);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, (uint8_t[]){ cmd }, 1);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7789_send_data(void * data, uint16_t length)
{
    gpio_put(gpio_dc, 1);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, data, length);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7789_send_color(void * data, size_t length)
{
    gpio_put(gpio_dc, 1);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, data, length);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7789_set_orientation(uint8_t orientation)
{
    st7789_send_cmd(ST7789_MADCTL);

    gpio_put(gpio_dc, 1);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, (uint8_t[]){ orientation }, 1);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    lcd_init_cmd_t st7789_init_cmds[] = {
        {0xCF, {0x00, 0x83, 0X30}, 3},
        {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
        {ST7789_PWCTRL2, {0x85, 0x01, 0x79}, 3},
        {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
        {0xF7, {0x20}, 1},
        {0xEA, {0x00, 0x00}, 2},
        {ST7789_LCMCTRL, {0x26}, 1},
        {ST7789_IDSET, {0x11}, 1},
        {ST7789_VCMOFSET, {0x35, 0x3E}, 2},
        {ST7789_CABCCTRL, {0xBE}, 1},
        {ST7789_MADCTL, {0x00}, 1}, // Set to 0x28 if your display is flipped
        {ST7789_COLMOD, {0x55}, 1},
		{ST7789_INVON, {0}, 0}, // set inverted mode
        {ST7789_RGBCTRL, {0x00, 0x1B}, 2},
        {0xF2, {0x08}, 1},
        {ST7789_GAMSET, {0x01}, 1},
        {ST7789_PVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17}, 14},
        {ST7789_NVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
        {ST7789_CASET, {0x00, 0x00, 0x00, 0xEF}, 4},
        {ST7789_RASET, {0x00, 0x00, 0x01, 0x3f}, 4},
        {ST7789_RAMWR, {0}, 0},
        {ST7789_GCTRL, {0x07}, 1},
        {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
        {ST7789_SLPOUT, {0}, 0x80},
        {ST7789_DISPON, {0}, 0x80},
        {ST7789_MADCTL, {0}, 0xC0},
        {0, {0}, 0xff},
    };

    spi_init(spi0, 125 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(gpio_clk, GPIO_FUNC_SPI);

    gpio_init(gpio_cs);
    gpio_init(gpio_dc);
    gpio_init(gpio_rst);
    gpio_init(gpio_bl);

    gpio_set_dir(gpio_cs, GPIO_OUT);
    gpio_set_dir(gpio_dc, GPIO_OUT);
    gpio_set_dir(gpio_rst, GPIO_OUT);
    gpio_set_dir(gpio_bl, GPIO_OUT);

    gpio_put(gpio_cs, 1);
    gpio_put(gpio_dc, 1);

    gpio_put(gpio_rst, 1);
    sleep_ms(100);
    gpio_put(gpio_rst, 0);
    sleep_ms(100);
    gpio_put(gpio_rst, 1);
    sleep_ms(100);

    //Send all the commands
    uint16_t cmd = 0;
    while (st7789_init_cmds[cmd].databytes!=0xff) {
        st7789_send_cmd(st7789_init_cmds[cmd].cmd);
        st7789_send_data(st7789_init_cmds[cmd].data, st7789_init_cmds[cmd].databytes&0x1F);
        if (st7789_init_cmds[cmd].databytes & 0x80) {
            sleep_ms(100);
        }
        cmd++;
    }

    
    st7789_set_orientation(0x00);

    gpio_put(gpio_bl, 1);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(disp_flush_enabled) {
        uint8_t data[4] = {0};

        uint16_t offsetx1 = area->x1;
        uint16_t offsetx2 = area->x2;
        uint16_t offsety1 = area->y1;
        uint16_t offsety2 = area->y2;

        /*Column addresses*/
        st7789_send_cmd(ST7789_CASET);
        data[0] = (offsetx1 >> 8) & 0xFF;
        data[1] = offsetx1 & 0xFF;
        data[2] = (offsetx2 >> 8) & 0xFF;
        data[3] = offsetx2 & 0xFF;
        st7789_send_data(data, 4);

        /*Page addresses*/
        st7789_send_cmd(ST7789_RASET);
        data[0] = (offsety1 >> 8) & 0xFF;
        data[1] = offsety1 & 0xFF;
        data[2] = (offsety2 >> 8) & 0xFF;
        data[3] = offsety2 & 0xFF;
        st7789_send_data(data, 4);

        /*Memory write*/
        st7789_send_cmd(ST7789_RAMWR);

        size_t size = (size_t)lv_area_get_width(area) * (size_t)lv_area_get_height(area);

        st7789_send_color((void*)color_p, size * 2);
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
