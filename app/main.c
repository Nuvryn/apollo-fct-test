/**
 * @file main.c
 * @brief Minimal Zephyr boot check over the board's USART1 console.
 */
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("Apollo V1 FCT: direct boot, USART1 PA9/PA10\n");
    return 0;
}
