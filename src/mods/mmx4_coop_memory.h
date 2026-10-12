#ifndef MMX4_COOP_MEMORY_H
#define MMX4_COOP_MEMORY_H
#include <stddef.h>
#include <stdint.h>

/* Private player/camera context copies. Guest ranges are RAM, scratchpad or
 * registered mod arenas; all stores retain the framework's host-store path. */
void mmx4_coop_capture(uint32_t address,void *data,size_t bytes);
void mmx4_coop_project(uint32_t address,const void *data,size_t bytes);
#endif
