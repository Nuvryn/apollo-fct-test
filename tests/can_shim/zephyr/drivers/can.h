/** @file can.h @brief Private host-test shim; never used by firmware. */
#ifndef TEST_CAN_NATIVE_H
#define TEST_CAN_NATIVE_H
#include <stdint.h>
#include "zephyr/device.h"
#define CAN_STD_ID_MASK 0x7ffU
#define CAN_EXT_ID_MASK 0x1fffffffU
#define CAN_FRAME_IDE 1U
#define CAN_FRAME_RTR 2U
#define CAN_FRAME_FDF 4U
#define CAN_FILTER_IDE 1U
#define CAN_MODE_NORMAL 0U
#define CAN_MODE_LOOPBACK 1U
#define CAN_MODE_LISTENONLY 2U
typedef uint32_t can_mode_t;
struct can_frame
{
    uint32_t id;
    uint8_t dlc;
    uint8_t flags;
    uint8_t data[8];
};
struct can_filter
{
    uint32_t id;
    uint32_t mask;
    uint8_t flags;
};
enum can_state
{
    CAN_STATE_ERROR_ACTIVE,
    CAN_STATE_ERROR_WARNING,
    CAN_STATE_ERROR_PASSIVE,
    CAN_STATE_BUS_OFF,
    CAN_STATE_STOPPED
};
typedef void (*can_tx_callback_t)(const struct device *, int, void *);
typedef void (*can_rx_callback_t)(const struct device *, struct can_frame *, void *);
int can_set_mode(const struct device *, can_mode_t mode);
int can_set_bitrate(const struct device *, uint32_t bitrate);
int can_start(const struct device *);
int can_stop(const struct device *);
int can_send(const struct device *, const struct can_frame *, int64_t timeout,
    can_tx_callback_t callback, void *argument);
int can_add_rx_filter(const struct device *, can_rx_callback_t callback,
    void *argument, const struct can_filter *filter);
void can_remove_rx_filter(const struct device *, int filter);
int can_get_state(const struct device *, enum can_state *state, void *counts);
#endif
