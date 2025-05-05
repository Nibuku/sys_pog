#include "common.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <arpa/inet.h>
#include <semaphore.h>

sem_t* log_sem;

void no_zombie() {
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, nullptr);
}

void connection(int client_fd, sockaddr_in client_addr, int range_max) {
    srand(getpid());
    int secret = rand() % range_max + 1;

    int32_t net_range = htonl(range_max);
    check(write(client_fd, &net_range, sizeof(net_range)));

    int guess;
    while (read(client_fd, &guess, sizeof(guess)) > 0) {
        int host_guess = ntohl(guess);

        char result;
        if (host_guess < secret) {
            result = '>';
        } else if (host_guess > secret) {
            result = '<';
        } else {
            result = '=';
        }

        check(write(client_fd, &result, sizeof(result)));
        sem_wait(log_sem);
        std::cout << client_addr << " guessed " << host_guess << std::endl;
        sem_post(log_sem);

        if (result == '=')
            break;
    }
    sem_wait(log_sem);
    std::cout << client_addr << " disconnected" << std::endl;
    sem_post(log_sem);
    close(client_fd);
    exit(0);
}

int main() {
    log_sem = check(sem_open("/sem", O_CREAT, S_IRWXU, 1));
    int port = SERVER_PORT;

    auto server_addr = local_addr(port);
    int server_fd = check(make_socket(SOCK_STREAM));

    int opt = 1;
    //setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    check(bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)));
    check(listen(server_fd, 10));

    no_zombie();

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = check(accept(server_fd, (sockaddr*)&client_addr, &client_len));

        pid_t pid = check(fork());

        if (pid == 0) {
            close(server_fd);
            sem_wait(log_sem);
            std::cout << client_addr << " connected" << std::endl;
            sem_post(log_sem);
            connection(client_fd, client_addr, 10000);
        } else {
            close(client_fd);
        }
    }
}
