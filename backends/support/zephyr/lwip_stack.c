/** @file lwip_stack.c @brief Start lwIP and test its real UDP loopback path. */
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "osal/osal.h"
#include "support/network_stack.h"

static osal_sem_t s_ready;
static bool s_started;
static bool s_initialized;

static void network_ready(void *arg)
{
    (void) arg;
    uintptr_t key = osal_critical_enter();
    s_initialized = true;
    osal_critical_exit(key);
    (void) osal_sem_post(s_ready);
}

int support_network_init(void)
{
    int result;
    uintptr_t key = osal_critical_enter();
    if (s_started)
    {
        osal_critical_exit(key);
        return -EALREADY;
    }
    s_started = true;
    osal_critical_exit(key);
    result = osal_sem_create(&s_ready, 0, 1);
    if (result != 0)
    {
        return result;
    }
    tcpip_init(network_ready, NULL);
    /* The callback and semaphore are static, including on timeout. */
    return osal_sem_wait(s_ready, 2000);
}

static int network_result(err_t result)
{
    if (result == ERR_OK)
    {
        return 0;
    }
    if (result == ERR_TIMEOUT)
    {
        return -ETIMEDOUT;
    }
    return result == ERR_MEM ? -ENOMEM : -EIO;
}

int support_network_selftest(uint32_t timeout_ms)
{
    static const char payload[] = "apollo-lwip-udp-loopback";
    struct netconn *receiver = NULL;
    struct netconn *sender = NULL;
    struct netbuf *transmit = NULL;
    struct netbuf *received = NULL;
    ip_addr_t loopback;
    u16_t port;
    char actual[sizeof(payload)];
    err_t result = ERR_MEM;
    uintptr_t key = osal_critical_enter();
    bool ready = s_initialized;
    osal_critical_exit(key);
    if (!ready)
    {
        return -EAGAIN;
    }
    if (timeout_ms == 0U || timeout_ms > 2000U)
    {
        return -EINVAL;
    }
    IP_ADDR4(&loopback, 127, 0, 0, 1);
    receiver = netconn_new(NETCONN_UDP);
    sender = netconn_new(NETCONN_UDP);
    if (receiver == NULL || sender == NULL)
    {
        goto cleanup;
    }
    netconn_set_recvtimeout(receiver, timeout_ms);
    result = netconn_bind(receiver, &loopback, 0);
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    result = netconn_getaddr(receiver, &loopback, &port, 1);
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    result = netconn_connect(sender, &loopback, port);
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    transmit = netbuf_new();
    if (transmit == NULL)
    {
        result = ERR_MEM;
        goto cleanup;
    }
    result = netbuf_ref(transmit, payload, sizeof(payload));
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    result = netconn_send(sender, transmit);
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    result = netconn_recv(receiver, &received);
    if (result != ERR_OK)
    {
        goto cleanup;
    }
    if (netbuf_len(received) != sizeof(payload) ||
        netbuf_copy(received, actual, sizeof(actual)) != sizeof(actual) ||
        memcmp(actual, payload, sizeof(payload)) != 0)
    {
        result = ERR_VAL;
    }
cleanup:
    if (received != NULL)
    {
        netbuf_delete(received);
    }
    if (transmit != NULL)
    {
        netbuf_delete(transmit);
    }
    if (sender != NULL)
    {
        err_t deleted = netconn_delete(sender);
        if (result == ERR_OK)
        {
            result = deleted;
        }
    }
    if (receiver != NULL)
    {
        err_t deleted = netconn_delete(receiver);
        if (result == ERR_OK)
        {
            result = deleted;
        }
    }
    return network_result(result);
}
