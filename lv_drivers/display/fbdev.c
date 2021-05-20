/**
 * @file fbdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "fbdev.h"
#include "../../SerialHelper.h"

#include <Windows.h>


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info{
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t xres;
    uint32_t yres;
    int bits_per_pixel;
 };

struct bsd_fb_fix_info{
    long int line_length;
    long int smem_len;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

// static struct fb_var_screeninfo vinfo;
// static struct fb_fix_screeninfo finfo;
static char *fbp = 0;
static long int screensize = 0;
static int fbfd = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
WindowPtr window;

void fbdev_init(void)
{
    window = FrontWindow();
}

void fbdev_exit(void)
{
}
static void putpixel(unsigned char* screen, int x, int y, int color) {

    if (color == 0) {

        return;
    }
    char log[255];
    sprintf(log, "in putpixel, x: %d, y: %d, c: %d", x, y, color);
        writeSerialPort(boutRefNum, log);
    // unsigned where = x + y * 512;
    // screen[where] |= 1 << color;

    // this should be the byte location of the pixel
    unsigned char* location = screen + y * 64 + ((x / 8) | 0);

    // now we need to set the individual bit for the pixel
    *location |= color << 7 - x % 8;

    // char log[255];
    // sprintf(log, "putpixel: x: %d, y: %d, color: %d, where: %d, x mod 8: %d, x / 8: %d", x, y, color, location, x % 8, (x / 8) | 0);
    // writeSerialPort(boutRefNum,log);
}


#define WINDOW_WIDTH 510
#define WINDOW_HEIGHT 302
/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
void fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
        writeSerialPort(boutRefNum, "in fbdev flush");
    // if(fbp == NULL ||
    //         area->x2 < 0 ||
    //         area->y2 < 0 ||
    //         area->x1 > (int32_t)vinfo.xres - 1 ||
    //         area->y1 > (int32_t)vinfo.yres - 1) {
    //     lv_disp_flush_ready(drv);
    //     return;
    // }

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > (int32_t)WINDOW_WIDTH - 1 ? (int32_t)WINDOW_WIDTH - 1 : area->x2;
    int32_t act_y2 = area->y2 > (int32_t)WINDOW_HEIGHT - 1 ? (int32_t)WINDOW_HEIGHT - 1 : area->y2;


    lv_coord_t w = (act_x2 - act_x1 + 1);
    long int location = 0;
    long int byte_location = 0;

    uint8_t * fbp8 = (uint8_t *)fbp;
    int32_t x;
    int32_t y;
    for(y = act_y1; y <= act_y2; y++) {
        for(x = act_x1; x <= act_x2; x++) {
            // location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
            // byte_location = location / 8; /* find the byte we need to change */
            // bit_location = location % 8; /* inside the byte found, find the bit we need to change */
            // fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
            // fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;


            if (color_p->full != 0 || color_p->ch.red != 0 || color_p->ch.blue != 0 || color_p->ch.green != 0) {
                char log[255];
                sprintf(log, "about to put pixel: x: %d, y: %d, color full: %d, red: %d, blue: %d, green: %d ", x, y, color_p->full, color_p->ch.red, color_p->ch.blue, color_p->ch.green);
                writeSerialPort(boutRefNum, log);
            }

            putpixel((unsigned char *)window->portBits.baseAddr, -window->portBits.bounds.left + x, -window->portBits.bounds.top + y, color_p->full);
            color_p++;
        }

        color_p += area->x2 - act_x2;
    }

    //May be some direct update command is required
    //ret = ioctl(state->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)rect));

    lv_disp_flush_ready(drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
