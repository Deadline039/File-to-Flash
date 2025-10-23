/**
 * @file    flash_fs.c
 * @author  Deadline039
 * @brief   Simple flash file system (Read only).
 * @version 0.1
 * @date    2025-09-28
 */

#include "flash_fs.h"

#include <string.h>
#include <stdlib.h>

typedef struct {
    flash_addr_t address;                  /*!< File start address */
    flash_size_t size;                     /*!< File size */
    char file_name[FLASH_FS_FILENAME_LEN]; /*!< File name */
} __attribute__((packed)) file_info_t;

static struct {
    uint32_t file_count; /*!< File count in flash. */
    /*!< File information */
    file_info_t file_info[FLASH_FS_MAX_FILE_NUM];
} __attribute__((packed)) file_list;

/**
 * @brief Initialize the file system.
 *
 */
void flash_fs_init(void) {
    FLASH_FS_READ(FLASH_FS_INFO_ADDR, &file_list, sizeof(file_list));
    if (file_list.file_count > FLASH_FS_MAX_FILE_NUM) {
        file_list.file_count = 0;
    }
}

/**
 * @brief Search file info by file name.
 *
 * @param file_name File name.
 * @return The info pointer.
 */
static file_info_t *search_file(const char *file_name) {
#if FLASH_FS_FILENAME_SORTED
    uint32_t left = 0;
    uint32_t right = file_list.file_count - 1;
    uint32_t middle;
    int res;

    while (left <= right) {
        middle = (left + right) / 2;
        res = strcmp(file_list.file_info[middle].file_name, file_name);

        if (res < 0) {
            left = middle + 1;
        } else if (res > 0) {
            right = middle - 1;
        } else {
            return &file_list.file_info[middle];
        }
    }
#else  /* FLASH_FS_FILENAME_SORTED */
    for (uint32_t i = 0; i < file_list.file_count; i++) {
        if (strcmp(file_list.file_info[i].file_name, file_name) == 0) {
            return &file_list.file_info[i];
        }
    }
#endif /* FLASH_FS_FILENAME_SORTED */
    return NULL;
}

/**
 * @brief Get file address by file name.
 *
 * @param file_name File name.
 * @return File address.
 */
flash_addr_t flash_fs_get_file_addr(const char *file_name) {
    if (file_list.file_count == 0) {
        return (flash_addr_t)0;
    }

    file_info_t *info = search_file(file_name);

    if (info == NULL) {
        return (flash_addr_t)0;
    }
    return (flash_addr_t)info->address;
}

/**
 * @brief Get file size by file name.
 *
 * @param file_name File name.
 * @return File size.
 */
flash_size_t flash_fs_get_file_size(const char *file_name) {
    if (file_list.file_count == 0) {
        return (flash_size_t)0;
    }

    file_info_t *info = search_file(file_name);

    if (info == NULL) {
        return (flash_size_t)0;
    }
    return (flash_addr_t)info->size;
}

/**
 * @brief Get file name by address.
 *
 * @param file_addr File address.
 * @return Returns `NULL` if not found or returns the file name string.
 */
char *flash_fs_get_file_name(const flash_addr_t file_addr) {
    if (file_list.file_count == 0) {
        return NULL;
    }

#if FLASH_FS_FILENAME_SORTED
    for (uint32_t i = 0; i < file_list.file_count; i++) {
        if (file_list.file_info[i].address == file_addr) {
            return file_list.file_info[i].file_name;
        }
    }
#else  /* FLASH_FS_FILENAME_SORTED */
    flash_addr_t left = file_list.file_info[0].address;
    flash_addr_t right = file_list.file_info[file_list.file_count - 1].address;
    flash_addr_t middle;

    while (left <= right) {
        middle = (left + right) / 2;
        if (file_list.file_info[middle].address < file_addr) {
            left = middle + 1;
        } else if (file_list.file_info[middle].address > file_addr) {
            right = middle - 1;
        } else {
            return file_list.file_info[middle].file_name;
        }
    }
#endif /* FLASH_FS_FILENAME_SORTED */

    return NULL;
}
