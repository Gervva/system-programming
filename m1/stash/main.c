#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLAG "OBFUSCATED"
#define KEY "qwerty"

void stash(const char *filename, const char *key) {
    FILE *file = fopen(filename, "rb+");
    if (!file) {
        perror("Error opening file");
        return;
    }

    int key_len = strlen(key);
    if (key_len == 0) {
        fprintf(stderr, "Key cannot be empty\n");
        fclose(file);
        return;
    }

    int i = 0, ch;
    while ((ch = fgetc(file)) != EOF) {
        fseek(file, -1, SEEK_CUR);
        fputc(ch ^ key[i % key_len], file);
        i++;
    }

    fwrite(FLAG, 1, strlen(FLAG), file);

    fclose(file);
}

void restore(const char *filename, const char *key) {
    FILE *file = fopen(filename, "rb+");
    if (!file) {
        perror("Error opening file");
        return;
    }

    int key_len = strlen(key);
    if (key_len == 0) {
        fprintf(stderr, "Key cannot be empty\n");
        fclose(file);
        return;
    }

    int i = 0, ch;
    while ((ch = fgetc(file)) != EOF) {
        fseek(file, -1, SEEK_CUR);
        fputc(ch ^ key[i % key_len], file);
        i++;
    }

    fseek(file, 0, SEEK_END);
    long new_size = ftell(file) - strlen(FLAG);
    fclose(file);

    truncate(filename, new_size);
}

int check_flag(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    size_t flag_len = strlen(FLAG);

    if (file_size < flag_len) {
        fclose(file);
        return 0;
    }

    fseek(file, -flag_len, SEEK_END);

    char buffer[flag_len + 1];
    fread(buffer, 1, flag_len, file);
    buffer[flag_len] = '\0';
    fclose(file);

    return strcmp(buffer, FLAG) == 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mode> <filename> <key>\n", argv[0]);
        fprintf(stderr, "\tmodes:\n\t\tstash - Obfuscate the file\n\t\trestore - Restore the file\n");
        return 1;
    }

    const char *mode = argv[1];
    const char *filename = argv[2];

    if (strcmp(mode, "stash") == 0) {
        if (check_flag(filename)) {
            printf("File '%s' is already stashed\n", filename);
            return 1;
        }

        stash(filename, KEY);
        printf("File '%s' stashed successfully\n", filename);
    } else if (strcmp(mode, "restore") == 0) {
        if (!check_flag(filename)) {
            printf("File '%s' is not stashed\n", filename);
            return 1;
        }

        restore(filename, KEY);
        printf("File '%s' restored successfully\n", filename);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        return 1;
    }

    return 0;
}
