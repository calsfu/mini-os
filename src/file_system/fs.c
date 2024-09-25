#include "fs.h"
#define MAX_FILDES 32
#define MAX_F_NAME 15
#define BLOCK_SIZE 4096
#define MAX_FILES 64
#define MAX_LENGTH 4*1024*BLOCK_SIZE

struct super_block {
    int fat_idx; // First block of the FAT
    int fat_len; // Length of FAT in blocks
    int dir_idx; // First block of directory
    int dir_len; // Length of directory in blocks
    int data_idx; // First block of file-data
};

struct dir_entry {
    int used; // Is this file-”slot” in use
    char name [MAX_F_NAME + 1]; // DOH!
    int size; // file size
    int head; // first data block of file
    int ref_cnt;
    // how many open file descriptors are there?
    // ref_cnt > 0 -> cannot delete file
};

struct file_descriptor {
    int used; // fd in use
    int file; // the first block of the file
    // (f) to which fd refers too
    int offset; // position of fd within f
    int dir_entry; // index into DIR of f
};

// struct FAT_entry {
//     int used;
//     int block; //block number
//     struct FAT_entry* next; //if -1, EOF
// }; 

struct super_block fs;
struct file_descriptor fds[MAX_FILDES]; // 32
// struct FAT_entry *FAT; // Will be populated with the FAT data
int* FAT; //array of ints .Value is next block
struct dir_entry *DIR; //only one directory

int new_block() {
    int i;
    for(i = 0;i < BLOCK_SIZE;i++) {
        if(FAT[i] == 0) {
            FAT[i] = -1;
            return i;
        }
    }
    return -1;
}

int starting_block(int fildes) {
    int head = DIR[fds[fildes].dir_entry].head;
    int block = fds[fildes].offset / BLOCK_SIZE;
    int curr = head;
    while(block > 0) {
        if(FAT[curr] == -1) { //offset is bigger than block allocation
            return -1;
        }
        curr = FAT[curr];
        block--;
    }
    return curr;
}

// struct FAT_entry* findFirstBlock(int fildes, int offset) {
//     int head = fds[fildes].file;
//     int block = offset / BLOCK_SIZE;
//     struct FAT_entry* curr = FAT;
//     while(curr->next != NULL) {
//         if(head == curr->block) {
//             break;
//         }
//         curr = curr->next;
//     }
//     while(block > 0) {
//         if(curr->next == NULL || curr->block == -1) {
//             return NULL;
//         }
//         curr = curr->next;
//         block--;
//     }
//     return curr;
// }


int make_fs(char *disk_name) {
    if(make_disk(disk_name) == -1) {
        return -1;
    }
    if(open_disk(disk_name) == -1) {
        return -1;
    }
    //write meta-information necessary to support file system operations
    struct super_block temp;
    temp.fat_idx = 1;
    temp.fat_len = 4;
    temp.dir_idx = 500;
    //max files is 64
    temp.dir_len = 1;
    //copy fs
    temp.data_idx = 4096; //first half reserved for metadata
    //copy temp to block 0 of virutla disk
    char* buf = calloc(BLOCK_SIZE, 1);
    memcpy(buf, &temp, sizeof(temp));
    block_write(0, buf);

    int *tempFAT = calloc(BLOCK_SIZE, sizeof(int));
    // memcpy(buf, &tempFAT, sizeof(temp));
    // block_write(temp.fat_idx, buf);
    int i;
    for(i = 0;i < temp.fat_len;i++) {
        memcpy(buf, tempFAT + i * 1024, sizeof(int) * 1024);
        block_write(temp.fat_idx + i, buf);
    }

    struct dir_entry* tempDIR = calloc(64, sizeof(struct dir_entry));
    memcpy(buf, &tempDIR, sizeof(tempDIR));
    block_write(temp.dir_idx, buf);
    //close disk
    if(close_disk(disk_name) == -1) {
        return -1;
    }

    free(tempFAT);
    free(tempDIR);
    free(buf);

    return 0;
}

int mount_fs(char *disk_name) {
    //open disk, load meta-information necessary 
    if(open_disk(disk_name) == -1) {
        return -1;
    }
    //load metadata to local variables
    char* buf = calloc(BLOCK_SIZE, 1);
    // FAT = calloc(BLOCK_SIZE, sizeof(struct FAT_entry));
    FAT = calloc(BLOCK_SIZE, sizeof(int)); //array of ints with size 4096
    DIR = calloc(64, sizeof(struct dir_entry)); //array of dir entries with size 64
    block_read(0, buf);
    memcpy(&fs, buf, sizeof(fs));
    // block_read(fs.fat_idx, buf);
    // memcpy(FAT, buf, sizeof(char) * sizeof(FAT));
    int i = 0;
    for(i = 0;i < fs.fat_len;i++) {
        block_read(fs.fat_idx + i, buf);
        memcpy(FAT + i * 1024, buf, sizeof(int) * 1024);
    }
    block_read(fs.dir_idx, buf);
    memcpy(DIR, buf, MAX_FILES * sizeof(struct dir_entry));
    //to support file system operations
    if(DIR[0].used > MAX_FILES) {
        DIR[0].used = 0;
    }
    // DIR[0].used = 0;
    FAT[0] = 0;
    free(buf);
    return 0;
}

