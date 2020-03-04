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
	uint16_t FirstIndex;
	uint8_t unused[10];
};

struct __attribute__((packed)) OpenFiles
{
	char* filename;
	int offset;
	uint32_t size;
	int dataIndex;
};

struct __attribute__((packed)) Disk
{
	Sblok superblock;
	uint16_t* fat;
	root rootDir;
};

disk mainDisk = NULL; // Global Disk
fdesc FileDesc[FS_FILE_MAX_COUNT];

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
	root Root = malloc(128 * sizeof(struct RootEntry));
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
	mainDisk = malloc(sizeof(struct Disk)); // Allocate the disk
	readSuper(); // Read the superblock
	if (strncmp("ECS150FS", mainDisk->superblock->signature,8))
	{
		perror("open");
		return -1;
	}
	if (mainDisk->superblock->numTotalBlocks != block_disk_count()) // Get superblock contents
	{
		perror("open");
		return -1;
	}
	readFAT(); // Get the FAT table
	mainDisk->fat[0] = FAT_EOC; // Make first FAT block FAT_EOC
	readRoot(); // Get the root directory
	return 0;
}

int fs_umount(void)
{
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) { // Check all files are closed
		if (FileDesc[i].filename != NULL) {
			printf("Duh\n");
			return -1;
		}
	}
	block_write(0,mainDisk->superblock); // Write the superblock back
	for(int i = 1; i < mainDisk->superblock->numFATblocks + 1; i++) //Write the FAT blocks back
	{
		block_write(i, mainDisk->fat);
	}
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir); // Write root block back
	free(mainDisk); // Delete the block from the disk
	mainDisk = NULL;
	if (block_disk_close() == -1) // Close the disk
	{
		perror("close");
	}
	return 0;
}

int fs_info(void)
{
	printf("FS Info:\n");
	printf("total_blk_count=%i\n",mainDisk->superblock->numTotalBlocks);
	printf("fat_blk_count=%i\n",mainDisk->superblock->numFATblocks);
	printf("rdir_blk=%i\n", mainDisk->superblock->rootIndex);
	printf("data_blk=%i\n", mainDisk->superblock->dataIndex);
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
	int space = freeSpace(); // Find and get the slot for this file
	if (space == -1){ // Return -1 if we have no free space
		printf("Out of space\n");
		return -1;
	}
	if (sizeof(filename) >= FS_FILENAME_LEN) { // Check the length of the file
		printf("This disk aint big enough for that long ass name\n");
		return -1;
	}
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) { // Check if there is a file of the same name
			if (!strcmp(filename, mainDisk->rootDir[i].Filename)) {
				printf("Copied\n");
				return -1;
		}
	}
	mainDisk->rootDir[space].FirstIndex = FAT_EOC; // Make start index FAT_EOC
	strcpy(mainDisk->rootDir[space].Filename, filename); // Read filename into the freespace
	mainDisk->rootDir[space].Filesize = 0; // Initialize the size to zero
	block_write(mainDisk->superblock->rootIndex, mainDisk->rootDir); // Give superblock updated root directory
	return 0;
}

static int findFile(const char *filename)
{
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
			if (!strcmp(filename, mainDisk->rootDir[i].Filename)) {
				return mainDisk->rootDir[i].FirstIndex;
		}
	}
	return -1;
}

static int findFileInDisk(const char *filename)
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
	int fileInd = findFileInDisk(filename); //Get index in root directory
	if (fileInd == -1) { // Make sure filename exists
		return -1;
	}
	// for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
	// 		if (!strcmp(FileDesc[i].filename , filename)) {
	// 			return -1;
	// 	}
	// }
	mainDisk->rootDir[fileInd].Filesize = 0; // Delete
	strcpy(mainDisk->rootDir[fileInd].Filename, "\0"); // Remove the filename
	int index = mainDisk->rootDir[fileInd].FirstIndex; // Erase entry from FAT table
	printf("Size %i\n", mainDisk->rootDir[fileInd].Filesize);
	printf("Index %i\n", fileInd);
	if (mainDisk->fat[index] != FAT_EOC && mainDisk->rootDir[fileInd].Filesize != 0){
		int entry = mainDisk->fat[index];
		int oldentry;
		while(mainDisk->fat[index] != FAT_EOC) {
			oldentry = entry;
			entry = mainDisk->fat[entry];
			mainDisk->fat[oldentry] = 0;
		}
		printf("Helloooooooooooooooooooooo\n");
		mainDisk->fat[index] = 0;
		writeFAT(); // Update it to disk
	}
	return 0;
}

