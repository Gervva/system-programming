#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_MSG "!start_msg!"
#define END_MSG "!end_msg!"

// Функция для добавления сообщения
void add_message(const char *filename, const char *message) {
    FILE *file = fopen(filename, "rb+");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    char *start = strstr(buffer, START_MSG);
    char *end = strstr(buffer, END_MSG);

    if (start && end && end > start) {
        fseek(file, end - buffer, SEEK_SET);
        fwrite(message, 1, strlen(message), file);
        fwrite("\n", 1, 1, file);
    } else {
        fseek(file, 0, SEEK_END);
        fwrite(START_MSG, 1, strlen(START_MSG), file);
        fwrite(message, 1, strlen(message), file);
        fwrite("\n", 1, 1, file);
        fwrite(END_MSG, 1, strlen(END_MSG), file);
    }

    free(buffer);
    fclose(file);
}

void read_message(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    char *start = strstr(buffer, START_MSG);
    char *end = strstr(buffer, END_MSG);

    if (start && end && end > start) {
        start += strlen(START_MSG);
        *end = '\0';
        printf("Скрытое сообщение:\n%s\n", start);
    } else {
        printf("Сообщение не найдено\n");
    }

    free(buffer);
    fclose(file);
}

// Функция для удаления сообщения
void delete_message(const char *filename) {
    FILE *file = fopen(filename, "rb+");
    if (!file) {
        perror("Ошибка открытия файла");
        return;
    }

    // Чтение всего содержимого файла
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    // Поиск меток
    char *start = strstr(buffer, START_MSG);
    char *end = strstr(buffer, END_MSG);

    if (start && end && end > start) {
        // Сдвигим остальную часть файла
        long new_size = file_size - (end - buffer + strlen(END_MSG));
        fseek(file, start - buffer, SEEK_SET);
        fwrite(end + strlen(END_MSG), 1, new_size, file);
        // Уменьшаем размер файла
        ftruncate(fileno(file), new_size + (start - buffer));
    } else {
        printf("Сообщение не найдено\n");
    }

    free(buffer);
    fclose(file);
}

// Основная функция
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mode> <path> [message]\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *filename = argv[2];

    if (strcmp(mode, "add") == 0 && argc == 4) {
        add_message(filename, argv[3]);
    } else if (strcmp(mode, "read") == 0) {
        read_message(filename);
    } else if (strcmp(mode, "delete") == 0) {
        delete_message(filename);
    } else {
        fprintf(stderr, "Неверные аргументы\n");
    }

    return 0;
}
