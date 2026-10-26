/** @file hal_can_runtime.c @brief Classic FDCAN1 backend with fixed ISR-safe contexts. */
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include "hal/hal_can.h"
#include "hal/hal_dev.h"
#include "osal/osal.h"

#define HAL_CAN_FILTER_SLOTS 4U
#define HAL_CAN_TX_SLOTS 8U
BUILD_ASSERT(!IS_ENABLED(CONFIG_USB_DEVICE_STACK) && !IS_ENABLED(CONFIG_USB_DEVICE_STACK_NEXT) &&
                 !IS_ENABLED(CONFIG_USB_HOST_STACK),
    "Apollo CAN1 shares PA11/PA12 and P10 with USB");
static const struct device *const s_device = DEVICE_DT_GET(DT_NODELABEL(fdcan1));
static int s_bus_identity;
K_MUTEX_DEFINE(s_can_lock);

typedef struct
{
    bool active;
    int token;
    int native_std;
    int native_ext;
    hal_can_filter_t filter;
    hal_can_rx_cb_t callback;
    void *argument;
} hal_can_rx_slot_t;
typedef struct
{
    bool active;
    hal_can_done_cb_t callback;
    void *argument;
} hal_can_tx_slot_t;
static hal_can_rx_slot_t s_rx[HAL_CAN_FILTER_SLOTS];
static hal_can_tx_slot_t s_tx[HAL_CAN_TX_SLOTS];
static int s_next_token = 1;

static bool can_bus_valid(hal_can_bus_t bus)
{
    return bus == &s_bus_identity && device_is_ready(s_device);
}

hal_can_bus_t hal_can_get_bus(const char *name)
{
    return name != NULL && (strcmp(name, "can1") == 0 || strcmp(name, s_device->name) == 0) &&
                   device_is_ready(s_device)
               ? &s_bus_identity
               : NULL;
}

int hal_can_configure(hal_can_bus_t bus, uint32_t bitrate, hal_can_mode_t mode)
{
    can_mode_t native;
    int result;
    if (!can_bus_valid(bus) || bitrate < 10000U || bitrate > 1000000U)
    {
        return -EINVAL;
    }
    switch (mode)
    {
        case HAL_CAN_MODE_NORMAL:
            native = CAN_MODE_NORMAL;
            break;
        case HAL_CAN_MODE_INTERNAL_LOOPBACK:
            native = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;
            break;
        case HAL_CAN_MODE_LISTEN_ONLY:
            native = CAN_MODE_LISTENONLY;
            break;
        default:
            return -EINVAL;
    }
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    result = can_set_mode(s_device, native);
    if (result == 0)
    {
        result = can_set_bitrate(s_device, bitrate);
    }
    (void) k_mutex_unlock(&s_can_lock);
    return result;
}

int hal_can_start(hal_can_bus_t bus)
{
    int result;
    if (!can_bus_valid(bus))
    {
        return -EINVAL;
    }
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    result = can_start(s_device);
    (void) k_mutex_unlock(&s_can_lock);
    return result;
}

int hal_can_stop(hal_can_bus_t bus)
{
    int result;
    if (!can_bus_valid(bus))
    {
        return -EINVAL;
    }
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    result = can_stop(s_device);
    (void) k_mutex_unlock(&s_can_lock);
    return result == -EALREADY ? 0 : result;
}

static void can_tx_complete(const struct device *device, int status, void *argument)
{
    hal_can_tx_slot_t *slot = argument;
    hal_can_done_cb_t callback;
    void *user;
    uintptr_t key = osal_critical_enter();
    (void) device;
    callback = slot->active ? slot->callback : NULL;
    user = slot->argument;
    slot->active = false;
    osal_critical_exit(key);
    if (callback != NULL)
    {
        callback(user, status);
    }
}

int hal_can_send(hal_can_bus_t bus, const hal_can_msg_t *message, uint32_t timeout_ms,
    hal_can_done_cb_t callback, void *argument)
{
    struct can_frame native = {0};
    hal_can_tx_slot_t *slot = NULL;
    int result;
    if (!can_bus_valid(bus) || message == NULL || timeout_ms > HAL_CAN_MAX_TX_WAIT_MS || message->len > 8U ||
        (message->flags & ~(HAL_CAN_EXT_ID | HAL_CAN_RTR)) != 0U ||
        message->id > ((message->flags & HAL_CAN_EXT_ID) != 0U ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK) ||
        ((message->flags & HAL_CAN_RTR) != 0U && message->len != 0U))
    {
        return -EINVAL;
    }
    native.id = message->id;
    native.dlc = message->len;
    native.flags = (message->flags & HAL_CAN_EXT_ID) != 0U ? CAN_FRAME_IDE : 0;
    if ((message->flags & HAL_CAN_RTR) != 0U)
    {
        native.flags |= CAN_FRAME_RTR;
    }
    memcpy(native.data, message->data, message->len);
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    uintptr_t key = osal_critical_enter();
    for (size_t idx = 0; idx < HAL_CAN_TX_SLOTS; idx++)
    {
        if (!s_tx[idx].active)
        {
            slot = &s_tx[idx];
            *slot = (hal_can_tx_slot_t){.active = true, .callback = callback, .argument = argument};
            break;
        }
    }
    osal_critical_exit(key);
    if (slot == NULL)
    {
        (void) k_mutex_unlock(&s_can_lock);
        return -EBUSY;
    }
    /* Always asynchronous, even with no application callback. */
    result = can_send(s_device, &native,
        K_MSEC(timeout_ms), can_tx_complete, slot);
    if (result != 0)
    {
        key = osal_critical_enter();
        slot->active = false;
        osal_critical_exit(key);
    }
    (void) k_mutex_unlock(&s_can_lock);
    return result == -EAGAIN ? -ETIMEDOUT : result;
}

