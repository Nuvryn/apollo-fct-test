/** @file can_backend_tests.c @brief Exercise real HAL wrapper against a fake native driver. */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include "hal/hal_can.h"
#include "hal/hal_dev.h"
#include "osal/osal.h"

const struct device test_can_device = {.name = "can@4000a000"};
static bool s_ready = true;
static bool s_running;
static unsigned int s_critical_depth;
static unsigned int s_remove_count;
static int s_add_failure_after = -1;
static int s_send_error;
static can_mode_t s_mode;
static uint32_t s_bitrate;
static enum can_state s_state = CAN_STATE_STOPPED;
static struct can_frame s_last_frame;
static unsigned int s_done_count;
static int s_done_status;
static hal_can_msg_t s_received;
static unsigned int s_received_count;
typedef struct
{
    bool active;
    can_tx_callback_t callback;
    void *argument;
} pending_t;
typedef struct
{
    bool active;
    can_rx_callback_t callback;
    void *argument;
    struct can_filter filter;
} rx_filter_t;
static pending_t s_pending[16];
static rx_filter_t s_filters[16];

uintptr_t osal_critical_enter(void)
{
    return s_critical_depth++;
}
void osal_critical_exit(uintptr_t key)
{
    assert(s_critical_depth == key + 1U);
    s_critical_depth--;
}
bool device_is_ready(const struct device *device)
{
    assert(device == &test_can_device);
    return s_ready;
}
int k_mutex_lock(struct k_mutex *mutex, int64_t timeout)
{
    (void) timeout;
    assert(mutex->locked == 0);
    mutex->locked = 1;
    return 0;
}
int k_mutex_unlock(struct k_mutex *mutex)
{
    assert(mutex->locked == 1);
    mutex->locked = 0;
    return 0;
}
int can_set_mode(const struct device *device, can_mode_t mode)
{
    (void) device;
    if (s_running)
    {
        return -EBUSY;
    }
    s_mode = mode;
    return 0;
}
int can_set_bitrate(const struct device *device, uint32_t bitrate)
{
    (void) device;
    s_bitrate = bitrate;
    return 0;
}
int can_start(const struct device *device)
{
    (void) device;
    s_running = true;
    s_state = CAN_STATE_ERROR_ACTIVE;
    return 0;
}
int can_stop(const struct device *device)
{
    if (!s_running)
    {
        return -EALREADY;
    }
    s_running = false;
    s_state = CAN_STATE_STOPPED;
    for (size_t idx = 0; idx < 16U; idx++)
    {
        if (s_pending[idx].active)
        {
            pending_t pending = s_pending[idx];
            s_pending[idx].active = false;
            pending.callback(device, -ENETDOWN, pending.argument);
        }
    }
    return 0;
}
int can_send(const struct device *device, const struct can_frame *frame, int64_t timeout,
    can_tx_callback_t callback, void *argument)
{
    (void) device;
    assert(timeout == 0 || timeout == 20);
    assert(callback != NULL); /* HAL NULL callback must not select blocking native send. */
    if (s_send_error != 0)
    {
        return s_send_error;
    }
    s_last_frame = *frame;
    for (size_t idx = 0; idx < 16U; idx++)
    {
        if (!s_pending[idx].active)
        {
            s_pending[idx] = (pending_t){.active = true, .callback = callback, .argument = argument};
            return 0;
        }
    }
    return -EAGAIN;
}
int can_add_rx_filter(const struct device *device, can_rx_callback_t callback,
    void *argument, const struct can_filter *filter)
{
    (void) device;
    if (s_add_failure_after == 0)
    {
        s_add_failure_after = -1;
        return -ENOSPC;
    }
    if (s_add_failure_after > 0)
    {
        s_add_failure_after--;
    }
    for (size_t idx = 0; idx < 16U; idx++)
    {
        if (!s_filters[idx].active)
        {
            s_filters[idx] = (rx_filter_t){.active = true, .callback = callback, .argument = argument, .filter = *filter};
            return (int) idx;
        }
    }
    return -ENOSPC;
}
void can_remove_rx_filter(const struct device *device, int filter)
{
    (void) device;
    assert(filter >= 0 && filter < 16 && s_filters[filter].active);
    s_filters[filter].active = false;
    s_remove_count++;
}
int can_get_state(const struct device *device, enum can_state *state, void *counts)
{
    (void) device;
    assert(counts == NULL);
    *state = s_state;
    return 0;
}
static void test_done(void *argument, int status)
{
    assert(argument == &s_done_count);
    s_done_count++;
    s_done_status = status;
}
static void test_receive(void *argument, const hal_can_msg_t *frame)
{
    assert(argument == &s_received_count);
    s_received_count++;
    s_received = *frame;
}
static void inject(struct can_frame frame)
{
    for (size_t idx = 0; idx < 16U; idx++)
    {
        rx_filter_t *filter = &s_filters[idx];
        if (filter->active && ((frame.flags & CAN_FRAME_IDE) != 0U) == ((filter->filter.flags & CAN_FILTER_IDE) != 0U) &&
            (frame.id & filter->filter.mask) == (filter->filter.id & filter->filter.mask))
        {
            filter->callback(&test_can_device, &frame, filter->argument);
        }
    }
}

