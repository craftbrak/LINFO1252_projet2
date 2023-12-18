#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CUnit/Basic.h"
#include "CUnit/Automated.h"
#include "CUnit/CUnit.h"

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

static int fd;
static int fd_empty;
void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

int init_suite(void) {
    fd = open("./tars/archive.tar", O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    return 0;
}
int init_suite_check_archive(){
    fd = open("./tars/archive.tar", O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    fd_empty = open("./tars/empty.tar", O_RDONLY);
    if (fd_empty == -1) {
        perror("open(tar_file)");
        return -1;
    }
    return 0;
}
int clean_suite(void) {
    return close(fd);
}
int clean_suite_check_archive(void) {
    close(fd_empty);
    return close(fd);
}

void test_print_tar_header(void);
void test_next_header(void);
void test_go_back_start(void);
void test_resolve_symlink(void);
void test_seek_to_file_data(void);
void test_calculate_tar_checksum(void);
void test_get_header_type(void);
void test_check_archive(void);
void test_exists(void);
void test_is_dir(void);
void test_is_file(void);
void test_is_symlink(void);
void test_list(void);
void test_read_file(void);


void test_check_archive(void){
    int res = check_archive(fd);
    printf("\n%d\n",res);
    CU_ASSERT_EQUAL(res,13)
    res = check_archive(fd_empty);
    printf("\n%d\n",res);
    CU_ASSERT_EQUAL(res,0)
}

void test_exists(void) {
    // Test case: File exists
    CU_ASSERT_TRUE(exists(fd, "fichier1"));
    CU_ASSERT_TRUE(exists(fd, "fichier2"));
    CU_ASSERT_TRUE(exists(fd, "fichier1"));
    CU_ASSERT_TRUE(exists(fd, "dir2/dir3/dir4/file5"));

    // Test case: Directory exists
    CU_ASSERT_TRUE(exists(fd, "dir2/"));
    CU_ASSERT_TRUE(exists(fd, "dir2/dir3/"));

    // Test case: Symlink exists
    CU_ASSERT_TRUE(exists(fd, "dir2/dir3/dir4/link_to_file5"));
    CU_ASSERT_TRUE(exists(fd, "link_to_link_to_file_5"));

    // Test case: File does not exist
    CU_ASSERT_FALSE(exists(fd, "nonexistent_file.txt"));

    //Test case: Directory does not exist
    CU_ASSERT_FALSE(exists(fd, "nonexistent_directory/"));
    CU_ASSERT_FALSE(exists(fd, "dir1/nonexistent_directory/"));

    // Test case: Symlink does not exist
    CU_ASSERT_FALSE(exists(fd, "dir1/link_to_nonexistent_file"));
}

void test_is_dir(void){
    // Test case: Directory exists
    CU_ASSERT_TRUE(is_dir(fd, "dir1/"));
    CU_ASSERT_TRUE(is_dir(fd, "dir2/"));
    CU_ASSERT_TRUE(is_dir(fd, "dir1/"));

    // Test case: Directory does not exist
    CU_ASSERT_FALSE(is_dir(fd, "nonexistent_dir/"));

    // Test case: Exists but is a file
    CU_ASSERT_FALSE(is_dir(fd, "dir2/dir3/dir4/file5"));

    // Test case: Exists but is a symlink
    CU_ASSERT_FALSE(is_dir(fd, "link_to_link_to_file_5"));
}

void test_is_file(void){
    // Test case: File exists
    CU_ASSERT_TRUE(is_file(fd, "fichier2"));
    CU_ASSERT_TRUE(is_file(fd, "dir2/file3"));
    CU_ASSERT_TRUE(is_file(fd, "fichier2"));

    // Test case: File does not exist
    CU_ASSERT_FALSE(is_file(fd, "dir1/nonexistent_file.txt"));

    // Test case: Exists but is a directory
    CU_ASSERT_FALSE(is_file(fd, "dir2/"));

    // Test case: Exists but is a symlink
    CU_ASSERT_FALSE(is_file(fd, "dir2/dir3/dir4/link_to_file5"));
}

void test_is_symlink(void){
    // Test case: Symlink exists
    CU_ASSERT_TRUE(is_symlink(fd, "link_to_link_to_file_5"));
    CU_ASSERT_TRUE(is_symlink(fd, "dir1/link_to_dir4"));
    CU_ASSERT_TRUE(is_symlink(fd, "dir2/dir3/brokenlink1"));

    // Test case: Symlink does not exist
    CU_ASSERT_FALSE(is_symlink(fd, "dir2/nonexistent_link"));

    // Test case: Exists but is a directory
    CU_ASSERT_FALSE(is_dir(fd, "dir2/dir3"));

    // Test case: Exists but is a file
    CU_ASSERT_FALSE(is_dir(fd, "fichier1"));
}

void test_list_1(void){
    size_t no_entries = 8;
    char** entries = (char**) malloc(100*sizeof(char)*no_entries);
    int err = list(fd, "dir1/", entries, &no_entries);
    CU_ASSERT_NOT_EQUAL(err, 0);
    CU_ASSERT_EQUAL(no_entries, 2);
    CU_ASSERT_STRING_EQUAL(entries[0], "dir1/file4");
    CU_ASSERT_STRING_EQUAL(entries[1], "dir1/link_to_dir4");
    free(entries);
}

void test_list_2(void){
    size_t no_entries = 8;
    char** entries = (char**) malloc(100*sizeof(char)*no_entries);
    CU_ASSERT_FALSE(list(fd, "dir1/file4", entries, &no_entries));
    CU_ASSERT_EQUAL(no_entries, 0);
    no_entries = 8;
    CU_ASSERT_FALSE(list(fd, "link_to_link_to_file_5", entries, &no_entries));
    CU_ASSERT_EQUAL(no_entries, 0);
    no_entries = 8;
    CU_ASSERT_FALSE(list(fd, "dir2/dir3/dir4/link_to_file5", entries, &no_entries));
    CU_ASSERT_EQUAL(no_entries, 0);
    free(entries);
}

void test_list_3(void){
    size_t no_entries = 8;
    char** entries = (char**) malloc(100*sizeof(char)*no_entries);
    int err = list(fd, "dir1/link_to_dir4", entries, &no_entries);
    CU_ASSERT_NOT_EQUAL(err, 0);
    CU_ASSERT_EQUAL(no_entries, 2);
    printf("%zu\n", no_entries);
//    CU_ASSERT_STRING_EQUAL(entries[0], "dir4/file5");
//    CU_ASSERT_STRING_EQUAL(entries[1], "dir4/link_to_file5");
    free(entries);
}

void print_archive(void){
    tar_header_t header;
    go_back_start(fd);
    while(next_header(fd, &header)>0){
        print_tar_header(&header);
    }
}

int main(int argc, char **argv) {
    // plus utile si on utilise toujours archive.tar
//    if (argc < 2) {
//        printf("Usage: %s tar_file\n", argv[0]);
//        return -1;
//    }

    //  initialize the CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

//    // add a suite to the registry
//    CU_pSuite pSuite0 = NULL;
//    pSuite0 = CU_add_suite("Suite_helper_functions", init_suite, clean_suite);
//    if (NULL == pSuite0) {
//        CU_cleanup_registry();
//        return CU_get_error();
//    }
//
//    // add the tests to the suite
//    if ((NULL == CU_add_test(pSuite0, "test of next_header function", test_next_header))||
//        (NULL == CU_add_test(pSuite0, "test of go_back_start function", test_go_back_start))||
//        (NULL == CU_add_test(pSuite0, "test of resolve_symlink function", test_resolve_symlink))||
//        (NULL == CU_add_test(pSuite0, "test of seek_to_file_data function", test_seek_to_file_data))||
//        (NULL == CU_add_test(pSuite0, "test of calculate_tar_checksum function", test_calculate_tar_checksum))||
//        (NULL == CU_add_test(pSuite0, "test of aget_header_type function", test_get_header_type))){
//        CU_cleanup_registry();
//        return CU_get_error();
//    }
//
    // add a suite to the registry
    CU_pSuite pSuite1 = NULL;
    pSuite1 = CU_add_suite("Suite_check_archive", init_suite_check_archive, clean_suite_check_archive);
    if (NULL == pSuite1) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // add the tests to the suite
    if ((NULL == CU_add_test(pSuite1, "test of check archive function", test_check_archive))){
        CU_cleanup_registry();
        return CU_get_error();
    }

    // add a suite to the registry
    CU_pSuite pSuite2 = NULL;
    pSuite2 = CU_add_suite("Suite_get_type_and_existence", init_suite, clean_suite);
    if (NULL == pSuite2) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // add the tests to the suite
    if ((NULL == CU_add_test(pSuite2, "test of exists function", test_exists))||
        (NULL == CU_add_test(pSuite2, "test of is_dir function", test_is_dir))||
        (NULL == CU_add_test(pSuite2, "test of is_file function", test_is_file))||
        (NULL == CU_add_test(pSuite2, "test of is_symlink function", test_is_symlink))){
        CU_cleanup_registry();
        return CU_get_error();
    }

    // add a suite to the registry
    CU_pSuite pSuite3 = NULL;
    pSuite3 = CU_add_suite("Suite_list", init_suite, clean_suite);
    if (NULL == pSuite3) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // add the tests to the suite
    if ((NULL == CU_add_test(pSuite3, "test 1 of list function (classical case)", test_list_1))||
        (NULL == CU_add_test(pSuite3, "test 2 of list function (paths that don't link to directory)", test_list_2))||
        (NULL == CU_add_test(pSuite3, "test 3 of list function (path is a symlink pointing to a directory)", test_list_3))){
        CU_cleanup_registry();
        return CU_get_error();
    }

//    // add a suite to the registry
//    CU_pSuite pSuite4 = NULL;
//    pSuite4 = CU_add_suite("Suite_read_file", init_suite, clean_suite);
//    if (NULL == pSuite4) {
//        CU_cleanup_registry();
//        return CU_get_error();
//    }
//
//    // add the tests to the suite
//    if ((NULL == CU_add_test(pSuite4, "test of read file function", test_read_file))){
//        CU_cleanup_registry();
//        return CU_get_error();
//    }

    // Run all tests using the CUnit Basic interface
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_automated_run_tests();
    CU_basic_show_failures(CU_get_failure_list()); //permet d'afficher le rapport
    CU_cleanup_registry();

    if (CU_get_number_of_tests_failed() == 0) {
        return 0;
    } else {
        return 1;
    }
//
//    test_exists(fd);
//    test_is_dir(fd);
//    int ret = check_archive(fd);
//    printf("check_archive returned %d\n", ret);
}