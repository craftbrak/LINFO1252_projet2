#include <string.h>
#include "lib_tar.h"
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
void print_tar_header(const tar_header_t *header)
{
    if (header == NULL) {
        printf("Header is NULL\n");
        return;
    }
    printf("Next Header \n \n \n");
    printf("Name: %s\n", header->name);
    printf("Mode: %s\n", header->mode);
    printf("UID: %s\n", header->uid);
    printf("GID: %s\n", header->gid);
    printf("Size: %s\n", header->size);
    printf("Mtime: %s\n", header->mtime);
    printf("Chksum: %s\n", header->chksum);
    printf("Typeflag: %c\n", header->typeflag);
    printf("Linkname: %s\n", header->linkname);
    printf("Magic: %s\n", header->magic);
    printf("Version: %s\n", header->version);
    printf("Uname: %s\n", header->uname);
    printf("Gname: %s\n", header->gname);
    printf("Devmajor: %s\n", header->devmajor);
    printf("Devminor: %s\n", header->devminor);
    printf("Prefix: %s\n", header->prefix);
    // Padding is not printed as it's usually not relevant for display
}

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
long next_header(int tar_fd, tar_header_t *header){
    ssize_t bytesRead = read(tar_fd, header, sizeof(tar_header_t));
    if (bytesRead < sizeof(tar_header_t)){
        return -2;
    }


    char *end;
    long size = strtol(header->size,&end,10);
    long skipblock = (size+BLOCKSIZE -1)/ BLOCKSIZE;
    long err = lseek(tar_fd,skipblock*BLOCKSIZE,SEEK_CUR);
    // Check if the header is completely zeroed out
    int isEmpty = 1;
    for (int i = 0; i < sizeof(tar_header_t); ++i) {
        if (((unsigned char*)header)[i] != 0) {
            isEmpty = 0;
            break;
        }
    }

    if (isEmpty) {
        // If the header is empty, recursively call next_header to check the next one
        return next_header(tar_fd, header);
    }
    return err;
}
/**
 * Resets the file descriptor to the start of the TAR archive.
 *
 * @param tar_fd File descriptor for the TAR archive.
 * @return The offset from the start of the file if successful, or -1 on error.
 */
long go_back_start(int tar_fd){
    return lseek(tar_fd, 0, SEEK_SET);
}
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
int resolve_symlink(int tar_fd, const char *symlink_path, char *resolved_path) {
    tar_header_t header;
    go_back_start(tar_fd);

    while (next_header(tar_fd, &header) > 0) {
        if (strcmp(header.name, symlink_path) == 0 && header.typeflag == SYMTYPE) {
            // Found the symlink, copy its target to resolved_path
            strncpy(resolved_path, header.linkname, MAX_PATH_SIZE);
            resolved_path[MAX_PATH_SIZE - 1] = '\0'; // Ensure null-termination
            return 0;
        }
    }

    // Symlink not found
    return -1;
}

/**
 * Seeks to the start of the file data in the TAR archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param header The header of the file whose data we want to seek to.
 * @param offset Offset from the start of the file data.
 *
 * @return 0 on success, -1 on failure.
 */
