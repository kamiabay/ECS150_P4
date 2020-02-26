#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define PADDING 4079
#define FAT_EOC 0xFFFF
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
	char Filename[FS_FILENAME_LEN];
	uint32_t Filesize;
	uint16_t FirstIndex;
	uint8_t unused[10];
};

struct __attribute__((packed)) Disk
{
	Sblok superblock;
	uint16_t* fat;
	root rootDir;
	int open[128];
};

// Global Disk
disk mainDisk = NULL;

// Array for open files


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

static void writeFAT()
{
	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++) {
		block_write(i ,&mainDisk->fat[(i - 1) * BLOCK_SIZE]);
	}
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

	// Make first FAT block FAT_EOC
	mainDisk->fat[0] = FAT_EOC;

	// Get the root directory
	readRoot();

	// All files start off as closed
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		mainDisk->open[i] = 0;
	}

	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	// Check all files are closed
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (mainDisk->open[i] == 1) {
			return -1;
		}
	}

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

static int freeSpace()
{
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (mainDisk->rootDir[i].Filename[0] == '\0') {
			return i;
		}
	}
	return -1;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */

	// Get the slot for this file
	int space = freeSpace();

	// Return -1 if we have no free space
	if (space == -1){
		printf("Out of space\n");
		return -1;
	}

	// Check the length of the file
	if (sizeof(filename) >= FS_FILENAME_LEN) {
		printf("This disk aint big enough for that long ass name\n");
		return -1;
	}

	// Check if there is a file of the same name
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
			if (!strcmp(filename, mainDisk->rootDir[i].Filename)) {
				printf("Copied\n");
				return -1;
		}
	}

	// Make start index FAT_EOC
	mainDisk->rootDir[space].FirstIndex = FAT_EOC;

	// Read filename into the freespace
//	for (int i = 0; i < FS_FILENAME_LEN; i++)
		strcpy(mainDisk->rootDir[space].Filename, filename);
		//mainDisk->rootDir[space].Filename[i] = filename[i];

	// Initialize the size to zero
	mainDisk->rootDir[space].Filesize = 0;

	// Give superblock updated root directory
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir);

	// Find FAT space
	while (mainDisk->fat[mainDisk->rootDir[space].FirstIndex] != 0) {
		mainDisk->rootDir[space].FirstIndex++;
		if (mainDisk->rootDir[space].FirstIndex) { // No space on FAT
			return -1;
		}
	}
	mainDisk->fat[mainDisk->rootDir[space].FirstIndex] = FAT_EOC;

	// Give disk updated FAT
	writeFAT();


	return 0;
}

static int findFile(const char *filename)
{
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
			if (!strcmp(filename, mainDisk->rootDir[i].Filename)) {
				return i;
		}
	}
	return -1;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */

	//Get index in root directory
	int fileInd = findFile(filename);

	// Make sure filename exists
	if (fileInd == -1) {
		return -1;
	}

	// Make sure file is closed
	if(mainDisk->open[fileInd] == 1) {
		return -1;
	}

	// Delete
	mainDisk->rootDir[fileInd].Filesize = 0;

	// Remove the filename
	for(int i = 0; i < FS_FILENAME_LEN; i++) {
		mainDisk->rootDir[fileInd].Filename[i] = '\0';
	}

	// Erase entry from FAT table
	int index = mainDisk->rootDir[fileInd].FirstIndex;
	int entry = mainDisk->fat[index];
	int oldentry;
	while(mainDisk->fat[index] != FAT_EOC) {
		oldentry = entry;
		entry = mainDisk->fat[entry];
		mainDisk->fat[oldentry] = 0;
	}
	mainDisk->fat[index] = 0;

	// Update it to disk
	writeFAT();


	return 0;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	printf("FS Ls:\n");
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (mainDisk->rootDir[i].Filename[0] != '\0') {
			printf("%s ",mainDisk->rootDir[i].Filename );
		}
	}
	printf("\n");
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
