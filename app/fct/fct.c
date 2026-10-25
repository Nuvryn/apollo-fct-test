/** @file fct.c @brief Single-worker test scheduling, cancellation and results. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fct/fct_internal.h"
#include "osal/osal.h"
#include "hal/hal_dev.h"
#include "hal/hal_i2c.h"
#include "support/test_fixture.h"

typedef struct
{
    fct_case_info_t info;
    int (*run)(fct_context_t *context);
} fct_case_t;

static const fct_case_t s_cases[] = {
    {{"gpio", "GPIO controller readiness only; no electrical test", false, 1000}, fct_run_gpio},
    {{"i2c", "I2C2 controller readiness only; no device function test", false, 1000}, fct_run_i2c},
    {{"led", "Observe DS0/DS1 blinking, then fct confirm pass|fail", true, 30000}, fct_run_led},
    {{"keys", "Release all keys, then press/release WK_UP KEY2 KEY1 KEY0", true, 60000}, fct_run_keys},
};

static osal_mutex_t s_lock;
static osal_mq_t s_requests;
static bool s_initialized;
static bool s_busy;
static bool s_cancel;
static bool s_waiting;
static int s_confirmation;
static fct_result_t s_results[sizeof(s_cases) / sizeof(s_cases[0])];
static void (*s_output)(const char *text);

static void fct_lock(void)
{
    (void) osal_mutex_lock(s_lock, OSAL_WAIT_FOREVER);
}
static void fct_unlock(void)
{
    (void) osal_mutex_unlock(s_lock);
}

size_t fct_count(void)
{
    return sizeof(s_cases) / sizeof(s_cases[0]);
}

const fct_case_info_t *fct_info(size_t index)
{
    return index < fct_count() ? &s_cases[index].info : NULL;
}

const char *fct_state_name(fct_state_t state)
{
    static const char *const names[] = {"NOT_RUN", "RUNNING", "PASS", "FAIL", "SKIP", "CANCELLED"};
    return (unsigned int) state < sizeof(names) / sizeof(names[0]) ? names[state] : "INVALID";
}

void fct_output(const char *format, ...)
{
    char text[192];
    va_list args;
    va_start(args, format);
    (void) vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (s_output != NULL)
    {
        s_output(text);
    }
}

int fct_result(size_t index, fct_result_t *out)
{
    if (!s_initialized || index >= fct_count() || out == NULL)
    {
        return -EINVAL;
    }
    fct_lock();
    *out = s_results[index];
    fct_unlock();
    return 0;
}

bool fct_busy(void)
{
    bool busy;
    if (!s_initialized)
    {
        return false;
    }
    fct_lock();
    busy = s_busy;
    fct_unlock();
    return busy;
}

int fct_checkpoint(const fct_context_t *context)
{
    bool cancel;
    fct_lock();
    cancel = s_cancel;
    fct_unlock();
    if (cancel)
    {
        return -ECANCELED;
    }
    return (uint32_t) (osal_get_tick_ms() - context->started_ms) >= context->timeout_ms ? -ETIMEDOUT : 0;
}

int fct_sleep(const fct_context_t *context, uint32_t delay_ms)
{
    uint32_t start_ms = osal_get_tick_ms();
    int result;
    do
    {
        result = fct_checkpoint(context);
        if (result != 0)
        {
            return result;
        }
        if ((uint32_t) (osal_get_tick_ms() - start_ms) >= delay_ms)
        {
            return 0;
        }
        osal_delay_ms(10);
    } while (true);
}

int fct_wait_confirmation(const fct_context_t *context)
{
    int result;
    int confirmation;
    fct_lock();
    s_waiting = true;
    s_confirmation = 0;
    fct_unlock();
    fct_output("Observe LEDs: fct confirm pass|fail (or fct stop)");
    do
    {
        result = fct_checkpoint(context);
        fct_lock();
        confirmation = s_confirmation;
        fct_unlock();
        if (result != 0 || confirmation != 0)
        {
            break;
        }
        osal_delay_ms(10);
    } while (true);
    fct_lock();
    s_waiting = false;
    fct_unlock();
    return result != 0 ? result : (confirmation > 0 ? 0 : -EIO);
}

int fct_confirm(bool passed)
{
    int result = 0;
    if (!s_initialized)
    {
        return -EAGAIN;
    }
    fct_lock();
    if (!s_waiting || s_confirmation != 0 || s_cancel)
    {
        result = -EINVAL;
    }
    else
    {
        s_confirmation = passed ? 1 : -1;
    }
    fct_unlock();
    return result;
}

int fct_stop(void)
{
    int result = 0;
    if (!s_initialized)
    {
        return -EAGAIN;
    }
    fct_lock();
    if (!s_busy)
    {
        result = -EALREADY;
    }
    else
    {
        s_cancel = true;
    }
    fct_unlock();
    return result;
}

int fct_start(const char *id)
{
    int selected = -1;
    int result;
    if (!s_initialized || id == NULL)
    {
        return -EINVAL;
    }
    if (strcmp(id, "all") != 0)
    {
        for (size_t idx = 0; idx < fct_count(); idx++)
        {
            if (strcmp(id, s_cases[idx].info.id) == 0)
            {
                selected = (int) idx;
                break;
            }
        }
        if (selected < 0)
        {
            return -ENOENT;
        }
    }
    fct_lock();
    if (s_busy)
    {
        fct_unlock();
        return -EBUSY;
    }
    s_busy = true;
    s_cancel = false;
    s_waiting = false;
    /* Reset only the selected session's results before publishing it. */
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        if (selected < 0 || selected == (int) idx)
        {
            s_results[idx] = (fct_result_t){.state = FCT_NOT_RUN};
        }
    }
    result = osal_mq_send(s_requests, &selected, OSAL_NO_WAIT);
    if (result != 0)
    {
        s_busy = false;
    }
    fct_unlock();
    return result;
}

