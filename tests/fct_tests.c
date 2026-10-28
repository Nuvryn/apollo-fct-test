/** @file fct_tests.c @brief Deterministic tests with fake time and hardware. */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "fct/fct_internal.h"
#include "fct/can_test.h"
#include "hal/hal_can.h"
#include "hal/hal_display.h"
#include "osal/osal.h"
#include "hal/hal_dev.h"
#include "hal/hal_i2c.h"
#include "support/test_fixture.h"
#include "support/network_stack.h"

static int s_display_error;
static int s_display_write_error;
static int s_display_cleanup_error;
static bool s_display_blank = true;
static unsigned int s_display_rows;
int hal_display_ready(void) { return s_display_error; }
int hal_display_blanking(bool blank)
{
    s_display_blank = blank;
    return blank && s_display_rows > 0U ? s_display_cleanup_error : 0;
}
int hal_display_write_row(uint16_t y, const uint16_t *pixels, uint16_t count)
{
    static const uint16_t colors[] = {0xf800, 0x07e0, 0x001f, 0xffff, 0};
    assert(count == HAL_DISPLAY_WIDTH && y < HAL_DISPLAY_HEIGHT);
    unsigned int pattern = s_display_rows / HAL_DISPLAY_HEIGHT;
    assert(y == s_display_rows % HAL_DISPLAY_HEIGHT && pattern < 7U);
    for (unsigned int x = 0; x < count; x++)
    {
        uint16_t expected = pattern < 5U ? colors[pattern] :
            (pattern == 5U ? colors[x * 5U / count] :
            ((x % 64U == 0U || y % 60U == 0U || x == count - 1U || y == HAL_DISPLAY_HEIGHT - 1U) ? 0xffffU : 0));
        assert(pixels[x] == expected);
    }
    s_display_rows++;
    return s_display_write_error;
}

static int s_network_result;

int support_network_selftest(uint32_t timeout_ms)
{
    assert(timeout_ms > 0U);
    return s_network_result; /* Real lwIP is exercised by lwip_tests. */
}

