#ifndef _STORAGE_H_
#define _STORAGE_H_
#include "pico/stdlib.h"

void load(void *buffer, const uint32_t size);
void save(void* buffer, const uint32_t size);

#endif
