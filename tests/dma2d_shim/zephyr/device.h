#ifndef TEST_DMA_DEVICE_H
#define TEST_DMA_DEVICE_H
struct device
{
    int unused;
};
extern const struct device test_display;
#define DT_NODELABEL(name) 0
#define DEVICE_DT_GET(node) (&test_display)
#endif