static uint32_t s_now;
static uint32_t s_cancel_at;
static int s_lock_token;
static int s_queue_token;
static int s_gpio_token;
static int s_i2c_token;
static int s_request;
static bool s_queued;
static bool s_locked;
static bool s_missing_gpio;
static bool s_missing_i2c;
static int s_task_result;
static int s_confirmation_mode;
static int s_current_key = -1;
static unsigned int s_key_step;
static bool s_animate_keys;
static int s_led_levels[2];
static unsigned int s_probe_count;
static bool s_cancel_on_start;
static int s_can_token;
static unsigned int s_critical_depth;
static bool s_can_started;
static bool s_can_corrupt;
static bool s_can_drop_tx;
static bool s_can_drop_rx;
static uint8_t s_can_external_flags;
static bool s_can_stop_failure;
static bool s_can_filter_failure;
static bool s_can_queue_failure;
static int s_can_tx_status;
static hal_can_state_t s_can_error_state = HAL_CAN_STATE_ERROR_ACTIVE;
static hal_can_mode_t s_can_mode;
static hal_can_rx_cb_t s_can_receive;
static void *s_can_receive_arg;
static hal_can_done_cb_t s_can_pending;
static void *s_can_pending_arg;
static hal_can_filter_t s_can_filter;
static unsigned int s_can_sends;
uintptr_t osal_critical_enter(void)
{
    return s_critical_depth++;
}
void osal_critical_exit(uintptr_t key)
{
    assert(s_critical_depth == key + 1U);
    s_critical_depth--;
}
const char *support_test_can_name(void)
{
    return "can1";
}
hal_can_bus_t hal_can_get_bus(const char *name)
{
    (void) name;
    return &s_can_token;
}
int hal_can_configure(hal_can_bus_t bus, uint32_t bitrate, hal_can_mode_t mode)
{
    assert(bus == &s_can_token && !s_can_started && bitrate >= 10000U);
    s_can_mode = mode;
    return 0;
}
int hal_can_start(hal_can_bus_t bus)
{
    assert(bus == &s_can_token && !s_can_started);
    s_can_started = true;
    if (s_can_mode == HAL_CAN_MODE_NORMAL && s_can_receive != NULL && !s_can_drop_rx)
    {
        hal_can_msg_t frame = {.id = s_can_filter.id, .flags = s_can_external_flags, .len = 2, .data = {0x12, 0x34}};
        s_can_receive(s_can_receive_arg, &frame);
    }
    return 0;
}
int hal_can_stop(hal_can_bus_t bus)
{
    assert(bus == &s_can_token);
    if (s_can_stop_failure && s_can_started)
    {
        return -EIO;
    }
    s_can_started = false;
    if (s_can_pending != NULL)
    {
        hal_can_done_cb_t callback = s_can_pending;
        s_can_pending = NULL;
        callback(s_can_pending_arg, -ENETDOWN);
    }
    return 0;
}
int hal_can_send(hal_can_bus_t bus, const hal_can_msg_t *message, uint32_t timeout_ms,
    hal_can_done_cb_t callback, void *argument)
{
    assert(bus == &s_can_token && s_can_started && timeout_ms == 20U);
    s_can_sends++;
    if (s_can_queue_failure)
    {
        return -EBUSY;
    }
    if (s_can_mode == HAL_CAN_MODE_INTERNAL_LOOPBACK && s_can_receive != NULL && !s_can_drop_rx)
    {
        hal_can_msg_t frame = *message;
        if (s_can_corrupt)
        {
            frame.data[0] ^= 1U;
        }
        s_can_receive(s_can_receive_arg, &frame);
    }
    if (s_can_drop_tx)
    {
        s_can_pending = callback;
        s_can_pending_arg = argument;
    }
    else
    {
        callback(argument, s_can_tx_status);
    }
    return 0;
}
int hal_can_add_rx_filter(hal_can_bus_t bus, const hal_can_filter_t *filter,
    hal_can_rx_cb_t callback, void *argument)
{
    assert(bus == &s_can_token && s_can_receive == NULL);
    if (s_can_filter_failure)
    {
        return -ENOSPC;
    }
    s_can_receive = callback;
    s_can_receive_arg = argument;
    s_can_filter = filter != NULL ? *filter : (hal_can_filter_t){0};
    return 1;
}
int hal_can_remove_filter(hal_can_bus_t bus, int filter)
{
    assert(bus == &s_can_token && filter == 1 && s_can_receive != NULL);
    s_can_receive = NULL;
    return 0;
}
int hal_can_get_state(hal_can_bus_t bus, hal_can_state_t *state)
{
    assert(bus == &s_can_token);
    *state = s_can_started ? s_can_error_state : HAL_CAN_STATE_STOPPED;
    return 0;
}

static const support_test_pin_t s_leds[] = {
    {"DS0", "gpio", 1, true, HAL_GPIO_MODE_IN},
    {"DS1", "gpio", 0, true, HAL_GPIO_MODE_IN},
};
static const support_test_pin_t s_keys[] = {
    {"WK_UP", "gpio", 2, false, HAL_GPIO_MODE_IN},
    {"KEY2", "gpio", 3, false, HAL_GPIO_MODE_IN},
    {"KEY1", "gpio", 4, false, HAL_GPIO_MODE_IN},
    {"KEY0", "gpio", 5, false, HAL_GPIO_MODE_IN},
};

