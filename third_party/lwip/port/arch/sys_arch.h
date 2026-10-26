/** @file sys_arch.h @brief lwIP handles backed by the existing OSAL contract. */
#ifndef LWIP_PORT_ARCH_SYS_ARCH_H
#define LWIP_PORT_ARCH_SYS_ARCH_H

#include "osal/osal.h"
typedef osal_sem_t sys_sem_t;
typedef osal_mutex_t sys_mutex_t;
typedef osal_mq_t sys_mbox_t;
typedef osal_task_t sys_thread_t;
typedef uintptr_t sys_prot_t;

#define sys_sem_valid(sem) ((sem) != NULL && *(sem) != NULL)
#define sys_sem_set_invalid(sem) (*(sem) = NULL)
#define sys_mutex_valid(mutex) ((mutex) != NULL && *(mutex) != NULL)
#define sys_mutex_set_invalid(mutex) (*(mutex) = NULL)
#define sys_mbox_valid(mbox) ((mbox) != NULL && *(mbox) != NULL)
#define sys_mbox_set_invalid(mbox) (*(mbox) = NULL)

void lwip_port_mark_core(void);
void lwip_port_check_core(void);
#endif
