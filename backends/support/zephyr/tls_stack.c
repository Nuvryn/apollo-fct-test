/** @file tls_stack.c @brief Selftest of Zephyr-native Mbed TLS and PSA integration. */
#include <errno.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <mbedtls/build_info.h>
#include <mbedtls/version.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>
#include "support/tls_stack.h"

LOG_MODULE_REGISTER(apollo_tls, LOG_LEVEL_INF);
_Static_assert(MBEDTLS_VERSION_MAJOR == 4, "Expected Zephyr-native Mbed TLS 4.x");
/* Single FCT worker owns these contexts; Zephyr owns PSA, entropy and heap. */
static mbedtls_ssl_config s_config;
static mbedtls_ssl_context s_ssl;

int support_tls_selftest(void)
{
    static const unsigned char expected[32] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    unsigned char digest[32];
    unsigned char random[32];
    size_t length = 0;
    if (mbedtls_version_get_number() != MBEDTLS_VERSION_NUMBER)
    {
        return -EPROTO;
    }
    if (psa_crypto_init() != PSA_SUCCESS)
    {
        return -EIO;
    }
    psa_status_t status = psa_hash_compute(PSA_ALG_SHA_256, (const unsigned char *) "abc", 3,
        digest, sizeof(digest), &length);
    if (status != PSA_SUCCESS || length != sizeof(expected) || memcmp(digest, expected, sizeof(expected)) != 0)
    {
        return -EIO;
    }
    if (psa_generate_random(random, sizeof(random)) != PSA_SUCCESS)
    {
        return -EIO;
    }
    mbedtls_ssl_config_init(&s_config);
    mbedtls_ssl_init(&s_ssl);
    int result = mbedtls_ssl_config_defaults(&s_config, MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (result == 0)
    {
        mbedtls_ssl_conf_authmode(&s_config, MBEDTLS_SSL_VERIFY_REQUIRED);
        result = mbedtls_ssl_setup(&s_ssl, &s_config);
    }
    mbedtls_ssl_free(&s_ssl);
    mbedtls_ssl_config_free(&s_config);
    if (result != 0)
    {
        LOG_ERR("Native Mbed TLS client setup failed: %d", result);
        return -EIO;
    }
    LOG_INF("Native Mbed TLS %s: SHA256/PSA RNG/client setup ready; no transport", MBEDTLS_VERSION_STRING);
    return 0;
}