int osal_mutex_create(osal_mutex_t *mutex)
{
    *mutex = &s_lock_token;
    return 0;
}
int osal_mutex_destroy(osal_mutex_t mutex)
{
    (void) mutex;
    return 0;
}
int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    (void) mutex;
    (void) timeout_ms;
    assert(!s_locked);
    s_locked = true;
    return 0;
}
int osal_mutex_unlock(osal_mutex_t mutex)
{
    (void) mutex;
    assert(s_locked);
    s_locked = false;
    return 0;
}
int osal_mq_create(osal_mq_t *queue, size_t size, size_t count)
{
    assert(size == sizeof(int) && count == 1U);
    *queue = &s_queue_token;
    s_queued = false;
    return 0;
}
int osal_mq_destroy(osal_mq_t queue)
{
    (void) queue;
    return 0;
}
int osal_mq_send(osal_mq_t queue, const void *message, uint32_t timeout_ms)
{
    (void) queue;
    (void) timeout_ms;
    if (s_queued)
    {
        return -ETIMEDOUT;
    }
    memcpy(&s_request, message, sizeof(s_request));
    s_queued = true;
    return 0;
}
int osal_mq_recv(osal_mq_t queue, void *message, uint32_t timeout_ms)
{
    (void) queue;
    (void) timeout_ms;
    if (!s_queued)
    {
        return -ETIMEDOUT;
    }
    memcpy(message, &s_request, sizeof(s_request));
    s_queued = false;
    return 0;
}
int osal_task_create(const osal_task_attr_t *attr)
{
    assert(attr->entry != NULL && attr->stack_size >= 2048U);
    return s_task_result; /* Dispatch is explicitly advanced by this test. */
}
uint32_t osal_get_tick_ms(void)
{
    return s_now;
}
void osal_delay_ms(uint32_t delay_ms)
{
    s_now += delay_ms;
    if (s_cancel_at != 0U && s_now >= s_cancel_at)
    {
        s_cancel_at = 0;
        assert(fct_stop() == 0);
    }
}

const support_test_pin_t *support_test_leds(size_t *count)
{
    *count = 2;
    return s_leds;
}
const support_test_pin_t *support_test_keys(size_t *count)
{
    *count = 4;
    return s_keys;
}
const char *support_test_i2c_name(void)
{
    return "i2c";
}
hal_gpio_port_t hal_gpio_get_port(const char *name)
{
    (void) name;
    return s_missing_gpio ? NULL : &s_gpio_token;
}
int hal_gpio_init(hal_gpio_port_t port, uint8_t pin, hal_gpio_mode_t mode)
{
    (void) pin;
    (void) mode;
    return port == NULL ? -ENODEV : 0;
}
int hal_gpio_write(hal_gpio_port_t port, uint8_t pin, int value)
{
    assert(port != NULL && pin < 2U);
    s_led_levels[pin] = value;
    return 0;
}
int hal_gpio_read(hal_gpio_port_t port, uint8_t pin)
{
    assert(port != NULL);
    if (s_animate_keys && (int) pin == s_current_key + 2)
    {
        unsigned int step = s_key_step++;
        if (step < 4U)
        {
            return (int) (step % 2U);
        } /* Bounce must not count. */
        return step < 8U ? 1 : 0;
    }
    return 0;
}
hal_i2c_bus_t hal_i2c_get_bus(const char *name)
{
    (void) name;
    return s_missing_i2c ? NULL : &s_i2c_token;
}
int hal_i2c_transfer(hal_i2c_bus_t bus, hal_i2c_msg_t *msgs, uint16_t count)
{
    assert(bus != NULL && count == 1U && msgs[0].len == 0U);
    assert(msgs[0].flags == HAL_I2C_WR);
    s_probe_count++;
    return -EIO;
}

static void test_output(const char *text)
{
    if (s_cancel_on_start && strncmp(text, "START ", 6) == 0)
    {
        s_cancel_on_start = false;
        assert(fct_stop() == 0);
    }
    if (strstr(text, "Observe hardware:") != NULL && s_confirmation_mode != 0)
    {
        assert(fct_confirm(s_confirmation_mode > 0) == 0);
        assert(fct_confirm(true) == -EINVAL);
    }
    for (size_t idx = 0; idx < 4; idx++)
    {
        char expected[64];
        (void) snprintf(expected, sizeof(expected), "Press then release %s", s_keys[idx].name);
        if (strcmp(text, expected) == 0)
        {
            s_current_key = (int) idx;
            s_key_step = 0;
        }
    }
}

static fct_result_t test_result(size_t index, fct_state_t expected)
{
    fct_result_t result;
    assert(fct_result(index, &result) == 0);
    assert(result.state == expected);
    return result;
}

