#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

/* TODO: Phase 1 */
typedef struct Superblock* Sblok;
typedef struct 

struct Superblock
{
	uint8_t signature[8];
	uint8_t TotalBlocksOnDisk[2];
	uint8_t rootDir[2];
	uint8_t startIndex[2];
	uint8_t numBlocks[2];
	uint8_t numblocksFAT;
	uint8_t Padding[4079];
};

struct RootDirect
{
	uint8_t Filename[16];
	uint8_t Filesize[4];
	uint8_t FirstIndex[2];
	uint8_t Unused[10];

};

struct Disk
{	
	Sblok superblock;
	uint16_t* FAT __attribute__((packed));
	struct RootDirect rootDir[128] __attribute__((packed));
};

struct Disk* disk;
struct RootDirect* rootdir;
Sblok superblock;

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	disk = malloc(sizeof(struct Disk*));
	superblock = malloc(sizeof(Sblok));
	block_read(0,&superblock);
	disk->superblock = superblock;

	rootdir = malloc(sizeof(struct RootDirect*));
	

	if (strcmp("ECS150FS", (uint64_t)diskname ))
	{
		perror("open");
		return -1;
	}
	if (block_disk_open(diskname) == -1) {
		perror("open");
		return -1;
	}



}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	return block_disk_close();
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

