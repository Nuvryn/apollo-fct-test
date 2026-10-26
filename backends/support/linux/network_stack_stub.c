/** @file network_stack_stub.c @brief No live stack in offline skeletons. */
#include <errno.h>
#include "support/network_stack.h"
int support_network_init(void)
{
    return -ENOSYS;
}
int support_network_selftest(uint32_t timeout_ms)
{
    (void) timeout_ms;
    return -ENOSYS;
}
