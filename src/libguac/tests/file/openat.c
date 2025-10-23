/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <CUnit/CUnit.h>
#include <guacamole/file.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Closes the given file descriptor if it is not an error code from a previous
 * call to open() or openat().
 *
 * @param fd
 *     The file descriptor to close.
 */
static void close_if_necessary(int fd) {
    if (fd != -1)
        close(fd);
}

/**
 * Returns whether a file with the given filename exists beneath the given
 * path.
 *
 * @param path
 *     The path to the directory that may contain the file.
 *
 * @param filename
 *     The filename to test.
 *
 * @return
 *     Non-zero if a file exists with the given filename beneath the given
 *     path, zero otherwise.
 */
static int exists(const char* path, const char* filename) {

    int result = 0;

    int dir_fd = open(path, O_RDONLY);
    if (dir_fd == -1)
        return 0;

    struct stat file_info;
    result = fstatat(dir_fd, filename, &file_info, 0);

    close_if_necessary(dir_fd);
    return result != -1;

}

/**
 * Removes the file with the given filename beneath the given path. If NULL is
 * provided instead of a filename, the final component of the given path is
 * removed as a directory.
 *
 * @param path
 *     The path to the directory that contains the file to remove.
 *
 * @param filename
 *     The filename to remove, or NULL if the containing directory should be
 *     removed instead.
 *
 * @return
 *     Non-zero if the operation succeeded, zero otherwise.
 */
static int remove_file(const char* path, const char* filename) {

    if (filename == NULL)
        return !rmdir(path);

    int dir_fd = open(path, O_RDONLY);
    if (dir_fd == -1)
        return 0;

    int result = unlinkat(dir_fd, filename, 0);

    close_if_necessary(dir_fd);
    return !result;

}

/**
 * Returns whether the file with the given filename beneath the given path has
 * the given permissions (mode). Any permission bits that apply to the file but
 * which are greater than the least-significant 12 bits are ignored.
 *
 * @param path
 *     The path to the directory that contains the file to test.
 *
 * @param filename
 *     The filename to test.
 *
 * @param mode
 *     The permissions (mode) that the file is expected to have.
 *
 * @return
 *     Non-zero if the file does have the given permissions, zero otherwise.
 */
static int has_mode(const char* path, const char* filename, mode_t mode) {


    int dir_fd = open(path, O_RDONLY);
    if (dir_fd == -1)
        return 0;

    struct stat file_info = { 0 };
    int result = fstatat(dir_fd, filename, &file_info, 0);

    close_if_necessary(dir_fd);
    return result == 0 && (file_info.st_mode & 07777) == mode;

}

#ifndef __MINGW32__
/**
 * Returns whether another process would be able to acquire the lock of the
 * given type on the given file. If no such lock can be acquired due to
 * conflicts (another process already holds a conflicting lock), non-zero is
 * returned.
 *
 * @param path
 *     The path to the directory that contains the file to test.
 *
 * @param filename
 *     The filename to test.
 *
 * @param lock_type
 *     The type of lock to check for, either F_RDLCK or F_WRLCK.
 *
 * @return
 *     Non-zero if an existing lock would conflict with the given lock type,
 *     zero otherwise.
 */
static int lock_conflicts(const char* path, const char* filename, short lock_type) {

    int result = 0;

    pid_t pid = fork();
    CU_ASSERT_NOT_EQUAL_FATAL(pid, -1);

    /* In child */
    if (pid == 0) {

        int dir_fd = open(path, O_RDONLY);
        if (dir_fd == -1)
            _exit(0);

        int fd = openat(dir_fd, filename, O_RDONLY);
        if (fd != -1) {

            struct flock file_lock = {
                .l_type = lock_type,
                .l_whence = SEEK_SET,
                .l_start  = 0,
                .l_len    = 0
            };

            if (fcntl(fd, F_GETLK, &file_lock) != -1)
                result = (file_lock.l_type != F_UNLCK);

        }

        close_if_necessary(dir_fd);
        close_if_necessary(fd);

        _exit(result);

    }

    return waitpid(pid, &result, 0) != -1 && WIFEXITED(result) && WEXITSTATUS(result);

}
#endif

/**
 * Verify general guac_openat() behavior when creating files within
 * directories, optionally first creating that directory.
 */
