/**
 * @file main.c
 * @brief Start the test worker; Zephyr initializes the serial shell.
 */
#include <zephyr/logging/log.h>
#include "fct/fct.h"
#include "fct/shell.h"
#include "support/network_stack.h"

LOG_MODULE_REGISTER(apollo_main, LOG_LEVEL_INF);

int main(void)
{
    int result = fct_init(fct_shell_output);
    if (result != 0)
    {
        LOG_ERR("FCT initialization failed: %d", result);
        return result;
    }
    /* Management and non-network tests are available during stack startup. */
    result = support_network_init();
    if (result != 0)
    {
        LOG_ERR("lwIP initialization failed: %d; physical Ethernet is not configured", result);
    }
    else
    {
        LOG_INF("lwIP ready: software loopback only, use fct run lwip");
    }
    return result;
}
