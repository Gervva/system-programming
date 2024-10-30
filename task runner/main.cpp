#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_program>" << std::endl;
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        int out_fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0700);
        int err_fd = open("err.txt", O_WRONLY | O_CREAT | O_TRUNC, 0700);
        
        dup2(out_fd, STDOUT_FILENO);
        dup2(err_fd, STDERR_FILENO);

        close(out_fd);
        close(err_fd);

        execlp(argv[1], argv[1], NULL);
        _exit(1);
    } else if (pid > 0) {
        wait(nullptr);
    } else {
        std::cerr << "Failed to fork" << std::endl;
        return 1;
    }

    return 0;
}