int fs_ls(void)
{
	printf("FS Ls:\n");
	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		if (mainDisk->rootDir[i].Filename[0] != '\0') {
			printf("file: %s, size: %i, data_blk: %i\n",mainDisk->rootDir[i].Filename,
													 mainDisk->rootDir[i].Filesize,
													 mainDisk->rootDir[i].FirstIndex );
		}
	}
	for (int i = 0; i < mainDisk->superblock->numFATblocks * 48; i++) {
		printf("fat val= %i\n", mainDisk->fat[i]);
	}
	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	int fd = 0; // initialize fd
	if (findFile(filename) == -1) {
		return -1;
	}
	int rootIndex = findFile(filename);
	int findname = 0;
	while (findname < FS_FILE_MAX_COUNT) {
		if (!strcmp(filename, mainDisk->rootDir[findname].Filename)) {
			break;
		}
		findname++;
	}
	while(fd < FS_OPEN_MAX_COUNT) {
		if (FileDesc[fd].filename == NULL) {
			FileDesc[fd].filename = (char*)filename;
			FileDesc[fd].offset = 0;
			FileDesc[fd].size = mainDisk->rootDir[findname].Filesize;
			FileDesc[fd].dataIndex = rootIndex;
			return fd;
		}
		fd++;
	}
	return -1; // It gets here when the maximum files are open
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT) {
		return -1;
	}
	if (FileDesc[fd].filename == NULL) {
		return -1;
	}
	FileDesc[fd].filename = NULL;
	FileDesc[fd].offset = 0;
	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT) {
		return -1;
	}
	if (FileDesc[fd].filename == NULL) {
		return -1;
	}
	return FileDesc[fd].size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT) {
		return -1;
	}
	if (offset > FileDesc[fd].size) {
		return -1;
	}
	if (FileDesc[fd].filename == NULL) {
		return -1;
	}
	FileDesc[fd].offset = offset;
	return 0;
}

static int findFirstFAT()
{
	for (int i = 0; i < mainDisk->superblock->numFATblocks * 2048; i++) {
		//printf("fat val= %i\n", mainDisk->fat[i]);

		if (mainDisk->fat[i] == 0) {
			return i;
		}
	}
	return -1;
}


static void updateFAT(int fd)
{
	int i = FileDesc[fd].dataIndex;
	//int init = i;
	while(mainDisk->fat[i] != FAT_EOC) {
		i = mainDisk->fat[i];
	}
	int newFreeblock = findFirstFAT();
	mainDisk->fat[i] = newFreeblock;
	mainDisk->fat[newFreeblock] = FAT_EOC;
}


