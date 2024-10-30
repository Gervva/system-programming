#include <iostream>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_program>" << std::endl;
        return 1;
    }

    if (access(argv[1], F_OK) == -1) {
        std::cerr << "Error: File '" << argv[1] << "' does not exist." << std::endl;
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        int null_fd = open("/dev/null", O_RDWR);

        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);

        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }

        setsid();
        execlp(argv[1], argv[1], NULL);
        _exit(1);
    } else if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
    }

    return 0;
}