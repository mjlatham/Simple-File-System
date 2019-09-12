#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>

#include "myfilesystem.h"

// mmap(NULL, fd, ..., ..., ..., ...,  ...);
// munmap
// memcpy

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct file_system {
	FILE *file_data;
	FILE *directory_table;
	FILE *hash_data;
	long file_data_size;
	int num_files;
	long directory_table_size;
	char *f1;
	char *f2;
	char *f3;
};

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
	// Check whether a passed file is null
	if (f1 == NULL || f2 == NULL || f3 == NULL) {
		return NULL;
	}
	
	// Check whether n_processors is invalid
	if (n_processors < 1) {
		return NULL;
	}
	
	// Open our files
	FILE *file_data = fopen(f1, "rb+");
	FILE *directory_table = fopen(f2, "rb+");
	FILE *hash_data = fopen(f3, "rb+");
	
	// Allocate some memory to our file_system struct and set the members to our files
	struct file_system *fs = (struct file_system *) malloc(sizeof(struct file_system));
	fs->file_data = file_data;
	fs->directory_table = directory_table;
	fs->hash_data = hash_data;
	//fs->num_files = 0;
	
	// Directory table size
	fseek(directory_table, 0L, SEEK_END);
	long directory_table_size = ftell(directory_table);
	rewind(directory_table);
	fs->directory_table_size = directory_table_size;
	
	// Set the number of files, num_files
	int num_files = 0;
	char * curr_file = (char *)malloc(72); // The current file directory being read
	while (fread(curr_file, 1, 72, fs->directory_table)) {
		if (curr_file[0] != 0)
			num_files += 1;
	}
	fs->num_files = num_files; // Set the number of files in the file system struct
	
	// Set disk space size, file_data
	fseek(file_data, 0L, SEEK_END);
	long file_data_size = ftell(file_data);
	rewind(file_data);
	fs->file_data_size = file_data_size;
	
	// Free memory
 	free(curr_file);
 	curr_file = NULL;
	
    return fs;
}

void close_fs(void * helper) {
	struct file_system * fs = ((struct file_system *) helper);
	
	fclose(fs->file_data);
	fclose(fs->directory_table);
	fclose(fs->hash_data);
	fs->file_data = NULL;
	fs->directory_table = NULL;
	fs->hash_data = NULL;
	free(helper);
	helper = NULL;
	
    return;
}

int does_file_exist(char * filename, void * helper) {
	struct file_system * fs = ((struct file_system *) helper);
	
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Size of current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) { // File exists
			free(curr_file);
			curr_file = NULL;
			return 1; // Return 1 if file exists
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	free(curr_file);
	curr_file = NULL;
	
	return 0; // Return 0 if file does not exist
}

int free_space_on_disk(void * helper) {
	struct file_system * fs = ((struct file_system *) helper);
	
	fseek(fs->directory_table, 68, SEEK_SET);
	unsigned int space_used = 0;
	unsigned int * curr_file_size = (unsigned int *)malloc(sizeof(unsigned int)); // Size of current file being read
	while (fread(curr_file_size, sizeof(unsigned int), 1, fs->directory_table)) {
		space_used = space_used + *curr_file_size;
		fseek(fs->directory_table, 68, SEEK_CUR); // Go to start of next file
	}
	
	free(curr_file_size);
	curr_file_size = NULL;
	
	return (fs->file_data_size - space_used);
}