int main(void)
{
    assert(hal_can_get_bus(NULL) == NULL && hal_can_get_bus("wrong") == NULL);
    s_ready = false;
    assert(hal_can_get_bus("can1") == NULL);
    s_ready = true;
    hal_can_bus_t bus = hal_can_get_bus("can1");
    assert(bus != NULL && hal_can_get_bus(test_can_device.name) == bus);
    assert(hal_can_configure(bus, 500000, HAL_CAN_MODE_INTERNAL_LOOPBACK) == 0);
    assert(s_mode == (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY) && s_bitrate == 500000U);
    assert(hal_can_configure(bus, 0, HAL_CAN_MODE_NORMAL) == -EINVAL);
    assert(hal_can_start(bus) == 0);
    assert(hal_can_configure(bus, 500000, HAL_CAN_MODE_NORMAL) == -EBUSY);
    hal_can_msg_t message = {.id = 0x18ff1234, .flags = HAL_CAN_EXT_ID, .len = 8, .data = {0, 1, 2, 3, 4, 5, 6, 7}};
    assert(hal_can_send(bus, &message, HAL_CAN_MAX_TX_WAIT_MS + 1U, NULL, NULL) == -EINVAL);
    assert(hal_can_send(bus, &message, 20, test_done, &s_done_count) == 0);
    assert(s_last_frame.id == message.id && s_last_frame.flags == CAN_FRAME_IDE &&
           s_last_frame.dlc == 8U && memcmp(s_last_frame.data, message.data, 8) == 0);
    pending_t completed = s_pending[0];
    s_pending[0].active = false;
    completed.callback(&test_can_device, 0, completed.argument);
    assert(s_done_count == 1U && s_done_status == 0);
    message.id = 0x800;
    message.flags = 0;
    assert(hal_can_send(bus, &message, 0, NULL, NULL) == -EINVAL);
    message.id = 0x123;
    message.len = 9;
    assert(hal_can_send(bus, &message, 0, NULL, NULL) == -EINVAL);
    message.len = 0;
    message.flags = HAL_CAN_RTR;
    assert(hal_can_send(bus, &message, 0, NULL, NULL) == 0);
    assert(s_last_frame.flags == CAN_FRAME_RTR && s_last_frame.dlc == 0U);
    assert(hal_can_stop(bus) == 0 && hal_can_stop(bus) == 0);
    assert(hal_can_start(bus) == 0);
    message.flags = 0;
    s_send_error = -EAGAIN;
    assert(hal_can_send(bus, &message, 20, NULL, NULL) == -ETIMEDOUT);
    s_send_error = 0;
    for (unsigned int idx = 0; idx < 8U; idx++)
    {
        assert(hal_can_send(bus, &message, 20, test_done, &s_done_count) == 0);
    }
    assert(hal_can_send(bus, &message, 20, NULL, NULL) == -EBUSY);
    assert(hal_can_stop(bus) == 0 && s_done_count == 9U && s_done_status == -ENETDOWN);
    assert(hal_can_start(bus) == 0 && hal_can_send(bus, &message, 20, NULL, NULL) == 0);
    assert(hal_can_stop(bus) == 0);

    hal_can_filter_t exact = {.id = 0x123, .mask = UINT32_MAX};
    s_add_failure_after = 1;
    assert(hal_can_add_rx_filter(bus, &exact, test_receive, &s_received_count) == -ENOSPC);
    assert(s_remove_count == 1U); /* STD half rolled back when EXT allocation failed. */
    int token = hal_can_add_rx_filter(bus, &exact, test_receive, &s_received_count);
    assert(token >= 0);
    struct can_frame frame = {.id = 0x123, .dlc = 2, .data = {0xab, 0xcd}};
    inject(frame);
    assert(s_received_count == 1U && s_received.len == 2U && s_received.data[1] == 0xcd);
    frame.flags = CAN_FRAME_IDE;
    inject(frame);
    assert(s_received_count == 2U && s_received.flags == HAL_CAN_EXT_ID);
    frame.flags = CAN_FRAME_IDE | CAN_FRAME_RTR;
    frame.dlc = 8;
    inject(frame);
    assert(s_received_count == 3U && s_received.len == 0U && (s_received.flags & HAL_CAN_RTR) != 0U);
    frame.flags = CAN_FRAME_FDF;
    inject(frame);
    assert(s_received_count == 3U);
    assert(hal_can_remove_filter(bus, token) == 0 && hal_can_remove_filter(bus, token) == -EINVAL);
    frame.flags = 0;
    inject(frame);
    assert(s_received_count == 3U);
    int replacement = hal_can_add_rx_filter(bus, NULL, test_receive, &s_received_count);
    assert(replacement > token && hal_can_remove_filter(bus, token) == -EINVAL);
    assert(hal_can_remove_filter(bus, replacement) == 0);
    int tokens[4];
    for (size_t idx = 0; idx < 4U; idx++)
    {
        tokens[idx] = hal_can_add_rx_filter(bus, NULL, test_receive, &s_received_count);
        assert(tokens[idx] >= 0);
    }
    assert(hal_can_add_rx_filter(bus, NULL, test_receive, &s_received_count) == -ENOSPC);
    for (size_t idx = 0; idx < 4U; idx++)
    {
        assert(hal_can_remove_filter(bus, tokens[idx]) == 0);
    }
    hal_can_state_t state;
    s_state = CAN_STATE_ERROR_WARNING;
    assert(hal_can_get_state(bus, &state) == 0 && state == HAL_CAN_STATE_ERROR_WARNING);
    s_state = CAN_STATE_BUS_OFF;
    assert(hal_can_get_state(bus, &state) == 0 && state == HAL_CAN_STATE_BUS_OFF);
    s_state = CAN_STATE_STOPPED;
    assert(hal_can_get_state(bus, &state) == 0 && state == HAL_CAN_STATE_STOPPED);
    puts("PASS: CAN native translation, fixed callback contexts, async TX, abort, filter rollback and state");
    return 0;
}
