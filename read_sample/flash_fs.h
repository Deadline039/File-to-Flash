/**
 * @file    flash_fs.c
 * @author  Deadline039
 * @brief   Simple flash file system (Read only).
 * @version 0.1
 * @date    2025-09-28
 */

#ifndef __FLASH_FS
#define __FLASH_FS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#define flash_addr_t          uint32_t
#define flash_size_t          uint32_t

#define FLASH_FS_INFO_ADDR    (flash_addr_t)(0x400)
#define FLASH_FS_FILENAME_LEN 58
#define FLASH_FS_MAX_FILE_NUM 62

/* You should implement this interface in your source file. */
extern void w25qxx_fs_read(flash_addr_t address, void *buf, flash_size_t size);
#define FLASH_FS_READ w25qxx_fs_read

void flash_fs_init(void);
flash_addr_t flash_fs_get_file_addr(const char *file_name);
flash_size_t flash_fs_get_file_size(const char *file_name);
char *flash_fs_get_file_name(const flash_addr_t file_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FLASH_FS */
