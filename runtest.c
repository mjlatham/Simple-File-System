#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST(x) test(x, #x)
#include "myfilesystem.h"

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

// TODO:
	// Write test to resize a file to a smaller value, checking if the rest of the file is successfully written to 0s

/* Some example unit test functions */
int success() {
    return 0;
}

int failure() {
    return 1;
}

int no_operation() {
    void * helper = init_fs("file1", "file2", "file3", 1); // Remember you need to provide your own test files and also check their contents as part of testing
    close_fs(helper);
    return 0;
}

int init_fs_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	//struct file_system * fs = ((struct file_system *)helper);
	
	int errors = 0;
	
	// if (((struct file_system *)helper)->num_files != 1) {
	// 	printf("init_fs_num_files() ERROR: num_files reads incorrectly as %d", ((struct file_system *)helper)->num_files);
	// }
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int create_file_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	int free_space = free_space_on_disk(helper);
	
	// Checks that calling create_file() with a size 1 larger than free space returns 2
	ret = create_file("afile", free_space + 1, helper);
	if (ret != 2) {
		printf("create_file_no_space() ERROR: "
			   "calling create_file() with a size larger than free space did not return 2\n");
	}
	
	// Checks that calling create_file() on a file that exists returns 1
	ret = create_file("hello", 10, helper);
	if (ret != 1) {
		errors += 1;
		printf("create_file_exists() ERROR: "
			   "calling create_file() on a file that exists did not return 1\n");
	}
	
	// Checks that creating a new file with valid input returns 0
	ret = create_file("new_file", 20, helper);
	if (ret != 0) {
		errors += 1;
		printf("create_file() ERROR: "
			   "calling create_file() with valid input does not return 0\n");
	}
	
	// Checks that the file we just created exists
	ret = does_file_exist("new_file", helper);
	if (ret != 1) {
		errors += 1;
		printf("create_file() ERROR: "
			   "newly created file does not seem to exist\n");
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int file_size_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	// Checks that calling file_size() on a non-existent file returns -1
	ret = file_size("doesntexist", helper);
	if (ret != -1) {
		errors += 1;
		printf("file_size_test() ERROR: "
			   "calling file_size() on a non-existent file did not return -1\n");
	}
	
	// Checks that calling file_size() works correctly
	ret = file_size("hello", helper);
	if (ret != 47) {
		errors += 1;
		printf("file_size_test() ERROR: "
			   "file size of hello returned as %zd instead of 47\n", file_size("hello", helper));
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int read_file_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	char buf[11];
	int ret;
	
	// Test that a count too large returns 2
	ret = read_file("hello", 5, 50, buf, helper);
	if (ret != 2) {
		errors += 1;
		printf("ERROR in read_test(): "
			   "calling read_file() with an offset too large did not return 2\n");
	}
	
	// Test that a non-existent file returns 1
	ret = read_file("blahblahblah", 0, 1, buf, helper);
	if (ret != 1) {
		errors += 1;
		printf("ERROR in read_test(): "
			   "calling read_file() with a non-existent file did not return 1\n");
	}
	
	// Test that reading Document.docx file_data.bin stores the correct data
	ret = read_file("Document.docx", 0, 10, buf, helper);
	if (strcmp(buf, "lotsofdata") != 0) {
		errors += 1;
		printf("ERROR in read_test(): "
			   "calling read_file() on Document.docx did not read the correct data into buf\n");
	}
	
	// Test that reading Document.docx in file_data.bin returns 0
	if (ret != 0) {
		errors += 1;
		printf("ERROR in read_test(): "
			   "calling read_file() on Document.docx did not return 2\n");
	}
	
	close_fs(helper);
	if (errors != 0)
		return 1;
	
	return 0;
}

int write_file_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	char * buf;
	char buf_read[11];
	
	// Checks that calling write_file() with an offset larger than the file returns 2
	buf = "your mum";
	ret = write_file("Document.docx", 1000, 4, buf, helper);
	if (ret != 2) {
		errors += 1;
		printf("ERROR in write_test(): "
			   "calling write_file() with an offset larger than the file did not return 2\n");
	}
	
	buf = "yourmother";
	ret = write_file("Document.docx", 0, 10, buf, helper);
	//printf("hi\n");
	if (ret != 0) {
		errors += 1;
		printf("ERROR in write_test(): "
			   "calling write_file() did not return 0\n");
	}
	
	
	read_file("Document.docx", 0, 10, buf_read, helper);
	
	if ((strcmp(buf_read, "yourmother") != 0)) {
		printf("ERROR in write_test(): "
			   "calling write_file() wrote incorrect data\n");
		errors += 1;
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int free_space_on_disk_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	// This tests that the free_space_on_disk() function returns the correct amount of free space
	// This function is useful for various others
	ret = free_space_on_disk(helper);
	if (ret != 1991) {
		errors += 1;
		printf("ERROR in free_space_on_disk_test(): "
			   "calling free_space_on_disk() returns an incorrect value of %d\n", ret);
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int resize_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	int resize_value;
	ssize_t size_of_file;
	
	// Test if resizing Document.docx to a size that fills up exactly the rest of the disk space
	// works and produces the correct return value
	size_of_file = file_size("Document.docx", helper);
	resize_value = free_space_on_disk(helper) + size_of_file;
	ret = resize_file("Document.docx", resize_value, helper);
	if (ret != 0 && file_size("Document.docx", helper) != resize_value) {
		errors += 1;
		printf("ERROR in resize_test(): resizing Document.docx to the amount of free space in disk."
			   " | file_size = %zd\n", file_size("Document.docx", helper));
		printf("return value: %d\n", resize_file("Document.docx", resize_value, helper));
	}
	
	// Test if resizing Document.docx to a size that fills up exactly the rest of the disk space + 1 byte
	// fails and produces the correct return value
	size_of_file = file_size("Document.docx", helper);
	resize_value = free_space_on_disk(helper) + file_size("Document.docx", helper) + 1;
	ret = resize_file("Document.docx", resize_value, helper);
	if (ret != 2 && file_size("Document.docx", helper) != size_of_file) {
		errors += 1;
		printf("ERROR in resize_test(): resizing Document.docx to a size too large."
			   " | file_size = %zd\n", file_size("Document.docx", helper));
	}
	
	// Test if resizing Document.docx to a smaller size works and produces correct return value
	ret = resize_file("Document.docx", 5, helper);
	if (ret != 0 && file_size("Document.docx", helper) != 5) {
		errors += 1;
		printf("ERROR in resize_test(): resizing Document.docx to a smaller size of 5"
			   "did not return 0 | file_size = %zd\n", file_size("Document.docx", helper));
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int delete_file_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	// Test that calling delete_file for a non-existent file returns 1
	ret = delete_file("hi", helper);
	if (ret != 1) {
		errors += 1;
		printf("ERROR in delete_file_test(): "
			   "calling delete_file() for a non-existent file returns an incorrect value of %d\n", ret);
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int contiguous_space_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	// Test that calling contiguous_space for a length larger than disk space returns -1
	ret = contiguous_space(20000, helper);
	if (ret != -1) {
		errors += 1;
		printf("ERROR in contiguous_space_test(): "
			   "calling contiguous_space_test() with length larger than disk returns an incorrect value of %d\n", ret);
	}
	
	// Test that calling contiguous_space for a length of 20 would fit in the first valid offset
	// of contiguous space
	ret = contiguous_space(20, helper);
	if (ret != 11) {
		errors += 1;
		printf("ERROR in contiguous_space_test(): "
			   "calling contiguous_space_test() for good length returns an incorrect value of %d\n", ret);
	}
	
	repack(helper);
	
	ret = contiguous_space(30000, helper);
	if (ret != -1) {
		errors += 1;
		printf("ERROR in contiguous_space_test(): "
			   "calling contiguous_space_test() returns an incorrect value of %d\n", ret);
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int create_file_with_repack() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	int ret;
	
	int free_space = free_space_on_disk(helper);
	
	// Checks that calling create_file() with a size 1 less than free space returns 0
	ret = create_file("afile", free_space - 1, helper);
	if (ret != 0) {
		errors += 1;
		printf("create_file_with_repack() ERROR: "
			   "calling create_file() with a size less than free space did not return 0\n");
	}
	
	ret = does_file_exist("afile", helper);
	if (ret != 1) {
		errors += 1;
		printf("create_file() ERROR: "
			   "newly created file does not seem to exist\n");
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int fletcher_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	uint8_t output[16];
	uint8_t correct_output[] = {223, 133, 94, 10, 205, 102, 30, 71, 190, 148, 152, 3, 110, 178, 233, 9};
	
	uint8_t buf[] = {182, 221, 177, 150, 245, 78, 22, 230, 195, 189, 208, 78, 211, 144, 207, 75, 149, 245, 219, 46, 6, 21, 26, 196};
	
	int errors = 0;
	
	// Calls fletcher
	fletcher(buf, 24, output);
	
	// Checks the output of the fletcher function against correct output
	for (int i = 0; i < 16; i++) {
		if (output[i] != correct_output[i]) {
			errors += 1;
			printf("fletcher_test() ERROR: "
			   "output is not correct\n");
		}
	}
	
	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int repack_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	FILE * fp = fopen("file_data.bin", "r");
	
	int errors = 0;
	
	// Calls repack
	repack(helper);
	
	fseek(fp, 101, SEEK_SET);
	char * buf = malloc(1);
	fread(buf, 1, 1, fp);
	if (strcmp(buf, "0") == 0) {
		errors += 1;
		printf("repack_test() ERROR: incorrect output\n");
	}
	free(buf);
	buf = NULL;
	fclose(fp);

	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

int hash_tree_test() {
	void * helper = init_fs("file_data.bin", "directory_table.bin", "hash_data.bin", 4);
	
	int errors = 0;
	
	// Calls compute_hash_tree
	compute_hash_tree(helper);

	close_fs(helper);
	
	if (errors != 0)
		return 1;
	
	return 0;
}

/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("Failed %s returned %d\n", function_name, ret);
    }
}
/************************/

int main(int argc, char * argv[]) {
    
    // You can use the TEST macro as TEST(x) to run a test function named "x"
    //TEST(no_operation);
	TEST(init_fs_test);
	TEST(file_size_test);
	TEST(free_space_on_disk_test);
	TEST(read_file_test);
	TEST(write_file_test);
	TEST(resize_test);
	TEST(contiguous_space_test);
	TEST(create_file_test);
	//TEST(create_file_with_repack);
	TEST(fletcher_test);
	TEST(delete_file_test);
	TEST(repack_test);
	TEST(hash_tree_test);

    return 0;
}
