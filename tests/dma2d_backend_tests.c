/** @file dma2d_backend_tests.c @brief Register/cache shim, not a hardware DMA simulation. */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <soc.h>
#include "hal/hal_display.h"

const struct device test_display = {0};
test_dma_regs_t test_regs;
static uint16_t s_frame[HAL_DISPLAY_WIDTH * HAL_DISPLAY_HEIGHT] __attribute__((aligned(32)));
static uint16_t s_pixels[HAL_DISPLAY_WIDTH];
static uint16_t s_readback[HAL_DISPLAY_WIDTH];
static void *s_source;
static int s_ready_error;
static int s_cache_error;
static int s_mode;
static int64_t s_time;
static unsigned int s_resets;
static unsigned int s_prepares;
static unsigned int s_invalidates;
static bool s_no_frame;

int hal_display_ready(void)
{
    return s_ready_error;
}
void *display_get_framebuffer(const struct device *dev)
{
    assert(dev == &test_display);
    return s_no_frame ? NULL : s_frame;
}
int sys_cache_data_flush_and_invd_range(void *addr, size_t size)
{
    assert((uintptr_t) addr % 32U == 0U && size % 32U == 0U);
    s_prepares++;
    return s_cache_error;
}
int sys_cache_data_flush_range(void *addr, size_t size)
{
    assert((uintptr_t) addr % 32U == 0U && size == sizeof(s_pixels));
    s_source = addr;
    return s_cache_error;
}
int sys_cache_data_invd_range(void *addr, size_t size)
{
    assert((uintptr_t) addr % 32U == 0U && size % 32U == 0U);
    assert((DMA2D->CR & DMA2D_CR_START) == 0U);
    s_invalidates++;
    return s_cache_error;
}
void test_dma_reset(void)
{
    s_resets++;
    memset(&test_regs, 0, sizeof(test_regs));
}
int64_t k_uptime_get(void)
{
    return s_time;
}
void k_sleep(int ms)
{
    s_time += ms;
    assert((DMA2D->CR & DMA2D_CR_START) != 0U);
    if (s_mode == 1)
    {
        return;
    } /* Never completes. */
    if (s_mode == 2)
    {
        DMA2D->ISR = DMA2D_ISR_TEIF;
        return;
    }
    if (s_mode == 3)
    {
        DMA2D->ISR = DMA2D_ISR_CEIF | DMA2D_ISR_TCIF;
        return;
    }
    assert(DMA2D->OPFCCR == 2U);
    uint32_t width = DMA2D->NLR >> 16;
    uint32_t height = DMA2D->NLR & 0xffffU;
    size_t offset = (uint32_t) (DMA2D->OMAR - (uint32_t) (uintptr_t) s_frame) / sizeof(uint16_t);
    assert(width + DMA2D->OOR == HAL_DISPLAY_WIDTH);
    assert(offset + (height - 1U) * HAL_DISPLAY_WIDTH + width <= HAL_DISPLAY_WIDTH * HAL_DISPLAY_HEIGHT);
    bool fill = (DMA2D->CR >> DMA2D_CR_MODE_Pos) == 3U;
    if (!fill)
    {
        assert(DMA2D->FGPFCCR == 2U && DMA2D->FGOR == 0U);
        assert(DMA2D->FGMAR == (uint32_t) (uintptr_t) s_source && height == 1U);
    }
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            s_frame[offset + y * HAL_DISPLAY_WIDTH + x] = fill ? (uint16_t) DMA2D->OCOLR : ((uint16_t *) s_source)[x];
        }
    }
    DMA2D->CR &= ~DMA2D_CR_START;
    DMA2D->ISR = DMA2D_ISR_TCIF;
}
int main(void)
{
    assert(hal_display_dma2d_fill(0, 0, 0, 1, 0) == -EINVAL);
    assert(hal_display_dma2d_fill(1023, 599, 2, 1, 0) == -EINVAL);
    assert(hal_display_dma2d_fill(0, 599, 1, 2, 0) == -EINVAL);
    assert(hal_display_dma2d_row(0, NULL, 1024) == -EINVAL);
    assert(hal_display_dma2d_row(600, s_pixels, 1024) == -EINVAL);
    assert(hal_display_dma2d_row(0, s_pixels, 1023) == -EINVAL);
    assert(hal_display_read_row(0, NULL, 1024) == -EINVAL);
    assert(s_resets == 0U);
    assert(hal_display_dma2d_fill(0, 0, 1024, 600, 0x1234) == 0);
    assert(hal_display_dma2d_fill(17, 19, 333, 37, 0xe72b) == 0);
    for (unsigned int y = 0; y < 600; y++)
    {
        for (unsigned int x = 0; x < 1024; x++)
        {
            assert(s_frame[y * 1024 + x] == (x >= 17 && x < 350 && y >= 19 && y < 56 ? 0xe72b : 0x1234));
        }
    }
    for (unsigned int x = 0; x < 1024; x++)
    {
        s_pixels[x] = (uint16_t) (x ^ 0xabcdU);
    }
    assert(hal_display_dma2d_row(599, s_pixels, 1024) == 0);
    assert(hal_display_read_row(599, s_readback, 1024) == 0);
    assert(memcmp(s_pixels, s_readback, sizeof(s_pixels)) == 0);
    assert(s_prepares == 3U && s_invalidates == 3U && s_resets == 6U);
    for (int mode = 1; mode <= 3; mode++)
    {
        s_mode = mode;
        assert(hal_display_dma2d_fill(0, 0, 1, 1, 0) == (mode == 1 ? -ETIMEDOUT : -EIO));
        assert(DMA2D->CR == 0U);
    }
    s_mode = 0;
    assert(hal_display_dma2d_fill(1023, 599, 1, 1, 0xffff) == 0);
    assert(s_frame[1024 * 600 - 1] == 0xffff);
    s_ready_error = -ENODEV;
    assert(hal_display_dma2d_fill(0, 0, 1, 1, 0) == -ENODEV);
    s_ready_error = 0;
    s_no_frame = true;
    assert(hal_display_dma2d_fill(0, 0, 1, 1, 0) == -ENODEV);
    s_no_frame = false;
    s_cache_error = -ENOTSUP;
    assert(hal_display_dma2d_fill(0, 0, 1, 1, 0) == -ENOTSUP);
    puts("PASS: DMA2D registers, strides, cache ordering, errors, timeout reset and reuse");
    return 0;
}
