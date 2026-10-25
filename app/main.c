/**
 * @file main.c
 * @brief Start the test worker; Zephyr initializes the serial shell.
 */
#include <zephyr/sys/printk.h>
#include "fct/fct.h"
#include "fct/shell.h"

int main(void)
{
    int result = fct_init(fct_shell_output);
    if (result != 0)
    {
        printk("FCT initialization failed: %d\n", result);
    }
    return result;
}
