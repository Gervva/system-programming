#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

int main() {
    int count_files = 0, count_dirs = 0, count_symlinks = 0;
    int count_fifos = 0, count_sockets = 0, count_blocks = 0;
    int count_chars = 0;

    DIR *currentDir;
    currentDir = opendir(".");
    if (currentDir == NULL) {
        perror("Не удалось открыть директорию");
        return 1;
    }

    struct dirent *dirEntry;
    struct stat statBuffer;
    while ((dirEntry = readdir(currentDir)) != NULL) {
        if (lstat(dirEntry->d_name, &statBuffer) == -1) {
            perror("Ошибка при получении информации о файле");
            continue;
        }

        if (dirEntry->d_name[0] == '.' && (dirEntry->d_name[1] == '\0' || 
                (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0'))) {
            continue;
        }

        if (S_ISDIR(statBuffer.st_mode)) {
            count_dirs++;
        } else if (S_ISLNK(statBuffer.st_mode)) {
            count_symlinks++;
        } else if (S_ISREG(statBuffer.st_mode)) {
            count_files++;
        } else if (S_ISCHR(statBuffer.st_mode)) {
            count_chars++;
        } else if (S_ISBLK(statBuffer.st_mode)) {
            count_blocks++;
        } else if (S_ISFIFO(statBuffer.st_mode)) {
            count_fifos++;
        } else if (S_ISSOCK(statBuffer.st_mode)) {
            count_sockets++;
        }
    }

    closedir(currentDir);

    printf("Directories: %d\n", count_dirs);
    printf("Regular files: %d\n", count_files);
    printf("Symbolic links: %d\n", count_symlinks);
    printf("Character devices: %d\n", count_chars);
    printf("Block devices: %d\n", count_blocks);
    printf("Named pipes (FIFOs): %d\n", count_fifos);
    printf("Sockets: %d\n", count_sockets);

    return 0;
}
