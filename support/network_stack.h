/** @file network_stack.h @brief Protocol-stack bring-up, without PHY claims. */
#ifndef SUPPORT_NETWORK_STACK_H
#define SUPPORT_NETWORK_STACK_H
#include <stdint.h>

/* Call once during startup. A failed asynchronous init cannot be restarted. */
int support_network_init(void);
/* UDP/IPv4 loopback only; no MAC/PHY/network cable is exercised.
 * Single caller at a time. timeout_ms must be 1..2000.
 */
int support_network_selftest(uint32_t timeout_ms);
#endif
