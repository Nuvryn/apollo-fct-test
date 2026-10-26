/** @file can_args.c @brief Platform-free validation of explicit CAN commands. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "fct/can_test.h"

static int can_number(const char *text, uint32_t maximum, uint32_t *value)
{
    char *end;
    unsigned long parsed;
    if (text == NULL || *text == '\0' || *text == '-' || *text == '+')
    {
        return -EINVAL;
    }
    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > maximum)
    {
        return -EINVAL;
    }
    *value = (uint32_t) parsed;
    return 0;
}
static int can_hex(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

int fct_can_parse(bool transmit, const char *bitrate, const char *format,
    const char *id, const char *data_or_wait, fct_can_request_t *output)
{
    fct_can_request_t request = {.operation = transmit ? FCT_CAN_TX : FCT_CAN_RX, .wait_ms = 1000};
    if (output == NULL || format == NULL || data_or_wait == NULL)
    {
        return -EINVAL;
    }
    if (strcmp(format, "ext") == 0)
    {
        request.message.flags = HAL_CAN_EXT_ID;
    }
    else if (strcmp(format, "std") != 0)
    {
        return -EINVAL;
    }
    if (can_number(bitrate, 1000000, &request.bitrate) != 0 ||
        can_number(id, request.message.flags != 0U ? 0x1fffffffU : 0x7ffU, &request.message.id) != 0)
    {
        return -EINVAL;
    }
    if (!transmit)
    {
        if (can_number(data_or_wait, 5000, &request.wait_ms) != 0)
        {
            return -EINVAL;
        }
    }
    else if (strcmp(data_or_wait, "-") != 0)
    {
        size_t length = strlen(data_or_wait);
        if (length == 0U || length > 16U || length % 2U != 0U)
        {
            return -EINVAL;
        }
        request.message.len = (uint8_t) (length / 2U);
        for (size_t idx = 0; idx < length / 2U; idx++)
        {
            int high = can_hex(data_or_wait[idx * 2U]);
            int low = can_hex(data_or_wait[idx * 2U + 1U]);
            if (high < 0 || low < 0)
            {
                return -EINVAL;
            }
            request.message.data[idx] = (uint8_t) ((high << 4) | low);
        }
    }
    int result = fct_can_validate(&request);
    if (result == 0)
    {
        *output = request;
    }
    return result;
}
