/*
 * SKELETON - compile-check only; see README.md.
 *
 * osal_mq.c
 *
 * Linux backend for the OSAL fixed-size message queue API - SKELETON.
 *
 * The exported symbol set and signatures are identical to the framework
 * implementation (ports/osal/linux/osal_mq.c); the bodies are not.
 * A real backend wraps a heap-allocated osal_mq_ctrl_t (k_malloc +
 * k_msgq_init/put/get/cleanup) behind the opaque handle; nothing of that
 * is implemented here. Every op returns OSAL_ERR_NOTSUP and is counted.
 */
#include <stdint.h>
#include <stddef.h>
#include "osal/osal.h"
#include "osal/osal_types.h"
/* File-local diagnostic counter for unimplemented skeleton calls (NOT externally observable). */
static uint32_t s_unimpl_count = 0;

int osal_mq_create(osal_mq_t *q, size_t msg_size, size_t max_msgs)
{
	(void)q;
	(void)msg_size;
	(void)max_msgs;
	s_unimpl_count++;
	return OSAL_ERR_NOTSUP;
}

int osal_mq_send(osal_mq_t q, const void *msg, uint32_t timeout_ms)
{
	(void)q;
	(void)msg;
	(void)timeout_ms;
	s_unimpl_count++;
	return OSAL_ERR_NOTSUP;
}

int osal_mq_recv(osal_mq_t q, void *msg, uint32_t timeout_ms)
{
	(void)q;
	(void)msg;
	(void)timeout_ms;
	s_unimpl_count++;
	return OSAL_ERR_NOTSUP;
}

int osal_mq_destroy(osal_mq_t q)
{
	(void)q;
	s_unimpl_count++;
	return OSAL_ERR_NOTSUP;
}