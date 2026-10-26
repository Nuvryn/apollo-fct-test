/** @file kernel.h @brief Private host-test shim; never used by firmware. */
#ifndef TEST_CAN_KERNEL_H
#define TEST_CAN_KERNEL_H
#include <stdint.h>
struct k_mutex
{
    int locked;
};
#define K_MUTEX_DEFINE(name) struct k_mutex name
#define K_FOREVER (-1LL)
#define K_MSEC(value) ((int64_t) (value))
#define IS_ENABLED(config) 0
#define BUILD_ASSERT(test, message) _Static_assert(test, message)
int k_mutex_lock(struct k_mutex *mutex, int64_t timeout);
int k_mutex_unlock(struct k_mutex *mutex);
#endif
