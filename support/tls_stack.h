/** @file tls_stack.h @brief Standalone TLS library bring-up; no transport is connected. */
#ifndef SUPPORT_TLS_STACK_H
#define SUPPORT_TLS_STACK_H
/* Single FCT worker only. Checks version, SHA256, entropy/DRBG and client setup.
 * Returns 0 or negative errno. Does not perform a handshake or verify a peer.
 */
int support_tls_selftest(void);
#endif
