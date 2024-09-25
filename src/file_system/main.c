#include "fs.h"

int main() {
    // Test Case: Create 16 1MB files, delete the 5th and 9th, then create a 2MB file

    char *disk_name = "file_creation_test_disk";
    int result;

    // Create filesystem on the disk
    result = make_fs(disk_name);
    if (result != 0) {
        fprintf(stderr, "Error creating filesystem.\n");
        return 1;
    }

    // Mount the filesystem
    result = mount_fs(disk_name);
    if (result != 0) {
        fprintf(stderr, "Error mounting filesystem.\n");
        return 1;
    }

    // Create 16 1MB files
    for (int i = 0; i < 16; ++i) {
        char file_name[20];
        sprintf(file_name, "file%d.txt", i);

        int fd = fs_create(file_name);
        if (fd < 0) {
            fprintf(stderr, "Error creating file %s.\n", file_name);
            return 1;
        }

        //open file
        fd = fs_open(file_name);

        // Write 1MB of data to each file
        size_t write_size_1mb = 1 * 1024 * 1024;  // 1MB
        char *write_buffer_1mb = (char *)malloc(write_size_1mb);
        memset(write_buffer_1mb, 'A' + i, write_size_1mb);  // Fill with characters 'A' to 'P'
        ssize_t bytes_written_1mb = fs_write(fd, write_buffer_1mb, write_size_1mb);
        free(write_buffer_1mb);

        // Check if fs_write returned the expected size
        if (bytes_written_1mb != write_size_1mb) {
            fprintf(stderr, "Error writing 1MB to file %s. Expected %zu bytes, but %zd bytes were written.\n",
                    file_name, write_size_1mb, bytes_written_1mb);
            return 1;
        }

        // Close the file
        result = fs_close(fd);
        if (result != 0) {
            fprintf(stderr, "Error closing file %s.\n", file_name);
            return 1;
        }
    }

    // Delete the 5th and 9th files
    char file_to_delete_5th[20], file_to_delete_9th[20];
    sprintf(file_to_delete_5th, "file%d.txt", 4);
    sprintf(file_to_delete_9th, "file%d.txt", 8);

    result = fs_delete(file_to_delete_5th);
    if (result != 0) {
        fprintf(stderr, "Error deleting file %s.\n", file_to_delete_5th);
        return 1;
    }

    result = fs_delete(file_to_delete_9th);
    if (result != 0) {
        fprintf(stderr, "Error deleting file %s.\n", file_to_delete_9th);
        return 1;
    }

    // Create a 2MB file
    char *file_2mb_name = "file_2mb.txt";
    int fd_2mb = fs_create(file_2mb_name);
    
    if (fd_2mb < 0) {
        fprintf(stderr, "Error creating 2MB file.\n");
        return 1;
    }
    fd_2mb = fs_open(file_2mb_name);
    // Write 2MB of data to the file
    size_t write_size_2mb = 2 * 1024 * 1024;  // 2MB
    char *write_buffer_2mb = (char *)malloc(write_size_2mb);
    memset(write_buffer_2mb, 'B', write_size_2mb);  // Fill with 'B' characters
    ssize_t bytes_written_2mb = fs_write(fd_2mb, write_buffer_2mb, write_size_2mb);
    free(write_buffer_2mb);

    // Check if fs_write returned the expected size
    if (bytes_written_2mb != write_size_2mb) {
        fprintf(stderr, "Error writing 2MB to the file. Expected %zu bytes, but %zd bytes were written.\n",
                write_size_2mb, bytes_written_2mb);
        return 1;
    }

    // Close the 2MB file
    result = fs_close(fd_2mb);
    if (result != 0) {
        fprintf(stderr, "Error closing 2MB file.\n");
        return 1;
    }

    // Unmount the filesystem (cleanup)
    result = umount_fs(disk_name);
    if (result != 0) {
        fprintf(stderr, "Error unmounting filesystem (cleanup).\n");
        return 1;
    }

    printf("Test case passed: Created 16 1MB files, deleted the 5th and 9th, then created a 2MB file.\n");

    return 0;
}
