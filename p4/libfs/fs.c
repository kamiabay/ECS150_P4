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
typedef struct flatArray *FAT;

struct Superblock
{
	char signature;
	uint16_t numTotalBlocks;
	uint16_t rootIndex;
	uint16_t dataIndex;
	uint16_t numDataBlocks;
	uint8_t numFATblocks;
	uint8_t unused[PADDING];
}__attribute__((packed));

struct RootEntry
{
	uint8_t Filename[16];
	uint32_t Filesize;
	uint16_t FirstIndex;
	uint8_t unused[10];
}__attribute__((packed));

struct Disk
{
	Sblok superblock;
	uint16_t* fat;
	root rootDir[FS_FILE_MAX_COUNT];
}__attribute__((packed));

// Global Disk
disk mainDisk;

static Sblok readSuper()
{
	Sblok superblock = malloc(sizeof(Sblok));
	block_read(0, &superblock);
	return superblock;
}

static void addFat()
{
	FAT oneFat = malloc(sizeof(FAT));
}

static void readFAT()
{
	int first = 1;
	uint16_t* FAT = malloc(BLOCK_SIZE * mainDisk->superblock->numFATblocks);
	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_read(i, &FAT[(i - 1) * BLOCK_SIZE]);
	}
	mainDisk->fat = FAT;
}

static void readRoot()
{
	block_read(mainDisk->superblock->rootIndex, &mainDisk->rootDir);
}

int fs_mount(const char *diskname)
{
	if (strncmp("ECS150FS", diskname, 8))
	{
		perror("open");
		return -1;
	}
	if (block_disk_open(diskname) == -1)
	{
		perror("open");
		return -1;
	}
	mainDisk = malloc(sizeof(disk));
	mainDisk->superblock = readSuper();

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
	block_disk_write(0,mainDisk->superblock);
	for(int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_disk_write(i, mainDisk->fat[i]);
	}
	block_disk_write(mainDisk->superblock->rootIndex, mainDisk->rootDir);
	free(mainDisk);
	mainDisk = NULL;
	return 0;
}
 
int fs_info(void)
{
	/* TODO: Phase 1 */
	printf("Total blocks: %i\n",mainDisk->superblock->numTotalBlocks);
	printf("Number of FAT blocks: %i\n",mainDisk->superblock->numFATblocks);
	printf("Number of data: %i\n",mainDisk->superblock->numDataBlocks);
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
