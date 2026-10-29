/** @file tls36_tests.c @brief Real 3.6 library; deterministic entropy is host-test only. */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include "support/tls_stack.h"
int64_t k_uptime_get(void)
{
    return 1234;
}
const struct device test_rng = {0};
static bool s_rng_ready = true;
static bool s_rng_failure;
static bool s_alloc_failure;
static size_t s_allocations;
static uint32_t s_random = 1;
bool device_is_ready(const struct device *dev)
{
    assert(dev == &test_rng);
    return s_rng_ready;
}
int entropy_get_entropy(const struct device *dev, void *buf, uint16_t len)
{
    assert(dev == &test_rng && len <= 32U);
    if (s_rng_failure)
    {
        return -EIO;
    }
    for (uint16_t idx = 0; idx < len; idx++)
    {
        s_random ^= s_random << 13;
        s_random ^= s_random >> 17;
        s_random ^= s_random << 5;
        ((unsigned char *) buf)[idx] = (unsigned char) s_random;
    }
    return 0;
}
void *k_calloc(size_t count, size_t size)
{
    if (s_alloc_failure)
    {
        return NULL;
    }
    void *ptr = calloc(count, size);
    if (ptr != NULL)
    {
        s_allocations++;
    }
    return ptr;
}
void k_free(void *ptr)
{
    if (ptr != NULL)
    {
        assert(s_allocations > 0);
        s_allocations--;
    }
    free(ptr);
}
int main(void)
{
    for (unsigned int idx = 0; idx < 4; idx++)
    {
        assert(support_tls_selftest() == 0);
        mbedtls_psa_crypto_free();
        assert(s_allocations == 0);
    }
    s_rng_ready = false;
    assert(support_tls_selftest() == -EIO);
    s_rng_ready = true;
    s_rng_failure = true;
    assert(support_tls_selftest() == -EIO);
    s_rng_failure = false;
    s_alloc_failure = true;
    assert(support_tls_selftest() == -EIO);
    s_alloc_failure = false;
    mbedtls_psa_crypto_free();
    assert(s_allocations == 0);
    assert(support_tls_selftest() == 0);
    mbedtls_psa_crypto_free();
    assert(s_allocations == 0);
    puts("PASS: Mbed TLS 3.6.6 SHA256, RNG port, client setup, cleanup and failures");
    return 0;
}
