/** @file basic_tests.c @brief Basic board tests using HAL and OSAL only. */
#include <errno.h>
#include <stdio.h>
#include "fct/fct_internal.h"
#include "hal/hal_dev.h"
#include "hal/hal_gpio.h"
#include "osal/osal.h"
#include "support/test_fixture.h"
#include "support/network_stack.h"

static int fct_led_level(const support_test_pin_t *pin, bool on)
{
    hal_gpio_port_t port = hal_gpio_get_port(pin->port_name);
    return port == NULL ? -ENODEV : hal_gpio_write(port, pin->pin, on != pin->active_low);
}

int fct_run_led(fct_context_t *context)
{
    size_t count;
    size_t configured = 0;
    int result = 0;
    const support_test_pin_t *leds = support_test_leds(&count);
    for (size_t idx = 0; idx < count; idx++)
    {
        hal_gpio_port_t port = hal_gpio_get_port(leds[idx].port_name);
        result = port == NULL ? -ENODEV : hal_gpio_init(port, leds[idx].pin, HAL_GPIO_MODE_OUT_PP);
        if (result != 0)
        {
            break;
        }
        configured++;
        result = fct_led_level(&leds[idx], false);
        if (result != 0)
        {
            break;
        }
    }
    for (size_t idx = 0; result == 0 && idx < count; idx++)
    {
        fct_output("Observe %s: three flashes", leds[idx].name);
        for (unsigned int pulse = 0; result == 0 && pulse < 3U; pulse++)
        {
            result = fct_led_level(&leds[idx], true);
            if (result == 0)
            {
                result = fct_sleep(context, 250);
            }
            if (result == 0)
            {
                result = fct_led_level(&leds[idx], false);
            }
            if (result == 0)
            {
                result = fct_sleep(context, 250);
            }
        }
    }
    if (result == 0)
    {
        result = fct_wait_confirmation(context);
    }
    for (size_t idx = 0; idx < configured; idx++)
    {
        int cleanup = fct_led_level(&leds[idx], false);
        if (cleanup != 0)
        {
            fct_output("LED cleanup failed: %s error %d", leds[idx].name, cleanup);
            if (result == 0)
            {
                result = cleanup;
            }
        }
    }
    (void) snprintf(context->reason, sizeof(context->reason), "LED sequence confirmed by operator");
    return result;
}

static int fct_wait_level(const fct_context_t *context, hal_gpio_port_t port,
    uint8_t pin, int wanted)
{
    unsigned int stable = 0;
    do
    {
        int result = fct_sleep(context, 10);
        int level;
        if (result != 0)
        {
            return result;
        }
        level = hal_gpio_read(port, pin);
        if (level < 0)
        {
            return level;
        }
        stable = level == wanted ? stable + 1U : 0U;
    } while (stable < 4U);
    return 0;
}

int fct_run_keys(fct_context_t *context)
{
    size_t count;
    const support_test_pin_t *keys = support_test_keys(&count);
    hal_gpio_port_t ports[4];
    int baseline[4];
    int result;
    if (count != 4U)
    {
        return -EINVAL;
    }
    fct_output("Release all four keys; sampling idle levels in 1 second");
    result = fct_sleep(context, 1000);
    if (result != 0)
    {
        return result;
    }
    for (size_t idx = 0; idx < count; idx++)
    {
        ports[idx] = hal_gpio_get_port(keys[idx].port_name);
        result = ports[idx] == NULL ? -ENODEV : hal_gpio_init(ports[idx], keys[idx].pin, keys[idx].input_mode);
        if (result != 0)
        {
            return result;
        }
        baseline[idx] = hal_gpio_read(ports[idx], keys[idx].pin);
        if (baseline[idx] < 0)
        {
            return baseline[idx];
        }
        for (unsigned int sample = 0; sample < 10U; sample++)
        {
            result = fct_sleep(context, 10);
            if (result != 0)
            {
                return result;
            }
            result = hal_gpio_read(ports[idx], keys[idx].pin);
            if (result < 0)
            {
                return result;
            }
            if (result != baseline[idx])
            {
                return -EIO;
            }
        }
        fct_output("%s idle raw=%d", keys[idx].name, baseline[idx]);
    }
    for (size_t idx = 0; idx < count; idx++)
    {
        fct_output("Press then release %s", keys[idx].name);
        result = fct_wait_level(context, ports[idx], keys[idx].pin, !baseline[idx]);
        if (result == 0)
        {
            result = fct_wait_level(context, ports[idx], keys[idx].pin, baseline[idx]);
        }
        if (result != 0)
        {
            fct_output("Incomplete key: %s", keys[idx].name);
            return result;
        }
        fct_output("%s completed", keys[idx].name);
    }
    (void) snprintf(context->reason, sizeof(context->reason), "four press/release cycles detected");
    return 0;
}

int fct_run_gpio(fct_context_t *context)
{
    size_t count;
    const support_test_pin_t *pins = support_test_leds(&count);
    for (size_t idx = 0; idx < count; idx++)
    {
        if (hal_gpio_get_port(pins[idx].port_name) == NULL)
        {
            return -ENODEV;
        }
    }
    pins = support_test_keys(&count);
    for (size_t idx = 0; idx < count; idx++)
    {
        if (hal_gpio_get_port(pins[idx].port_name) == NULL)
        {
            return -ENODEV;
        }
    }
    (void) snprintf(context->reason, sizeof(context->reason), "GPIO controllers ready; electrical behavior untested");
    return 0;
}

int fct_run_i2c(fct_context_t *context)
{
    if (hal_i2c_get_bus(support_test_i2c_name()) == NULL)
    {
        return -ENODEV;
    }
    (void) snprintf(context->reason, sizeof(context->reason), "I2C2 controller ready; devices and bus signals untested");
    return 0;
}

int fct_run_lwip(fct_context_t *context)
{
    int result = fct_checkpoint(context);
    if (result == 0)
    {
        result = support_network_selftest(1000);
    }
    (void) snprintf(context->reason, sizeof(context->reason),
        "lwIP UDP/IPv4 127.0.0.1 payload verified; MAC/PHY/cable untested");
    return result;
}
