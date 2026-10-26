/** @file test_fixture.h @brief Pure-value board wiring for functional tests. */
#ifndef SUPPORT_TEST_FIXTURE_H
#define SUPPORT_TEST_FIXTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hal/hal_gpio.h"

typedef struct
{
    const char *name;
    const char *port_name;
    uint8_t pin;
    bool active_low;
    hal_gpio_mode_t input_mode;
} support_test_pin_t;

const support_test_pin_t *support_test_leds(size_t *count);
const support_test_pin_t *support_test_keys(size_t *count);
const char *support_test_i2c_name(void);
const char *support_test_can_name(void);

#endif
