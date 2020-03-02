#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define PADDING 4079
#define
FAT_EOC 0xFFFF typedef struct Superblock *Sblok;
typedef struct RootEntry *root;
typedef struct OpenFiles fdesc;
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
	uint16_t FirstIndexInFat; /// first index of the fat table
	uint8_t unused[10];
};

struct __attribute__((packed)) OpenFiles
{
	char *filename;
	size_t offset;
	uint32_t size;
	int rootIndex;
};

struct __attribute__((packed)) Disk
{
	Sblok superblock;
	uint16_t *fat;
	root rootDir;
	int numFree = 128;
	int numFreeFATS;
};

disk mainDisk = NULL; // Global Disk
fdesc FileDesc[FS_FILE_MAX_COUNT];

static void readSuper()
{
	Sblok superblock = malloc(BLOCK_SIZE);
	if (block_read(0, superblock) == -1)
	{
		printf("read this failed\n");
	}
	mainDisk->superblock = superblock;
}

static void readFAT()
{
	uint16_t *FAT = malloc(BLOCK_SIZE * mainDisk->superblock->numFATblocks * sizeof(uint16_t));
	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_read(i, &FAT[(i - 1) * BLOCK_SIZE]);
	}
	mainDisk->fat = FAT;
}

static void readRoot()
{
	root Root = malloc(128 * sizeof(struct RootEntry));
	block_read(mainDisk->superblock->rootIndex, Root);
	mainDisk->rootDir = Root;
}

static void writeFAT()
{
	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++)
	{
		block_write(i, &mainDisk->fat[(i - 1) * BLOCK_SIZE]);
	}
}

int fs_mount(const char *diskname)
{
	if (block_disk_open(diskname) == -1)
	{
		perror("open");
		return -1;
	}
	mainDisk = malloc(sizeof(struct Disk)); // Allocate the disk
	readSuper();							// Read the superblock
	if (strncmp("ECS150FS", mainDisk->superblock->signature, 8))
	{
		perror("open");
		return -1;
	}
	if (mainDisk->superblock->numTotalBlocks != block_disk_count()) // Get superblock contents
	{
		perror("open");
		return -1;
	}
	readFAT();					// Get the FAT table
	mainDisk->fat[0] = FAT_EOC; // Make first FAT block FAT_EOC //Remove
	readRoot();					// Get the root directory
	return 0;
}

int fs_umount(void)
{
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{ // Check all files are closed
		if (mainDisk->open[i] == 1)
		{
			return -1;
		}
	}
	block_write(0, mainDisk->superblock);							 // Write the superblock back
	for (int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++) //Write the FAT blocks back
	{
		block_write(i, mainDisk->fat);
	}
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir); // Write root block back
	free(mainDisk);													 // Delete the block from the disk
	mainDisk = NULL;
	if (block_disk_close() == -1) // Close the disk
	{
		perror("close");
	}
	return 0;
}

int fs_info(void)
{
	printf("FS Info\n");
	printf("total_blk_count= %i\n", mainDisk->superblock->numTotalBlocks);
	printf("fat_blk_count= %i\n", mainDisk->superblock->numFATblocks);
	printf("rdir_blk= %i\n", mainDisk->superblock->rootIndex);
	printf("data_blk= %i\n", mainDisk->superblock->dataIndex);
	printf("data_blk_count=%i\n", mainDisk->superblock->numDataBlocks);
	int freefats = 0;
	for (int i = 0; i < mainDisk->superblock->numDataBlocks; i++)
	{
		if (mainDisk->fat[i] == 0)
		{
			freefats++;
		}
	} /// fat struct
	printf("fat_free_ratio=%i/%i\n", freefats, mainDisk->superblock->numDataBlocks);
	printf("rdir_free_ratio=%i/%i\n", mainDisk->numFree, FS_FILE_MAX_COUNT);
	return 0;
}

static int findFreeSpace()
{
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (mainDisk->rootDir[i].Filename[0] == '\0')
		{
			return i;
		}
	}
	return -1;
}

