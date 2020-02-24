#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define PADDING 4079
typedef struct Superblock *Sblok;
typedef struct RootDirect *root;
typedef struct Disk *disk;
typedef struct flatArray *FAT;

struct Superblock
{
	uint64_t signature;
	uint16_t numTotalBlocks;
	uint16_t rootIndex;
	uint16_t dataIndex;
	uint16_t numDataBlocks;
	uint8_t numFATblocks;
	uint8_t unused[PADDING];
};

struct flatArray
{
	struct flatArray *next;
	uint16_t *value;
};

struct RootDirect
{
	uint8_t Filename[16];
	uint32_t Filesize;
	uint16_t FirstIndex;
	uint8_t unused[10];
};

struct Disk
{
	Sblok superblock;
	FAT fat;
	root rootDir;
};

disk mainDisk;
root rootdir;

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
static void readFAT(Sblok super)
{
	int first = 1;
	struct flatArray *arr[super->numFATblocks];
	//void * fatArray = malloc(sizeof(struct flatArray ) * super->numFATblocks);
	//mainDisk->fat = fatArray;
	for (int i = 1; i < super->numFATblocks + 1; i++)
	{
		arr[i] = malloc(sizeof(struct flatArray));
		//FAT oneFat = malloc(sizeof(FAT));
		block_read(i, &arr[i - 1]->value);
	}
}
static root readRoot(Sblok super)
{
	rootdir = malloc(sizeof(root));
	block_read(super->rootIndex, rootdir);
	return rootdir;
}

int fs_mount(const char *diskname)
{
	block_disk_open(diskname);
	mainDisk = malloc(sizeof(disk));
	mainDisk->superblock = readSuper();
	readFAT(mainDisk->superblock);
	mainDisk->rootDir = readRoot(mainDisk->superblock);
	//readData();

	if (strcmp("ECS150FS", (uint64_t)diskname))
	{
		perror("open");
		return -1;
	}
	if (block_disk_open(diskname) == -1)
	{
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