static void test_led(int confirmation, fct_state_t expected)
{
    s_confirmation_mode = confirmation;
    assert(fct_start("led") == 0);
    assert(fct_process_next(0) == 0);
    (void) test_result(2, expected);
    assert(s_led_levels[0] == 1 && s_led_levels[1] == 1);
    assert(!fct_busy());
}

static void test_can_cases(void)
{
    fct_can_request_t request;
    assert(fct_can_parse(true, "500000", "std", "0x123", "01aB", &request) == 0);
    assert(request.message.len == 2 && request.message.data[1] == 0xab);
    assert(fct_can_parse(true, "500000", "std", "0x800", "00", &request) == -EINVAL);
    assert(fct_can_parse(true, "500000", "ext", "0x20000000", "00", &request) == -EINVAL);
    assert(fct_can_parse(true, "500000", "std", "1", "0", &request) == -EINVAL);
    assert(fct_can_parse(true, "500000", "std", "1", "gg", &request) == -EINVAL);
    assert(fct_can_parse(true, "500000", "std", "1", "000000000000000000", &request) == -EINVAL);
    assert(fct_can_parse(false, "500000", "std", "1", "5001", &request) == -EINVAL);
    assert(fct_can_parse(false, "500000", "std", "1", "9", &request) == -EINVAL);
    assert(fct_can_parse(true, "-1", "std", "1", "-", &request) == -EINVAL);
    assert(fct_start("can-tx") == -EINVAL && fct_start("can-rx") == -EINVAL);
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_PASS);
    s_can_corrupt = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_corrupt = false;
    s_can_drop_tx = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    assert(s_can_pending == NULL);
    s_can_drop_tx = false;
    s_can_drop_rx = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_drop_rx = false;
    s_cancel_at = s_now + 30U;
    s_can_drop_rx = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_CANCELLED);
    s_can_drop_rx = false;
    s_can_filter_failure = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_filter_failure = false;
    s_can_error_state = HAL_CAN_STATE_BUS_OFF;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_error_state = HAL_CAN_STATE_ERROR_ACTIVE;
    s_can_queue_failure = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_queue_failure = false;
    s_can_stop_failure = true;
    assert(fct_start("can-loopback") == 0 && fct_process_next(0) == 0);
    (void) test_result(5, FCT_FAIL);
    s_can_stop_failure = false;
    assert(fct_can_parse(true, "500000", "ext", "0x18ff1234", "-", &request) == 0);
    assert(fct_start_can(&request) == 0 && fct_start("all") == -EBUSY);
    assert(fct_process_next(0) == 0);
    (void) test_result(6, FCT_PASS);
    s_can_tx_status = -ENETUNREACH;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(6, FCT_FAIL);
    s_can_tx_status = 0;
    assert(fct_can_parse(false, "500000", "std", "0x123", "100", &request) == 0);
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(7, FCT_PASS);
    s_can_drop_rx = true;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(7, FCT_FAIL);
    s_can_drop_rx = false;
    s_can_external_flags = HAL_CAN_EXT_ID;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(7, FCT_FAIL); /* Same numeric ID in wrong format is not a match. */
    request.message.flags = HAL_CAN_EXT_ID;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(7, FCT_PASS);
    s_can_external_flags = 0;
    request.message.flags = 0;
    s_can_drop_rx = true;
    s_cancel_at = s_now + 30U;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(7, FCT_CANCELLED);
    s_can_drop_rx = false;
    assert(fct_can_parse(true, "500000", "std", "0x123", "-", &request) == 0);
    s_can_drop_tx = true;
    assert(fct_start_can(&request) == 0 && fct_process_next(0) == 0);
    (void) test_result(6, FCT_FAIL);
    s_can_drop_tx = false;
    unsigned int sent_before_batch = s_can_sends;
    assert(fct_start("all") == 0 && fct_process_next(0) == 0);
    assert(s_can_sends == sent_before_batch + 2U); /* only two internal frames */
    (void) test_result(6, FCT_SKIP);
    (void) test_result(7, FCT_SKIP);
    assert(!s_can_started && s_can_receive == NULL && s_can_pending == NULL);
}

