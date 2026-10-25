/* Compile-check skeleton: no console device is connected. */
#include <errno.h>
#include "support/console.h"

void console_out(const char *data, size_t len)
{
    (void)data;
    (void)len;
}

int console_printf(const char *fmt, ...)
{
    (void)fmt;
    return -ENOSYS;
}

void console_flush(void)
{
}

void console_set_stop_handler(void (*fn)(void))
{
    (void)fn;
}