int umount_fs(char *disk_name) {
    if(close_disk(disk_name) == -1) {
        return -1;
    }
    if(open_disk(disk_name) == -1) {
        return -1;
    }
    // //write meta-information necessary to support file system operations
    char* buf = calloc(1, sizeof(struct super_block));
    memcpy(buf, &fs, sizeof(struct super_block));
    block_write(0, buf);
    // memcpy(buf, FAT, sizeof(char) * sizeof(FAT));
    // block_write(fs.fat_idx, buf);
    int i;
    for( i = 0;i < fs.fat_len;i++) {
        int temp[1024];
        char* FATbuf = calloc(1, sizeof(int) * 1024);
        memcpy(temp, FAT + i * 1024, sizeof(int) * 1024);
        memcpy(FATbuf, temp, sizeof(int) * 1024);
        block_write(fs.fat_idx + i, FATbuf);
        free(FATbuf);
    }
    char* newbuf = calloc(1, MAX_FILES * sizeof(struct dir_entry));
    memcpy(newbuf, DIR, MAX_FILES * sizeof(struct dir_entry));
    block_write(fs.dir_idx, newbuf);
    free(newbuf);
    free(buf);
    free(FAT);
    free(DIR);
    if(close_disk(disk_name) == -1) {
        return -1;
    }
    return 0;
}

int fs_open(char *name) {
    //find file in directory
    int i;
    //see if file exists
    for(i = 0; i < MAX_FILES; i++) {
        if(strcmp(DIR[i].name, name) == 0) {
            break;
        }
    }
    //if file not found, return -1
    if(i == MAX_FILES) {
        return -1;
    }
    int j;
    //see if file descriptors are available
    for(j = 0;j < MAX_FILDES;j++) {
        if(fds[j].used == 0) {
            break;
        }
    }
    //if no file descriptors available, return -1
    
    if(j == MAX_FILDES) {
        return -1;
    }
    //create new file descriptor
    fds[j].used = 1;
    fds[j].file = DIR[i].head;
    fds[j].offset = 0;
    fds[j].dir_entry = i;
    DIR[i].ref_cnt++;
    return j;
}

int fs_close(int fildes) {
    //check if file descriptor is in use
    if(fds[fildes].used == 0 || fildes >= MAX_FILDES) {
        return -1;
    }
    //decrement ref_cnt
    DIR[fds[fildes].dir_entry].ref_cnt--;
    fds[fildes].used = 0;
    return 0;
}

//
int fs_create(char *name) {
    int i;
    int first = -1;
    //check if name is too long
    if(strlen(name) > MAX_F_NAME) {
        return -1;
    }
    //check if file already exists
    for(i = 0;i < MAX_FILES;i++) {
        if(strcmp(DIR[i].name, name) == 0) {
            return -1;
        }
        if(first == -1 && DIR[i].used == 0) {
            first = i;
        }
    }
    if(first == -1) {
        return -1;
    }
    //find first free entry in directory
    //if no free entry, return -1
    //create new file
    DIR[first].used = 1;
    strcpy(DIR[first].name, name);
    DIR[first].size = 0;
    DIR[first].head = -1;
    DIR[first].ref_cnt = 0;
    return 0;

}

int fs_delete(char *name) {
    int i;
    for(i = 0;i < MAX_FILES;i++) {
        if(strcmp(DIR[i].name, name) == 0) {
            break;
        }
    }
    if(i == MAX_FILES) {
        return -1;
    }
    if(DIR[i].used == 0) {
        return -1;
    }
    if(DIR[i].ref_cnt > 0) {
        return -1;
    }
    int head = DIR[i].head;
    while(head != -1) {
        int temp = FAT[head];
        FAT[head] = 0;
        head = temp;
    }
    FAT[head] = 0;
    DIR[i].used = 0;
    DIR[i].name[0] = '\0';
    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte) {
    if(fds[fildes].used == 0) {
       return -1;
   }
    if(nbyte < 0) {
        return -1;
    }
    int offset = fds[fildes].offset % BLOCK_SIZE;

    if(fds[fildes].offset == DIR[fds[fildes].dir_entry].size) {
        return 0;
    }
    if(DIR[fds[fildes].dir_entry].size == 0) {
        return 0;
    }
    int currBlock = fds[fildes].file;
    
    // int fp = fds[fildes].offset;
    // struct FAT_entry* curr = findFirstBlock(fildes, fp);
    int bytes = nbyte;
    char* tempBuf = calloc(nbyte, 1);
    while(bytes > 0) {

        char* fullBlock = calloc(BLOCK_SIZE, 1);
        block_read(currBlock + fs.data_idx, fullBlock);
        char* actual = calloc(BLOCK_SIZE, 1);
        // char* actual = tempBuf + offset;
        size_t copy_length = (bytes < BLOCK_SIZE - offset) ? bytes : BLOCK_SIZE - offset;
        // strncpy(actual, fullBlock + offset, copy_length);
        memcpy(actual, fullBlock + offset, copy_length);
        // actual[copy_length] = '\0';
        // if(bytes < BLOCK_SIZE - offset) {
        //     memcpy(buf, actual, bytes);
        // }
        printf("blcok: %d, last letter in block: %c\n", currBlock, actual[copy_length - 1]);
        strcat(tempBuf, actual);
        bytes -= copy_length;
        offset = 0;
        free(actual);
        free(fullBlock);
        fds[fildes].offset += copy_length;
        if(FAT[currBlock] == -1) { //EOF
            break;
        }
        currBlock = FAT[currBlock];
        // free(tempBuf);
        
    }
    memcpy(buf, tempBuf, nbyte - bytes);
    // fds[fildes].offset += nbyte - bytes;
    free(tempBuf);
    // DIR[fds[fildes].file].size -= bytes;

    return nbyte - bytes;
}

