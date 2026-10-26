/** @file lwip_platform.c @brief Zephyr-specific diagnostics, panic and entropy. */
#include <stdarg.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include "arch/cc.h"
LOG_MODULE_REGISTER(lwip_port, LOG_LEVEL_INF);

uint32_t lwip_port_random(void)
{
    return sys_rand32_get();
}
void lwip_port_diag(const char *format, ...)
{
    char text[160];
    va_list args;
    va_start(args, format);
    (void) vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    LOG_INF("%s", text);
}
void lwip_port_assert(const char *message, const char *file, int line)
{
    LOG_ERR("lwIP assertion: %s (%s:%d)", message, file, line);
    k_panic();
}
