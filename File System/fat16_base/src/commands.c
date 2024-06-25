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

//Encontra um ponteiro para uma entrada de diretório no array de diretórios fornecido com o nome especificado.
struct fat_dir *find_pointer(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb) {
    int i;
    for (i = 0; i < bpb->possible_rentries; i++) {
        if (strcmp((char *)dirs[i].name, filename) == 0) {
            return &dirs[i]; 
        }
    }
    return NULL;
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
    // Abre o arquivo de origem
    FILE *src_fp = fopen(src_filename, "rb");

    if (!src_fp) {
        fprintf(stderr, "Error opening source file %s\n", src_filename);
        return;
    }

    // Obtém o tamanho do arquivo de origem
    fseek(src_fp, 0, SEEK_END);
    long file_size = ftell(src_fp);
    rewind(src_fp);


    // Aloca um buffer para ler o arquivo
    char *buffer = malloc(file_size);
    if (!buffer) {
        fclose(src_fp);
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

     // Lê o arquivo de origem para o buffer
    fread(buffer, 1, file_size, src_fp);
    fclose(src_fp);


    // Carrega as entradas de diretório da imagem FAT16
    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        free(buffer);
        fprintf(stderr, "Error listing directories\n");
        return;
    }

    // Encontra uma entrada de diretório livre para o novo arquivo
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


    // Prepara a nova entrada de diretório
    memset(free_dir, 0, sizeof(struct fat_dir));
    char *new_name = padding(dest_filename);
    if (!new_name) {
        free(buffer);
        free(dirs);
        fprintf(stderr, "Error allocating memory for new file name\n");
        return;
    }
    
    strcpy((char *)free_dir->name, new_name);
    free(new_name);
    free_dir->attr = 0; // Set appropriate attributes
    free_dir->starting_cluster = 2; // Set appropriate starting cluster (for simplicity, using cluster 2)
    free_dir->file_size = file_size;

    // Escreve a nova entrada de diretório na imagem FAT16
    fseek(fp, bpb_froot_addr(bpb) + (free_dir - dirs) * sizeof(struct fat_dir), SEEK_SET);
    fwrite(free_dir, 1, sizeof(struct fat_dir), fp);

    // Escreve os dados do arquivo na imagem FAT16
    fseek(fp, bpb_fdata_addr(bpb) + (free_dir->starting_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust, SEEK_SET);
    fwrite(buffer, 1, file_size, fp);

    // Limpeza
    free(buffer);
    free(dirs);
}



void rm(FILE *fp, char *filename, struct fat_bpb *bpb) {
    // Carrega as entradas de diretório da imagem FAT16
    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        fprintf(stderr, "Error listing directories\n");
        return;
    }

    // Encontra o ponteiro para a entrada de diretório correspondente ao arquivo especificado
    struct fat_dir *dir = find_pointer(dirs, filename, bpb);
    if (!dir) {
        fprintf(stderr, "Error finding file %s\n", filename);
        free(dirs);
        return;
    }

    // Limpa o conteúdo do arquivo na imagem FAT16
    if (wipe(fp, dir, bpb) != 0) {
        fprintf(stderr, "Error wiping file %s\n", filename);
        free(dirs);
        return;
    }

    // Marca a entrada de diretório como livre
    dir->attr = DIR_FREE_ENTRY;
    fseek(fp, bpb_froot_addr(bpb) + (dir - dirs) * sizeof(struct fat_dir), SEEK_SET);
    fwrite(dir, 1, sizeof(*dir), fp);

    // Libera a memória alocada para as entradas de diretório
    free(dirs);
}

void cp(FILE *fp, char *filename, char *dest_filename, struct fat_bpb *bpb) {
    // Listar diretórios na FAT16
    struct fat_dir *dirs = ls(fp, bpb);
    if (!dirs) {
        fprintf(stderr, "Erro ao listar diretórios\n");
        return;
    }
    
    // Encontrar o arquivo de origem na FAT16
    struct fat_dir *dir = find_pointer(dirs, filename, bpb);
    if (!dir) {
        fprintf(stderr, "Erro ao encontrar arquivo %s\n", filename);
        free(dirs);
        return;
    }

    // Abrir arquivo de destino na máquina host
    FILE *dest_fp = fopen(dest_filename, "wb");
    if (!dest_fp) {
        fprintf(stderr, "Erro ao abrir arquivo de destino %s\n", dest_filename);
        free(dirs);
        return;
    }

    uint32_t inicial_data_location = bpb_fdata_addr(bpb) + (dir->starting_cluster - 2) * bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t file_size = dir->file_size;
    

    // Posicionar o ponteiro do arquivo no início dos dados do arquivo
    fseek(fp, inicial_data_location, SEEK_SET);

    // Buffer para leitura/escrita
    uint8_t buffer[1024]; 

    // Ler e escrever dados em blocos até o arquivo ser completamente copiado
    uint32_t counter = file_size;
    while (counter > 0) {
        size_t size = (counter > sizeof(buffer)) ? sizeof(buffer) : counter;
        fread(buffer, 1, size, fp);
        fwrite(buffer, 1, size, dest_fp);
        counter -= size;
    }

    // Fechar arquivos e liberar memória alocada
    fclose(dest_fp);
    free(dirs);
}