int seek_to_file_data(int tar_fd, const tar_header_t *header, size_t offset) {
    char *end;
    long size = strtol(header->size, &end, 10); // Convert size from ASCII to long
    if (size == 0 || *end != '\0') {
        // Invalid size in header or non-numeric characters in size field
        return -1;
    }

    // Calculate the position of the start of the file data
    off_t position = lseek(tar_fd, 0, SEEK_CUR);
    if (position == (off_t)-1) {
        // Error in getting current position
        return -1;
    }

    // Each file's data is padded to fill a complete block in TAR format
    size_t padded_size = ((size + BLOCKSIZE - 1) / BLOCKSIZE) * BLOCKSIZE;
    off_t new_position = position + padded_size + BLOCKSIZE; // Add TAR_BLOCK_SIZE to skip over the header

    // Adjust with the offset
    new_position += offset;

    // Seek to the calculated position
    if (lseek(tar_fd, new_position, SEEK_SET) == (off_t)-1) {
        // Error in seeking to new position
        return -1;
    }

    return 0;
}

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
unsigned int calculate_tar_checksum(const struct posix_header *header) {
    const unsigned char *bytes = (const unsigned char *)header;
    unsigned int checksum = 0;

    for (int i = 0; i < 512; ++i) {
        if (i >= 148 && i < 156) {
            checksum += 32; // Treat chksum field as spaces
        } else {
            checksum += bytes[i];
        }
    }

    return checksum;
}

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
int get_header_type(int tar_fd, char *path, tar_header_t *header){
    go_back_start(tar_fd);
    while(1){
        long err = next_header(tar_fd, header);
        if (err == -2){
            break;
        } else if (err == -1){
            printf("Error from lseek");
            return -1;
        }
        if (strncmp(header->name, path, sizeof(header->name))==0){
            switch (header->typeflag) {
                case DIRTYPE:
                    return 2;
                case SYMTYPE:
                    return 3;
                case REGTYPE:
                    return 1;
                default:
                    return 1;
            }
        }
    }
    return 0;
}

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
int check_archive(int tar_fd) {
    tar_header_t header;
    go_back_start(tar_fd);
    while(next_header(tar_fd, &header)>0){
        if (strncmp(header.magic,TMAGIC, TMAGLEN)!=0 ){
            return -1;
        }
        if(strncmp(header.version, TVERSION, TVERSLEN)!=0){
            return -2;
        }
        unsigned int calculated_checksum = calculate_tar_checksum(&header);

        // Convert the chksum field to an integer for comparison
        unsigned int stored_checksum = (unsigned int)strtol(header.chksum,NULL,8);
        if (calculated_checksum != stored_checksum) {
            // Handle checksum mismatch
            return -3; // For example, as per your documentation
        }
    }

    return 1;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    tar_header_t header;
    return get_header_type(tar_fd, path, &header);
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    tar_header_t header;
    if (get_header_type(tar_fd, path,&header)== 2 ) return 1;
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    tar_header_t header;
    if (get_header_type(tar_fd, path,&header)== 1 ) return 1;
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
    tar_header_t header;
    if (get_header_type(tar_fd, path,&header)== 3 ) return 1;
    return 0;
}


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
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    // ATTENTION avant toute chose faire le check que le chemin est bien vers un dir (si chemin vers un fichier/symlink qui pointe vers un fichier: return 0)
    // si le repertoir a lister est un symlink: le nom du repertoire a inclure est celui du chemin reel et pas celui du simlink (ex sym_a->a, inclure a/nom et pas sym_a/nom)
    // lire un simlink: trouver le nom dans le champs linkname du header du symlink (on ne resoud pas les symlinks a l'interieur du dossier)
    // pas oublier d'update le buffer entries (avec le chemin complet) ET le size_t no_entries
    // -> si no_entries est plus petit que le nombre d'entries du directory, lister les no_entries premiers elements (dans l'ordre des headers)
    // attention a update no_entries dans tous les cas (pas oublier de le mettre a 0 quand on retourne 0)
    // attention ajouter les / pour les folders (ATTENTION le / n'est pas compris dans le linkname d'un symlink dons l'ajouter si besoin -> avec strcat?) mais ne pas ajouter les sous-dossier a la suite

    tar_header_t header;
    int type = get_header_type(tar_fd, path, &header);
    char* directory;
    size_t entries_length = *no_entries;
    *no_entries = 0;
    if (type <= 1) {
        return 0;
    } else if (type == 3) {
        //we have a symlink
        //check if the symlink points to a directory or a file...
        tar_header_t header_bis;
        directory = header.linkname;
        if (get_header_type(tar_fd, directory, &header_bis)<= 2) {
            return 0;
        }
        strcat(directory, "/");
    } else {
        directory = header.name;
    }

    go_back_start(tar_fd);
    tar_header_t  header_sub;
    //for loop that get all the entries of the directory ??
    while(*no_entries < entries_length){
        long err = next_header(tar_fd, &header_sub);
        if (err == -2){
            break;
        } else if (err == -1){
            printf("Error from lseek");
            return -1;
        }

        if (strncmp(header_sub.name, directory, strlen(directory))==0){
            const char* remaining_path = header_sub.name + strlen(directory);
            if (strchr(remaining_path, '/')== NULL){
                entries[*no_entries] = strdup(header_sub.name);
                if (entries[*no_entries] == NULL){
                    printf("Error of strdup");
                    return 0;
                }
                (*no_entries)++;
            }
        }

    }

    return 1;
}

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
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    tar_header_t header;
    int type= get_header_type(tar_fd,path,&header);
    if (type == 0 ) return -1;
    if (type == 2) return -1;
    char *end;
    long size = strtol(header.size,&end,10);
    if (size <= offset) return -2;
    if (type == 3) { // Symlink
        // Resolve symlink (you need to implement resolve_symlink)
        char resolved_path[MAX_PATH_SIZE];
        if (resolve_symlink(tar_fd, path, resolved_path) == -1) {
            return -1; // Error resolving symlink
        }
        return read_file(tar_fd, resolved_path, offset, dest, len);
    }



    size_t to_read = size - offset;
    if (*len < to_read) {
        to_read = *len; // Adjust if dest buffer is smaller than the file size
    }

    // Seek to the start of the file data plus the offset
    // (You need to implement seek_to_file_data)
    if (seek_to_file_data(tar_fd, &header, offset) == -1) {
        return -1; // Error seeking to file data
    }

    // Read file data into dest
    ssize_t bytes_read = read(tar_fd, dest, to_read);
    if (bytes_read == -1) {
        // Handle read error
        return -1;
    }

    *len = bytes_read; // Update the number of bytes actually read
    return size > bytes_read + offset ? size - bytes_read - offset : 0; // Remaining bytes
}