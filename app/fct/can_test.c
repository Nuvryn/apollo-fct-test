/** @file can_test.c @brief Classic-CAN tests; callbacks only update static ISR-safe state. */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "fct/fct_internal.h"
#include "hal/hal_dev.h"
#include "osal/osal.h"
#include "support/test_fixture.h"

typedef struct
{
    bool active;
    bool tx_done;
    bool rx_done;
    int tx_status;
    uint32_t expected_id;
    uint8_t expected_flags;
    hal_can_msg_t received;
} fct_can_events_t;
static fct_can_events_t s_events;
static uint32_t s_nonce;

int fct_can_validate(const fct_can_request_t *request)
{
    if (request == NULL || (request->operation != FCT_CAN_TX && request->operation != FCT_CAN_RX) ||
        request->bitrate < 10000U || request->bitrate > 1000000U || request->wait_ms < 10U ||
        request->wait_ms > 5000U || request->message.len > 8U ||
        (request->message.flags & ~HAL_CAN_EXT_ID) != 0U ||
        request->message.id > ((request->message.flags & HAL_CAN_EXT_ID) != 0U ? 0x1fffffffU : 0x7ffU))
    {
        return -EINVAL;
    }
    return 0;
}

static void can_events_reset(const hal_can_msg_t *expected)
{
    uintptr_t key = osal_critical_enter();
    s_events = (fct_can_events_t){.active = true,
        .expected_id = expected->id,
        .expected_flags = expected->flags};
    osal_critical_exit(key);
}
static void can_received(void *argument, const hal_can_msg_t *message)
{
    (void) argument;
    uintptr_t key = osal_critical_enter();
    if (s_events.active && !s_events.rx_done && message->len <= 8U &&
        message->id == s_events.expected_id && message->flags == s_events.expected_flags)
    {
        s_events.received = *message;
        s_events.rx_done = true;
    }
    osal_critical_exit(key);
}
static void can_completed(void *argument, int status)
{
    (void) argument;
    uintptr_t key = osal_critical_enter();
    if (s_events.active)
    {
        s_events.tx_done = true;
        s_events.tx_status = status;
    }
    osal_critical_exit(key);
}

static int can_wait(fct_context_t *context, bool transmit, bool receive, uint32_t wait_ms,
    hal_can_msg_t *message)
{
    uint32_t started = osal_get_tick_ms();
    do
    {
        fct_can_events_t events;
        int result = fct_checkpoint(context);
        if (result != 0)
        {
            return result;
        }
        uintptr_t key = osal_critical_enter();
        events = s_events;
        osal_critical_exit(key);
        if (transmit && events.tx_done && events.tx_status != 0)
        {
            return events.tx_status;
        }
        if ((!transmit || events.tx_done) && (!receive || events.rx_done))
        {
            if (message != NULL)
            {
                *message = events.received;
            }
            return 0;
        }
        if ((uint32_t) (osal_get_tick_ms() - started) >= wait_ms)
        {
            return -ETIMEDOUT;
        }
        result = fct_sleep(context, 10);
        if (result != 0)
        {
            return result;
        }
    } while (true);
}

static int can_close(hal_can_bus_t bus, int filter, int result)
{
    uintptr_t key = osal_critical_enter();
    s_events.active = false;
    osal_critical_exit(key);
    if (bus != NULL)
    {
        hal_can_state_t state;
        int status = hal_can_get_state(bus, &state);
        if (status == 0)
        {
            fct_output("CAN state before stop=%u", (unsigned int) state);
        }
        if (result == 0 && (status != 0 || state != HAL_CAN_STATE_ERROR_ACTIVE))
        {
            result = status != 0 ? status : -EIO;
        }
        int stopped = hal_can_stop(bus);
        if (stopped != 0)
        {
            fct_output("CAN stop/abort failed: %d", stopped);
            if (result == 0)
            {
                result = stopped;
            }
        }
        if (filter >= 0)
        {
            int removed = hal_can_remove_filter(bus, filter);
            if (removed != 0)
            {
                fct_output("CAN filter removal failed: %d", removed);
                if (result == 0)
                {
                    result = removed;
                }
            }
        }
    }
    if (result != 0)
    {
        fct_output("CAN test failed: %d; inspect bitrate, P10, peer, termination and state", result);
    }
    return result;
}

