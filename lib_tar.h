#ifndef LIB_TAR_H
#define LIB_TAR_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

typedef struct posix_header
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
} tar_header_t;

#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  6
#define TVERSION "00"           /* 00 and no null */
#define TVERSLEN 2

#define BLOCKSIZE 512
#define MAX_PATH_SIZE 100
/* Values used in typeflag field.  */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define DIRTYPE  '5'            /* directory */

/* Converts an ASCII-encoded octal-based number into a regular integer */
#define TAR_INT(char_ptr) strtol(char_ptr, NULL, 8)

/**
 * Prints the contents of a TAR header to standard output.
 *
 * This function displays the information contained within a TAR header,
 * such as file name, mode, user/group IDs, size, modification time, checksum,
 * type flag, link name, and other metadata. It is useful for debugging or
 * inspecting TAR file contents.
 *
 * @param header Pointer to a tar_header_t structure containing the TAR header to print.
 *
 * Note: If the header pointer is NULL, the function prints a warning message
 *       and returns without printing any further information.
 */
void print_tar_header(const tar_header_t *header);

/**
 * Reads the next header in a TAR archive and advances past the corresponding file data.
 *
 * @param tar_fd File descriptor for the TAR archive.
 * @param header Pointer to store the read header.
 *
 * @return Returns the position in the archive after the current file's data. Returns -2
 *         if a complete header cannot be read, indicating the end of the archive or an
 *         error. Returns -1 on seek errors.
 *
 * Note: Assumes proper definition of tar_header_t and BLOCKSIZE. The file descriptor
 *       should be at the start of a header.
 */
long next_header(int tar_fd, tar_header_t *header);
/**
 * Resets the file descriptor to the start of the TAR archive.
 *
 * @param tar_fd File descriptor for the TAR archive.
 * @return The offset from the start of the file if successful, or -1 on error.
 */
long go_back_start(int tar_fd);
/**
 * Resolves a symbolic link to its target within a TAR archive.
 *
 * This function searches through the TAR archive for the specified symbolic link
 * and, upon finding it, provides the path to which the symbolic link points.
 * It assumes that symlinks are not nested, meaning a symlink directly points
 * to a regular file and not to another symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param symlink_path The path of the symbolic link within the tar archive to be resolved.
 * @param resolved_path A buffer where the resolved path will be stored. The buffer
 *                      should be large enough to hold the maximum possible path.
 *                      The resolved path is null-terminated.
 *
 * @return Returns 0 if the symlink was successfully resolved, -1 if the symlink
 *         is not found or if an error occurs during the resolution process.
 *
 * Note: The function does not handle nested symlinks. If the symlink points to
 * another symlink, this function will not resolve it further. Also, the function
 * scans the archive linearly, which may be inefficient for large archives.
 */
int resolve_symlink(int tar_fd, const char *symlink_path, char *resolved_path);

/**
 * Seeks to the start of the file data in the TAR archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param header The header of the file whose data we want to seek to.
 * @param offset Offset from the start of the file data.
 *
 * @return 0 on success, -1 on failure.
 */
int seek_to_file_data(int tar_fd, const tar_header_t *header, size_t offset);

/**
 * Calculates the checksum for a TAR header block.
 *
 * This function computes the checksum for a 512-byte TAR header block.
 * The checksum is the sum of all bytes in the header, with the 8-byte
 * chksum field (bytes 148 to 155) treated as if filled with spaces (ASCII 32).
 *
 * @param header Pointer to a posix_header structure representing the TAR header.
 * @return The calculated checksum as an unsigned integer.
 *
 * Note: The function assumes the header is 512 bytes and formatted according to
 *       TAR specifications.
 */
unsigned int calculate_tar_checksum(const struct posix_header *header);

/**
 * Returns the type of a file if it exists.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @param header A out argument that will be set to the header corresponding to the entry if it exists
 *
 * @return zero if no entry at the given path exists in the archive,
 *         1 file,
 *         2 directory,
 *         3 symlink.
 */
int get_header_type(int tar_fd, char *path, tar_header_t *header);


/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd);

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path);

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path);

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path);

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path);


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries);

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len);

#endif