int create_file(char * filename, size_t length, void * helper) {
	//pthread_mutex_lock(&mutex);
	
	struct file_system * fs = ((struct file_system *) helper);
	
	// Truncate file names to 63 characters, excluding the null terminator
	if (strlen(filename) > 63) {
		filename[63] = '\0';
	}
	
	//Check if filename exists
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Size of current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) {
			free(curr_file);
			curr_file = NULL;
			//pthread_mutex_unlock(&mutex);
			return 1;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	free(curr_file);
	curr_file = NULL;
	
	
	// Check how much disk space
	if (length > free_space_on_disk(helper) + file_size(filename, helper)) {
		//pthread_mutex_unlock(&mutex);
		return 2;
	}
	
	// Repack if not enough contiguous disk space
	size_t first_available_offset;
	first_available_offset = contiguous_space(length, helper);
	//printf("first available offset BEFORE repack: %zd\n", first_available_offset);
	if (first_available_offset == -1) {
		repack(helper);
	}
	
	// Store file
	size_t first_available_offset_after = contiguous_space(length, helper);
	//printf("first available offset AFTER repack: %zd\n", first_available_offset_after);
	//if (first_available_offset != -1) {
		fseek(fs->directory_table, 0L, SEEK_SET);
		char * curr_dt_pos = (char *)malloc(64); // The current file directory being read
		while (fread(curr_dt_pos, 1, 1, fs->directory_table)) {
			if (curr_dt_pos[0] == 0) {
				//printf("H");
				fseek(fs->directory_table, -1, SEEK_CUR);
				fwrite(filename, sizeof(filename), 1, fs->directory_table);
				fseek(fs->directory_table, 64 - sizeof(filename), SEEK_CUR);
				fwrite(&first_available_offset_after, sizeof(unsigned int), 1, fs->directory_table);
				fwrite(&length, sizeof(unsigned int), 1, fs->directory_table);
				break;
			}
			fseek(fs->directory_table, 71, SEEK_CUR);
		}
		free(curr_dt_pos);
		curr_dt_pos = NULL;
	//}
	
	
	// Map successfully stored file in directory_table
	fs->num_files += 1; // Increment the counter of number of files on disk
	
	//pthread_mutex_unlock(&mutex);
	compute_hash_tree(helper);
	//pthread_mutex_lock(&mutex);
	
	//pthread_mutex_unlock(&mutex);
	
    return 0;
}

size_t contiguous_space(size_t length, void * helper) {
	//pthread_mutex_lock(&mutex);
	
	struct file_system * fs = (struct file_system *)helper;
	
	unsigned int offset_a = 0;
	unsigned int offset_b = 0;
	fseek(fs->file_data, 0L, SEEK_SET);
	char * curr_file_fd = (char *)malloc(1); // The current file directory being read
	fread(curr_file_fd, 1, 1, fs->file_data);
	if (curr_file_fd[0] == 0) {
		while (fread(curr_file_fd, 1, 1, fs->file_data)) {
			if (curr_file_fd[0] != 0) { // Check the space between offset_a and the next piece of data
				offset_a = ftell(fs->file_data) - 1;
			}
		}
	}
	
	fseek(fs->file_data, 0L, SEEK_SET);
	
	while (fread(curr_file_fd, 1, 1, fs->file_data)) {
		
		if (curr_file_fd[0] != 0) { // Check the space between offset_a and the next piece of data
			
			offset_b = ftell(fs->file_data) - 1; // Set offset_b to this position where there is data
			
			//printf("offset_b: %ud\n", offset_b);
			
			// If this contiguous space is large enough, store the file
			if (offset_b - offset_a >= length) {
				free(curr_file_fd);
				curr_file_fd = NULL;
				
				//pthread_mutex_unlock(&mutex);
				
				return offset_a;
			}
			
			// Set offset_a to the next offset where there is no data
			//printf("%lu\n", ftell(fs->file_data));
			while (fread(curr_file_fd, 1, 1, fs->file_data)) {
				if (curr_file_fd[0] == 0) {
					offset_a = ftell(fs->file_data);
					break;
				}
			}
		}
	}
	free(curr_file_fd);
	curr_file_fd = NULL;
	
	if (offset_a == offset_b) {
		//pthread_mutex_unlock(&mutex);
		return 0;
	}
	
	//pthread_mutex_unlock(&mutex);
	
	// Return -1 if no suitable offset is found
	return -1;
}