static void can_receive(const struct device *device, struct can_frame *frame, void *argument)
{
    hal_can_rx_slot_t *slot = argument;
    hal_can_rx_cb_t callback;
    void *user;
    hal_can_msg_t message = {.id = frame->id};
    (void) device;
    if (frame->dlc > 8U || (frame->flags & CAN_FRAME_FDF) != 0U)
    {
        return;
    }
    uintptr_t key = osal_critical_enter();
    callback = slot->active && ((frame->id & slot->filter.mask) ==
                                   (slot->filter.id & slot->filter.mask))
                   ? slot->callback
                   : NULL;
    user = slot->argument;
    osal_critical_exit(key);
    if (callback == NULL)
    {
        return;
    }
    if ((frame->flags & CAN_FRAME_IDE) != 0U)
    {
        message.flags |= HAL_CAN_EXT_ID;
    }
    if ((frame->flags & CAN_FRAME_RTR) != 0U)
    {
        message.flags |= HAL_CAN_RTR;
    }
    message.len = (message.flags & HAL_CAN_RTR) != 0U ? 0U : frame->dlc;
    memcpy(message.data, frame->data, message.len);
    callback(user, &message);
}

int hal_can_add_rx_filter(hal_can_bus_t bus, const hal_can_filter_t *filter,
    hal_can_rx_cb_t callback, void *argument)
{
    hal_can_rx_slot_t *slot = NULL;
    struct can_filter native;
    int result = -ENOSPC;
    if (!can_bus_valid(bus) || callback == NULL ||
        (filter != NULL && filter->id > CAN_EXT_ID_MASK))
    {
        return -EINVAL;
    }
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    for (size_t idx = 0; idx < HAL_CAN_FILTER_SLOTS; idx++)
    {
        if (!s_rx[idx].active)
        {
            slot = &s_rx[idx];
            break;
        }
    }
    if (slot == NULL || s_next_token == INT_MAX)
    {
        goto done;
    }
    uintptr_t key = osal_critical_enter();
    *slot = (hal_can_rx_slot_t){.filter = filter != NULL ? *filter : (hal_can_filter_t){0},
        .native_std = -1,
        .native_ext = -1,
        .callback = callback,
        .argument = argument};
    osal_critical_exit(key);
    /* The public contract matches numeric IDs in both frame formats. */
    if ((slot->filter.id & slot->filter.mask & ~CAN_STD_ID_MASK) == 0U)
    {
        native = (struct can_filter){.id = slot->filter.id & CAN_STD_ID_MASK,
            .mask = slot->filter.mask & CAN_STD_ID_MASK};
        slot->native_std = can_add_rx_filter(s_device, can_receive, slot, &native);
        if (slot->native_std < 0)
        {
            result = slot->native_std;
            goto done;
        }
    }
    native = (struct can_filter){.id = slot->filter.id,
        .mask = slot->filter.mask & CAN_EXT_ID_MASK,
        .flags = CAN_FILTER_IDE};
    slot->native_ext = can_add_rx_filter(s_device, can_receive, slot, &native);
    if (slot->native_ext < 0)
    {
        result = slot->native_ext;
        if (slot->native_std >= 0)
        {
            can_remove_rx_filter(s_device, slot->native_std);
        }
        goto done;
    }
    key = osal_critical_enter();
    slot->token = s_next_token++;
    slot->active = true;
    result = slot->token;
    osal_critical_exit(key);
done:
    (void) k_mutex_unlock(&s_can_lock);
    return result;
}

int hal_can_remove_filter(hal_can_bus_t bus, int token)
{
    int result = -EINVAL;
    if (!can_bus_valid(bus))
    {
        return -EINVAL;
    }
    (void) k_mutex_lock(&s_can_lock, K_FOREVER);
    for (size_t idx = 0; idx < HAL_CAN_FILTER_SLOTS; idx++)
    {
        hal_can_rx_slot_t *slot = &s_rx[idx];
        if (!slot->active || slot->token != token)
        {
            continue;
        }
        uintptr_t key = osal_critical_enter();
        slot->active = false;
        osal_critical_exit(key);
        if (slot->native_std >= 0)
        {
            can_remove_rx_filter(s_device, slot->native_std);
        }
        if (slot->native_ext >= 0)
        {
            can_remove_rx_filter(s_device, slot->native_ext);
        }
        result = 0;
        break;
    }
    (void) k_mutex_unlock(&s_can_lock);
    return result;
}

int hal_can_get_state(hal_can_bus_t bus, hal_can_state_t *state)
{
    enum can_state native;
    int result;
    if (!can_bus_valid(bus) || state == NULL)
    {
        return -EINVAL;
    }
    result = can_get_state(s_device, &native, NULL);
    if (result != 0)
    {
        return result;
    }
    switch (native)
    {
        case CAN_STATE_ERROR_ACTIVE:
            *state = HAL_CAN_STATE_ERROR_ACTIVE;
            break;
        case CAN_STATE_ERROR_WARNING:
            *state = HAL_CAN_STATE_ERROR_WARNING;
            break;
        case CAN_STATE_ERROR_PASSIVE:
            *state = HAL_CAN_STATE_ERROR_PASSIVE;
            break;
        case CAN_STATE_BUS_OFF:
            *state = HAL_CAN_STATE_BUS_OFF;
            break;
        case CAN_STATE_STOPPED:
            *state = HAL_CAN_STATE_STOPPED;
            break;
        default:
            return -EIO;
    }
    return 0;
}
