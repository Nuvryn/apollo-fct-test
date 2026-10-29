#ifndef TLS_TEST_DEVICE_H
#define TLS_TEST_DEVICE_H
#include <stdbool.h>
struct device { int unused; };
extern const struct device test_rng;
#define DT_CHOSEN(name) 0
#define DEVICE_DT_GET(node) (&test_rng)
bool device_is_ready(const struct device *dev);
#endif
