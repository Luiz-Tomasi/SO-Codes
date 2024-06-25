#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "commands.h"
#include "fat16.h"
#include "support.h"

off_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

struct fat_dir find(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb) {
    struct fat_dir curdir = {0};
    int i;

    for (i = 0; i < bpb->possible_rentries; i++) {
        if (strcmp((char *)dirs[i].name, filename) == 0) {
            curdir = dirs[i];
            break;
        }
    }
    return curdir;
}

struct fat_dir *find_pointer(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb) {
    int i;
    for (i = 0; i < bpb->possible_rentries; i++) {
        if (strcmp((char *)dirs[i].name, filename) == 0) {
            return &dirs[i]; // Return a pointer to the matching directory entry
        }
    }
    return NULL; // Return NULL if no matching directory entry is found
}

struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb) {
    int i;
    struct fat_dir *dirs = malloc(sizeof(struct fat_dir) * bpb->possible_rentries);
    if (!dirs) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    for (i = 0; i < bpb->possible_rentries; i++) {
        uint32_t offset = bpb_froot_addr(bpb) + i * sizeof(struct fat_dir);
        if (read_bytes(fp, offset, &dirs[i], sizeof(dirs[i])) != 0) {
            free(dirs);
            return NULL;
        }
    }
    return dirs;
}

int write_dir(FILE *fp, char *fname, struct fat_dir *dir) {
    char *name = padding(fname);
    if (!name) {
        return -1;
    }
    strcpy((char *)dir->name, name);
    free(name);
    if (fwrite(dir, 1, sizeof(struct fat_dir), fp) != sizeof(struct fat_dir))
        return -1;
    return 0;
}

int write_data(FILE *fp, char *fname, struct fat_dir *dir, struct fat_bpb *bpb) {
    FILE *localf = fopen(fname, "rb");
    if (!localf) {
        return -1;
    }
    int c;
    while ((c = fgetc(localf)) != EOF) {
        if (fputc(c, fp) != c) {
            fclose(localf);
            return -1;
        }
    }
    fclose(localf);
    return 0;
}

int wipe(FILE *fp, struct fat_dir *dir, struct fat_bpb *bpb) {
    int start_offset = bpb_froot_addr(bpb) + (bpb->bytes_p_sect * dir->starting_cluster);
    int limit_offset = start_offset + dir->file_size;

    while (start_offset <= limit_offset) {
        fseek(fp, start_offset++, SEEK_SET);
        if (fputc(0x0, fp) != 0x0)
            return -1;
    }
    return 0;
}

void mv(FILE *fp, char *src_filename, char *dest_filename, struct fat_bpb *bpb) {
    // Read source file
    FILE *src_fp = fopen(src_filename, "rb");
    if (!src_fp) {
        fprintf(stderr, "Error opening source file %s\n", src_filename);
        return;
    }

    fseek(src_fp, 0, SEEK_END);
    long file_size = ftell(src_fp);
    rewind(src_fp);
    char *buffer = malloc(file_size);
    if (!buffer) {
        fclose(src_fp);
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    fread(buffer, 1, file_size, src_fp);
    fclose(src_fp);

    // Load directory entries
    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        free(buffer);
        fprintf(stderr, "Error listing directories\n");
        return;
    }

    // Find source directory entry
    struct fat_dir *src_dir = find_pointer(dirs, src_filename, bpb);
    if (!src_dir) {
        free(buffer);
        free(dirs);
        fprintf(stderr, "Error finding source file %s\n", src_filename);
        return;
    }

    // Prepare new directory entry
    struct fat_dir new_dir = {0};
    char *new_name = padding(dest_filename);
    if (!new_name) {
        free(buffer);
        free(dirs);
        fprintf(stderr, "Error allocating memory for new file name\n");
        return;
    }
    strcpy((char *)new_dir.name, new_name);
    free(new_name);
    new_dir.attr = src_dir->attr;
    new_dir.starting_cluster = src_dir->starting_cluster;
    new_dir.file_size = file_size;

    // Find a free directory entry for the new file
    struct fat_dir *free_dir = NULL;
    for (int i = 0; i < bpb->possible_rentries; i++) {
        if (dirs[i].name[0] == 0x00 || dirs[i].name[0] == 0xE5) {
            free_dir = &dirs[i];
            break;
        }
    }

    if (!free_dir) {
        free(buffer);
        free(dirs);
        fprintf(stderr, "Error finding free directory entry\n");
        return;
    }

    // Write new directory entry
    *free_dir = new_dir;
    fseek(fp, bpb_froot_addr(bpb) + (free_dir - dirs) * sizeof(struct fat_dir), SEEK_SET);
    fwrite(free_dir, 1, sizeof(struct fat_dir), fp);

    // Write file data to the image
    fseek(fp, bpb_froot_addr(bpb) + new_dir.starting_cluster * bpb->bytes_p_sect, SEEK_SET);
    fwrite(buffer, 1, file_size, fp);

    // Mark old directory entry as free
    src_dir->name[0] = 0xE5;  // Mark entry as deleted
    fseek(fp, bpb_froot_addr(bpb) + (src_dir - dirs) * sizeof(struct fat_dir), SEEK_SET);
    fwrite(src_dir, 1, sizeof(struct fat_dir), fp);

    // Clean up
    free(buffer);
    free(dirs);
}


void rm(FILE *fp, char *filename, struct fat_bpb *bpb) {
    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        fprintf(stderr, "Error listing directories\n");
        return;
    }
    struct fat_dir *dir = find_pointer(dirs, filename, bpb);
    if (!dir) {
        fprintf(stderr, "Error finding file %s\n", filename);
        free(dirs);
        return;
    }
    if (wipe(fp, dir, bpb) != 0) {
        fprintf(stderr, "Error wiping file %s\n", filename);
        free(dirs);
        return;
    }
    dir->attr = DIR_FREE_ENTRY;
    fseek(fp, bpb_froot_addr(bpb) + (dir - dirs) * sizeof(struct fat_dir), SEEK_SET);
    fwrite(dir, 1, sizeof(*dir), fp);
    free(dirs);
}

void cp(FILE *fp, char *filename, struct fat_bpb *bpb) {
    FILE *src_fp = fopen(filename, "rb");
    if (!src_fp) {
        fprintf(stderr, "Error opening source file %s\n", filename);
        return;
    }

    fseek(src_fp, 0, SEEK_END);
    long file_size = ftell(src_fp);
    rewind(src_fp);
    char *buffer = malloc(file_size);
    if (!buffer) {
        fclose(src_fp);
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    fread(buffer, 1, file_size, src_fp);
    fclose(src_fp);

    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        free(buffer);
        fprintf(stderr, "Error listing directories\n");
        return;
    }

    struct fat_dir new_dir = {0};
    strcpy((char *)new_dir.name, filename);
    new_dir.attr = DIR_ATTR_ARCHIVE;
    new_dir.starting_cluster = bpb->possible_rentries - 1; // Simple way to find the next free cluster
    new_dir.file_size = file_size;

    fseek(fp, bpb_froot_addr(bpb) + new_dir.starting_cluster * sizeof(struct fat_dir), SEEK_SET);
    fwrite(&new_dir, 1, sizeof(new_dir), fp);
    fwrite(buffer, 1, file_size, fp);

    free(buffer);
    free(dirs);
}