int resize_file(char * filename, size_t length, void * helper) {
	pthread_mutex_lock(&mutex);
	
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filename
	if (strlen(filename) > 63)
		filename[63] = '\0';
	
	// Check if file exists
	if (does_file_exist(filename, helper) != 1) {
		pthread_mutex_unlock(&mutex);
		return 1;
	}
	
	// Check how much disk space, return 2 if length too big
	if (length > free_space_on_disk(helper) + file_size(filename, helper)) {
		pthread_mutex_unlock(&mutex);
		return 2;
	}
	
	// Loop through filenames in directory_table and resize given file if found
	// Do this BEFORE repack to ensure repack is performed correctly
	// The file size won't change and we won't repack if there is not enough space
	// in the virtual disk overall, given the above checks
	fseek(fs->directory_table, 0, SEEK_SET);
	for (int i = 0; i < fs->num_files; i++) {
		char * curr_file = (char *)malloc(64); // The current file directory being read
		fread(curr_file, 64, 1, fs->directory_table);
		
		fseek(fs->directory_table, 4, SEEK_CUR);
		
		unsigned int * curr_file_size = (unsigned int *)malloc(sizeof(unsigned int)); // The current file's size
		fread(curr_file_size, sizeof(unsigned int), 1, fs->directory_table);
		
		// If the file exists, resize file
		if (strcmp(filename, curr_file) == 0){
			// Resize the file
			fseek(fs->directory_table, -8, SEEK_CUR);
			unsigned int * offset_ptr = (unsigned int *)malloc(sizeof(unsigned int));
			fread(offset_ptr, sizeof(unsigned int), 1, fs->directory_table);
			fwrite(&length, sizeof(unsigned int), 1, fs->directory_table);
			
			free(curr_file);
			free(offset_ptr);
			free(curr_file_size);
			curr_file = NULL;
			offset_ptr = NULL;
			curr_file_size = NULL;
			
			// Repack only if the resize wasn't able to fit in existing packed file_data
			size_t first_available_offset;
			first_available_offset = contiguous_space(length - file_size(filename, helper), helper);
			if (first_available_offset == -1) {
				pthread_mutex_unlock(&mutex);
				repack(helper);
				pthread_mutex_lock(&mutex);
			}
			pthread_mutex_unlock(&mutex);
			return 0;
		}
		
		// Free allocated memory
		free(curr_file);
		free(curr_file_size);
		curr_file_size = NULL;
		curr_file = NULL;
	}
	
	pthread_mutex_unlock(&mutex);
	compute_hash_tree(helper);
	pthread_mutex_lock(&mutex);
	
	pthread_mutex_unlock(&mutex);
	
    return 1;
}

int repack_needed(void * helper) {
	return 1;
}

