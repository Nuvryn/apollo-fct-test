/** @file hal_display_runtime.c @brief LTDC adapter with driver-owned SDRAM framebuffer. */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include "hal/hal_display.h"

static const struct device *const s_display = DEVICE_DT_GET(DT_NODELABEL(ltdc));

int hal_display_ready(void)
{
    struct display_capabilities caps;
    if (!device_is_ready(DEVICE_DT_GET(DT_NODELABEL(sdram))) || !device_is_ready(s_display))
    {
        return -ENODEV;
    }
    display_get_capabilities(s_display, &caps);
    return caps.x_resolution == HAL_DISPLAY_WIDTH && caps.y_resolution == HAL_DISPLAY_HEIGHT &&
                   caps.current_pixel_format == PIXEL_FORMAT_RGB_565
               ? 0
               : -ENOTSUP;
}

int hal_display_blanking(bool blank)
{
    int result = hal_display_ready();
    return result != 0 ? result : (blank ? display_blanking_on(s_display) : display_blanking_off(s_display));
}

int hal_display_write_row(uint16_t y, const uint16_t *pixels, uint16_t count)
{
    struct display_buffer_descriptor desc = {
        .buf_size = (size_t) count * sizeof(*pixels), .width = count, .height = 1, .pitch = count};
    if (pixels == NULL || count != HAL_DISPLAY_WIDTH || y >= HAL_DISPLAY_HEIGHT)
    {
        return -EINVAL;
    }
    int result = hal_display_ready();
    return result != 0 ? result : display_write(s_display, 0, y, &desc, pixels);
}
