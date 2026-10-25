/** @file hal_gpio_runtime.c @brief Real GPIO backend; interrupts deferred. */
#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "hal/hal_gpio.h"
#include "hal/hal_dev.h"

hal_gpio_port_t hal_gpio_get_port(const char *name)
{
    const struct device *device;
    if (name == NULL)
    {
        return NULL;
    }
    device = device_get_binding(name);
    return device != NULL && device_is_ready(device) ? (void *) device : NULL;
}

int hal_gpio_init(hal_gpio_port_t port, uint8_t pin, hal_gpio_mode_t mode)
{
    gpio_flags_t flags;
    if (port == NULL || pin >= 16U)
    {
        return -EINVAL;
    }
    switch (mode)
    {
        case HAL_GPIO_MODE_OUT_PP:
            flags = GPIO_OUTPUT;
            break;
        case HAL_GPIO_MODE_OUT_OD:
            flags = GPIO_OUTPUT | GPIO_OPEN_DRAIN;
            break;
        case HAL_GPIO_MODE_IN:
            flags = GPIO_INPUT;
            break;
        case HAL_GPIO_MODE_IN_PU:
            flags = GPIO_INPUT | GPIO_PULL_UP;
            break;
        case HAL_GPIO_MODE_IN_PD:
            flags = GPIO_INPUT | GPIO_PULL_DOWN;
            break;
        default:
            return -EINVAL;
    }
    return gpio_pin_configure(port, pin, flags);
}

int hal_gpio_write(hal_gpio_port_t port, uint8_t pin, int value)
{
    return port == NULL || pin >= 16U ? -EINVAL : gpio_pin_set_raw(port, pin, value != 0);
}

int hal_gpio_read(hal_gpio_port_t port, uint8_t pin)
{
    return port == NULL || pin >= 16U ? -EINVAL : gpio_pin_get_raw(port, pin);
}

int hal_gpio_toggle(hal_gpio_port_t port, uint8_t pin)
{
    return port == NULL || pin >= 16U ? -EINVAL : gpio_pin_toggle(port, pin);
}

int hal_gpio_set_int(hal_gpio_port_t port, uint8_t pin, hal_gpio_int_mode_t mode,
    hal_gpio_cb_t callback, void *user_data)
{
    (void) port;
    (void) pin;
    (void) mode;
    (void) callback;
    (void) user_data;
    return -ENOSYS;
}
