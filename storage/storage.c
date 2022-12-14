#include "storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define MAGIC 0x99
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

static int find_unused_page()
{
    for (int page = 0; page < FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE; page++)
    {
        const uint32_t *ptr = (const uint32_t *)(XIP_BASE + FLASH_TARGET_OFFSET + FLASH_PAGE_SIZE * page);
        if (ptr[0] == 0xffffffff)
            return page;
    }
    return -1;
}

void load(void *buffer, const uint32_t size)
{
    int page = find_unused_page();
    if (page == -1)
        page = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE;
    page--;
    const uint8_t *flash_data = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET + FLASH_PAGE_SIZE * page);
    if (flash_data[0] != MAGIC)
        return;

    memcpy(buffer, flash_data + 1, size);
}

void save(void *buffer, const uint32_t size)
{
    int page = find_unused_page();
    if (page == -1)
    {
        flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
        page = 0;
    }

    uint8_t flash_data[FLASH_PAGE_SIZE];
    memset(flash_data, 0xff, FLASH_PAGE_SIZE);
    flash_data[0] = MAGIC;
    memcpy(flash_data + 1, buffer, size);
    const uint32_t ints = save_and_disable_interrupts();
    flash_range_program(FLASH_TARGET_OFFSET + FLASH_PAGE_SIZE * page, flash_data, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}