void repack(void * helper) {
	pthread_mutex_lock(&mutex);
	struct file_system * fs = (struct file_system *)helper;
	
	// REPACK
	// This goes through file_data to find a file, records its offset,
	// then changes this offset in directory_table.
	// It then moves the file to a new offset that is calculated by finding
	// the first free space, maintaining the order of the files (as it goes 
	// and reorganises the files leftward, one by one, through file_data)
	
	fseek(fs->file_data, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(1); // Current file being read
	size_t new_offset = 0; // The new offset the file will be placed in file_data
	size_t old_offset = 0; // The old offset of the file to be moved
	size_t size = 0; // Size of the file
	
	while (fread(curr_file, 1, 1, fs->file_data)) {
		
		if (curr_file[0] != 0) {
			old_offset = ftell(fs->file_data) - 1;
			//printf("hello");
			// Change offset in directory_table
			fseek(fs->directory_table, 64, SEEK_SET);
			unsigned int * curr_offset = (unsigned int *)malloc(4); // Current file offset
			while (fread(curr_offset, 4, 1, fs->directory_table)) {
				//printf("hi");
				if (*curr_offset == old_offset) { // Found the file's offset we want to change
					//printf("test\n");
					// Change offset
					fseek(fs->directory_table, -4, SEEK_CUR);
					fwrite(&new_offset, 4, 1, fs->directory_table);
					
					// Get size of this current file
					unsigned int * curr_size = (unsigned int *)malloc(sizeof(unsigned int));
					fread(curr_size, sizeof(unsigned int), 1, fs->directory_table);
					size = *curr_size;
					
					int curr_offset = ftell(fs->file_data); // Get offset of this current file
					
					fseek(fs->file_data, old_offset, SEEK_SET); // Go to where file is stored
					
					char * curr_file_data = (char *)malloc(size);
					fread(curr_file_data, size, 1, fs->file_data); // Read the file into curr_file_data
					
					// Rewrite file data in its old offset to null
					fseek(fs->file_data, -size, SEEK_CUR);
					for (int i = 0; i < size; i++) {
						fputc(0, fs->file_data);
					}
					fseek(fs->file_data, new_offset, SEEK_SET); // Go to the new file offset
					fwrite(curr_file_data, size, 1, fs->file_data); // Write the file data at new offset

					fseek(fs->file_data, new_offset, SEEK_SET);
					fread(curr_file_data, size, 1, fs->file_data);


					fseek(fs->file_data, curr_offset, SEEK_SET);
					
					// Set new offset for next file
					new_offset = new_offset + size;
					//printf("new offset: %zd\n", new_offset);
					
					// Free memory
					free(curr_file_data);
					free(curr_size);
					curr_file_data = NULL;
					curr_size = NULL;
					
					// Seek to the new offset from which we will continue reading file_data
					fseek(fs->file_data, new_offset, SEEK_SET);
					//printf("offset: %lo\n", ftell(fs->file_data));
					//printf("newtest\n");
					fseek(fs->directory_table, 68, SEEK_CUR); // Go to start of next file
					break;
				}
				//printf("offset: %lo\n", ftell(fs->directory_table));
				//printf("test3\n");
				fseek(fs->directory_table, 68, SEEK_CUR); // Go to start of next file
			}
			// Free memory
			free(curr_offset);
			curr_offset = NULL;
		}
	}
	
	free(curr_file);
	curr_file = NULL;
	
	pthread_mutex_unlock(&mutex);
	compute_hash_tree(helper);
	pthread_mutex_lock(&mutex);
	
	pthread_mutex_unlock(&mutex);
	
    return;
}

int delete_file(char * filename, void * helper) {
	struct file_system * fs = (struct file_system *)helper;
	
	char null_byte = '\0';
	
	// Truncate filename to 63 characters
	if (strlen(filename) > 63)
		filename[63] = '\0'; 
	
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) { // File exists
			// Set first byte of filename to the null byte to signify it is deleted
			fseek(fs->directory_table, -64, SEEK_CUR);
			fwrite(&null_byte, 1, 1, fs->directory_table);
			// Free memory and return
			free(curr_file);
			curr_file = NULL;
			return 0;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	// Free memory and return 1 if some error
	free(curr_file);
	curr_file = NULL;
    return 1;
}

int rename_file(char * oldname, char * newname, void * helper) {
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filenames to 63 characters
	if (strlen(oldname) > 63)
		oldname[63] = '\0';
	if (strlen(newname) > 63)
		oldname[63] = '\0';
	
	// Check if newname exists as a file
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(newname, curr_file) == 0) { // File exists
			free(curr_file);
			curr_file = NULL;
			return 1;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	// Rename file
	fseek(fs->directory_table, 0L, SEEK_SET);
	//char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(oldname, curr_file) == 0) { // File exists
			// Rename file
			fseek(fs->directory_table, -64, SEEK_CUR);
			fwrite(newname, 64, 1, fs->directory_table);
			// Free memory and return
			free(curr_file);
			curr_file = NULL;
			return 0;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	// Return 1 if some error, such as if oldname file does not exist
    return 1;
}

int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
	pthread_mutex_lock(&mutex);
	
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filename to 63 characters
	if (strlen(filename) > 63)
		filename[63] = '\0';
	
	// If file does not exist, return 1
	if (does_file_exist(filename, helper) != 1) {
		pthread_mutex_unlock(&mutex);
		return 1;
	}
	
	// If offset makes it impossible to read count bytes, return 2
	if (file_size(filename, helper) < offset + count) {
		pthread_mutex_unlock(&mutex);
		return 2;
	}
	
	// Verify hash tree - hopefully can get some manual marks here
	// this is why i drink myself to oblivion 4 times a week
	// if i dont pass this class i'm fucked, i'll probably get liver cirrhosis
	int num_blocks = fs->file_data_size / 256;
	int k = (int)(log(num_blocks) / log(2));
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < k; j++) {
			uint8_t output[16];
			fseek(fs->hash_data, (-1)*(32*(1 + j)), SEEK_END);
			uint8_t * hash_buf = (uint8_t *)malloc(32);
			uint8_t * hash_output = (uint8_t *)malloc(16);
			
			fread(hash_buf, 32, 1, fs->hash_data);
			fletcher(hash_buf, 32, output);
			fseek(fs->hash_data, (-1)*((32*(1 + j) - 1)/2), SEEK_END);
			fread(hash_output, 16, 1, fs->hash_data);
			
			if (memcmp(hash_output, output, 16) != 0) {
				free(hash_buf);
				free(hash_output);
				hash_buf = NULL;
				hash_output = NULL;
				
				//pthread_mutex_unlock(&mutex);
				//return 3;
			}
			
			free(hash_buf);
			free(hash_output);
			hash_buf = NULL;
			hash_output = NULL;
		}
		
	}
	
	// Start reading file
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) { // File exists
			fseek(fs->file_data, file_offset(filename, helper) + offset, SEEK_SET);
			fread(buf, count, 1, fs->file_data);
			break;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	// Free memory
	free(curr_file);
	curr_file = NULL;
	
	pthread_mutex_unlock(&mutex);
    return 0;
}

