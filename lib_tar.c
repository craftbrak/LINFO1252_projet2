#include <string.h>
#include "lib_tar.h"

void print_tar_header(const tar_header_t *header)
{
    if (header == NULL) {
        printf("Header is NULL\n");
        return;
    }

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
 * Reads the next header in the archive
 * @param tar_fd
 * @param header
 * @return -1 if at the end of the archive
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
    return err;
}

long go_back_start(int tar_fd){
    return lseek(tar_fd, 0, SEEK_SET);
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

    }
    return 0;
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
    go_back_start(tar_fd);
    while(1){
        long err = next_header(tar_fd, &header);
        if (err == -2){
            break;
        } else if (err == -1){
            printf("Error from lseek");
            return -1;
        }
        if (strncmp(header.name, path, sizeof(header.name))==0){
            return 1 ;
        }
    }
    return 0;
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
    return 0;
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
    return 0;
}