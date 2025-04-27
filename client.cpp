#include "common.hpp"
#include <arpa/inet.h>

int main() {
    auto dest_address = local_addr(SERVER_PORT);
    int sock_fd = check(make_socket(SOCK_STREAM));
    check(connect(sock_fd, (sockaddr*)&dest_address, sizeof(dest_address)));

    int32_t net_range;
    check(recv(sock_fd, &net_range, sizeof(net_range), MSG_WAITALL));
    int range = ntohl(net_range);

    int low = 1;
    int high = range;
    char response;
    auto start_time = std::chrono::high_resolution_clock::now();
    while (low <= high) {
        sleep(3);
        int guess = low + (high - low) / 2;
        int32_t net_guess = htonl(guess);
        check(send(sock_fd, &net_guess, sizeof(net_guess), MSG_WAITALL));
        check(recv(sock_fd, &response, sizeof(response), MSG_WAITALL));

        std::cout << "Try: " << guess << ", answer: " << response << std::endl;
        //sleep(5);

        if (response == '=') {
            std::cout << "Correct number: " << guess << std::endl;
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            std::cout << "Time: " << duration << " ms\n";
            break;
        }
        else if (response == '>') {
            low = guess + 1;
        }
        else if (response == '<') {
            high = guess - 1;
        }
    }

    close(sock_fd);
    return 0;
}