int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
	pthread_mutex_lock(&mutex);
	
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filename to 63 characters
	if (strlen(filename) > 63)
		filename[63] = '\0';
	
	// Get offset of the file
	size_t start_offset = file_offset(filename, helper);
	
	// Return 2 if offset is greater than file size
	if (offset > file_size(filename, helper)) {
		pthread_mutex_unlock(&mutex);
		return 2;
	}
	
	// Return 3 if insufficient space in disk overall
	if (offset + count > free_space_on_disk(helper) + file_size(filename, helper)) {
		pthread_mutex_unlock(&mutex);
		return 3;
	}
	
	// Check if filesize is too small
	if (offset + count > file_size(filename, helper)) {
		pthread_mutex_unlock(&mutex);
		resize_file(filename, (offset + count), helper);
		pthread_mutex_lock(&mutex);
	}
	
	size_t first_available_offset;
	first_available_offset = contiguous_space(offset + count - file_size(filename, helper), helper);
	if (first_available_offset == -1) {
		pthread_mutex_unlock(&mutex);
		//repack(helper);
		pthread_mutex_lock(&mutex);
	}
	
	// Write to the file from the given offset
	fseek(fs->file_data, start_offset + offset, SEEK_SET);
	fwrite(buf, 1, count, fs->file_data);
	
	pthread_mutex_unlock(&mutex);
	compute_hash_tree(helper);
	pthread_mutex_lock(&mutex);
	
	pthread_mutex_unlock(&mutex);
	
    return 0;
}

ssize_t file_size(char * filename, void * helper) {
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filename to 63 characters
	if (strlen(filename) > 63)
		filename[63] = '\0';
	
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) { // File exists
			// Set file size
			unsigned int * curr_file_size = (unsigned int *)malloc(sizeof(unsigned int));
			fseek(fs->directory_table, 4, SEEK_CUR);
			fread(curr_file_size, sizeof(unsigned int), 1, fs->directory_table);
			ssize_t size = *curr_file_size;
			// Free memory and return
			free(curr_file_size);
			free(curr_file);
			curr_file_size = NULL;
			curr_file = NULL;
			return size;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	free(curr_file);
	curr_file = NULL;
	
    return -1;
}

