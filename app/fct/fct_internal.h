/** @file fct_internal.h @brief Worker/test boundary, not a public contract. */
#ifndef FCT_INTERNAL_H
#define FCT_INTERNAL_H

#include "fct/fct.h"
#include "fct/can_test.h"

typedef struct
{
    uint32_t started_ms;
    uint32_t timeout_ms;
    char reason[FCT_REASON_SIZE];
    fct_can_request_t can_request;
} fct_context_t;

void fct_output(const char *format, ...);
int fct_checkpoint(const fct_context_t *context);
int fct_sleep(const fct_context_t *context, uint32_t delay_ms);
int fct_wait_confirmation(const fct_context_t *context);
int fct_run_dma2d(fct_context_t *context);
int fct_run_lcd_dma2d(fct_context_t *context);
int fct_run_lcd(fct_context_t *context);
int fct_run_led(fct_context_t *context);
int fct_run_keys(fct_context_t *context);
int fct_run_gpio(fct_context_t *context);
int fct_run_i2c(fct_context_t *context);
int fct_run_lwip(fct_context_t *context);
int fct_run_can_loopback(fct_context_t *context);
int fct_run_can_tx(fct_context_t *context);
int fct_run_can_rx(fct_context_t *context);
/* A single bounded dispatch iteration, also used by host unit tests. */
int fct_process_next(uint32_t timeout_ms);

#endif
