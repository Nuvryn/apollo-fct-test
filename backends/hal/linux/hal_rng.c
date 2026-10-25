/*
 * SKELETON - compile-check only; see README.md.
 *
 * hal_rng.c
 *
 * Linux backend for the RNG HAL interface contract (hal_rng.h).
 *
 * The exported symbol set and signatures are identical to the framework
 * implementation (ports/hal/linux/hal_rng.c); the bodies are not:
 *   hal_rng_get_dev -> NULL (contract: device not available)
 *   hal_rng_read    -> -ENOSYS
 *
 * No real Linux entropy_get_entropy() logic is implemented. Every
 * operation is counted in s_unimpl_count so a caller can never mistake a
 * skeleton for a working RNG backend.
 */
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include "hal/hal_rng.h"
/* File-local diagnostic counter for unimplemented skeleton calls (NOT externally observable). */
static uint32_t s_unimpl_count = 0;

hal_rng_dev_t hal_rng_get_dev(const char *name)
{
	(void)name;
	/* Contract: device discovery is not available in the skeleton. */
	return NULL;
}

int hal_rng_read(hal_rng_dev_t dev, uint8_t *buf, size_t len)
{
	(void)dev;
	(void)buf;
	(void)len;
	s_unimpl_count++;
	return -ENOSYS;
}