/** @file lwip_tests.c @brief Real lwIP stack tests on a host OSAL fixture. */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "support/network_stack.h"

unsigned int host_osal_allocations(void);
void host_osal_fail_next_allocation(void);
uint32_t lwip_port_random(void)
{
    return (uint32_t) rand();
}
void lwip_port_diag(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    (void) vfprintf(stderr, format, args);
    va_end(args);
}
void lwip_port_assert(const char *message, const char *file, int line)
{
    (void) fprintf(stderr, "lwIP assertion: %s at %s:%d\n", message, file, line);
    abort();
}

typedef struct
{
    sys_sem_t *sem;
    sys_mbox_t *mbox;
} poster_t;
static void post_later(void *argument)
{
    poster_t *poster = argument;
    osal_delay_ms(30);
    if (poster->mbox != NULL)
    {
        sys_mbox_post(poster->mbox, NULL);
    }
    sys_sem_signal(poster->sem);
}

static void test_port(void)
{
    sys_sem_t sem = NULL;
    sys_mbox_t mbox = NULL;
    sys_mutex_t mutex = NULL;
    void *message = &sem;
    uintptr_t outer = sys_arch_protect();
    uintptr_t inner = sys_arch_protect();
    sys_arch_unprotect(inner);
    sys_arch_unprotect(outer);
    host_osal_fail_next_allocation();
    assert(sys_sem_new(&sem, 0) == ERR_MEM && !sys_sem_valid(&sem));
    assert(sys_sem_new(&sem, 0) == ERR_OK);
    assert(sys_arch_sem_wait(&sem, 10) == SYS_ARCH_TIMEOUT);
    assert(sys_mbox_new(&mbox, 1) == ERR_OK);
    assert(sys_arch_mbox_tryfetch(&mbox, &message) == SYS_MBOX_EMPTY);
    assert(sys_mbox_trypost(&mbox, &sem) == ERR_OK);
    assert(sys_mbox_trypost(&mbox, NULL) == ERR_MEM);
    assert(sys_arch_mbox_fetch(&mbox, &message, 10) != SYS_ARCH_TIMEOUT && message == &sem);
    assert(sys_arch_mbox_fetch(&mbox, NULL, 10) == SYS_ARCH_TIMEOUT);
    assert(sys_mbox_trypost_fromisr(&mbox, NULL) == ERR_IF);
    assert(sys_mutex_new(&mutex) == ERR_OK);
    sys_mutex_lock(&mutex);
    sys_mutex_unlock(&mutex);
    sys_mutex_free(&mutex);
    assert(!sys_mutex_valid(&mutex));
    poster_t poster = {.sem = &sem, .mbox = &mbox};
    (void) sys_thread_new("poster", post_later, &poster, 4096, 16);
    assert(sys_arch_sem_wait(&sem, 0) != SYS_ARCH_TIMEOUT); /* zero must mean forever */
    assert(sys_arch_mbox_fetch(&mbox, &message, 0) != SYS_ARCH_TIMEOUT && message == NULL);
    sys_mbox_free(&mbox);
    sys_sem_free(&sem);
    assert(!sys_mbox_valid(&mbox) && !sys_sem_valid(&sem));
}

static void test_sockets(void)
{
    const char payload[] = "lwip-prefixed-socket";
    char actual[sizeof(payload)];
    struct sockaddr_in address = {.sin_family = AF_INET,
        .sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK)};
    socklen_t size = sizeof(address);
    const struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    int receiver = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    int sender = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    assert(receiver >= 0 && sender >= 0);
    assert(lwip_setsockopt(receiver, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);
    assert(lwip_bind(receiver, (struct sockaddr *) &address, sizeof(address)) == 0);
    assert(lwip_getsockname(receiver, (struct sockaddr *) &address, &size) == 0);
    assert(lwip_sendto(sender, payload, sizeof(payload), 0,
               (struct sockaddr *) &address, size) == (int) sizeof(payload));
    assert(lwip_recv(receiver, actual, sizeof(actual), 0) == (int) sizeof(actual));
    assert(memcmp(actual, payload, sizeof(actual)) == 0);
    assert(lwip_close(sender) == 0 && lwip_close(receiver) == 0);
}

int main(void)
{
    assert(support_network_selftest(100) == -EAGAIN);
    test_port();
    assert(support_network_init() == 0);
    assert(support_network_init() == -EALREADY);
    assert(support_network_selftest(0) == -EINVAL);
    assert(support_network_selftest(2001) == -EINVAL);
    assert(support_network_selftest(1000) == 0);
    unsigned int baseline = host_osal_allocations();
    host_osal_fail_next_allocation();
    assert(support_network_selftest(1000) == -ENOMEM);
    assert(host_osal_allocations() == baseline);
    for (unsigned int idx = 0; idx < 40U; idx++)
    {
        assert(support_network_selftest(1000) == 0);
    }
    assert(host_osal_allocations() == baseline);
    struct netconn *receiver = netconn_new(NETCONN_UDP);
    assert(receiver != NULL);
    ip_addr_t loopback;
    IP_ADDR4(&loopback, 127, 0, 0, 1);
    assert(netconn_bind(receiver, &loopback, 0) == ERR_OK);
    netconn_set_recvtimeout(receiver, 20);
    struct netbuf *buffer = NULL;
    assert(netconn_recv(receiver, &buffer) == ERR_TIMEOUT && buffer == NULL);
    assert(netconn_delete(receiver) == ERR_OK);
    test_sockets();
    assert(host_osal_allocations() == baseline);
    puts("PASS: real lwIP sys_arch, UDP netconn x41, prefixed sockets, allocation failure, timeout, cleanup");
    return 0;
}