static void fct_execute(size_t index, bool batch)
{
    fct_context_t context = {.started_ms = osal_get_tick_ms(),
        .timeout_ms = s_cases[index].info.timeout_ms};
    fct_result_t outcome = {.state = FCT_RUNNING};
    int result;
    fct_lock();
    s_results[index] = outcome;
    fct_unlock();
    result = fct_checkpoint(&context);
    if (result == 0 && batch && s_cases[index].info.interactive)
    {
        outcome.state = FCT_SKIP;
        (void) snprintf(outcome.reason, sizeof(outcome.reason), "interactive; run individually");
    }
    else
    {
        if (result == 0)
        {
            fct_output("START %s: %s", s_cases[index].info.id, s_cases[index].info.description);
            result = s_cases[index].run(&context);
            if (result == 0)
            {
                result = fct_checkpoint(&context);
            }
        }
        outcome.state = result == 0 ? FCT_PASS : (result == -ECANCELED ? FCT_CANCELLED : FCT_FAIL);
        if (result != 0)
        {
            (void) snprintf(outcome.reason, sizeof(outcome.reason), "%s (error %d)",
                result == -ECANCELED ? "cancelled" : (result == -ETIMEDOUT ? "timeout" : "test failed"), result);
        }
        else
        {
            (void) snprintf(outcome.reason, sizeof(outcome.reason), "%s", context.reason);
        }
    }
    outcome.elapsed_ms = osal_get_tick_ms() - context.started_ms;
    fct_lock();
    s_results[index] = outcome;
    fct_unlock();
    fct_output("%s %s: %s", s_cases[index].info.id, fct_state_name(outcome.state), outcome.reason);
}

int fct_process_next(uint32_t timeout_ms)
{
    int selected;
    int result = osal_mq_recv(s_requests, &selected, timeout_ms);
    if (result != 0)
    {
        return result;
    }
    for (size_t idx = 0; idx < fct_count(); idx++)
    {
        if (selected < 0 || selected == (int) idx)
        {
            fct_execute(idx, selected < 0);
        }
    }
    fct_lock();
    s_busy = false;
    s_waiting = false;
    fct_unlock();
    return 0;
}

static void fct_worker(void *arg)
{
    (void) arg;
    while (true)
    {
        (void) fct_process_next(OSAL_WAIT_FOREVER);
    }
}

int fct_init(void (*output)(const char *text))
{
    const osal_task_attr_t task = {.name = "fct", .entry = fct_worker, .stack_size = 4096, .prio = 16};
    int result;
    if (s_initialized)
    {
        return -EALREADY;
    }
    result = osal_mutex_create(&s_lock);
    if (result != 0)
    {
        return result;
    }
    result = osal_mq_create(&s_requests, sizeof(int), 1);
    if (result != 0)
    {
        (void) osal_mutex_destroy(s_lock);
        return result;
    }
    s_output = output;
    result = osal_task_create(&task);
    if (result != 0)
    {
        (void) osal_mq_destroy(s_requests);
        (void) osal_mutex_destroy(s_lock);
        return result;
    }
    s_initialized = true;
    return 0;
}

int fct_i2c_probe(uint16_t address)
{
    int result;
    hal_i2c_bus_t bus;
    hal_i2c_msg_t message = {.addr = address, .flags = HAL_I2C_WR};
    if (!s_initialized || address < 0x08U || address > 0x77U)
    {
        return -EINVAL;
    }
    /* Serialize manual diagnostics with complete test sessions. */
    fct_lock();
    if (s_busy)
    {
        fct_unlock();
        return -EBUSY;
    }
    s_busy = true;
    fct_unlock();
    bus = hal_i2c_get_bus(support_test_i2c_name());
    result = bus == NULL ? -ENODEV : hal_i2c_transfer(bus, &message, 1);
    fct_lock();
    s_busy = false;
    fct_unlock();
    return result;
}
