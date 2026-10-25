/*
 * SKELETON - compile-check only; see README.md.
 *
 * hal_pwm.c
 *
 * Linux backend for the PWM HAL interface contract (hal_pwm.h).
 *
 * The exported symbol set and signatures are identical to the framework
 * implementation (ports/hal/linux/hal_pwm.c); the bodies are not:
 *   hal_pwm_get_dev   -> NULL (contract: device not available)
 *   hal_pwm_set_freq  -> -ENOSYS
 *   hal_pwm_set_duty  -> -ENOSYS
 *   hal_pwm_enable    -> -ENOSYS
 *   hal_pwm_disable   -> -ENOSYS
 *
 * No real Linux pwm_set_cycles()/pwm_get_cycles_per_sec() logic is
 * implemented. Every operation is counted in s_unimpl_count so a caller
 * can never mistake a skeleton for a working PWM backend.
 */
#include <errno.h>
#include <stdint.h>

#include "hal/hal_pwm.h"
/* File-local diagnostic counter for unimplemented skeleton calls (NOT externally observable). */
static uint32_t s_unimpl_count = 0;

hal_pwm_dev_t hal_pwm_get_dev(const char *name)
{
	(void)name;
	/* Contract: device discovery is not available in the skeleton. */
	return NULL;
}

int hal_pwm_set_freq(hal_pwm_dev_t dev, uint32_t freq_hz)
{
	(void)dev;
	(void)freq_hz;
	s_unimpl_count++;
	return -ENOSYS;
}

int hal_pwm_set_duty(hal_pwm_dev_t dev, uint8_t channel, uint16_t duty_permille)
{
	(void)dev;
	(void)channel;
	(void)duty_permille;
	s_unimpl_count++;
	return -ENOSYS;
}

int hal_pwm_enable(hal_pwm_dev_t dev, uint8_t channel)
{
	(void)dev;
	(void)channel;
	s_unimpl_count++;
	return -ENOSYS;
}

int hal_pwm_disable(hal_pwm_dev_t dev, uint8_t channel)
{
	(void)dev;
	(void)channel;
	s_unimpl_count++;
	return -ENOSYS;
}