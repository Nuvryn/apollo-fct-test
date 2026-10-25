/** @file fct.h @brief Platform-independent functional-test control API. */
#ifndef FCT_FCT_H
#define FCT_FCT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FCT_REASON_SIZE 96U
typedef enum
{
    FCT_NOT_RUN,
    FCT_RUNNING,
    FCT_PASS,
    FCT_FAIL,
    FCT_SKIP,
    FCT_CANCELLED
} fct_state_t;

typedef struct
{
    const char *id;
    const char *description;
    bool interactive;
    uint32_t timeout_ms;
} fct_case_info_t;

typedef struct
{
    fct_state_t state;
    uint32_t elapsed_ms;
    char reason[FCT_REASON_SIZE];
} fct_result_t;

/* Output callback must accept calls from the worker thread. */
int fct_init(void (*output)(const char *text));
size_t fct_count(void);
const fct_case_info_t *fct_info(size_t index);
const char *fct_state_name(fct_state_t state);
int fct_result(size_t index, fct_result_t *out);
bool fct_busy(void);
int fct_start(const char *id);
int fct_stop(void);
int fct_confirm(bool passed);
int fct_i2c_probe(uint16_t address);

#endif
