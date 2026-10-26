/** @file host_osal.c @brief pthread test fixture, not a Linux production port. */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "osal/osal.h"

static atomic_uint s_allocations;
static atomic_bool s_fail_allocation;
static pthread_once_t s_protect_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t s_protect;
static _Thread_local uintptr_t s_depth;
static _Thread_local int s_thread_identity;
static _Thread_local void *s_current;

unsigned int host_osal_allocations(void)
{
    return atomic_load(&s_allocations);
}
void host_osal_fail_next_allocation(void)
{
    atomic_store(&s_fail_allocation, true);
}
void *osal_malloc(size_t size)
{
    if (atomic_exchange(&s_fail_allocation, false))
    {
        return NULL;
    }
    void *result = calloc(1, size);
    if (result != NULL)
    {
        (void) atomic_fetch_add(&s_allocations, 1);
    }
    return result;
}
void osal_free(void *memory)
{
    if (memory != NULL)
    {
        (void) atomic_fetch_sub(&s_allocations, 1);
        free(memory);
    }
}
static void host_protect_init(void)
{
    pthread_mutexattr_t attr;
    assert(pthread_mutexattr_init(&attr) == 0);
    assert(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0);
    assert(pthread_mutex_init(&s_protect, &attr) == 0);
    assert(pthread_mutexattr_destroy(&attr) == 0);
}
uintptr_t osal_critical_enter(void)
{
    assert(pthread_once(&s_protect_once, host_protect_init) == 0);
    assert(pthread_mutex_lock(&s_protect) == 0);
    return s_depth++;
}
void osal_critical_exit(uintptr_t key)
{
    assert(s_depth == key + 1U);
    s_depth = key;
    assert(pthread_mutex_unlock(&s_protect) == 0);
}
osal_task_t osal_task_current(void)
{
    return s_current != NULL ? s_current : &s_thread_identity;
}
static void *host_task(void *argument)
{
    osal_task_attr_t *attr = argument;
    s_current = argument;
    attr->entry(attr->arg);
    osal_free(argument);
    return NULL;
}
int osal_task_start(osal_task_t *task, const osal_task_attr_t *attr)
{
    pthread_t thread;
    if (task == NULL || attr == NULL || attr->entry == NULL)
    {
        return -EINVAL;
    }
    *task = NULL;
    osal_task_attr_t *copy = osal_malloc(sizeof(*copy));
    if (copy == NULL)
    {
        return -ENOMEM;
    }
    *copy = *attr;
    *task = copy;
    int result = pthread_create(&thread, NULL, host_task, copy);
    if (result != 0)
    {
        *task = NULL;
        osal_free(copy);
        return -result;
    }
    assert(pthread_detach(thread) == 0);
    return 0;
}
int osal_task_create(const osal_task_attr_t *attr)
{
    osal_task_t task;
    return osal_task_start(&task, attr);
}
uint32_t osal_get_tick_ms(void)
{
    struct timespec now;
    assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
    return (uint32_t) ((uint64_t) now.tv_sec * 1000U + (uint64_t) now.tv_nsec / 1000000U);
}
void osal_delay_ms(uint32_t ms)
{
    struct timespec delay = {.tv_sec = ms / 1000U, .tv_nsec = (long) (ms % 1000U) * 1000000L};
    while (nanosleep(&delay, &delay) != 0)
    {
        assert(errno == EINTR);
    }
}
static struct timespec host_deadline(uint32_t ms)
{
    struct timespec deadline;
    assert(timespec_get(&deadline, TIME_UTC) == TIME_UTC);
    deadline.tv_sec += ms / 1000U;
    deadline.tv_nsec += (long) (ms % 1000U) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    return deadline;
}
static int host_wait(pthread_cond_t *condition, pthread_mutex_t *lock,
    uint32_t ms, const struct timespec *deadline)
{
    if (ms == OSAL_NO_WAIT)
    {
        return ETIMEDOUT;
    }
    return ms == OSAL_WAIT_FOREVER ? pthread_cond_wait(condition, lock) : pthread_cond_timedwait(condition, lock, deadline);
}

int osal_mutex_create(osal_mutex_t *mutex)
{
    *mutex = osal_malloc(sizeof(pthread_mutex_t));
    if (*mutex == NULL)
    {
        return -ENOMEM;
    }
    assert(pthread_mutex_init(*mutex, NULL) == 0);
    return 0;
}
int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    (void) timeout_ms;
    return -pthread_mutex_lock(mutex);
}
int osal_mutex_unlock(osal_mutex_t mutex)
{
    return -pthread_mutex_unlock(mutex);
}
int osal_mutex_destroy(osal_mutex_t mutex)
{
    if (mutex != NULL)
    {
        assert(pthread_mutex_destroy(mutex) == 0);
        osal_free(mutex);
    }
    return 0;
}

typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t condition;
    uint32_t count;
    uint32_t maximum;
} host_sem_t;
int osal_sem_create(osal_sem_t *output, uint32_t initial, uint32_t maximum)
{
    *output = NULL;
    if (maximum == 0U || initial > maximum)
    {
        return -EINVAL;
    }
    host_sem_t *sem = osal_malloc(sizeof(*sem));
    if (sem == NULL)
    {
        return -ENOMEM;
    }
    assert(pthread_mutex_init(&sem->lock, NULL) == 0);
    assert(pthread_cond_init(&sem->condition, NULL) == 0);
    sem->count = initial;
    sem->maximum = maximum;
    *output = sem;
    return 0;
}
int osal_sem_wait(osal_sem_t input, uint32_t timeout_ms)
{
    host_sem_t *sem = input;
    struct timespec deadline = host_deadline(timeout_ms);
    int result = 0;
    assert(pthread_mutex_lock(&sem->lock) == 0);
    while (sem->count == 0U && result == 0)
    {
        result = host_wait(&sem->condition, &sem->lock, timeout_ms, &deadline);
    }
    if (result == 0)
    {
        sem->count--;
    }
    assert(pthread_mutex_unlock(&sem->lock) == 0);
    return -result;
}
int osal_sem_post(osal_sem_t input)
{
    host_sem_t *sem = input;
    assert(pthread_mutex_lock(&sem->lock) == 0);
    if (sem->count < sem->maximum)
    {
        sem->count++;
    }
    assert(pthread_cond_signal(&sem->condition) == 0);
    assert(pthread_mutex_unlock(&sem->lock) == 0);
    return 0;
}
int osal_sem_destroy(osal_sem_t input)
{
    host_sem_t *sem = input;
    if (sem != NULL)
    {
        assert(pthread_mutex_destroy(&sem->lock) == 0);
        assert(pthread_cond_destroy(&sem->condition) == 0);
        osal_free(sem);
    }
    return 0;
}

typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t readable;
    pthread_cond_t writable;
    size_t size, capacity, used, head, tail;
    unsigned char data[];
} host_queue_t;
int osal_mq_create(osal_mq_t *output, size_t size, size_t count)
{
    *output = NULL;
    if (size == 0U || count == 0U)
    {
        return -EINVAL;
    }
    host_queue_t *queue = osal_malloc(sizeof(*queue) + size * count);
    if (queue == NULL)
    {
        return -ENOMEM;
    }
    assert(pthread_mutex_init(&queue->lock, NULL) == 0);
    assert(pthread_cond_init(&queue->readable, NULL) == 0);
    assert(pthread_cond_init(&queue->writable, NULL) == 0);
    queue->size = size;
    queue->capacity = count;
    *output = queue;
    return 0;
}
int osal_mq_send(osal_mq_t input, const void *message, uint32_t timeout_ms)
{
    host_queue_t *queue = input;
    struct timespec deadline = host_deadline(timeout_ms);
    int result = 0;
    assert(pthread_mutex_lock(&queue->lock) == 0);
    while (queue->used == queue->capacity && result == 0)
    {
        result = host_wait(&queue->writable, &queue->lock, timeout_ms, &deadline);
    }
    if (result == 0)
    {
        memcpy(queue->data + queue->tail * queue->size, message, queue->size);
        queue->tail = (queue->tail + 1U) % queue->capacity;
        queue->used++;
        assert(pthread_cond_signal(&queue->readable) == 0);
    }
    assert(pthread_mutex_unlock(&queue->lock) == 0);
    return -result;
}
int osal_mq_recv(osal_mq_t input, void *message, uint32_t timeout_ms)
{
    host_queue_t *queue = input;
    struct timespec deadline = host_deadline(timeout_ms);
    int result = 0;
    assert(pthread_mutex_lock(&queue->lock) == 0);
    while (queue->used == 0U && result == 0)
    {
        result = host_wait(&queue->readable, &queue->lock, timeout_ms, &deadline);
    }
    if (result == 0)
    {
        memcpy(message, queue->data + queue->head * queue->size, queue->size);
        queue->head = (queue->head + 1U) % queue->capacity;
        queue->used--;
        assert(pthread_cond_signal(&queue->writable) == 0);
    }
    assert(pthread_mutex_unlock(&queue->lock) == 0);
    return -result;
}
int osal_mq_destroy(osal_mq_t input)
{
    host_queue_t *queue = input;
    assert(pthread_mutex_destroy(&queue->lock) == 0);
    assert(pthread_cond_destroy(&queue->readable) == 0);
    assert(pthread_cond_destroy(&queue->writable) == 0);
    osal_free(queue);
    return 0;
}