int fs_write(int fd, void *buf, size_t count)
{
	count = count;
	size_t offset = FileDesc[fd].offset;
	int fatIndex = findFirstFAT();
	if (fd < 0 || fd > FS_OPEN_MAX_COUNT || FileDesc[fd].filename == NULL || fatIndex == -1)
	{
		return -1;
	}
	FileDesc[fd].dataIndex = fatIndex;
	//size_t offset = FileDesc[fd].offset;
	// double numBlocksD = count / BLOCK_SIZE;
	// int numBlocks = count / BLOCK_SIZE;
	// if ((double)numBlocks != numBlocksD) {
	// 	numBlocks++;
	// }
	int numBlocks = count / BLOCK_SIZE + 1;
		printf("blocks = %i\n", numBlocks);
		printf("fat = %i\n", fatIndex);
		void *total = malloc(BLOCK_SIZE * numBlocks);
     	void* buff = malloc(BLOCK_SIZE * numBlocks);
	//void* total = malloc(BLOCK_SIZE * numBlocks);
//	memcpy(buff, buf , count );
int currDataIndex = FileDesc[fd].dataIndex;
// double moreBlocksD = count / BLOCK_SIZE;
// int moreBlocks = count / BLOCK_SIZE;
// if ((double)moreBlocks != moreBlocksD) {
// 	moreBlocks++;
// }
int moreBlocks = count / BLOCK_SIZE + 1;
if (FileDesc[fd].size == 0) {

	while (   mainDisk->fat[currDataIndex] != FAT_EOC   )
	{
	block_write(mainDisk->superblock->dataIndex + currDataIndex, buf );
	strcat(total, buf);
	currDataIndex = mainDisk->fat[currDataIndex];
	}
	block_write(mainDisk->superblock->dataIndex + currDataIndex, buf );
	strcat(total, buf);
	memcpy(buff + offset, buf, count);


	// block_read(mainDisk->superblock->dataIndex + fatIndex, buff );
	// printf("blocks = %s\n", (char*)buff);
	mainDisk->rootDir[  findFileInDisk(FileDesc[fd].filename)  ].FirstIndex = fatIndex;

	mainDisk->fat[fatIndex] = FAT_EOC;
	if (moreBlocks > 1)
	{
		for (int i = 0; i < moreBlocks - 1; i++) {
			updateFAT(fd);
		}
	}
	FileDesc[fd].size += count;
}
else {
	for (int i = 0; i < moreBlocks; i++) {
		updateFAT(fd);
	}
}

 // for (int i = 0; i< numBlocks ; i++){
	// if (mainDisk->fat[fatIndex] == FAT_EOC) {
	// 	break;
	// }
	// //block_write(mainDisk->superblock->dataIndex + fatIndex, buf );
	// fatIndex++;
 // }
 // memcpy(buf, )
	// while (    || numBlocks != i   )
	// {
	// 	// 		printf("%i\n", fatIndex);
	// 	// printf("%i\n", mainDisk->superblock->dataIndex);
	// 	block_write(mainDisk->superblock->dataIndex + fatIndex, buff );
	// 	fatIndex++;
	// 	i++;
	// 	//strcat(buf, buff);
	// //	fatIndex = mainDisk->fat[fatIndex];
	// }
	// if (FileDesc[fd].size == 0) {
	// 	mainDisk->fat[findFAT] = FAT_EOC;
	// 	FileDesc[fd].rootIndex = findFAT;
	// }
printf("FDFFFFFFFF   %i\n", findFileInDisk(FileDesc[fd].filename));
mainDisk->rootDir[  findFileInDisk(FileDesc[fd].filename)  ].Filesize = count;

	return count;
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	if (fd < 0 || fd > FS_OPEN_MAX_COUNT)
	{
		return -1;
	}
	if (FileDesc[fd].filename == NULL)
	{
		return -1;
	}

	// double numBlocksD = count / BLOCK_SIZE;
	// int numBlocks = count / BLOCK_SIZE;
	// if ((double)numBlocks != numBlocksD) {
	// 	numBlocks++;
	// }
	int numBlocks = count / BLOCK_SIZE + 1;
	void *buff = malloc(BLOCK_SIZE * numBlocks);
	void *total = malloc(BLOCK_SIZE * numBlocks);
	size_t offset = FileDesc[fd].offset;
	int currDataIndex = FileDesc[fd].dataIndex;
	while (   mainDisk->fat[currDataIndex] != FAT_EOC   )
	{
		printf("%i\n", currDataIndex);
		block_read(mainDisk->superblock->dataIndex + currDataIndex, buff );
		strcat(total, buff);
		currDataIndex = mainDisk->fat[currDataIndex];
	}
	block_read(mainDisk->superblock->dataIndex + currDataIndex, buff );
	strcat(total, buff);
	memcpy(buf, total + offset, count);

	return count;
}
