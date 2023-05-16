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
#define MY_DISP_HOR_RES    320
#define MY_DISP_VER_RES    480

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

    // 内存过小,所以不全刷可能会花边.
    // disp_drv.full_refresh = 1;

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
static const int gpio_clk = 2;
static const int gpio_din = 3;
static const int gpio_cs = 5;
static const int gpio_dc = 6;
static const int gpio_rst = 7;

static void st7796s_send_cmd(uint8_t cmd)
{
    gpio_put(gpio_dc, 0);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, (uint8_t[]){ cmd }, 1);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7796s_send_data(void * data, uint16_t length)
{
    gpio_put(gpio_dc, 1);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, data, length);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7796s_send_color(void * data, size_t length)
{
    gpio_put(gpio_dc, 1);
    gpio_put(gpio_cs, 0);
    sleep_us(1);

    spi_write_blocking(spi0, data, length);
   
    sleep_us(1);
    gpio_put(gpio_cs, 1);
}

static void st7796s_set_orientation(uint8_t orientation)
{
    st7796s_send_cmd(0x36);

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
	lcd_init_cmd_t init_cmds[] = {
		{0xCF, {0x00, 0x83, 0X30}, 3},
		{0xED, {0x64, 0x03, 0X12, 0X81}, 4},
		{0xE8, {0x85, 0x01, 0x79}, 3},
		{0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
		{0xF7, {0x20}, 1},
		{0xEA, {0x00, 0x00}, 2},
		{0xC0, {0x26}, 1},		 /*Power control*/
		{0xC1, {0x11}, 1},		 /*Power control */
		{0xC5, {0x35, 0x3E}, 2}, /*VCOM control*/
		{0xC7, {0xBE}, 1},		 /*VCOM control*/
		{0x36, {0x28}, 1},		 /*Memory Access Control*/
		{0x3A, {0x05}, 1},		 /*Pixel Format Set*/
		{0xB1, {0x00, 0x1B}, 2},
		{0xF2, {0x08}, 1},
		{0x26, {0x01}, 1},
		{0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
		{0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
		{0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
		{0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
		{0x2C, {0}, 0},
		{0xB7, {0x07}, 1},
		{0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
		{0x11, {0}, 0x80},
		{0x29, {0}, 0x80},
		{0, {0}, 0xff},
	};

    spi_init(spi0, 62.5 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(gpio_clk, GPIO_FUNC_SPI);

    gpio_init(gpio_cs);
    gpio_init(gpio_dc);
    gpio_init(gpio_rst);

    gpio_set_dir(gpio_cs, GPIO_OUT);
    gpio_set_dir(gpio_dc, GPIO_OUT);
    gpio_set_dir(gpio_rst, GPIO_OUT);

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
	while (init_cmds[cmd].databytes != 0xff)
	{
		st7796s_send_cmd(init_cmds[cmd].cmd);
		st7796s_send_data(init_cmds[cmd].data, init_cmds[cmd].databytes & 0x1F);
		if (init_cmds[cmd].databytes & 0x80)
		{
			sleep_ms(100);
		}
		cmd++;
	}
    
    // "PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE", "LANDSCAPE_INVERTED"
    // 0x48, 0x88, 0x28, 0xE8
	st7796s_set_orientation(0x48);

    st7796s_send_cmd(0x21);
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
        	uint8_t data[4];

        	/*Column addresses*/
            st7796s_send_cmd(0x2A);
            data[0] = (area->x1 >> 8) & 0xFF;
            data[1] = area->x1 & 0xFF;
            data[2] = (area->x2 >> 8) & 0xFF;
            data[3] = area->x2 & 0xFF;
            st7796s_send_data(data, 4);

            /*Page addresses*/
            st7796s_send_cmd(0x2B);
            data[0] = (area->y1 >> 8) & 0xFF;
            data[1] = area->y1 & 0xFF;
            data[2] = (area->y2 >> 8) & 0xFF;
            data[3] = area->y2 & 0xFF;
            st7796s_send_data(data, 4);

            /*Memory write*/
            st7796s_send_cmd(0x2C);

            uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

            st7796s_send_color((void *)color_p, size * 2);

            lv_disp_flush_ready(disp_drv);
    }   
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
