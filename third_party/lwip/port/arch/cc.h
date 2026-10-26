/** @file cc.h @brief Compiler/platform hooks; no OS types leak into lwIP. */
#ifndef LWIP_PORT_ARCH_CC_H
#define LWIP_PORT_ARCH_CC_H

#include <stdint.h>
uint32_t lwip_port_random(void);
void lwip_port_diag(const char *format, ...);
void lwip_port_assert(const char *message, const char *file, int line);
void lwip_port_mark_core(void);
void lwip_port_check_core(void);
#define BYTE_ORDER LITTLE_ENDIAN
#define LWIP_RAND() lwip_port_random()
#define LWIP_PLATFORM_DIAG(args) lwip_port_diag args
#define LWIP_PLATFORM_ASSERT(message) lwip_port_assert(message, __FILE__, __LINE__)

#endif
