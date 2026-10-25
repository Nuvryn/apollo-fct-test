/** @file fct_tests.c @brief Deterministic tests with fake time and hardware. */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "fct/fct_internal.h"
#include "osal/osal.h"
#include "hal/hal_dev.h"
#include "hal/hal_i2c.h"
#include "support/test_fixture.h"

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
    if (strstr(text, "Observe LEDs:") != NULL && s_confirmation_mode != 0)
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
    puts("PASS: dispatch, failure isolation, confirmation, cancel, timeout, debounce, wrap and probe");
    return 0;
}
