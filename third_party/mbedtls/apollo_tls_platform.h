/** @file apollo_tls_platform.h @brief Pure C allocation hooks for the 3.6 library. */
#ifndef APOLLO_TLS_PLATFORM_H
#define APOLLO_TLS_PLATFORM_H
#include <stddef.h>
void *apollo_tls_calloc(size_t count, size_t size);
void apollo_tls_free(void *ptr);
#endif
