#include <stdint.h>
#include <zephyr/device.h>
int entropy_get_entropy(const struct device *dev, void *buf, uint16_t len);
