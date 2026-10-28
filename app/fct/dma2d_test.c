/** @file dma2d_test.c @brief Hardware fill/copy verified by CPU SDRAM readback. */
#include <errno.h>
#include <stdio.h>
#include "fct/fct_internal.h"
#include "hal/hal_display.h"

static uint16_t s_readback[HAL_DISPLAY_WIDTH];
static uint16_t s_source[HAL_DISPLAY_WIDTH];

static uint16_t fct_dma2d_pixel(uint16_t x)
{
    return (uint16_t) ((uint32_t) x * 73U ^ 0x5aa5U);
}

int fct_run_dma2d(fct_context_t *context)
{
    int result = hal_display_ready();
    if (result != 0)
    {
        return result;
    }
    result = hal_display_blanking(true);
    if (result == 0)
    {
        result = hal_display_dma2d_fill(0, 0, HAL_DISPLAY_WIDTH, HAL_DISPLAY_HEIGHT, 0x1234);
    }
    /* Odd x/width tests output stride and preservation of shared cache lines. */
    if (result == 0)
    {
        result = hal_display_dma2d_fill(17, 19, 333, 37, 0xe72b);
    }
    for (uint16_t y = 0; result == 0 && y < HAL_DISPLAY_HEIGHT; y++)
    {
        result = fct_checkpoint(context);
        if (result == 0)
        {
            result = hal_display_read_row(y, s_readback, HAL_DISPLAY_WIDTH);
        }
        for (uint16_t x = 0; result == 0 && x < HAL_DISPLAY_WIDTH; x++)
        {
            uint16_t expected = x >= 17U && x < 350U && y >= 19U && y < 56U ? 0xe72bU : 0x1234U;
            if (s_readback[x] != expected)
            {
                fct_output("DMA2D fill mismatch x=%u y=%u got=%04x expected=%04x", x, y, s_readback[x], expected);
                result = -EIO;
            }
        }
    }
    for (uint16_t x = 0; x < HAL_DISPLAY_WIDTH; x++)
    {
        s_source[x] = fct_dma2d_pixel(x);
    }
    if (result == 0)
    {
        result = hal_display_dma2d_row(21, s_source, HAL_DISPLAY_WIDTH);
    }
    /* Read all pixels again to catch overwrite outside the copied row. */
    for (uint16_t y = 0; result == 0 && y < HAL_DISPLAY_HEIGHT; y++)
    {
        result = fct_checkpoint(context);
        if (result == 0)
        {
            result = hal_display_read_row(y, s_readback, HAL_DISPLAY_WIDTH);
        }
        for (uint16_t x = 0; result == 0 && x < HAL_DISPLAY_WIDTH; x++)
        {
            uint16_t expected = y == 21U ? fct_dma2d_pixel(x) : (x >= 17U && x < 350U && y >= 19U && y < 56U ? 0xe72bU : 0x1234U);
            if (s_readback[x] != expected)
            {
                fct_output("DMA2D copy mismatch x=%u y=%u got=%04x expected=%04x", x, y, s_readback[x], expected);
                result = -EIO;
            }
        }
    }
    int cleanup = hal_display_blanking(true);
    if (result == 0)
    {
        result = cleanup;
    }
    (void) snprintf(context->reason, sizeof(context->reason), "DMA2D RGB565 fill/stride/copy SDRAM readback verified; panel untested");
    return result;
}
