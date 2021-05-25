#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>
#define mkdir(x) _mkdir(x)
#else
#include <dlfcn.h>
#include "linoodle_lib.h"
#define mkdir(x) mkdir(x, 0777)
#endif

#include "utils.h"
#include "oo2core_dll.h"

typedef int OodLZ_DecompressFunc(uint8_t *src_buf, int src_len, uint8_t *dst, size_t dst_size,
    int fuzz, int crc, int verbose,
    uint8_t *dst_base, size_t e, void *cb, void *cb_ctx, void *scratch, size_t scratch_size, int threadPhase);

OodLZ_DecompressFunc *OodLZ_Decompress;

int oodle_init(void)
{
    FILE *oo2core = fopen("oo2core_8_win64.dll", "rb");

    if (!oo2core) {
        oo2core = fopen("oo2core_8_win64.dll", "wb");

        if (!oo2core)
            return -1;
        
        fwrite(oo2core_dll, 1, sizeof(oo2core_dll), oo2core);
        fclose(oo2core);
    }
    else {
        fclose(oo2core);
    }

#ifdef _WIN32
    HMODULE oodle = LoadLibraryA("./oo2core_8_win64.dll");
    OodLZ_Decompress = (OodLZ_DecompressFunc*)GetProcAddress(oodle, "OodleLZ_Decompress");
#else
    FILE *linoodle = fopen("liblinoodle.so", "rb");

    if (!linoodle) {
        linoodle = fopen("liblinoodle.so", "wb");

        if (!linoodle)
            return -1;
        
        fwrite(linoodle_lib, 1, sizeof(linoodle_lib), linoodle);
        fclose(linoodle);
    }
    else {
        fclose(linoodle);
    }

    void *oodle = dlopen("./liblinoodle.so", RTLD_LAZY);
    OodLZ_Decompress = dlsym(oodle, "OodleLZ_Decompress");
#endif

    if (!OodLZ_Decompress)
        return -1;
    
    return 0;
}

