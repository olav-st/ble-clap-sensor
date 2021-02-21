#include <zephyr.h>
#include <logging/log.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <device.h>
#include <stdio.h>

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#define FLASH_TEST_OFFSET FLASH_AREA_OFFSET(image_1_nonsecure)
#else
#define FLASH_TEST_OFFSET FLASH_AREA_OFFSET(image_1)
#endif

#define FLASH_PAGE_SIZE   4096
#define TEST_DATA_WORD_0  0x1122
#define TEST_DATA_WORD_1  0xaabb
#define TEST_DATA_WORD_2  0xabcd
#define TEST_DATA_WORD_3  0x1234

#define FLASH_TEST_OFFSET2 0x41234
#define FLASH_TEST_PAGE_IDX 37

LOG_MODULE_REGISTER(flash_storage);

const struct device *flash_dev;

int flash_storage_init(void)
{
    flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
    if(flash_dev == NULL)
    {
        return 1;
    }
    return 0;
}

uint32_t flash_storage_read(uint32_t offset)
{
    uint32_t result;
    int err = flash_read(flash_dev, offset, &result, sizeof(uint32_t));
    if(err)
    {
        LOG_ERR("Failed to read from flash: %d", err);
    }
    return result;
}
uint32_t flash_storage_write(uint32_t offset, uint32_t value)
{
    flash_write_protection_set(flash_dev, false);
    int err = flash_write(flash_dev, offset, &value, sizeof(uint32_t));
    flash_write_protection_set(flash_dev, true);  
    return err; 
}