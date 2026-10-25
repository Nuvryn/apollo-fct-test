/** @file test_fixture.c @brief Translate board DT into platform-free values. */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include "support/test_fixture.h"

#define TEST_PIN(node, text, mode) {                                   \
    .name = text,                                                      \
    .port_name = DEVICE_DT_NAME(DT_GPIO_CTLR(node, gpios)),            \
    .pin = DT_GPIO_PIN(node, gpios),                                   \
    .active_low = (DT_GPIO_FLAGS(node, gpios) & GPIO_ACTIVE_LOW) != 0, \
    .input_mode = mode,                                                \
}

static const support_test_pin_t s_leds[] = {
    TEST_PIN(DT_ALIAS(led0), "DS0", HAL_GPIO_MODE_IN),
    TEST_PIN(DT_ALIAS(led1), "DS1", HAL_GPIO_MODE_IN),
};
static const support_test_pin_t s_keys[] = {
    TEST_PIN(DT_NODELABEL(fct_key_up), "WK_UP", HAL_GPIO_MODE_IN),
    TEST_PIN(DT_NODELABEL(fct_key2), "KEY2", HAL_GPIO_MODE_IN),
    TEST_PIN(DT_NODELABEL(fct_key1), "KEY1", HAL_GPIO_MODE_IN),
    TEST_PIN(DT_NODELABEL(fct_key0), "KEY0", HAL_GPIO_MODE_IN),
};

const support_test_pin_t *support_test_leds(size_t *count)
{
    *count = sizeof(s_leds) / sizeof(s_leds[0]);
    return s_leds;
}

const support_test_pin_t *support_test_keys(size_t *count)
{
    *count = sizeof(s_keys) / sizeof(s_keys[0]);
    return s_keys;
}

const char *support_test_i2c_name(void)
{
    return DEVICE_DT_NAME(DT_NODELABEL(i2c2));
}
