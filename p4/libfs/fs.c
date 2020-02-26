#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define PADDING 4079
typedef struct Superblock *Sblok;
typedef struct RootEntry *root;
typedef struct Disk *disk;
// make clean
// ./fs_make.x disk.fs 4096
// make
// ./test_fs.x info disk.fs

// make clean & ./fs_make.x disk.fs 4096 & make

struct __attribute__((packed)) Superblock
{
	char signature[8];
	uint16_t numTotalBlocks;
	uint16_t rootIndex;
	uint16_t dataIndex;
	uint16_t numDataBlocks;
	uint8_t numFATblocks;
	uint8_t unused[PADDING];
};

struct __attribute__((packed)) RootEntry
{
	uint8_t Filename[16];
	uint32_t Filesize;
	uint16_t FirstIndex;
	uint8_t unused[10];
};

struct __attribute__((packed)) Disk
{
	Sblok superblock;
	uint16_t* fat;
	root rootDir;
};

// Global Disk
disk mainDisk = NULL;
static void readSuper()
{

	Sblok superblock = malloc(BLOCK_SIZE);
	if (block_read(0,superblock) == -1){
		printf("read this failed\n");
	}
	mainDisk->superblock = superblock;

}

static void readFAT()
{
	uint16_t* FAT = malloc(BLOCK_SIZE * mainDisk->superblock->numFATblocks * sizeof(uint16_t));

	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_read(i, &FAT[(i - 1) * BLOCK_SIZE]);
	}
	mainDisk->fat = FAT;
}

static void readRoot()
{
	root Root = malloc(BLOCK_SIZE * sizeof(uint8_t));
	block_read(mainDisk->superblock->rootIndex, Root);
	mainDisk->rootDir = Root;
}

int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) == -1)
	{
		perror("open");
		return -1;
	}

	// Allocate the disk
	mainDisk = malloc(sizeof(struct Disk));
	// Read the superblock
	readSuper();

	if (strncmp("ECS150FS", mainDisk->superblock->signature,8))
	{
		perror("open");
		return -1;
	}

	// Get superblock contents
	if (mainDisk->superblock->numTotalBlocks != block_disk_count())
	{
		perror("open");
		return -1;
	}

	// Get the FAT table
	readFAT();

	// Get the root directory
	readRoot();

	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	// Write the superblock back
	block_write(0,mainDisk->superblock);

	//Write the FAT blocks back
	for(int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_write(i, mainDisk->fat);
	}

	// Write root block back
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir);

	// Delete the block from the disk
	free(mainDisk);
	mainDisk = NULL;

	// Close the disk
	if (block_disk_close() == -1)
	{
		perror("close");
	}
	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
	printf("FS Info\n");
	printf("total_blk_count= %i\n",mainDisk->superblock->numTotalBlocks);
	printf("fat_blk_count= %i\n",mainDisk->superblock->numFATblocks);
	printf("rdir_blk= %i\n", mainDisk->superblock->rootIndex);
	printf("data_blk= %i\n", mainDisk->superblock->dataIndex);
	printf("data_blk_count=%i\n",mainDisk->superblock->numDataBlocks);
	int freefats = 0;
	int freeroot = 0;
	for (int i = 0; i < mainDisk->superblock->numDataBlocks; i++) {
		if (mainDisk->fat[i] == 0) {
			freefats++;
		}
	}
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (mainDisk->rootDir[i].Filename[0] == '\0') {
			freeroot++;
		}
	}
	printf("fat_free_ratio=%i/%i\n",freefats,mainDisk->superblock->numDataBlocks);
	printf("rdir_free_ratio=%i/%i\n",freeroot,FS_FILE_MAX_COUNT);
	return 0;
}

int fs_create(__attribute__((unused))const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_delete(__attribute__((unused))const char *filename)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	return 0;
}

int fs_open(__attribute__((unused))const char *filename)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_close(__attribute__((unused))int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_stat(__attribute__((unused))int fd)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_lseek(__attribute__((unused))int fd, __attribute__((unused))size_t offset)
{
	/* TODO: Phase 3 */
	return 0;
}

int fs_write(__attribute__((unused))int fd, __attribute__((unused))void *buf, __attribute__((unused))size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}

int fs_read(__attribute__((unused))int fd,__attribute__((unused)) void *buf, __attribute__((unused))size_t count)
{
	/* TODO: Phase 4 */
	return 0;
}
