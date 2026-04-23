#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 4096

//s not null
//size_t
void reverse_string(char *s) {
    int l = 0;
    int r = strlen(s) - 1;

    while (l < r) {
        char temp = s[l];
        s[l] = s[r];
        s[r] = temp;
        l++;
        r--;
    }
}

char *get_name_from_path(const char *path) {
    const char *last_slash = strrchr(path, '/');

    if (last_slash == NULL) {
        return strdup(path);
    }

    return strdup(last_slash + 1);
}

//обработать косяки
void reverse_file_copy(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    if (src == NULL) {
        perror("fopen src");
        return;
    }

    FILE *dst = fopen(dst_path, "wb");
    if (dst == NULL) {
        perror("fopen dst");
        fclose(src);
        return;
    }

    fseek(src, 0, SEEK_END);
    long file_size = ftell(src);

    char buffer[BUFFER_SIZE];

    while (file_size > 0) {
        long chunk_size;

        if (file_size >= BUFFER_SIZE) {
            chunk_size = BUFFER_SIZE;
        } else {
            chunk_size = file_size;
        }

        file_size -= chunk_size;

        fseek(src, file_size, SEEK_SET);
        fread(buffer, 1, chunk_size, src);

        for (long i = 0; i < chunk_size / 2; i++) {
            char temp = buffer[i];
            buffer[i] = buffer[chunk_size - 1 - i];
            buffer[chunk_size - 1 - i] = temp;
        }

        fwrite(buffer, 1, chunk_size, dst);
    }

    fclose(src);
    fclose(dst);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    char *src_dir_path = argv[1];

    DIR *dir = opendir(src_dir_path);
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    //no pointer check null
    char *src_dir_name = get_name_from_path(src_dir_path);
    char *dst_dir_name = strdup(src_dir_name);
    reverse_string(dst_dir_name);

    if (mkdir(dst_dir_name, 0755) != 0) {
        perror("mkdir");
        free(src_dir_name);
        free(dst_dir_name);
        closedir(dir);
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_file_path[1024];
        snprintf(src_file_path, sizeof(src_file_path), "%s/%s", src_dir_path, entry->d_name);

        struct stat st;
        if (stat(src_file_path, &st) != 0) {
            perror("stat");
            continue;
        }

        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        char *dst_file_name = strdup(entry->d_name);
        reverse_string(dst_file_name);

        char dst_file_path[1024];
        snprintf(dst_file_path, sizeof(dst_file_path), "%s/%s", dst_dir_name, dst_file_name);

        reverse_file_copy(src_file_path, dst_file_path);

        free(dst_file_name);
    }

    free(src_dir_name);
    free(dst_dir_name);
    closedir(dir);

    return 0;
}