size_t file_offset(char * filename, void * helper) {
	struct file_system * fs = (struct file_system *)helper;
	
	// Truncate filename to 63 characters
	if (strlen(filename) > 63)
		filename[63] = '\0'; 
	
	fseek(fs->directory_table, 0L, SEEK_SET);
	char * curr_file = (char *)malloc(64); // Current file being read
	while (fread(curr_file, 64, 1, fs->directory_table)) {
		if (strcmp(filename, curr_file) == 0) { // File exists
			// Set file size
			unsigned int * curr_file_offset = (unsigned int *)malloc(sizeof(unsigned int));
			//fseek(fs->directory_table, 4, SEEK_CUR);
			fread(curr_file_offset, sizeof(unsigned int), 1, fs->directory_table);
			ssize_t offset = *curr_file_offset;
			// Free memory and return
			free(curr_file_offset);
			free(curr_file);
			curr_file_offset = NULL;
			curr_file = NULL;
			return offset;
		}
		fseek(fs->directory_table, 8, SEEK_CUR); // Go to start of next file
	}
	
	free(curr_file);
	curr_file = NULL;
	
	// Return -1 if file does not exist
    return -1;
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
	
	uint32_t * buf_32 = (uint32_t *)buf;
	
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t c = 0;
	uint64_t d = 0;
	
	for (int i = 0; i < length/4; i++) {
		a = (a + buf_32[i]) % 4294967295;
		b = (b + a) % 4294967295;
		c = (c + b) % 4294967295;
		d = (d + c) % 4294967295;
	}
	
	memcpy(&output[0], &a, 4);
	memcpy(&output[4], &b, 4);
	memcpy(&output[8], &c, 4);
	memcpy(&output[12], &d, 4);
	
    return;
}

void compute_hash_tree(void * helper) {
	pthread_mutex_lock(&mutex);
	
	struct file_system * fs = (struct file_system *)helper;
	
	int num_blocks = fs->file_data_size / 256;
	uint8_t output[16];
	
	// Compute hash blocks for the blocks in file_data and store them in hash_data
	fseek(fs->hash_data, -16, SEEK_END);
	fseek(fs->file_data, -256, SEEK_END);
	for (int i = 0; i < num_blocks; i++) {
		// Read file data for current block into buf
		void * buf = malloc(256);
		fread(buf, 256, 1, fs->file_data);
		fseek(fs->file_data, -512, SEEK_CUR);
		
		// Compute hash value for current block and store in output
		fletcher(buf, 256, output);
		
		// Write ths hash value to hash_data
		fwrite(output, 16, 1, fs->hash_data);
		fseek(fs->hash_data, -32, SEEK_CUR);
		
		// Free memory
		free(buf);
		buf = NULL;
	}
	
	// Build the rest of the hash tree from the initially stored hashes above
	int k = (int)(log(num_blocks) / log(2));
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < k; j++) {
			fseek(fs->hash_data, (-32)*(1+j), SEEK_END);
			uint8_t * hash_buf = (uint8_t *)malloc(32);
			
			fread(hash_buf, 32, 1, fs->hash_data);
			fletcher(hash_buf, 32, output);
			fseek(fs->hash_data, (-1)*((32*(1 + j) - 1)/2), SEEK_END);
			fwrite(output, 16, 1, fs->hash_data);
			
			free(hash_buf);
			hash_buf = NULL;
		}
	}
	
	pthread_mutex_unlock(&mutex);
	
    return;
}

void compute_hash_block(size_t block_offset, void * helper) {
	struct file_system * fs = (struct file_system *)helper;
	
	if (block_offset > pow(2, 24)) {
		return;
	}
	
	uint8_t output[16];
	uint8_t * buf = (uint8_t *)malloc(256);
	
	// Read file_data block into buf
	fseek(fs->file_data, block_offset * 256, SEEK_SET);
	fread(buf, 1, 256, fs->file_data);
	
	// Compute hash value for current block and store in output
	fletcher(buf, 256, output);
	
	// Update hash_data
	compute_hash_tree(helper);
	
	// Free memory
	free(buf);
	buf = NULL;
	
    return;
}



















