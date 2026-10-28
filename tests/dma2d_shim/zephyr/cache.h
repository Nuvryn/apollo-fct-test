#include <stddef.h>
int sys_cache_data_flush_range(void *addr, size_t size);
int sys_cache_data_invd_range(void *addr, size_t size);
int sys_cache_data_flush_and_invd_range(void *addr, size_t size);
