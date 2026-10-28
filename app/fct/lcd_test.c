/** @file lcd_test.c @brief Operator-confirmed panel colors and geometry test. */
#include <stdio.h>
#include "fct/fct_internal.h"
#include "hal/hal_display.h"

/* Worker-only scratch row avoids placing a full framebuffer in internal RAM. */
static uint16_t s_row[HAL_DISPLAY_WIDTH];
static const uint16_t s_colors[] = {0xf800, 0x07e0, 0x001f, 0xffff, 0x0000};

static int fct_lcd_pattern(fct_context_t *context, unsigned int pattern)
{
    for (uint16_t y = 0; y < HAL_DISPLAY_HEIGHT; y++)
    {
        int result = fct_checkpoint(context);
        if (result != 0)
        {
            return result;
        }
        for (uint16_t x = 0; x < HAL_DISPLAY_WIDTH; x++)
        {
            if (pattern < 5U)
            {
                s_row[x] = s_colors[pattern];
            }
            else if (pattern == 5U)
            {
                s_row[x] = s_colors[((unsigned int) x * 5U) / HAL_DISPLAY_WIDTH];
            }
            else
            {
                bool line = x % 64U == 0U || y % 60U == 0U ||
                            x == HAL_DISPLAY_WIDTH - 1U || y == HAL_DISPLAY_HEIGHT - 1U;
                s_row[x] = line ? 0xffffU : 0x0000U;
            }
        }
        result = hal_display_write_row(y, s_row, HAL_DISPLAY_WIDTH);
        if (result != 0)
        {
            return result;
        }
    }
    return 0;
}

int fct_run_lcd(fct_context_t *context)
{
    static const char *const names[] = {"red", "green", "blue", "white", "black", "color bars", "grid"};
    int result = hal_display_ready();
    if (result != 0)
    {
        return result;
    }
    result = hal_display_blanking(true);
    for (unsigned int pattern = 0; result == 0 && pattern < 7U; pattern++)
    {
        result = fct_lcd_pattern(context, pattern);
        if (result == 0)
        {
            result = hal_display_blanking(false);
        }
        if (result == 0)
        {
            fct_output("LCD observe %s", names[pattern]);
            result = fct_sleep(context, 1200);
        }
    }
    if (result == 0)
    {
        fct_output("Check grid edges, colors and missing lines");
        result = fct_wait_confirmation(context);
    }
    int cleanup = hal_display_blanking(true);
    if (cleanup != 0)
    {
        fct_output("LCD backlight cleanup failed: %d", cleanup);
        if (result == 0)
        {
            result = cleanup;
        }
    }
    (void) snprintf(context->reason, sizeof(context->reason), "LCD colors/grid confirmed by operator");
    return result;
}