int main(int argc, char **argv)
{
    char buffer[8192];
    setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

    printf("EternalResourceExtractor v1.0 by PowerBall253\n\n");

    if (argc > 1 && !strcmp(argv[1], "--help")) {
        printf("Usage:\n");
        printf("%s [path to .resources file] [out path]\n", argv[0]);
        fflush(stdout);
        return 0;
    }

    char resource_path[PATH_MAX];
    char out_path[PATH_MAX];

    switch (argc) {
        case 1:
            printf("Input the path to the .resources file: ");
            fflush(stdout);
            fgets(resource_path, PATH_MAX, stdin);

            printf("Input the path to the out directory: ");
            fflush(stdout);
            fgets(out_path, PATH_MAX, stdin);

            printf("\n");
            break;
        case 2:
            strncpy(resource_path, argv[1], PATH_MAX);
            resource_path[PATH_MAX - 1] = '\0';

            printf("Input the path to the out directory: ");
            fflush(stdout);
            fgets(out_path, PATH_MAX, stdin);

            printf("\n");
            break;
        default:
            strncpy(resource_path, argv[1], PATH_MAX);
            strncpy(out_path, argv[2], PATH_MAX);

            resource_path[PATH_MAX - 1] = '\0';
            out_path[PATH_MAX - 1] = '\0';

            break;
    }

    strcpy(resource_path, fmt_path(resource_path));
    strcpy(out_path, fmt_path(out_path));

#ifdef _WIN32
    char *full_path = _fullpath(NULL, out_path, PATH_MAX);

    if (!full_path) {
        fflush(stdout);
        fprintf(stderr, "\nERROR: Failed to get absolute path from out path!\n");
        press_any_key();
        return 1;
    }

    strcpy(out_path, full_path);

    free(full_path);
#endif

    FILE *resource = fopen(resource_path, "rb");

    if (!resource) {
        fflush(stdout);
        fprintf(stderr, "\nERROR: Failed to open %s for reading!\n", resource_path);
        press_any_key();
        return 1;
    }

    char signature[4];
    fread(signature, 1, sizeof(signature), resource);

    if (memcmp(signature, "IDCL", 4)) {
        fflush(stdout);
        fprintf(stderr, "\nERROR: %s is not a valid .resources file!\n", resource_path);
        press_any_key();
        return 1;
    }

    if (mkdir(out_path) == -1 && errno != EEXIST) {
        fflush(stdout);
        fprintf(stderr, "\nERROR: Failed to create out directory!\n");
        return 1;
    }

    fseek(resource, 28, SEEK_CUR);

    uint32_t file_count;
    fread(&file_count, sizeof(file_count), 1, resource);

    fseek(resource, 4, SEEK_CUR);

    uint32_t dummy_count;
    fread(&dummy_count, sizeof(dummy_count), 1, resource);

    fseek(resource, 20, SEEK_CUR);

    uint64_t names_offset;
    fread(&names_offset, sizeof(names_offset), 1, resource);

    fseek(resource, 8, SEEK_CUR);

    uint64_t info_offset;
    fread(&info_offset, sizeof(info_offset), 1, resource);

    fseek(resource, 8, SEEK_CUR);

    uint64_t dummy_offset;
    fread(&dummy_offset, sizeof(dummy_offset), 1, resource);
    dummy_offset += dummy_count * sizeof(uint32_t);

    uint64_t data_offset;
    fread(&data_offset, sizeof(data_offset), 1, resource);

    fseek(resource, names_offset, SEEK_SET);

    uint64_t name_count;
    fread(&name_count, sizeof(name_count), 1, resource);

    char **names = malloc(name_count * sizeof(char*));

    if (!names) {
        fflush(stdout);
        fprintf(stderr, "\nERROR: Failed to allocate memory!\n");
        press_any_key();
        return 1;
    }

    long curr_pos = ftell(resource);

    for (int i = 0; i < name_count; i++) {
        fseek(resource, curr_pos + i * 8, SEEK_SET);

        uint64_t curr_name_offset;
        fread(&curr_name_offset, sizeof(curr_name_offset), 1, resource);
        
        fseek(resource, names_offset + name_count * 8 + curr_name_offset + 8, SEEK_SET);

        char *name = NULL;
        size_t len = 0;
        getdelim(&name, &len, '\0', resource);

        names[i] = name;
    }

    fseek(resource, info_offset, SEEK_SET);

    for (int i = 0; i < file_count; i++) {
        fseek(resource, 24, SEEK_CUR);

        uint64_t type_id_offset;
        fread(&type_id_offset, sizeof(type_id_offset), 1, resource);

        uint64_t name_id_offset;
        fread(&name_id_offset, sizeof(name_id_offset), 1, resource);

        fseek(resource, 16, SEEK_CUR);

        uint64_t offset;
        fread(&offset, sizeof(offset), 1, resource);

        uint64_t z_size;
        fread(&z_size, sizeof(z_size), 1, resource);

        uint64_t size;
        fread(&size, sizeof(size), 1, resource);

        fseek(resource, 32, SEEK_CUR);

        uint64_t zip_flags;
        fread(&zip_flags, sizeof(zip_flags), 1, resource);

        fseek(resource, 24, SEEK_CUR);

        type_id_offset = type_id_offset * 8 + dummy_offset;
        name_id_offset = (name_id_offset + 1) * 8 + dummy_offset;

        curr_pos = ftell(resource);

        fseek(resource, type_id_offset, SEEK_SET);

        uint64_t type_id;
        fread(&type_id, sizeof(type_id), 1, resource);

        fseek(resource, name_id_offset, SEEK_SET);

        uint64_t name_id;
        fread(&name_id, sizeof(name_id), 1, resource);

        char *name = names[name_id];

        printf("Extracting %s...\n", name);

        char *path = malloc(strlen(name) + strlen(out_path) + 10);

        if (!path) {
            fflush(stdout);
            fprintf(stderr, "\nERROR: Failed to allocate memory!\n");
            press_any_key();
            return 1;
        }

        *path = '\0';

#ifdef _WIN32
        strcat(path, "\\\\?\\");
#endif

        strcat(path, out_path);

        if (*(path + strlen(path) - 1) != separator_char) {
            char separator_str[2] = {separator_char, '\0'};
            strcat(path, separator_str);
        }

        strcat(path, name);
        change_separator(path);

#ifdef _WIN32
        wchar_t *path_wstr = char_to_wchar(path);

        if (!path_wstr) {
            fflush(stdout);
            fprintf(stderr, "\nERROR: Failed to create path for extraction!\n");
            return 1;
        }

        long start_pos = 4 + strlen(out_path);

        if (*(out_path + strlen(out_path)) == separator_char)
            start_pos = start_pos + 1;
        
        int res = mkpath(path_wstr, start_pos);
#else
        long start_pos = strlen(out_path);

        if (*(out_path + strlen(out_path)) == separator_char)
            start_pos = start_pos + 1;

        int res = mkpath(path, start_pos);
#endif

        if (res == -1) {
            fflush(stdout);
            fprintf(stderr, "\nERROR: Failed to create path for extraction!\n");
            press_any_key();
            return 1;
        }

        if (size == z_size) {
            fseek(resource, offset, SEEK_SET);

            unsigned char *file_bytes = malloc(z_size);

            if (!file_bytes) {
                fflush(stdout);
                fprintf(stderr, "\nERROR: Failed to allocate memory!\n");
                press_any_key();
                return 1;
            }

            fread(file_bytes, 1, z_size, resource);

#ifdef _WIN32
            FILE *f = _wfopen(path_wstr, L"wb");
            free(path_wstr);
#else
            FILE *f = fopen(path, "wb");
#endif

            if (!f) {
                fflush(stdout);
                fprintf(stderr, "\nERROR: Failed to open %s for writing!\n", path);
                press_any_key();
                return 1;
            }

            fwrite(file_bytes, 1, z_size, f);
            fclose(f);

            free(file_bytes);
        }
        else {
            if (zip_flags == 4) {
                offset += 12;
                z_size -= 12;
            }

            fseek(resource, offset, SEEK_SET);

            unsigned char *enc_bytes = malloc(z_size);

            if (!enc_bytes) {
                fflush(stdout);
                fprintf(stderr, "\nERROR: Failed to allocate memory!\n");
                press_any_key();
                return 1;
            }

            fread(enc_bytes, 1, z_size, resource);

            unsigned char *dec_bytes = malloc(size);

            if (!dec_bytes) {
                fprintf(stderr, "\nERROR: Failed to allocate memory!\n");
                press_any_key();
                return 1;
            }

            if (!OodLZ_Decompress) {
                if (oodle_init() == -1) {
                    fflush(stdout);
                    fprintf(stderr, "\nERROR: Failed to init oodle for decompressing!\n");
                    press_any_key();
                    return 1;
                }
            }

            if (OodLZ_Decompress(enc_bytes, z_size, dec_bytes, size, 0, 0, 0, NULL, 0, NULL, NULL, NULL, 0, 0) != size) {
                fflush(stdout);
                fprintf(stderr, "\nERROR: Failed to decompress %s!\n", name);
                press_any_key();
                return 1;
            }

            free(enc_bytes);

#ifdef _WIN32
            FILE *f = _wfopen(path_wstr, L"wb");
            free(path_wstr);
#else
            FILE *f = fopen(path, "wb");
#endif

            if (!f) {
                fflush(stdout);
                fprintf(stderr, "\nERROR: Failed to open %s for writing!\n", path);
                press_any_key();
                return 1;
            }

            fwrite(dec_bytes, 1, size, f);
            fclose(f);

            free(dec_bytes);
        }

        free(path);

        fseek(resource, curr_pos, SEEK_SET);
    }

    fclose(resource);

    free(names);

    printf("\nDone, %d files extracted.\n", file_count);
    fflush(stdout);
    press_any_key();
}