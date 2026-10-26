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
#define OSAL_WORKER_COUNT 4U
K_THREAD_STACK_ARRAY_DEFINE(s_worker_stacks, OSAL_WORKER_COUNT, OSAL_WORKER_STACK_SIZE);
typedef struct
{
    struct k_thread native;
    atomic_t claimed;
    osal_task_attr_t attr;
} osal_zephyr_task_t;
static osal_zephyr_task_t s_workers[OSAL_WORKER_COUNT];
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "OSAL IRQ protection requires a single-core target");

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
    osal_zephyr_task_t *worker = arg;
    (void) unused1;
    (void) unused2;
    worker->attr.entry(worker->attr.arg);
}

int osal_task_create(const osal_task_attr_t *attr)
{
    osal_task_t task;
    return osal_task_start(&task, attr);
}

int osal_task_start(osal_task_t *task, const osal_task_attr_t *attr)
{
    if (task == NULL)
    {
        return -EINVAL;
    }
    *task = NULL;
    if (attr == NULL || attr->entry == NULL || attr->stack_size == 0U ||
        attr->stack_size > OSAL_WORKER_STACK_SIZE || attr->prio > 31U)
    {
        return -EINVAL;
    }
    for (size_t idx = 0; idx < OSAL_WORKER_COUNT; idx++)
    {
        osal_zephyr_task_t *worker = &s_workers[idx];
        if (!atomic_cas(&worker->claimed, 0, 1))
        {
            continue;
        }
        worker->attr = *attr;
        k_thread_create(&worker->native, s_worker_stacks[idx],
            K_THREAD_STACK_SIZEOF(s_worker_stacks[idx]), osal_worker_entry,
            worker, NULL, NULL, 31 - (int) attr->prio, 0, K_FOREVER);
        if (attr->name != NULL)
        {
            (void) k_thread_name_set(&worker->native, attr->name);
        }
        *task = &worker->native;
        k_thread_start(&worker->native);
        return 0;
    }
    return -ENOMEM;
}

osal_task_t osal_task_current(void)
{
    return k_current_get();
}
uintptr_t osal_critical_enter(void)
{
    return irq_lock();
}
void osal_critical_exit(uintptr_t key)
{
    irq_unlock((unsigned int) key);
}

void osal_task_delete(osal_task_t task)
{
    for (size_t idx = 0; idx < OSAL_WORKER_COUNT; idx++)
    {
        if (task == &s_workers[idx].native && atomic_get(&s_workers[idx].claimed) != 0)
        {
            k_thread_abort(&s_workers[idx].native);
            return;
        }
    }
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
        int result = k_msgq_cleanup(&native->native);
        if (result != 0)
        {
            return result;
        }
        k_free(native->buffer);
        k_free(native);
    }
    return 0;
}

int osal_sem_create(osal_sem_t *sem, uint32_t initial, uint32_t maximum)
{
    struct k_sem *native;
    if (sem == NULL)
    {
        return -EINVAL;
    }
    *sem = NULL;
    if (maximum == 0U || initial > maximum)
    {
        return -EINVAL;
    }
    native = k_malloc(sizeof(*native));
    if (native == NULL)
    {
        return -ENOMEM;
    }
    (void) k_sem_init(native, initial, maximum);
    *sem = native;
    return 0;
}
int osal_sem_wait(osal_sem_t sem, uint32_t timeout_ms)
{
    return sem == NULL ? -EINVAL : osal_wait_result(k_sem_take(sem, osal_timeout(timeout_ms)));
}
int osal_sem_post(osal_sem_t sem)
{
    if (sem == NULL)
    {
        return -EINVAL;
    }
    k_sem_give(sem);
    return 0;
}
int osal_sem_destroy(osal_sem_t sem)
{
    /* Contract requires callers to join/quiesce every waiter before destroy. */
    k_free(sem);
    return 0;
}
