/** @file hal_display.c @brief Display skeleton; no hardware access. */
#include <errno.h>
#include "hal/hal_display.h"
int hal_display_ready(void)
{
    return -ENOSYS;
}
int hal_display_blanking(bool blank)
{
    (void) blank;
    return -ENOSYS;
}
int hal_display_write_row(uint16_t y, const uint16_t *pixels, uint16_t count)
{
    (void) y;
    (void) pixels;
    (void) count;
    return -ENOSYS;
}

int hal_display_dma2d_fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    (void) x;
    (void) y;
    (void) width;
    (void) height;
    (void) color;
    return -ENOSYS;
}
int hal_display_dma2d_row(uint16_t y, const uint16_t *pixels, uint16_t count)
{
    (void) y;
    (void) pixels;
    (void) count;
    return -ENOSYS;
}
int hal_display_read_row(uint16_t y, uint16_t *pixels, uint16_t count)
{
    (void) y;
    (void) pixels;
    (void) count;
    return -ENOSYS;
}