int main(void)
{
    s_task_result = -ENOMEM;
    assert(fct_init(test_output) == -ENOMEM);
    s_task_result = 0;
    assert(fct_init(test_output) == 0);
    assert(fct_init(test_output) == -EALREADY);
    assert(fct_process_next(0) == -ETIMEDOUT);
    assert(fct_confirm(true) == -EINVAL);
    assert(fct_stop() == -EALREADY);
    assert(fct_start("missing") == -ENOENT);
    assert(fct_start("all") == 0);
    assert(fct_start("led") == -EBUSY);
    assert(fct_i2c_probe(0x50) == -EBUSY);
    assert(fct_process_next(0) == 0);
    (void) test_result(0, FCT_PASS);
    (void) test_result(1, FCT_PASS);
    (void) test_result(2, FCT_SKIP);
    (void) test_result(3, FCT_SKIP);

    s_missing_gpio = true;
    assert(fct_start("all") == 0 && fct_process_next(0) == 0);
    (void) test_result(0, FCT_FAIL);
    (void) test_result(1, FCT_PASS);
    s_missing_gpio = false;
    assert(fct_start("all") == 0 && fct_stop() == 0 && fct_process_next(0) == 0);
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        (void) test_result(idx, FCT_CANCELLED);
    }

    test_led(1, FCT_PASS);
    test_led(-1, FCT_FAIL);
    test_led(0, FCT_FAIL); /* No human confirmation must time out, never pass. */
    s_cancel_at = s_now + 30U;
    test_led(0, FCT_CANCELLED);

    s_animate_keys = true;
    s_current_key = -1;
    assert(fct_start("keys") == 0 && fct_process_next(0) == 0);
    (void) test_result(3, FCT_PASS);
    s_animate_keys = false;
    assert(fct_start("keys") == 0 && fct_process_next(0) == 0);
    assert(test_result(3, FCT_FAIL).elapsed_ms >= 60000U);

    s_now = UINT32_MAX - 100U; /* Relative deadlines must survive tick wrap. */
    test_led(1, FCT_PASS);
    s_cancel_on_start = true;
    assert(fct_start("gpio") == 0 && fct_process_next(0) == 0);
    (void) test_result(0, FCT_CANCELLED);

    assert(fct_i2c_probe(0x07) == -EINVAL && fct_i2c_probe(0x78) == -EINVAL);
    assert(s_probe_count == 0U);
    assert(fct_i2c_probe(0x50) == -EIO && s_probe_count == 1U);
    s_missing_i2c = true;
    assert(fct_i2c_probe(0x50) == -ENODEV && !fct_busy());
    s_network_result = -ETIMEDOUT;
    assert(fct_start("lwip") == 0 && fct_process_next(0) == 0);
    (void) test_result(4, FCT_FAIL);
    s_network_result = 0;
    assert(fct_start("lwip") == 0 && fct_process_next(0) == 0);
    (void) test_result(4, FCT_PASS);
    s_confirmation_mode = 1;
    s_display_rows = 0;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_PASS);
    assert(s_display_rows == 4200U && s_display_blank);
    s_confirmation_mode = -1;
    s_display_rows = 0;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_FAIL);
    assert(s_display_blank);
    s_confirmation_mode = 0;
    s_display_rows = 0;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_FAIL);
    assert(s_display_blank);
    s_display_rows = 0;
    s_display_write_error = -EIO;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_FAIL);
    assert(s_display_blank && s_display_rows == 1U);
    s_display_write_error = 0;
    s_display_rows = 0;
    s_cancel_at = s_now + 30U;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_CANCELLED);
    assert(s_display_blank);
    s_display_rows = 0;
    s_display_error = -ENODEV;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_FAIL);
    assert(s_display_rows == 0U);
    s_display_error = 0;
    s_display_rows = 0;
    s_confirmation_mode = 1;
    s_display_cleanup_error = -EIO;
    assert(fct_start("lcd") == 0 && fct_process_next(0) == 0);
    (void)test_result(8, FCT_FAIL);
    s_display_cleanup_error = 0;
    test_can_cases();
    puts("PASS: dispatch, failure isolation, confirmation, cancel, timeout, debounce, wrap, probe, lwip, CAN");
    return 0;
}
