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
#endif
