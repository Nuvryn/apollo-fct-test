/* Configuration storage is unavailable in the compile-check skeleton. */
#include <errno.h>
#include <stddef.h>
#include "support/config_source.h"

int config_source_load(config_source_t *out, const char *src)
{
    (void)src;
    if (out == NULL) {
        return -EINVAL;
    }
    out->text = NULL;
    out->owns = false;
    return -ENOSYS;
}

void config_source_free(config_source_t *cfg)
{
    (void)cfg;
}
