/** @file tls_stack_stub.c @brief TLS unavailable in offline skeleton builds. */
#include <errno.h>
#include "support/tls_stack.h"
int support_tls_selftest(void)
{
    return -ENOSYS;
}
