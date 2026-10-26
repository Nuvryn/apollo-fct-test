/** @file device.h @brief Private host-test shim; never used by firmware. */
#ifndef TEST_CAN_DEVICE_H
#define TEST_CAN_DEVICE_H
#include <stdbool.h>
struct device
{
    const char *name;
};
extern const struct device test_can_device;
bool device_is_ready(const struct device *device);
#define DT_NODELABEL(name) 0
#define DEVICE_DT_GET(node) (&test_can_device)
#endif