int fs_write(int fildes, void *buf, size_t nbyte) {
    if(fds[fildes].used == 0) {
        return -1;
    }
    if(nbyte < 0) {
        return -1;
    }
    int fp = fds[fildes].offset;
    int offset = fp % BLOCK_SIZE;
    if(fp == MAX_LENGTH) {
        return 0;
    }

    int currBlock = starting_block(fildes);
    if(currBlock == -1) { //create FAT entry
        currBlock = new_block();
        if(currBlock == -1) { //No more space
            return 0;
        }
        fds[fildes].file = currBlock;
        DIR[fds[fildes].dir_entry].head = currBlock;
        FAT[currBlock] = -1;
    }
    
    int bytes = nbyte;
    while(bytes > 0) {
        char* tempBuf = calloc(BLOCK_SIZE, 1);
        block_read(currBlock + fs.data_idx, tempBuf);
        size_t copy_length = (bytes < BLOCK_SIZE - offset) ? bytes : BLOCK_SIZE - offset;
        // strncpy(tempBuf + offset, buf, copy_length);
        memcpy(tempBuf + offset, buf, copy_length);
        //NUll termiante
        // tempBuf[offset + copy_length] = '\0';
        bytes -= copy_length;
        block_write(currBlock + fs.data_idx, tempBuf);
        
        fds[fildes].offset += copy_length;
        offset = 0;
        free(tempBuf);
        //fix
        if(FAT[currBlock] == -1 && bytes > 0) { //create FAT entry
            FAT[currBlock] = new_block();
            if(FAT[currBlock] == -1) { //check if disk is full
                break;
            }
            FAT[FAT[currBlock]] = -1;
        }
        currBlock = FAT[currBlock];
    }
    DIR[fds[fildes].dir_entry].size += nbyte;
    // fds[fildes].offset += nbyte;
    return nbyte - bytes;
}

int fs_get_filesize(int fildes) {
    if(fds[fildes].used == 0) {
        return -1;
    }
    return DIR[fds[fildes].dir_entry].size;
}

int fs_listfiles(char ***files) {
    if(files == NULL) {
        return -1;
    }
    int i;
    int count = 0;
    for(i = 0;i < MAX_FILES;i++) {
        if(DIR[i].used == 1) {
            count++;
        }
    }
    if(count == 0) {
        return -1;
    }
    char** temp = calloc(count + 1, sizeof(char*));
    // char** temp = malloc((count + 1) * sizeof(char*));
    //allocate files with count + 1

    for(i = 0;i < MAX_FILES;i++) {
        if(DIR[i].used == 1) {
            (temp)[i] = malloc(MAX_F_NAME + 1);
            //copy string
            strcpy((temp)[i], DIR[i].name);
        }
    }
    //set last element to NULL
    (temp)[count] = NULL;
    *files = temp;
    return 0;
}

int fs_lseek(int fildes, off_t offset) {
    if(fds[fildes].used == 0) {
        return -1;
    }
    if(offset < 0 || offset > DIR[fds[fildes].dir_entry].size) {
        return -1;
    }
    fds[fildes].offset = offset;
    return 0;
}

int fs_truncate(int fildes, off_t length) {
    if(fds[fildes].used == 0) {
        return -1;
    }
    if(length < 0 || length > DIR[fds[fildes].dir_entry].size) {
        return -1;
    }
    DIR[fds[fildes].dir_entry].size = length;
    //resize
    int newBlock = length / BLOCK_SIZE + 1;
    int currBlock = fds[fildes].file;
    while(FAT[currBlock] != -1) {
        if(newBlock == 0) {
            FAT[currBlock] = -1;
        }
        else if(newBlock <= 0) {
            int temp = FAT[currBlock];
            FAT[currBlock] = 0;
            currBlock = temp;
        }
        currBlock = FAT[currBlock];
        newBlock--;
    }



    return 0;
}