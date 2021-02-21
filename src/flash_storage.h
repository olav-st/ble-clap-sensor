#ifdef __cplusplus
extern "C" {
#endif

int flash_storage_init();
uint32_t flash_storage_read(uint32_t offset);
int flash_storage_write(uint32_t offset, uint32_t value);

#ifdef __cplusplus
}
#endif