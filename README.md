# File to Flash

A simple tool that can convert files to binary format. It makes managing external resources (such as audio, photos, fonts, etc.) convenient and simple.

# How to use

Download the executable file from the Github Release.

Modify the start address, flash size, and alignment as needed.

Click the `Add` button to add a file.

You can see each file's address and name in the table below.

You can adjust the file location using the `Up` and `Down` buttons, or delete it with the `Remove` button.

It is recommended to add a file index to the binary file. This will add the file address, name, and size into the binary file. The file table structure is as follows:

```c
struct {
    uint32_t file_count; /*!< File count in flash. */

    struct {
        flash_addr_t address;                  /*!< File start address */
        flash_size_t size;                     /*!< File size */
        char file_name[FLASH_FS_FILENAME_LEN]; /*!< File name */
    } __attribute__((packed)) file_info[FLASH_FS_MAX_FILE_NUM];
} __attribute__((packed)) file_list;
```

You can use the sample in the `read_sample` folder, or implement it in your own way.

Select the output format (recommended: hex and s19, which contain address information) and output directory.

Download the binary file into flash.

## Read sample

A sample for reading files is provided in the `read_sample` folder.

First, you should implement the read interface as follows:

```c
/* You should implement this interface in your source file. */
extern void w25qxx_fs_read(flash_addr_t address, void *buf, flash_size_t size);
#define FLASH_FS_READ w25qxx_fs_read
```

Second, call `flash_fs_init` to initialize the file system.

Then you can call `flash_fs_get_file_addr` to get the file address by file name, and read it.