void test_file__openat(void) {

    int fd;
    char filename[1024];

    char temp_dir[64] = "/tmp/guacamole-server-test_file__openat.XXXXXX";
    CU_ASSERT_PTR_NOT_NULL_FATAL(mkdtemp(temp_dir));

    guac_open_how how = {
        .oflags = O_CREAT | O_WRONLY,
        .mode = S_IRUSR | S_IWUSR | S_IRGRP
    };

    /* File should be successfully created where there are no other files */

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(exists(temp_dir, "foo"));
    CU_ASSERT_TRUE(has_mode(temp_dir, "foo", how.mode));
    close_if_necessary(fd);

    /* File should NOT be successfully created if it already exists and we are
     * requesting exclusivity */

    how.oflags |= O_EXCL;

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    how.oflags &= ~O_EXCL;

    /* File should be successfully opened if it already exists and we are NOT
     * requesting exclusivity */

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    close_if_necessary(fd);

    /* If unique suffix handling is requested, things should still fail if no
     * destination buffer is provided for the updated filename */

    how.flags |= GUAC_O_UNIQUE_SUFFIX;

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    /* If unique suffix handling is requested, a numeric suffix should be added
     * if the file already exists */

    how.filename = filename;
    how.filename_size = sizeof(filename);

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(exists(temp_dir, "foo.1"));
    CU_ASSERT_TRUE(has_mode(temp_dir, "foo.1", how.mode));
    close_if_necessary(fd);

    if (fd != -1)
        CU_ASSERT_STRING_EQUAL(filename, "foo.1");

    /* Numeric suffixes should continue to increase as necessary */

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(exists(temp_dir, "foo.2"));
    CU_ASSERT_TRUE(has_mode(temp_dir, "foo.2", how.mode));
    close_if_necessary(fd);

    if (fd != -1)
        CU_ASSERT_STRING_EQUAL(filename, "foo.2");

    /* Creation within non-existent directories should fail by default ... */

    CU_ASSERT_TRUE(remove_file(temp_dir, "foo"));
    CU_ASSERT_TRUE(remove_file(temp_dir, "foo.1"));
    CU_ASSERT_TRUE(remove_file(temp_dir, "foo.2"));
    CU_ASSERT_TRUE(remove_file(temp_dir, NULL));

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    /* ... but should succeed if automatic path creation is requested */

    how.flags |= GUAC_O_CREATE_PATH;

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(exists(temp_dir, "foo"));
    CU_ASSERT_TRUE(has_mode(temp_dir, "foo", how.mode));
    close_if_necessary(fd);

    /* The resulting filename should be produced so long as
     * GUAC_O_UNIQUE_SUFFIX is specified, even if no change is made to the
     * filename */

    if (fd != -1)
        CU_ASSERT_STRING_EQUAL(filename, "foo");

    /* Clean up all remaining temporary files */

    CU_ASSERT_TRUE(remove_file(temp_dir, "foo"));
    CU_ASSERT_TRUE(remove_file(temp_dir, NULL));

}

/**
 * Verify guac_openat() behavior when the filename provided contains path
 * components.
 */
void test_file__openat_not_filename(void) {

    int fd;

    char temp_dir[64] = "/tmp/guacamole-server-test_file__openat_not_filename.XXXXXX";
    CU_ASSERT_PTR_NOT_NULL_FATAL(mkdtemp(temp_dir));

    guac_open_how how = {
        .oflags = O_RDONLY
    };

    /* Path separators should be permitted only in the path */

    fd = guac_openat(temp_dir, "foo/bar", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    fd = guac_openat(temp_dir, "foo\\bar", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    /* References to current and parent directories should not be permitted in
     * filename (NOTE: All other occurrences are implicitly covered by the path
     * separator check) */

    fd = guac_openat(temp_dir, ".", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    fd = guac_openat(temp_dir, "..", &how);
    CU_ASSERT_EQUAL(fd, -1);
    close_if_necessary(fd);

    /* Clean up all remaining temporary files */
    CU_ASSERT_TRUE(remove_file(temp_dir, NULL));

}

/**
 * Verify guac_openat() behavior when locking is requested vs. not requested.
 */
void test_file__openat_locked(void) {
#ifdef __MINGW32__
    /* Skipped under Windows platforms (see documentation for GUAC_O_LOCKED) */
#else

    int fd;

    char temp_dir[64] = "/tmp/guacamole-server-test_file__openat_locked.XXXXXX";
    CU_ASSERT_PTR_NOT_NULL_FATAL(mkdtemp(temp_dir));

    guac_open_how how = {
        .oflags = O_CREAT | O_WRONLY,
        .mode = S_IRUSR | S_IWUSR | S_IRGRP
    };

    /* We should not have any locks unless requested */

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_FALSE(lock_conflicts(temp_dir, "foo", F_WRLCK));
    CU_ASSERT_FALSE(lock_conflicts(temp_dir, "foo", F_RDLCK));
    close_if_necessary(fd);

    /* We should have a write lock on the file if opened for writing while
     * GUAC_O_LOCKED is set */

    how.flags = GUAC_O_LOCKED;

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(lock_conflicts(temp_dir, "foo", F_WRLCK));
    CU_ASSERT_TRUE(lock_conflicts(temp_dir, "foo", F_RDLCK));
    close_if_necessary(fd);

    /* We should have a read lock on the file if opened for reading while
     * GUAC_O_LOCKED is set */

    how.oflags = O_RDONLY;
    how.mode = 0;

    fd = guac_openat(temp_dir, "foo", &how);
    CU_ASSERT_NOT_EQUAL(fd, -1);
    CU_ASSERT_TRUE(lock_conflicts(temp_dir, "foo", F_WRLCK));
    CU_ASSERT_FALSE(lock_conflicts(temp_dir, "foo", F_RDLCK));
    close_if_necessary(fd);

    /* Clean up all remaining temporary files */
    CU_ASSERT_TRUE(remove_file(temp_dir, "foo"));
    CU_ASSERT_TRUE(remove_file(temp_dir, NULL));

#endif
}
