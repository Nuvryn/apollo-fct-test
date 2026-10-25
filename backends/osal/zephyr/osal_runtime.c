/** @file osal_runtime.c
 * Real Zephyr backend used only by the firmware target.
 * The separate osal_*.c skeletons remain selected for offline checks.
 */
#include <errno.h>
#include <limits.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include "osal/osal.h"

#define OSAL_WORKER_STACK_SIZE 4096U
K_THREAD_STACK_DEFINE(s_worker_stack, OSAL_WORKER_STACK_SIZE);
static struct k_thread s_worker;
static atomic_t s_worker_claimed;
static osal_task_attr_t s_worker_attr;

static k_timeout_t osal_timeout(uint32_t timeout_ms)
{
    return timeout_ms == OSAL_WAIT_FOREVER ? K_FOREVER : K_MSEC(timeout_ms);
}

static int osal_wait_result(int result)
{
    return (result == -EAGAIN || result == -EBUSY) ? -ETIMEDOUT : result;
}

static void osal_worker_entry(void *arg, void *unused1, void *unused2)
{
    (void) arg;
    (void) unused1;
    (void) unused2;
    s_worker_attr.entry(s_worker_attr.arg);
}

int osal_task_create(const osal_task_attr_t *attr)
{
    if (attr == NULL || attr->entry == NULL || attr->stack_size == 0U ||
        attr->stack_size > OSAL_WORKER_STACK_SIZE || attr->prio > 31U)
    {
        return -EINVAL;
    }
    /* One permanent worker; its slot is deliberately not recycled. */
    if (!atomic_cas(&s_worker_claimed, 0, 1))
    {
        return -ENOMEM;
    }
    s_worker_attr = *attr;
    k_thread_create(&s_worker, s_worker_stack, K_THREAD_STACK_SIZEOF(s_worker_stack),
        osal_worker_entry, NULL, NULL, NULL,
        31 - (int) attr->prio, 0, K_FOREVER);
    if (attr->name != NULL)
    {
        (void) k_thread_name_set(&s_worker, attr->name);
    }
    k_thread_start(&s_worker);
    return 0;
}

void osal_task_delete(osal_task_t task)
{
    /* v1 create has no output handle; the permanent worker is not deletable. */
    (void) task;
}

void osal_task_yield(void)
{
    k_yield();
}
void osal_delay_ms(uint32_t ms)
{
    if (ms == 0U)
    {
        k_yield();
    }
    else
    {
        (void) k_msleep(ms);
    }
}
void osal_delay_us(uint32_t us)
{
    k_busy_wait(us);
}
uint32_t osal_get_tick_ms(void)
{
    return k_uptime_get_32();
}
void *osal_malloc(size_t size)
{
    return size == 0U ? NULL : k_malloc(size);
}
void osal_free(void *ptr)
{
    k_free(ptr);
}

int osal_mutex_create(osal_mutex_t *mutex)
{
    struct k_mutex *native;
    if (mutex == NULL)
    {
        return -EINVAL;
    }
    *mutex = NULL;
    native = k_malloc(sizeof(*native));
    if (native == NULL)
    {
        return -ENOMEM;
    }
    (void) k_mutex_init(native);
    *mutex = native;
    return 0;
}

int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    struct k_mutex *native = mutex;
    int result;
    if (native == NULL)
    {
        return -EINVAL;
    }
    /* Zephyr mutexes are recursive; the public OSAL contract is not. */
    result = k_mutex_lock(native, osal_timeout(timeout_ms));
    if (result == 0 && native->lock_count > 1U)
    {
        (void) k_mutex_unlock(native);
        return -EDEADLK;
    }
    if (timeout_ms == OSAL_NO_WAIT && result == -EBUSY)
    {
        return -EBUSY;
    }
    return osal_wait_result(result);
}

int osal_mutex_unlock(osal_mutex_t mutex)
{
    return mutex == NULL ? -EINVAL : k_mutex_unlock(mutex);
}

int osal_mutex_destroy(osal_mutex_t mutex)
{
    struct k_mutex *native = mutex;
    if (native != NULL && native->owner != NULL)
    {
        return -EBUSY;
    }
    k_free(native);
    return 0;
}

typedef struct
{
    struct k_msgq native;
    char *buffer;
} osal_zephyr_queue_t;

int osal_mq_create(osal_mq_t *queue, size_t msg_size, size_t max_msgs)
{
    osal_zephyr_queue_t *native;
    if (queue == NULL)
    {
        return -EINVAL;
    }
    *queue = NULL;
    if (msg_size == 0U || max_msgs == 0U || max_msgs > UINT_MAX ||
        msg_size > SIZE_MAX / max_msgs)
    {
        return -EINVAL;
    }
    native = k_malloc(sizeof(*native));
    if (native == NULL)
    {
        return -ENOMEM;
    }
    native->buffer = k_malloc(msg_size * max_msgs);
    if (native->buffer == NULL)
    {
        k_free(native);
        return -ENOMEM;
    }
    k_msgq_init(&native->native, native->buffer, msg_size, (uint32_t) max_msgs);
    *queue = native;
    return 0;
}

int osal_mq_send(osal_mq_t queue, const void *message, uint32_t timeout_ms)
{
    osal_zephyr_queue_t *native = queue;
    if (native == NULL || message == NULL)
    {
        return -EINVAL;
    }
    return osal_wait_result(k_msgq_put(&native->native, message, osal_timeout(timeout_ms)));
}

int osal_mq_recv(osal_mq_t queue, void *message, uint32_t timeout_ms)
{
    osal_zephyr_queue_t *native = queue;
    if (native == NULL || message == NULL)
    {
        return -EINVAL;
    }
    return osal_wait_result(k_msgq_get(&native->native, message, osal_timeout(timeout_ms)));
}

int osal_mq_destroy(osal_mq_t queue)
{
    osal_zephyr_queue_t *native = queue;
    if (native != NULL)
    {
        (void) k_msgq_cleanup(&native->native);
        k_free(native->buffer);
        k_free(native);
    }
    return 0;
}
