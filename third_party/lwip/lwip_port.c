/** @file lwip_port.c @brief OSAL mapping for lwIP 2.2.1 sys_arch. */
#include <limits.h>
#include "lwip/sys.h"
#include "lwip/debug.h"

static sys_thread_t s_core_thread;

static void port_require(int result)
{
    if (result != 0)
    {
        LWIP_PLATFORM_ASSERT("OSAL operation failed");
    }
}

void sys_init(void)
{
}
u32_t sys_now(void)
{
    return osal_get_tick_ms();
}
u32_t sys_jiffies(void)
{
    return osal_get_tick_ms();
}

static uint32_t port_timeout(u32_t timeout_ms)
{
    /* lwIP zero means forever; OSAL zero means no wait. */
    if (timeout_ms == 0U)
    {
        return OSAL_WAIT_FOREVER;
    }
    /* Reserve OSAL's sentinel: even the largest lwIP timeout stays finite. */
    return timeout_ms == OSAL_WAIT_FOREVER ? OSAL_WAIT_FOREVER - 1U : timeout_ms;
}

err_t sys_sem_new(sys_sem_t *sem, u8_t initial)
{
    return osal_sem_create(sem, initial, UINT_MAX) == 0 ? ERR_OK : ERR_MEM;
}
void sys_sem_signal(sys_sem_t *sem)
{
    LWIP_ASSERT("invalid semaphore", sys_sem_valid(sem));
    port_require(osal_sem_post(*sem));
}
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    u32_t start = sys_now();
    int result;
    LWIP_ASSERT("invalid semaphore", sys_sem_valid(sem));
    result = osal_sem_wait(*sem, port_timeout(timeout_ms));
    if (result == OSAL_ERR_TIMEOUT)
    {
        return SYS_ARCH_TIMEOUT;
    }
    LWIP_ASSERT("semaphore wait", result == 0);
    return sys_now() - start;
}
void sys_sem_free(sys_sem_t *sem)
{
    LWIP_ASSERT("invalid semaphore", sys_sem_valid(sem));
    port_require(osal_sem_destroy(*sem));
    sys_sem_set_invalid(sem);
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
    return osal_mutex_create(mutex) == 0 ? ERR_OK : ERR_MEM;
}
void sys_mutex_lock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("invalid mutex", sys_mutex_valid(mutex));
    port_require(osal_mutex_lock(*mutex, OSAL_WAIT_FOREVER));
}
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    LWIP_ASSERT("invalid mutex", sys_mutex_valid(mutex));
    port_require(osal_mutex_unlock(*mutex));
}
void sys_mutex_free(sys_mutex_t *mutex)
{
    LWIP_ASSERT("invalid mutex", sys_mutex_valid(mutex));
    port_require(osal_mutex_destroy(*mutex));
    sys_mutex_set_invalid(mutex);
}

err_t sys_mbox_new(sys_mbox_t *mbox, int capacity)
{
    if (mbox == NULL)
    {
        return ERR_ARG;
    }
    *mbox = NULL;
    if (capacity <= 0)
    {
        return ERR_ARG;
    }
    return osal_mq_create(mbox, sizeof(void *), (size_t) capacity) == 0 ? ERR_OK : ERR_MEM;
}
void sys_mbox_post(sys_mbox_t *mbox, void *message)
{
    LWIP_ASSERT("invalid mailbox", sys_mbox_valid(mbox));
    port_require(osal_mq_send(*mbox, &message, OSAL_WAIT_FOREVER));
}
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *message)
{
    LWIP_ASSERT("invalid mailbox", sys_mbox_valid(mbox));
    return osal_mq_send(*mbox, &message, OSAL_NO_WAIT) == 0 ? ERR_OK : ERR_MEM;
}
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *message)
{
    /* OSAL mailbox API is thread-only. A future RX ISR must defer to a task. */
    (void) mbox;
    (void) message;
    return ERR_IF;
}
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **message, u32_t timeout_ms)
{
    void *discard;
    u32_t start = sys_now();
    int result;
    LWIP_ASSERT("invalid mailbox", sys_mbox_valid(mbox));
    result = osal_mq_recv(*mbox, message != NULL ? message : &discard, port_timeout(timeout_ms));
    if (result == OSAL_ERR_TIMEOUT)
    {
        return SYS_ARCH_TIMEOUT;
    }
    LWIP_ASSERT("mailbox fetch", result == 0);
    return sys_now() - start;
}
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **message)
{
    void *discard;
    int result;
    LWIP_ASSERT("invalid mailbox", sys_mbox_valid(mbox));
    result = osal_mq_recv(*mbox, message != NULL ? message : &discard, OSAL_NO_WAIT);
    if (result == OSAL_ERR_TIMEOUT)
    {
        return SYS_MBOX_EMPTY;
    }
    LWIP_ASSERT("mailbox tryfetch", result == 0);
    return 0;
}
void sys_mbox_free(sys_mbox_t *mbox)
{
    LWIP_ASSERT("invalid mailbox", sys_mbox_valid(mbox));
    port_require(osal_mq_destroy(*mbox));
    sys_mbox_set_invalid(mbox);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn entry, void *arg, int stacksize, int priority)
{
    sys_thread_t task = NULL;
    osal_task_attr_t attr = {.name = name, .entry = entry, .arg = arg, .stack_size = (uint32_t) stacksize, .prio = (uint32_t) priority};
    LWIP_ASSERT("thread attributes", stacksize > 0 && priority >= 0 && priority <= 31);
    port_require(osal_task_start(&task, &attr));
    return task;
}
sys_prot_t sys_arch_protect(void)
{
    return osal_critical_enter();
}
void sys_arch_unprotect(sys_prot_t key)
{
    osal_critical_exit(key);
}

void lwip_port_mark_core(void)
{
    sys_prot_t key = sys_arch_protect();
    s_core_thread = osal_task_current();
    sys_arch_unprotect(key);
}
void lwip_port_check_core(void)
{
    sys_prot_t key = sys_arch_protect();
    sys_thread_t core = s_core_thread;
    sys_arch_unprotect(key);
    /* lwip_init runs before tcpip_thread exists. Raw APIs are core-only later. */
    LWIP_ASSERT("lwIP raw API called outside tcpip thread", core == NULL || core == osal_task_current());
}
