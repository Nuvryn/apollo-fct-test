#include <stdint.h>
#define __aligned(n) __attribute__((aligned(n)))
#define K_MSEC(n) (n)
int64_t k_uptime_get(void);
void k_sleep(int ms);
