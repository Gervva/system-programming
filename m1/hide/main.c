#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#define HIDE_DIR ".hide_dir/"

int create_hide_dir() {
    if (access(HIDE_DIR, F_OK) == -1) {
        if (mkdir(HIDE_DIR, 0700) != 0) {
            perror("Ошибка при создании \"темного\" каталога");
            return -1;
        }
    }
    return 0;
}

int hide_file(const char *fname) {
    char new_path[64];
    snprintf(new_path, sizeof(new_path), "%s%s", HIDE_DIR, fname);

    if (rename(fname, new_path) != 0) {
        perror("Ошибка при переносе файла");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *fname = argv[1];

    if (create_hide_dir() != 0) {
        return 1;
    }

    if (hide_file(fname) != 0) {
        return 1;
    }

    return 0;
}
