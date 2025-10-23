/**
 * @file    test.c
 * @author  Deadline039
 * @brief   Simple flash file system (Read only) Test.
 * @version 0.1
 * @date    2025-10-23
 */

#include "flash_fs.h"
#include <stdio.h>

FILE *bin_file;
int main(void) {
    bin_file = fopen("../test.bin", "r");
    if (bin_file == NULL) {
        printf("Failed to open binary file\n");
        return 1;
    }
    flash_fs_init();

    const char *files[] = {
        "Photo1.jpg",
        "Photo2.jpg",
        "Photo3.jpg",
        "Photo4.jpg",
    };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        printf("File name: %s, address: %#x, size: %u byte\n", files[i],
               flash_fs_get_file_addr(files[i]),
               flash_fs_get_file_size(files[i]));
    }

    fclose(bin_file);
    return 0;
}

void file_read(flash_addr_t address, void *buf, flash_size_t size) {
    (void)address;
    fread(buf, size, size, bin_file);
}
