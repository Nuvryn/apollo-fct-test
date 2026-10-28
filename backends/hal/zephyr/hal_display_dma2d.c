/** @file hal_display_dma2d.c @brief Exclusive polled DMA2D RGB565 display accelerator. */
#include <errno.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include "hal/hal_display.h"

#define DISPLAY_DMA_TIMEOUT_MS 100
#define DISPLAY_DMA_RGB565 2U
/* Internal AXI SRAM, not a caller stack which could be in inaccessible DTCM. */
static uint16_t s_dma_row[HAL_DISPLAY_WIDTH] __aligned(32);

static uint16_t *display_dma_framebuffer(void)
{
    return display_get_framebuffer(DEVICE_DT_GET(DT_NODELABEL(ltdc)));
}

static int display_dma_transfer(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    uint32_t value, bool fill)
{
    int result = hal_display_ready();
    if (result != 0)
    {
        return result;
    }
    uint16_t *frame = display_dma_framebuffer();
    if (frame == NULL)
    {
        return -ENODEV;
    }
    /* Whole scanlines are cache-aligned and include untouched edge pixels.
     * Clean first so invalidating partial rectangles cannot discard CPU writes.
     * No CPU or another DMA writer may modify this span during the transfer.
     */
    void *cache_start = frame + (size_t) y * HAL_DISPLAY_WIDTH;
    size_t cache_size = (size_t) height * HAL_DISPLAY_WIDTH * sizeof(*frame);
    result = sys_cache_data_flush_and_invd_range(cache_start, cache_size);
    if (result != 0)
    {
        return result;
    }
    if (!fill)
    {
        result = sys_cache_data_flush_range(s_dma_row, sizeof(s_dma_row));
        if (result != 0)
        {
            return result;
        }
    }
    __HAL_RCC_DMA2D_CLK_ENABLE();
    /* This backend is the only DMA2D owner; reset also clears stale flags. */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
    DMA2D->CR = fill ? (3U << DMA2D_CR_MODE_Pos) : 0U;
    DMA2D->OPFCCR = DISPLAY_DMA_RGB565;
    DMA2D->OOR = HAL_DISPLAY_WIDTH - width;
    DMA2D->OMAR = (uint32_t) (uintptr_t) (frame + (size_t) y * HAL_DISPLAY_WIDTH + x);
    DMA2D->NLR = ((uint32_t) width << DMA2D_NLR_PL_Pos) | height;
    if (fill)
    {
        /* Direct OCOLR takes packed RGB565, unlike HAL_DMA2D_Start's ARGB input. */
        DMA2D->OCOLR = value;
    }
    else
    {
        DMA2D->FGPFCCR = DISPLAY_DMA_RGB565;
        DMA2D->FGOR = 0;
        DMA2D->FGMAR = (uint32_t) (uintptr_t) s_dma_row;
    }
    __DSB();
    int64_t started_ms = k_uptime_get();
    DMA2D->CR |= DMA2D_CR_START;
    for (;;)
    {
        uint32_t flags = DMA2D->ISR;
        if ((flags & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) != 0U)
        {
            result = -EIO;
            break;
        }
        if ((flags & DMA2D_ISR_TCIF) != 0U)
        {
            result = 0;
            break;
        }
        if (k_uptime_get() - started_ms >= DISPLAY_DMA_TIMEOUT_MS)
        {
            result = -ETIMEDOUT;
            break;
        }
        k_sleep(K_MSEC(1));
    }
    /* Reset on every exit guarantees no writes continue after an error/timeout. */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
    __DSB();
    int cache_result = sys_cache_data_invd_range(cache_start, cache_size);
    return result != 0 ? result : cache_result;
}

int hal_display_dma2d_fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    if (width == 0U || height == 0U || x >= HAL_DISPLAY_WIDTH || y >= HAL_DISPLAY_HEIGHT ||
        width > HAL_DISPLAY_WIDTH - x || height > HAL_DISPLAY_HEIGHT - y)
    {
        return -EINVAL;
    }
    return display_dma_transfer(x, y, width, height, color, true);
}

int hal_display_dma2d_row(uint16_t y, const uint16_t *pixels, uint16_t count)
{
    if (pixels == NULL || count != HAL_DISPLAY_WIDTH || y >= HAL_DISPLAY_HEIGHT)
    {
        return -EINVAL;
    }
    memcpy(s_dma_row, pixels, sizeof(s_dma_row));
    return display_dma_transfer(0, y, count, 1, 0, false);
}

int hal_display_read_row(uint16_t y, uint16_t *pixels, uint16_t count)
{
    if (pixels == NULL || count != HAL_DISPLAY_WIDTH || y >= HAL_DISPLAY_HEIGHT)
    {
        return -EINVAL;
    }
    int result = hal_display_ready();
    if (result != 0)
    {
        return result;
    }
    uint16_t *frame = display_dma_framebuffer();
    if (frame == NULL)
    {
        return -ENODEV;
    }
    memcpy(pixels, frame + (size_t) y * HAL_DISPLAY_WIDTH, (size_t) count * sizeof(*pixels));
    return 0;
}
