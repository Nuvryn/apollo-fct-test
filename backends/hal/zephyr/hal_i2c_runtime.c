/** @file hal_i2c_runtime.c @brief Bounded native I2C transactions. */
#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include "hal/hal_i2c.h"
#include "hal/hal_dev.h"

#define HAL_ZEPHYR_I2C_MAX_MSGS 8U
K_MUTEX_DEFINE(s_i2c_lock);

hal_i2c_bus_t hal_i2c_get_bus(const char *name)
{
    const struct device *device;
    if (name == NULL)
    {
        return NULL;
    }
    device = device_get_binding(name);
    return device != NULL && device_is_ready(device) ? (void *) device : NULL;
}

int hal_i2c_transfer(hal_i2c_bus_t bus, hal_i2c_msg_t *msgs, uint16_t count)
{
    struct i2c_msg native[HAL_ZEPHYR_I2C_MAX_MSGS];
    int result;
    if (bus == NULL || msgs == NULL || count == 0U || count > HAL_ZEPHYR_I2C_MAX_MSGS)
    {
        return -EINVAL;
    }
    for (uint16_t idx = 0U; idx < count; idx++)
    {
        uint16_t direction = msgs[idx].flags & (HAL_I2C_RD | HAL_I2C_WR);
        if ((msgs[idx].flags & HAL_I2C_ADDR_10BIT) != 0U)
        {
            return -ENOTSUP;
        }
        if ((msgs[idx].flags & ~(HAL_I2C_RD | HAL_I2C_WR | HAL_I2C_NO_STOP)) != 0U ||
            (direction != HAL_I2C_RD && direction != HAL_I2C_WR) ||
            msgs[idx].addr > 0x7fU || msgs[idx].addr != msgs[0].addr ||
            (msgs[idx].len > 0U && msgs[idx].buf == NULL) ||
            (msgs[idx].len == 0U && direction == HAL_I2C_RD))
        {
            return -EINVAL;
        }
        native[idx].buf = msgs[idx].buf;
        native[idx].len = msgs[idx].len;
        native[idx].flags = direction == HAL_I2C_RD ? I2C_MSG_READ : I2C_MSG_WRITE;
        if ((msgs[idx].flags & HAL_I2C_NO_STOP) == 0U)
        {
            native[idx].flags |= I2C_MSG_STOP;
        }
        if (idx > 0U && (msgs[idx - 1U].flags & HAL_I2C_NO_STOP) != 0U)
        {
            native[idx].flags |= I2C_MSG_RESTART;
        }
    }
    if ((msgs[count - 1U].flags & HAL_I2C_NO_STOP) != 0U)
    {
        return -EINVAL;
    }
    result = k_mutex_lock(&s_i2c_lock, K_MSEC(200));
    if (result != 0)
    {
        return result;
    }
    result = i2c_transfer(bus, native, (uint8_t) count, msgs[0].addr);
    (void) k_mutex_unlock(&s_i2c_lock);
    return result;
}

int hal_i2c_read_reg(hal_i2c_bus_t bus, uint8_t addr, uint8_t reg,
    uint8_t *buffer, uint16_t count)
{
    hal_i2c_msg_t messages[] = {
        {.addr = addr, .flags = HAL_I2C_WR | HAL_I2C_NO_STOP, .buf = &reg, .len = 1},
        {.addr = addr, .flags = HAL_I2C_RD, .buf = buffer, .len = count},
    };
    return count == 0U ? -EINVAL : hal_i2c_transfer(bus, messages, 2);
}

int hal_i2c_write_reg(hal_i2c_bus_t bus, uint8_t addr, uint8_t reg,
    const uint8_t *buffer, uint16_t count)
{
    uint8_t *payload;
    int result;
    hal_i2c_msg_t message;
    if (buffer == NULL || count == 0U || count == UINT16_MAX)
    {
        return -EINVAL;
    }
    payload = k_malloc((size_t) count + 1U);
    if (payload == NULL)
    {
        return -ENOMEM;
    }
    payload[0] = reg;
    memcpy(&payload[1], buffer, count);
    message = (hal_i2c_msg_t){.addr = addr, .flags = HAL_I2C_WR, .buf = payload, .len = (uint16_t) (count + 1U)};
    result = hal_i2c_transfer(bus, &message, 1);
    k_free(payload);
    return result;
}
