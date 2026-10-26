/** @file can_test.h @brief Explicit parameters for external classic-CAN tests. */
#ifndef FCT_CAN_TEST_H
#define FCT_CAN_TEST_H
#include <stdbool.h>
#include "hal/hal_can.h"

typedef enum
{
    FCT_CAN_TX,
    FCT_CAN_RX
} fct_can_operation_t;
typedef struct
{
    fct_can_operation_t operation;
    uint32_t bitrate;
    hal_can_msg_t message;
    uint32_t wait_ms;
} fct_can_request_t;

/* External tests are never selected implicitly by run all. */
int fct_start_can(const fct_can_request_t *request);
int fct_can_validate(const fct_can_request_t *request);
int fct_can_parse(bool transmit, const char *bitrate, const char *format,
    const char *id, const char *data_or_wait, fct_can_request_t *output);
#endif
