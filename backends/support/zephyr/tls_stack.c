/** @file tls_stack.c @brief Zephyr entropy/allocator port and Mbed TLS 3.6 bring-up. */
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log.h>
#include <mbedtls/build_info.h>
#include <mbedtls/version.h>
#include <mbedtls/platform_time.h>
#include <mbedtls/sha256.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>
#include "apollo_tls_platform.h"
#include "support/tls_stack.h"

LOG_MODULE_REGISTER(apollo_tls, LOG_LEVEL_INF);
_Static_assert(MBEDTLS_VERSION_NUMBER == 0x03060600, "Only Mbed TLS 3.6.6 may be linked here");
/* Worker-owned contexts avoid exhausting the 4KiB FCT task stack. */
static mbedtls_entropy_context s_entropy;
static mbedtls_ctr_drbg_context s_drbg;
static mbedtls_ssl_config s_config;
static mbedtls_ssl_context s_ssl;

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    return k_uptime_get();
}

void *apollo_tls_calloc(size_t count, size_t size)
{
    return k_calloc(count, size);
}
void apollo_tls_free(void *ptr)
{
    k_free(ptr);
}
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    const struct device *rng = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
    (void) data;
    *olen = 0;
    if (!device_is_ready(rng))
    {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
    while (*olen < len)
    {
        size_t chunk = len - *olen;
        if (chunk > 32U)
        {
            chunk = 32U;
        }
        if (entropy_get_entropy(rng, output + *olen, (uint16_t) chunk) != 0)
        {
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        }
        *olen += chunk;
    }
    return 0;
}
int support_tls_selftest(void)
{
    static const unsigned char expected[32] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    unsigned char digest[32];
    unsigned char random[32];
    static const unsigned char personalization[] = "apollo-fct-tls36";
    if (mbedtls_version_get_number() != MBEDTLS_VERSION_NUMBER)
    {
        return -EPROTO;
    }
    int result = mbedtls_sha256((const unsigned char *) "abc", 3, digest, 0);
    if (result != 0 || memcmp(digest, expected, sizeof(digest)) != 0)
    {
        return -EIO;
    }
    mbedtls_entropy_init(&s_entropy);
    mbedtls_ctr_drbg_init(&s_drbg);
    mbedtls_ssl_config_init(&s_config);
    mbedtls_ssl_init(&s_ssl);
    result = mbedtls_ctr_drbg_seed(&s_drbg, mbedtls_entropy_func, &s_entropy,
        personalization, sizeof(personalization) - 1U);
    if (result == 0)
    {
        result = mbedtls_ctr_drbg_random(&s_drbg, random, sizeof(random));
    }
    if (result == 0 && psa_crypto_init() != PSA_SUCCESS)
    {
        result = -EIO;
    }
    if (result == 0)
    {
        result = mbedtls_ssl_config_defaults(&s_config, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    }
    if (result == 0)
    {
        mbedtls_ssl_conf_rng(&s_config, mbedtls_ctr_drbg_random, &s_drbg);
        mbedtls_ssl_conf_authmode(&s_config, MBEDTLS_SSL_VERIFY_REQUIRED);
        result = mbedtls_ssl_setup(&s_ssl, &s_config);
    }
    mbedtls_ssl_free(&s_ssl);
    mbedtls_ssl_config_free(&s_config);
    mbedtls_ctr_drbg_free(&s_drbg);
    mbedtls_entropy_free(&s_entropy);
    if (result != 0)
    {
        LOG_ERR("Mbed TLS 3.6 selftest failed: %d", result);
        return -EIO;
    }
    LOG_INF("Mbed TLS %s: SHA256/RNG/client setup ready; no TLS transport", MBEDTLS_VERSION_STRING);
    return 0;
}