int fs_create(const char *filename)
{
	if (sizeof(filename) >= FS_FILENAME_LEN)
	{ // Check the length of the file
		printf("This disk aint big enough for that long ass name\n");
		return -1;
	}
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{ // Check if there is a file of the same name
		if (!strcmp(filename, mainDisk->rootDir[i].Filename))
		{
			printf("Copied\n");
			return -1;
		}
	}
	int index = findFreeSpace(); // Find and get the slot for this file
	if (index == -1)
	{ // Return -1 if we have no free space
		printf("Out of space\n");
		return -1;
	}
	mainDisk->numFree--;
	mainDisk->emptyRootIndex++;
	mainDisk->rootDir[index].FirstIndexInFat = FAT_EOC;				 // Make start index FAT_EOC
	strcpy(mainDisk->rootDir[index].Filename, filename);			 // Read filename into the freespace
	mainDisk->rootDir[index].Filesize = 0;							 // Initialize the size to zero
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir); // Give superblock updated root directory
	return 0;
}

static int findRootIndex(const char *filename)
{
	int rootIndex = 0;
	while (rootIndex < FS_FILE_MAX_COUNT) // goes thorugh all 128 root enteries
	{
		if (!strcmp(filename, mainDisk->rootDir[rootIndex].Filename))
		{
			return rootIndex;
		}
		rootIndex++;
	}
	return -1;
}

int fs_delete(const char *filename)
{
	int rootIndex = findRootIndex(filename); //Get index in root directory
	if (rootIndex == -1)					 // Make sure filename exists
		return -1;
	// if(mainDisk->open[fileInd] == 1) { // Make sure file is closed
	// 	return -1;
	// }
	mainDisk->rootDir[rootIndex].Filesize = 0;				  // Delete
	strcpy(mainDisk->rootDir[rootIndex].Filename, "\0");	  // Remove the filename
	int index = mainDisk->rootDir[rootIndex].FirstIndexInFat; // Erase entry from FAT table
	if (index != FAT_EOC)
	{
		uint16_t entry = mainDisk->fat[index]; // Seg fault occur here
		uint16_t oldentry;
		while (mainDisk->fat[index] != FAT_EOC)
		{
			oldentry = entry;
			entry = mainDisk->fat[entry];
			mainDisk->fat[oldentry] = FAT_EOC;
		}
		mainDisk->fat[index] = FAT_EOC;
		mainDisk->numFree++;
		writeFAT(); // Update it to disk
	}
	return 0;
}

int fs_ls(void)
{
	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (mainDisk->rootDir[i].Filename[0] != '\0')
		{
			printf("file: %s, size: %i, data_blk: %i\n", mainDisk->rootDir[i].Filename,
				   mainDisk->rootDir[i].Filesize,
				   mainDisk->rootDir[i].FirstIndexInFat);
		}
	}
	return 0;
}

int fs_open(const char *filename)
{
	if (findRootIndex(filename) == -1)
		return -1;
	int rootIndex = findRootIndex(filename);
	int fd = 0; // initialize fd
	while (fd < FS_OPEN_MAX_COUNT)
	{
		if (FileDesc[fd].filename == NULL) // first empty slot
		{
			FileDesc[fd].filename = (char *)filename;
			FileDesc[fd].offset = 0;
			FileDesc[fd].size = mainDisk->rootDir[rootIndex].Filesize;
			FileDesc[fd].rootIndex = rootIndex;
			return fd;
		}
		fd++;
	}
	return -1; // It gets here when the maximum files are open
}

int fs_close(int fd)
{
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	if (FileDesc[fd].filename == NULL)
	{
		return -1;
	}
	FileDesc[fd].filename = "\0";
	FileDesc[fd].offset = 0;
	FileDesc[fd].size = 0;
	FileDesc[fd].rootIndex = -1;
	return 0;
}

int fs_stat(int fd)
{
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	if (FileDesc[fd].filename == NULL)
	{
		return -1;
	}
	return FileDesc[fd].size;
}

int fs_lseek(int fd, size_t offset)
{
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	if (offset > FileDesc[fd].size)
	{
		return -1;
	}
	if (FileDesc[fd].filename == NULL)
	{
		return -1;
	}
	FileDesc[fd].offset = offset;
	return 0;
}

int fs_write(__attribute__((unused)) int fd, __attribute__((unused)) void *buf, __attribute__((unused)) size_t count)
{

	return 0;
}

static void readData()
{
}
static int dataBlockSpan(size_t offset, size_t count)
{
}
int fs_read(int fd, void *buf, size_t count)
{

	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	if (FileDesc[fd].filename == NULL)
	{
		return -1;
	}
	void *buff;
	size_t offset = FileDesc[fd].offset;
	int FirstDataIndex = FileDesc[fd].rootIndex;
	size_t readSize = offset + count;
	int span = dataBlockSpan(offset, count);
	while ()
	{
		block_read(count, );
	}

	return 0;
}
