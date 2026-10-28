/** @file hal_display.h @brief Board RGB565 display contract; task context only. */
#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H
#include <stdbool.h>
#include <stdint.h>
/* Fixed panel profile: 1024x600. Row data is copied synchronously.
 * All calls must be serialized by the caller (the FCT worker owns this device).
 * Returns 0 on success or a negative errno; no native driver types leak here.
 */
#define HAL_DISPLAY_WIDTH 1024U
#define HAL_DISPLAY_HEIGHT 600U
int hal_display_ready(void);
int hal_display_blanking(bool blank);
int hal_display_write_row(uint16_t y, const uint16_t *pixels, uint16_t count);
/* DMA2D operations, synchronous and bounded to 100ms per transfer.
 * Rectangles must be nonempty and inside the panel. RGB565 colors use native
 * 16-bit values. DMA row copy stages caller data in DMA-accessible SRAM.
 * All display operations share the FCT worker ownership contract above.
 */
int hal_display_dma2d_fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
int hal_display_dma2d_row(uint16_t y, const uint16_t *pixels, uint16_t count);
/* CPU readback of one complete row, including DMA completion/cache visibility. */
int hal_display_read_row(uint16_t y, uint16_t *pixels, uint16_t count);
#endif