static int can_open(hal_can_bus_t *bus, uint32_t bitrate, hal_can_mode_t mode)
{
    *bus = hal_can_get_bus(support_test_can_name());
    if (*bus == NULL)
    {
        return -ENODEV;
    }
    int result = hal_can_stop(*bus);
    if (result == 0)
    {
        result = hal_can_configure(*bus, bitrate, mode);
    }
    return result;
}

int fct_run_can_loopback(fct_context_t *context)
{
    hal_can_bus_t bus = NULL;
    int filter = -1;
    int result = can_open(&bus, 500000, HAL_CAN_MODE_INTERNAL_LOOPBACK);
    if (result != 0)
    {
        return can_close(bus, filter, result);
    }
    filter = hal_can_add_rx_filter(bus, NULL, can_received, NULL);
    if (filter < 0)
    {
        return can_close(bus, filter, filter);
    }
    result = hal_can_start(bus);
    for (unsigned int idx = 0; result == 0 && idx < 2U; idx++)
    {
        hal_can_msg_t expected = {.id = idx == 0U ? 0x321U : 0x18ff1234U,
            .flags = idx == 0U ? 0 : HAL_CAN_EXT_ID,
            .len = 8};
        hal_can_msg_t actual;
        uint32_t nonce = ++s_nonce;
        for (unsigned int byte = 0; byte < 4U; byte++)
        {
            expected.data[byte] = (uint8_t) (nonce >> (byte * 8U));
            expected.data[byte + 4U] = (uint8_t) (0xa0U + idx * 8U + byte);
        }
        can_events_reset(&expected);
        result = hal_can_send(bus, &expected, 20, can_completed, NULL);
        if (result == 0)
        {
            result = can_wait(context, true, true, 1000, &actual);
        }
        if (result == 0 && (actual.id != expected.id || actual.flags != expected.flags ||
                               actual.len != expected.len || memcmp(actual.data, expected.data, expected.len) != 0))
        {
            result = -EBADMSG;
        }
    }
    result = can_close(bus, filter, result);
    if (result == 0)
    {
        (void) snprintf(context->reason, sizeof(context->reason),
            "500k internal std/ext payloads verified; pins/P10/transceiver/bus untested");
    }
    return result;
}

int fct_run_can_tx(fct_context_t *context)
{
    hal_can_bus_t bus = NULL;
    const fct_can_request_t *request = &context->can_request;
    int result = can_open(&bus, request->bitrate, HAL_CAN_MODE_NORMAL);
    if (result == 0)
    {
        result = hal_can_start(bus);
    }
    if (result == 0)
    {
        fct_output("CAN external TX: %u bit/s id=0x%08x; peer ACK required", request->bitrate, request->message.id);
        can_events_reset(&request->message);
        result = hal_can_send(bus, &request->message, 20, can_completed, NULL);
    }
    if (result == 0)
    {
        result = can_wait(context, true, false, request->wait_ms, NULL);
    }
    result = can_close(bus, -1, result);
    if (result == 0)
    {
        (void) snprintf(context->reason, sizeof(context->reason),
            "external CAN TX acknowledged; peer payload/application not verified");
    }
    return result;
}

int fct_run_can_rx(fct_context_t *context)
{
    hal_can_bus_t bus = NULL;
    int filter = -1;
    hal_can_msg_t received;
    const fct_can_request_t *request = &context->can_request;
    int result = can_open(&bus, request->bitrate, HAL_CAN_MODE_NORMAL);
    if (result == 0)
    {
        hal_can_filter_t match = {.id = request->message.id, .mask = UINT32_MAX};
        filter = hal_can_add_rx_filter(bus, &match, can_received, NULL);
        if (filter < 0)
        {
            result = filter;
        }
    }
    if (result == 0)
    {
        can_events_reset(&request->message);
        result = hal_can_start(bus);
    }
    if (result == 0)
    {
        fct_output("CAN external RX: %u bit/s id=0x%08x; normal mode provides ACK", request->bitrate, request->message.id);
        result = can_wait(context, false, true, request->wait_ms, &received);
    }
    result = can_close(bus, filter, result);
    if (result == 0)
    {
        char payload[17] = {0};
        for (unsigned int idx = 0; idx < received.len; idx++)
        {
            (void) snprintf(&payload[idx * 2U], 3, "%02x", received.data[idx]);
        }
        fct_output("CAN RX %s id=0x%08x len=%u data=%s",
            (received.flags & HAL_CAN_EXT_ID) != 0U ? "ext" : "std", received.id, received.len, payload);
        (void) snprintf(context->reason, sizeof(context->reason), "external frame received, id=0x%08x len=%u",
            received.id, received.len);
    }
    return result;
}
